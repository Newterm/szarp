/* 
  SZARP: SCADA software 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/* 
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'params' (root element) class implementation.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>

#include <errno.h>
#include <ctype.h>
#include <string>
#include <dirent.h>
#include <assert.h>
#include <sstream>

#include <algorithm>

#ifdef MINGW32
#include "mingw32_missing.h"
#else
#include <libgen.h>
#endif

#include "szarp_config.h"
#include "parcook_cfg.h"
#include "line_cfg.h"
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "liblog.h"
#include "insort.h"
#include "conversion.h"
#include "definable_parser.h"
#include "szbbase.h"
#include "loptcalculate.h"


using namespace std;

#define FREE(x)	if (x != NULL) free(x)

/**
 * Swap bit first (from right, from 0) with second in data. */
void
swap_bits(unsigned long *data, int first, int second)
{
#define GET_BIT(d,i) (((d) & (1UL << (i))) && 1)
#define SET_BIT1(d,i) (d) |= (1UL << i)
#define SET_BIT0(d,i) (d) &= ~(1UL << i)
    int fb, sb;
    fb = GET_BIT(*data, first);
    sb = GET_BIT(*data, second);
    if (fb)
	SET_BIT1(*data, second);
    else
	SET_BIT0(*data, second);
    if (sb)
	SET_BIT1(*data, first);
    else
	SET_BIT0(*data, first);
   sz_log(9, "swap_bits(): data 0x%lx first %d (%d) second %d (%d)", *data, first, fb, second, sb);
#undef GET_BIT
#undef SET_BIT1
#undef SET_BIT0
}

/**
 * Data type for third argument of comp_draws and swap_draws functions.
 * @see comp_draws
 * @see swap_draws
 */
struct sort_draws_data_str {
    tDraw *draws;		/**< Table of draws. */
    unsigned long *id;		/**< Pointer to draws' window identifier. */
};
typedef struct sort_draws_data_str sort_draws_data_type;

/**
 * Function used to compare draws, @see insort.
 */
int
comp_draws(int a, int b, void *data)
{
    tDraw *draws = ((sort_draws_data_type *) data)->draws;

    if (draws[a].data >= 0) {
	if (draws[b].data >= 0) {
	    if (draws[a].data < draws[b].data)
		return -1;
	    if (draws[a].data > draws[b].data)
		return 1;
	    if (a < b)
		return -1;
	    if (a > b)
		return 1;
	    return 0;
	} else
	    return -1;
    } else if (draws[b].data >= 0)
	return 1;
    else {
	if (a < b)
	    return -1;
	if (a < b)
	    return 1;
    }
    return 0;
}

/**
 * Function used to swap draws, @see insort.
 */
void
swap_draws(int a, int b, void *data)
{
    tDraw tmp;
    tDraw *draws = ((sort_draws_data_type *) data)->draws;
    unsigned long *id = ((sort_draws_data_type *) data)->id;

    memcpy(&tmp, &draws[a], sizeof(tDraw));
    memcpy(&draws[a], &draws[b], sizeof(tDraw));
    memcpy(&draws[b], &tmp, sizeof(tDraw));

    swap_bits(id, a, b);
}


TSzarpConfig::TSzarpConfig() :
	devices(NULL), defined(NULL),
	drawdefinable(NULL), title(),
	prefix(), seasons(NULL),
	unit_counter(1),
	use_names_cache(false)
{
}

TSzarpConfig::~TSzarpConfig()
{
	if( devices )       delete devices      ;
	if( defined )       delete defined      ;
	if( drawdefinable ) delete drawdefinable;
	if( seasons )       delete seasons      ;
}

TUnit* TSzarpConfig::createUnit(TDevice* parent)
{
	return new TUnit(unit_counter++, parent, this);
}

const std::wstring&
TSzarpConfig::GetTitle() const
{
    return title;
}

const std::wstring& 
TSzarpConfig::GetPrefix() const
{
    return prefix;
}

int
TSzarpConfig::loadXMLDOM(const std::wstring& path, const std::wstring& _prefix) {

	xmlDocPtr doc;
	int ret;

	prefix = _prefix;

	xmlLineNumbersDefault(1);
	doc = xmlParseFile(SC::S2A(path).c_str());
	if (doc == NULL) {
		return 1;
	}

	ret = parseXML(doc);
	xmlFreeDoc(doc);

	return ret;
}

int
TSzarpConfig::loadXMLReader(const std::wstring &path, const std::wstring& _prefix)
{
	prefix = _prefix;
	xmlTextReaderPtr reader = xmlNewTextReaderFilename(SC::S2A(path).c_str());

	if (NULL == reader) {
		sz_log(1,"Unable to open XML document\n");
		return 1;
	}

	int ret = xmlTextReaderRead(reader);

	if (ret == 1) {
		try {
			ret = parseXML(reader);
		}
	       	catch (XMLWrapperException) {
			ret = 1;
		}
	}
	else {
		ret = 1;
	}

	xmlFreeTextReader(reader);

	return ret;
}

int
TSzarpConfig::loadXML(const std::wstring &path, const std::wstring &prefix)
{
#ifdef USE_XMLREADER
	int res = loadXMLReader(path,prefix);
#else
	int res = loadXMLDOM(path,prefix);
#endif
	if( res ) return res;

	return 0;
}

int
TSzarpConfig::parseXML(xmlTextReaderPtr reader)
{
	TDevice *td = NULL;

	assert(devices == NULL);
	assert(defined == NULL);

	XMLWrapper xw(reader);

	const char* ignored_trees[] = { "mobile", "checker:rules",  0 };
	xw.SetIgnoredTrees(ignored_trees);

	TAttribHolder::parseXML(reader);

	title = SC::L2S(getAttribute<std::string>("title", ""));

	size_t device_no = 0;

	for (;;) {
		if (xw.IsTag("params")) {
			if (xw.IsBeginTag()) {

				const char* need_attr_params[] = { "read_freq" , "send_freq", "version", 0 };
				if (!xw.AreValidAttr(need_attr_params)) {
					throw XMLWrapperException();
				}

				for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
					const xmlChar *attr = xw.GetAttr();
					storeAttribute(SC::U2L(xw.GetAttrName()), SC::U2L(attr));
				}

				if (!hasAttribute("version") || getAttribute<std::string>("version", "0.0") != "1.0") {
					sz_log(0, "Error regarding \"version\" attribute!");
					return 1;
				}

			} else {
				break;
			}
		} else
		if (xw.IsTag("device")) {
			if (xw.IsBeginTag()) {
				++device_no;

				if (devices == NULL)
					devices = td = new TDevice(this);
				else
					td = td->Append(new TDevice(this));
				assert(devices != NULL);
				try {
					td->parseXML(reader);
				} catch(const std::exception& e) {
					sz_log(0, "XML error in device no %d: %s", device_no, e.what());
					return 1;
				}
			}
		} else
		if (xw.IsTag("defined")) {
			if (xw.IsBeginTag()) {
				TParam * _par = TDefined::parseXML(reader, this);
				if (_par)
					defined = _par;
			}
		} else
		if (xw.IsTag("drawdefinable")) {
			if (xw.IsBeginTag()) {
				TParam * _par = TDrawdefinable::parseXML(reader, this);
				if (_par)
					drawdefinable = _par;
			}
		} else
		if (xw.IsTag("seasons")) {
			if (xw.IsBeginTag()) {
				seasons = new TSSeason();
				if (seasons->parseXML(reader)) {
					delete seasons;
					seasons = NULL;
					xw.XMLError("'<seasons>' parse problem");
				}
			}
		} 	else {
			xw.XMLErrorNotKnownTag("params");
		}
		if (!xw.NextTag())
			return 1;
	}

// why? copy/paste from parse reader
	if (seasons == NULL)
		seasons = new TSSeason();

	return 0;
}


int
TSzarpConfig::parseXML(xmlDocPtr doc)
{
    TDevice *td = NULL;
    TParam *p = NULL;
    xmlNodePtr node, ch;
	size_t device_no = 0;

    node = doc->children;
	if (!node || SC::U2A(node->name) != "params") {
		sz_log(0, "No \"params\" node in xml! Cannot continue!");
		return 1;
	}

	TAttribHolder::parseXML(node);

	title = SC::L2S(getAttribute<std::string>("title", ""));

	if (!hasAttribute("version") || getAttribute<std::string>("version", "0.0") != "1.0") {
		sz_log(0, "Error regarding \"version\" attribute!");
		return 1;
	}

    assert(devices == NULL);
    assert(defined == NULL);

    for (ch = node->children; ch; ch = ch->next) {
	if (!strcmp((char *) ch->name, "device")) {
		++device_no;

	    if (devices == NULL)
		devices = td = new TDevice(this);
	    else
		td = td->Append(new TDevice(this));
	    assert(devices != NULL);

		try {
			td->parseXML(ch);
		} catch(const std::exception& e) {
			sz_log(0, "XML error in device no %d: %s", device_no, e.what());
			return 1;
		}
	}
	else if (!strcmp((char *) ch->name, "defined")) {
	    for (xmlNodePtr ch2 = ch->children; ch2; ch2 = ch2->next)
		if (!strcmp((char *) ch2->name, "param")) {
		    if (defined == NULL) {
			defined = p = new TParam(NULL, this);
		    } 
		    else {
			p = p->Append(new TParam(NULL, this));
		    }
		    assert(p != NULL);
		    if (p->parseXML(ch2))
			return 1;
		}
	}
	else if (!strcmp((char *) ch->name, "drawdefinable")) {
	    for (xmlNodePtr ch2 = ch->children; ch2; ch2 = ch2->next)
		if (!strcmp((char *) ch2->name, "param")) {
		    if (drawdefinable == NULL) {
			drawdefinable = p = new TParam(NULL, this);
		    } else {
			p = p->Append(new TParam(NULL, this));
		    }
		    assert(p != NULL);
		    if (p->parseXML(ch2))
			return 1;
		}
	}
	else if (!strcmp((char *)ch->name, "seasons")) {
		seasons = new TSSeason();
		if (seasons->parseXML(ch)) {
			delete seasons;
			seasons = NULL;
			return 1;
		}
	}
    }
    if (devices == NULL) {
		sz_log(1, "XML file error: 'device' elements not found");
		return 1;
    }

    if (seasons == NULL)
	    seasons = new TSSeason();

	return 0;
}

int
TSzarpConfig::PrepareDrawDefinable()
{
    if (NULL == drawdefinable)
	return 0;

    TParam * p = drawdefinable;
    while (p) {
	if (p->IsDefinable())
		try {
			p->PrepareDefinable();
		} catch( TCheckException& e ) {
			sz_log(1,"Invalid drawdefinable formula %s", SC::S2L(p->GetName()).c_str());
		}
	p = p->GetNext();
    }

    return 0;
}

TParam *
TSzarpConfig::getParamByName(const std::wstring& name) const
{

    if (use_names_cache) {
	auto it = params_map.find(name);
	if (it == params_map.end())
	    return NULL;
	return it->second;
    }

    for (TParam * p = GetFirstParam(); p; p = p->GetNextGlobal())
	if (!p->GetName().empty() && !name.empty() && p->GetName() == name)
	    return p;

    for (TParam * p = drawdefinable; p; p = p->GetNext())
	if (!p->GetName().empty() && !name.empty() && p->GetName() == name)
	    return p;

    return NULL;
}

TParam * TSzarpConfig::GetFirstParam() const
{
    for (auto d = GetFirstDevice(); d; d = d->GetNext())
	    for (TUnit * u = d->GetFirstUnit(); u; u = u->GetNext()) {
		TParam *p = u->GetFirstParam();
		if (p)
		    return p;
	    }
    if (defined)
	return defined;
    if (drawdefinable)
	return drawdefinable;
    return NULL;
}

int
TSzarpConfig::GetParamsCount() const
{
    int i = 0;
    for (auto d = GetFirstDevice(); d; d = d->GetNext())
	i += d->GetParamsCount();
    return i;
}

int
TSzarpConfig::GetDevicesCount()
{
    int i = 0;
    for (auto d = GetFirstDevice(); d; d = d->GetNext())
	i++;
    return i;
}

int
TSzarpConfig::GetDefinedCount()
{
    int i = 0;
    for (TParam * p = GetFirstDefined(); p; p = p->GetNext())
	i++;
    return i;
}

int
TSzarpConfig::GetDrawDefinableCount()
{
    int i = 0;
    for (TParam * p = drawdefinable; p; p = p->GetNextGlobal())
	i++;
    return i;
}

TDevice *
TSzarpConfig::DeviceById(int id) const
{
    auto d = GetFirstDevice();
    for (; d && (id > 1); d = d->GetNext(), id--);
    return d;
}

std::wstring 
TSzarpConfig::GetFirstRaportTitle()
{
    std::wstring title;
    for (TParam * p = GetFirstParam(); p; p = p->GetNextGlobal())
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (title.empty() || r->GetTitle().compare(title) < 0)
		title = r->GetTitle();
    return title;
}

std::wstring
TSzarpConfig::GetNextRaportTitle(const std::wstring &cur)
{
    std::wstring title;
    for (TParam * p = GetFirstParam(); p; p = p->GetNextGlobal())
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (r->GetTitle().compare(cur) > 0 && (title.empty() || r->GetTitle().compare(title) < 0))
		title = r->GetTitle();
    return title;
}

TRaport *
TSzarpConfig::GetFirstRaportItem(const std::wstring& title)
{
    for (TParam * p = GetFirstParam(); p; p = p->GetNextGlobal())
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (title == r->GetTitle())
		return r;
    return NULL;
}

const TSSeason* TSzarpConfig::GetSeasons() const {
	return seasons;
}

void TSzarpConfig::UseNamesCache()
{
	use_names_cache = true;
}

void TSzarpConfig::AddParamToNamesCache(TParam * _param)
{
	if (!use_names_cache)
		return;

	params_map[_param->GetName()] = _param;
}

size_t TSzarpConfig::GetFirstParamIpcInd(const TDevice &d) const {
	return d.GetFirstUnit()->GetFirstParam()->GetIpcInd();
}

const std::vector<size_t> TSzarpConfig::GetSendIpcInds(const TDevice &d) const {
	std::vector<size_t> ret;

	size_t no_sends = 0;
	for (TUnit* u = d.GetFirstUnit(); u != nullptr; u = u->GetNext()) {
		no_sends += u->GetSendParamsCount();
	}

	ret.reserve(no_sends);

	for (TUnit* u = d.GetFirstUnit(); u != nullptr; u = u->GetNext()) {
		for (TSendParam *s = u->GetFirstSendParam(); s; s = s->GetNext()) {
			auto ipc_param = s->GetParamToSend();

			if (ipc_param == nullptr) continue; // value-type send param (ignore it)
			else ret.push_back(ipc_param->GetIpcInd());
		}
	}

	return ret;
}

IPCParamInfo* TSzarpConfig::getIPCParamByName(const std::wstring& name) const {
	return getParamByName(name);
}

TParam* TSzarpConfig::createParam(TUnit* parent) {
	return new TParam(parent, this);
}

