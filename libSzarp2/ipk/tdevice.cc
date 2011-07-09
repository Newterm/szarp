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
 * IPK 'device' class implementation.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <string>
#include <dirent.h>
#include <assert.h>

#include "conversion.h"
#include "szarp_config.h"
#include "parcook_cfg.h"
#include "line_cfg.h"
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "liblog.h"

using namespace std;

#define FREE(x)	if (x != NULL) free(x)

int TDevice::parseXML(xmlNodePtr node)
{
#define X (xmlChar *)
	xmlChar *c;
	int i;
	xmlNodePtr ch;
	
	c = xmlGetNoNsProp(node, X"daemon");
	if (!c)
		goto inside;
	daemon = SC::U2S(c);
	xmlFree(c);

	c = xmlGetNoNsProp(node, X"options");
	if (c) {
		options = SC::U2S(c);
		xmlFree(c);
	}
	c = xmlGetNoNsProp(node, X"path");
	if (!c)
		goto inside;
	path = SC::U2S(c);
	xmlFree(c);
	c = xmlGetNoNsProp(node, X"speed");
	if (!c)
		goto inside;
	speed = atoi((char*) c);
	xmlFree(c);
	c = xmlGetNoNsProp(node, X"stop");
	if (!c)
		goto inside;
	stop = atoi((char*) c);
	xmlFree(c);
	c = xmlGetNoNsProp(node, X"protocol");
	if (!c)
		goto inside;
	protocol = atoi((char*)(c));
	xmlFree(c);
inside:
	c = xmlGetNoNsProp(node, X"special");
	if (c) {
		special = 1;
		special_value = atoi((char*)c);
		xmlFree(c);
	}
	for (i = 0, ch = node->children; ch; ch = ch->next)
		if (!strcmp((char *)ch->name, "radio"))
			i++;
	if (i <= 0) {
		radios = new TRadio(this);
		assert (radios != NULL);
		return radios->parseXML(node);
	}
	if (special) {
		sz_log(1, "XML file error - cannot set attribute 'special' for \
device with radio modems (line %ld)",
			xmlGetLineNo(node));
		return 1;
	}
	radios = NULL;
	TRadio* r = NULL;
	for (i = 0, ch = node->children; ch; ch = ch->next)
		if (!strcmp((char *)ch->name, "radio")) {
			if (radios == NULL) {
				r = radios = new TRadio(this);
			} else {
				r = r->Append(new TRadio(this));
			}
			assert(r != NULL);
			if (r->parseXML(ch))
				return 1;
		}
	return 0;
#undef X 
}


int TDevice::parseXML(xmlTextReaderPtr reader)
{
	TRadio* r = NULL;

	XMLWrapper xw(reader);

	const char* need_attr[] = { "daemon", 0 };
	if (!xw.AreValidAttr(need_attr)) {
		throw XMLWrapperException();
	}

	for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
		const xmlChar *attr = xw.GetAttr();

		if (xw.IsAttr("daemon")) {
			daemon = SC::U2S(attr);
		} else
		if (xw.IsAttr("path")) {
			path = SC::U2S(attr);
		} else
		if (xw.IsAttr("speed")) {
			speed = boost::lexical_cast<int>(attr);
		} else
		if (xw.IsAttr("stop")) {
			stop = boost::lexical_cast<int>(attr);
		} else
		if (xw.IsAttr("protocol")) {
			protocol = boost::lexical_cast<int>(attr);
		} else
		if (xw.IsAttr("special")) {
			special = 1;
			special_value = boost::lexical_cast<int>(attr);
		} else
		if (xw.IsAttr("options")) {
			options = SC::U2S(attr);
		} else {
			xw.XMLWarningNotKnownAttr();
		}
	}

	xw.NextTag();

	for (;;) {
		if (xw.IsTag("radio")) {
			if (xw.IsBeginTag()) {
				if (radios == NULL) {
					r = radios = new TRadio(this);
				} else {
					r = r->Append(new TRadio(this));
				}
				assert (r != NULL);
				if (r->parseXML(reader))
					return 1;
			}
			xw.NextTag();
		} else
		if (xw.IsTag("unit")) {
			if (xw.IsBeginTag()) {
				if (radios == NULL) {
					r = radios = new TRadio(this);
				} else {
					r = r->Append(new TRadio(this));
				}
				assert (r != NULL);
				if (r->parseXML(reader))
					return 1;
			}
		break;
//			xw.NextTag(); // don't take next tag - <unit> set it
		} else
		if (xw.IsTag("device")) {
			break;
		}
		else {
			xw.XMLErrorNotKnownTag("device");
		}
	}

	return 0;
}

TUnit* TDevice::searchUnitById(const wchar_t id, int uniq)
{
	TUnit *u = NULL;
	TRadio *r;
	
	for (r = GetFirstRadio(); r; r = GetNextRadio(r)) {
		for (u = r->GetFirstUnit(); u; u = r->GetNextUnit(u))
			if (u->GetId() == id)
				goto out;
	}
out:
	if (u == NULL)
		return NULL;
	if (uniq == 0)
		return u;
	for (; r; r = GetNextRadio(r))
		for (TUnit* u2 = r->GetNextUnit(u); u2; u2 = r->GetNextUnit(u2))
			if (u2->GetId() == id)
				return NULL;
	return u;
}

TParam *TDevice::getParamByNum(int num)
{
	TParam* p = GetFirstRadio()->GetFirstUnit()->GetFirstParam()->
			GetNthParam(num);
	if (p == NULL)
		return NULL;
	if (p->GetDevice() == this)
		return p;
	return NULL;
}

xmlNodePtr TDevice::generateXMLNode(void)
{
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define BUF X(buffer)
	char buffer[10];
	xmlNodePtr r;
	
	r = xmlNewNode(NULL, (unsigned char*)"device");
	if (!daemon.empty())
		xmlSetProp(r, X"daemon", SC::S2U(daemon).c_str());
	if (!path.empty())
		xmlSetProp(r, X"path", SC::S2U(path).c_str());
	if (speed > 0) {
		ITOA(speed);
		xmlSetProp(r, X"speed", BUF);
	}
	if (protocol >= 0) {
		ITOA(protocol);
		xmlSetProp(r, X"protocol", BUF);
	}
	if (special) {
		ITOA(special_value);
		xmlSetProp(r, X"special", BUF);
	}
	if (!options.empty()) 
		xmlSetProp(r, X"options", SC::S2U(options).c_str());
	if (GetFirstRadio()->GetId())
		for (TRadio* rd = GetFirstRadio(); rd; rd = GetNextRadio(rd)) 
			xmlAddChild(r, rd->generateXMLNode());
	else
		for (TUnit* u = GetFirstRadio()->GetFirstUnit(); u;
				u = GetFirstRadio()->GetNextUnit(u))
			xmlAddChild(r, u->generateXMLNode());
	return r;
#undef X
#undef ITOA
#undef BUF
}

TRadio* TDevice::AddRadio(TRadio *r)
{
	assert (r != NULL);
	if (radios == NULL)
		radios = r;
	else
		radios->Append(r);
	return r;
}

TRadio* TDevice::GetNextRadio(TRadio* r)
{
	if (r == NULL)
		return NULL;
	return r->GetNext();
}

TDevice* TDevice::Append(TDevice* d)
{
	TDevice* t = this;
	while (t->next)
		t = t->next;
	t->next = d;
	return d;
}

int TDevice::GetNum()
{
	int i;
	TDevice* d;
	TSzarpConfig* s = GetSzarpConfig();
	assert (s != NULL);
	for (i = 0, d = s->GetFirstDevice(); d && (d != this); i++,
			d = s->GetNextDevice(d));
	assert(d != NULL);
	return i;
}

int TDevice::GetParamsCount()
{
	int i = 0;
	for (TRadio* r = GetFirstRadio(); r; r = GetNextRadio(r))
	for (TUnit* u = r->GetFirstUnit(); u; u = r->GetNextUnit(u))
		i += u->GetParamsCount();
	return i;
}

int TDevice::GetRadiosCount()
{
	int i = 0;
	for (TRadio* r = GetFirstRadio(); r; r = GetNextRadio(r))
		i++;
	return i;
}

TDevice::~TDevice()
{
	delete next;
	delete radios;
}

