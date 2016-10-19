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

#include "log_params.h"
#define LOG_PREFIX_LEN 8
#define LOG_PREFIX L"Activity"


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


TSzarpConfig::TSzarpConfig( bool logparams ) :
	read_freq(0), send_freq(0),
	devices(NULL), defined(NULL),
	drawdefinable(NULL), title(),
	prefix(), seasons(NULL) ,
	logparams(logparams),
       	device_counter(1),
       	radio_counter(1),
       	unit_counter(1),
	use_names_cache(false)
{
}

TSzarpConfig::~TSzarpConfig(void)
{
	if( devices )       delete devices      ;
	if( defined )       delete defined      ;
	if( drawdefinable ) delete drawdefinable;
	if( seasons )       delete seasons      ;
}


TDevice* TSzarpConfig::createDevice(const std::wstring& _daemon, const std::wstring& _path)
{
	return new TDevice(device_counter++, this, _daemon, _path);
}

TRadio* TSzarpConfig::createRadio(TDevice* parent, wchar_t _id, TUnit* _units)
{
	return new TRadio(radio_counter++, parent, _id, _units);
}

TUnit* TSzarpConfig::createUnit(TRadio* parent, wchar_t _id, int _type, int _subtype)
{
	return new TUnit(unit_counter++, parent, _id, _type, _subtype);
}

const std::wstring&
TSzarpConfig::GetTitle()
{
    return title;
}

const std::wstring&
TSzarpConfig::GetDocumentationBaseURL()
{
    return documentation_base_url;
}

void TSzarpConfig::SetName(const std::wstring& title, const std::wstring& prefix)
{
    this->title=title;
    this->prefix=prefix;
}

const std::wstring& 
TSzarpConfig::GetPrefix() const
{
    return prefix;
}

void
TSzarpConfig::AddDefined(TParam * p)
{
    assert(p != NULL);
    if (defined == NULL) {
	defined = p;
    } else {
	defined->Append(p);
    }
}

void
TSzarpConfig::AddDrawDefinable(TParam * p)
{
    assert(p != NULL);
    if (drawdefinable == NULL) {
	drawdefinable = p;
    } else {
	drawdefinable->Append(p);
    }
}

void TSzarpConfig::RemoveDrawDefinable(TParam *p)
{
	if (drawdefinable == p) {
		drawdefinable = drawdefinable->GetNext(false);
		return;
	}


	TParam *pv = drawdefinable, *c = drawdefinable->GetNext(false); 
	while (c && c != p) {
		pv = c;
		c = c->GetNext(false);
	}

	if (c)
		pv->SetNext(c->GetNext(false));
}

TParam *
TSzarpConfig::getParamByIPC(int ipc_ind)
{
    TParam *p = GetFirstParam();
    if (p == NULL)
	return NULL;
    return p->GetNthParam(ipc_ind, true);
}

xmlDocPtr
TSzarpConfig::generateXML(void)
{
    using namespace SC;
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define BUF X(buffer)
    char buffer[10];
    xmlDocPtr doc;
    xmlNodePtr node;

    doc = xmlNewDoc(X "1.0");
    doc->children = xmlNewDocNode(doc, NULL, X "params", NULL);
    xmlNewNs(doc->children, SC::S2U(IPK_NAMESPACE_STRING).c_str(), NULL);
    xmlSetProp(doc->children, X "version", X "1.0");
    ITOA(read_freq);
    xmlSetProp(doc->children, X "read_freq", BUF);
    ITOA(send_freq);
    xmlSetProp(doc->children, X "send_freq", BUF);
    if (!title.empty())
	xmlSetProp(doc->children, X "title", S2U(title).c_str());
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
	xmlAddChild(doc->children, d->generateXMLNode());
    if (defined > 0) {
	node = xmlNewChild(doc->children, NULL, X "defined", NULL);
	for (TParam * p = defined; p; p = p->GetNext())
	    xmlAddChild(node, p->generateXMLNode());
    }
    if (drawdefinable) {
	node = xmlNewChild(doc->children, NULL, X "drawdefinable", NULL);
	for (TParam * p = drawdefinable; p; p = p->GetNext())
	    xmlAddChild(node, p->generateXMLNode());
    }
    if (seasons) {
	node = seasons->generateXMLNode();
	xmlAddChild(doc->children, node);
    }
    return doc;
#undef X
#undef ITOA
#undef BUF
}

int
TSzarpConfig::saveXML(const std::wstring &path)
{
    xmlDocPtr d;
    int r;

    d = generateXML();
    if (d == NULL)
	return -1;
    r = xmlSaveFormatFile(SC::S2A(path).c_str(), d, 1);
    xmlFreeDoc(d);
    return r;
}

int
TSzarpConfig::loadXMLDOM(const std::wstring& path, const std::wstring& prefix) {

	xmlDocPtr doc;
	int ret;

	this->prefix = prefix;

	xmlLineNumbersDefault(1);
	doc = xmlParseFile(SC::S2A(path).c_str());
	if (doc == NULL) {
		sz_log(1, "XML document not wellformed\n");
		return 1;
	}

	ret = parseXML(doc);
	xmlFreeDoc(doc);

	return ret;
}

int
TSzarpConfig::loadXMLReader(const std::wstring &path, const std::wstring& prefix)
{
	this->prefix = prefix;
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
			sz_log(1, "XML document not wellformed, look at previous logs\n");
			ret = 1;
		}
	}
	else {
		sz_log(1, "XML document not wellformed\n");
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

	if( logparams ) CreateLogParams();

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

	nativeLanguage = L"pl";

	for (;;) {
		if (xw.IsTag("params")) {
			if (xw.IsBeginTag()) {

				const char* need_attr_params[] = { "read_freq" , "send_freq", "version", 0 };
				if (!xw.AreValidAttr(need_attr_params)) {
					throw XMLWrapperException();
				}

				for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
					const xmlChar *attr = xw.GetAttr();
					try {
						if (xw.IsAttr("read_freq")) {
							read_freq = boost::lexical_cast<int>(attr);
							if (read_freq <= 0)
								xw.XMLError("read_freq attribute <= 0");
						} else
						if (xw.IsAttr("send_freq")) {
							send_freq = boost::lexical_cast<int>(attr);
							if (send_freq <= 0)
								xw.XMLError("send_freq attribute <= 0");
						} else
						if (xw.IsAttr("version")) {
							if (!xmlStrEqual(attr, (unsigned char*) "1.0")) {
								xw.XMLError("incorrect version (1.0 expected)");
							}
						} else
						if (xw.IsAttr("title")) {
							title = SC::U2S(attr);
						} else
						if (xw.IsAttr("language")) {
							nativeLanguage = SC::U2S(attr);
						} else
						if (xw.IsAttr("documentation_base_url")) {
							documentation_base_url = SC::U2S(attr);
						} else
						if (xw.IsAttr("xmlns")) {
						} else {
							xw.XMLWarningNotKnownAttr();
						}
					} catch (boost::bad_lexical_cast &) {
						xw.XMLErrorWrongAttrValue();
					}
				} 

			} else {
				break;
			}
		} else
		if (xw.IsTag("device")) {
			if (xw.IsBeginTag()) {
				if (devices == NULL)
					devices = td = createDevice();
				else
					td = td->Append(createDevice());
				assert(devices != NULL);
				td->parseXML(reader);
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
    using namespace SC;

#define NEED(p, n) \
	if (!p) { \
	sz_log(1, "XML parsing error: expected '%s' (line %ld)", n, \
			xmlGetLineNo(p)); \
		return 1; \
	} \
	if (strcmp((const char*)p->name, n)) { \
	sz_log(1, "XML parsing error: expected '%s', found '%s' \
 (line %ld)", \
			n, U2A(p->name).c_str(), xmlGetLineNo(p)); \
		return 1; \
	}
#define NOATR(p, n) \
	{ \
	sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, U2A(p->name).c_str(), xmlGetLineNo(p)); \
			return 1; \
	}
#define NEEDATR(p, n) \
	if (c) free(c); \
	c = xmlGetNoNsProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (xmlChar *)

    unsigned char *c = NULL;
    int i;
    int ret = 1;
    xmlChar *_ps_addres, *_ps_port, *_documenation_base_url;
    TDevice *td = NULL;
    TParam *p = NULL;
    xmlNodePtr node, ch;

    node = doc->children;
    NEED(node, "params");
    NEEDATR(node, "version");

    unsigned char* _language = xmlGetNoNsProp(node, X "language");
    if (_language) {
	nativeLanguage = SC::U2S(_language);
	xmlFree(_language);
    } else
    	nativeLanguage = L"pl";
	 

    unsigned char* _title = xmlGetNoNsProp(node, X "title");
    title = U2S(_title);
    xmlFree(_title);

    if (strcmp((char*)c, "1.0")) {
sz_log(1, "XML parsing error: incorrect version (1.0 expected) (line %ld)", xmlGetLineNo(node));
	goto at_end;
    }

    _documenation_base_url = xmlGetNoNsProp(node, X "documentation_base_url");
    if (_documenation_base_url) {
	    documentation_base_url = U2S(_documenation_base_url);
	    xmlFree(_documenation_base_url);
    }

    _ps_addres =  xmlGetNoNsProp(node, X "ps_address");
    _ps_port = xmlGetNoNsProp(node, X "ps_port");
    if (_ps_addres)
	    ps_address =  U2S(_ps_addres);
    if (_ps_port)
	    ps_port = U2S(_ps_port);
    xmlFree(_ps_addres);
    xmlFree(_ps_port);

    NEEDATR(node, "read_freq");
    if ((i = atoi((char*)c)) <= 0) {
sz_log(1, "XML file error: read_freq attribute <= 0 (line %ld)", xmlGetLineNo(node));
	goto at_end;
    }
    read_freq = i;
    NEEDATR(node, "send_freq");
    if ((i = atoi((char*)c)) <= 0) {
sz_log(1, "XML file error: send_freq attribute <= 0 (line %ld)", xmlGetLineNo(node));
	goto at_end;
    }
    free(c);
    c = NULL;
    send_freq = i;

    assert(devices == NULL);
    assert(defined == NULL);

    for (i = 0, ch = node->children; ch; ch = ch->next) {
	if (!strcmp((char *) ch->name, "device")) {
	    if (devices == NULL)
		devices = td = createDevice();
	    else
		td = td->Append(createDevice());
	    assert(devices != NULL);
	    if (td->parseXML(ch))
		goto at_end;
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
			goto at_end;
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
			goto at_end;
		}
	}
	else if (!strcmp((char *)ch->name, "seasons")) {
		seasons = new TSSeason();
		if (seasons->parseXML(ch)) {
			delete seasons;
			seasons = NULL;
			goto at_end;
		}
	}
    }
    if (devices == NULL) {
sz_log(1, "XML file error: 'device' elements not found");
	goto at_end;
    }

    if (seasons == NULL)
	    seasons = new TSSeason();

    ret = 0;
  at_end:
    return ret;
#undef X
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
			sz_log(0,"Invalid drawdefinable formula %s", SC::S2L(p->GetName()).c_str());
		}
	p = p->GetNext();
    }

    return 0;
}

TParam *
TSzarpConfig::getParamByName(const std::wstring& name)
{

    if (use_names_cache) {
	std::map<std::wstring, TParam *>::iterator it = params_map.find(name);
	if (it == params_map.end())
	    return NULL;
	return it->second;
    }

    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	if (!p->GetName().empty() && !name.empty() && p->GetName() == name)
	    return p;

    for (TParam * p = drawdefinable; p; p = p->GetNext())
	if (!p->GetName().empty() && !name.empty() && p->GetName() == name)
	    return p;

    return NULL;
}

TParam *
TSzarpConfig::getParamByBaseInd(int base_ind)
{
    TParam *p;
    int max, i;

    for (p = GetFirstParam(); p; p = GetNextParam(p)) {
	if (p->GetBaseInd() == base_ind) {
	    return p;
	}
    }
    /* draw definables have base indexes starting from max */
    max = GetParamsCount() + GetDefinedCount();
    for (i = max, p = drawdefinable; p; p = p->GetNext(), i++) {
	if (i == base_ind) {
	    return p;
	}
    }
    return NULL;
}

std::wstring
TSzarpConfig::absoluteName(const std::wstring &name, const std::wstring &ref)
{
    int i;
    std::wstring w1, w2;

    assert(!name.empty());
    assert(!ref.empty());
    w1 = name;
    w2 = ref;
    if (name.length() > 4 && name.substr(0, 4) == L"*:*:") {
	i = w2.rfind(':');
	w1.replace(0, 3, w2.substr(0, i));
    } else if (name.length() > 2 && name.substr(0, 2) == L"*:") {
	i = w2.find(':');
	w1.replace(0, 1, w2.substr(0, i));
    }
    return w1;
}

int
TSzarpConfig::AddDefined(const std::wstring &formula)
{
    TParam *p = new TParam(NULL, this, formula);

    assert(p != NULL);
    p->SetFormulaType(TParam::RPN);
    if (defined == NULL)
	defined = p;
    else
	defined->Append(p);
    return GetParamsCount() + GetDefinedCount() - 1;
}

int
TSzarpConfig::GetMaxBaseInd()
{
    int m = -1, n;
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p)) {
	n = p->GetBaseInd();
	if (n > m)
	    m = n;
    }
    return m;
}

void
TSzarpConfig::CreateLogParams()
{
	TDevice* d = createDevice(L"/opt/szarp/bin/logdmn");
	TRadio * r = createRadio ( d );
	TUnit  * u = createUnit( r );
	TParam * p;
	TDraw  * w;

	   AddDevice( d );
	d->AddRadio ( r );
	r->AddUnit  ( u );

	std::wstringstream wss;
	for( int i=0 ; i<LOG_PARAMS_NUMBER ; ++i )
	{
		p = new TParam ( u );
		p->Configure( LOG_PARAMS[i][0] ,
		              LOG_PARAMS[i][1] ,
		              LOG_PARAMS[i][2] ,
		              L"-" , NULL , 0 , -1 , 1 );

		if( i % 12 == 0 ) {
			wss.str(L"");
			wss << LOG_PREFIX << " " << i/12 + 1;
		}

		w = new TDraw( L"" , wss.str() , -1 , i , 0 , 100 , 1 , .5 , 2 );

		u->AddParam( p );
		p->AddDraw ( w );
	}
}

TParam *
TSzarpConfig::GetFirstParam()
{
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
	for (TRadio * r = d->GetFirstRadio(); r; r = d->GetNextRadio(r))
	    for (TUnit * u = r->GetFirstUnit(); u; u = r->GetNextUnit(u)) {
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

TParam *
TSzarpConfig::GetNextParam(TParam * p)
{
    return (NULL == p ? NULL : p->GetNext(true));
}

TDevice *
TSzarpConfig::AddDevice(TDevice * d)
{
    assert(d != NULL);
    if (devices == NULL)
	devices = d;
    else
	devices->Append(d);
    return d;
}

TDevice *
TSzarpConfig::GetNextDevice(TDevice * d)
{
    if (d == NULL)
	return NULL;
    return d->GetNext();
}

int
TSzarpConfig::GetParamsCount()
{
    int i = 0;
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
	i += d->GetParamsCount();
    return i;
}

int
TSzarpConfig::GetDevicesCount()
{
    int i = 0;
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
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
    for (TParam * p = drawdefinable; p; p = p->GetNext())
	i++;
    return i;
}

int
TSzarpConfig::GetAllDefinedCount()
{
    int i = 0;
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	if ((!p->GetFormula().empty() || !p->GetParentUnit()) && (p->GetFormulaType() != TParam::DEFINABLE))
	    i++;
    return i;
}

TDevice *
TSzarpConfig::DeviceById(int id)
{
    TDevice *d;
    for (d = GetFirstDevice(); d && (id > 1); d = GetNextDevice(d), id--);
    return d;
}

std::wstring 
TSzarpConfig::GetFirstRaportTitle()
{
    std::wstring title;
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (title.empty() || r->GetTitle().compare(title) < 0)
		title = r->GetTitle();
    return title;
}

std::wstring
TSzarpConfig::GetNextRaportTitle(const std::wstring &cur)
{
    std::wstring title;
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (r->GetTitle().compare(cur) > 0 && (title.empty() || r->GetTitle().compare(title) < 0))
		title = r->GetTitle();
    return title;
}

TRaport *
TSzarpConfig::GetFirstRaportItem(const std::wstring& title)
{
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (title == r->GetTitle())
		return r;
    return NULL;
}

TRaport *
TSzarpConfig::GetNextRaportItem(TRaport * cur)
{
    std::wstring title = cur->GetTitle();
    for (TRaport * r = cur->GetNext(); r; r = r->GetNext())
	if (title == r->GetTitle())
	    return r;
    for (TParam * p = GetNextParam(cur->GetParam()); p; p = GetNextParam(p))
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (title == r->GetTitle())
		return r;
    return NULL;
}

bool TSzarpConfig::checkConfiguration()
{
	bool ret = true;
	ret = checkRepetitions(false) && ret;
	ret = checkFormulas()         && ret;
	ret = checkSend()             && ret;
	return ret;
}

bool TSzarpConfig::checkFormulas()
{
	bool ret = true;
	bool lua_syntax_ok = true;
	try {
		PrepareDrawDefinable();
	} catch( TCheckException& e) {
		return false;
	}

	/** This loop checks every formula and return false if any is invalid */
	for( TParam* p=GetFirstParam(); p ; p=GetNextParam(p) )
		try {
			p->GetParcookFormula();
		} catch( TCheckException& e ) {
			ret = false;
		}
	for( TParam* p=GetFirstParam(); p ; p=GetNextParam(p) )
		try {
			p->GetDrawFormula();
		} catch( TCheckException& e ) {
			ret = false;
		}

	/** This loop checks every formula for lua syntax */
	for(TParam* p = GetFirstParam(); p; p=GetNextParam(p)) {
		if(p->GetLuaScript() && !checkLuaSyntax(p)){
			lua_syntax_ok = false;
		}
	}

	/** This loop check drawdefinables formulas for SZARP optimalization if lua syntax was correct */
	if(lua_syntax_ok)	{
		IPKContainer::Init(L"/opt/szarp/", L"/opt/szarp/", L"pl");
		for(TParam* p = GetFirstDrawDefinable(); p; p = p->GetNext(false)) {
			if(p->GetLuaScript() && !optimizeLuaParam(p)) {
					ret = false;
			}
		}
	}

	ret = ret && lua_syntax_ok;
	return ret;
}

bool TSzarpConfig::checkLuaSyntax(TParam *p)
{
	lua_State* lua = Lua::GetInterpreter();
	if (compileLuaFormula(lua, (const char*) p->GetLuaScript(), (const char*)SC::S2U(p->GetName()).c_str()))
		return true;
	else {
		sz_log(1, "Error compiling param %s: %s\n", SC::S2U(p->GetName()).c_str(), lua_tostring(lua, -1));
		return false;
	}
}

bool TSzarpConfig::compileLuaFormula(lua_State *lua, const char *formula, const char *formula_name)
{
	std::ostringstream paramfunction;

	using std::endl;

	paramfunction <<
	"return function ()"	<< endl <<
	"	local p = szbase"	<< endl <<
	"	local PT_MIN10 = ProbeType.PT_MIN10" << endl <<
	"	local PT_HOUR = ProbeType.PT_HOUR" << endl <<
	"	local PT_HOUR8 = ProbeType.PT_HOUR8" << endl <<
	"	local PT_DAY = ProbeType.PT_DAY"	<< endl <<
	"	local PT_WEEK = ProbeType.PT_WEEK" << endl <<
	"	local PT_MONTH = ProbeType.PT_MONTH" << endl <<
	"	local PT_YEAR = ProbeType.PT_YEAR" << endl <<
	"	local PT_CUSTOM = ProbeType.PT_CUSTOM"	<< endl <<
	"	local szb_move_time = szb_move_time" << endl <<
	"	local state = {}" << endl <<
	"	return function (t,pt)" << endl <<
	"		local v = nil" << endl <<
	formula << endl <<
	"		return v" << endl <<
	"	end" << endl <<
	"end"	<< endl;

	std::string str = paramfunction.str();

	const char* content = str.c_str();

	int ret = luaL_loadbuffer(lua, content, std::strlen(content), formula_name);
	if (ret != 0)
		return false;

	ret = lua_pcall(lua, 0, 1, 0);
	if (ret != 0)
		return false;

	return true;
}

bool TSzarpConfig::optimizeLuaParam(TParam* p) {
	LuaExec::SzbaseParam* ep = new LuaExec::SzbaseParam;
	p->SetLuaExecParam(ep);

	IPKContainer* container = IPKContainer::GetObject();
	if (LuaExec::optimize_lua_param(p, container))
		return true;
	return false;
}

bool TSzarpConfig::checkRepetitions(int quiet)
{
	std::vector<std::wstring> str;
	int all_repetitions_number = 0, the_same_repetitions_number=1;

	for (TParam* p = GetFirstParam(); p; p = GetNextParam(p)) {
		str.push_back(p->GetSzbaseName());
	}
	std::sort(str.begin(), str.end());

	for( size_t j=0 ; j<str.size() ; ++j )
	{
		if( j<str.size()-1 && str[j] == str[j+1] )
			++the_same_repetitions_number;
		else if( the_same_repetitions_number > 1 ) {
			if( !quiet )
				sz_log(1, "There are %d repetitions of: %s", the_same_repetitions_number, SC::S2L(str[j-1]).c_str());

			all_repetitions_number += the_same_repetitions_number;
			the_same_repetitions_number = 1;
		}
	}

	return all_repetitions_number == 0;
}

bool TSzarpConfig::checkSend()
{
	bool ret = true;
	for( TDevice* d = GetFirstDevice(); d; d = GetNextDevice(d) )
		for( TRadio* r = d->GetFirstRadio(); r; r = d->GetNextRadio(r) )
			for( TUnit* u = r->GetFirstUnit(); u; u = r->GetNextUnit(u) )
				for( TSendParam* sp = u->GetFirstSendParam(); sp; sp = u->GetNextSendParam(sp) )
				{
					if( !sp->IsConfigured() || sp->GetParamName().empty() )
						continue;

					TParam* p = getParamByName(sp->GetParamName()); 
					if( p == NULL ) {
						sz_log(1, "Cannot find parameter to send (%s)", SC::S2A(sp->GetParamName()).c_str());
						ret = false;
					} else if( p->IsDefinable() ) {
						sz_log(1, "Cannot use drawdefinable param in send (%s)", SC::S2A(sp->GetParamName()).c_str());
						ret = false;
					}
				}
	return ret;
}


void TSzarpConfig::ConfigureSeasonsLimits() {
	if (seasons == NULL)
		seasons = new TSSeason();
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

