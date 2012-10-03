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
 * IPK 'radio' class implementation.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <sstream>
#include <string>
#include <dirent.h>
#include <assert.h>

#include "szarp_config.h"
#include "conversion.h"
#include "parcook_cfg.h"
#include "line_cfg.h"
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "liblog.h"

using namespace std;

#define FREE(x)	if (x != NULL) free(x)


TUnit* TRadio::unitById(wchar_t id)
{
	for (TUnit* u = GetFirstUnit(); u; u = GetNextUnit(u))
		if (u->GetId() == id)
			return u;
	return NULL;
}

int TRadio::parseXML(xmlTextReaderPtr reader)
{
	assert(units == NULL);
	TUnit* u = NULL;

	XMLWrapper xw(reader);

	const char* ignored_trees[] = { "rate:period", 0 };
	xw.SetIgnoredTrees(ignored_trees);

	for (;;) {

		if (xw.IsTag("radio")) {
			if (xw.IsBeginTag()) {
				const char *need_attr[] = { "id", 0 };
				if (!xw.AreValidAttr(need_attr)) {
					throw XMLWrapperException();
				}

				for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
					const xmlChar *attr = xw.GetAttr();

					if (xw.IsAttr("id")) {
						id = SC::U2S(attr)[0];
					} else {
						xw.XMLWarningNotKnownAttr();
					}
				}
					xw.NextTag();
			} else {
				break;
			}
		} else
		if (xw.IsTag("unit")) {
			if (xw.IsBeginTag()) {
				if (units == NULL)
					units = u = new TUnit(this);
				else {
					u = u->Append(new TUnit(this));
				}
				assert(u != NULL);
				if (u->parseXML(reader))
					return 1;
			}
			xw.NextTag();
		} else
		if (xw.IsTag("device")) {
			if (xw.IsEndTag()) {
				break;
			} else
				assert(0 ==1  && "workaound parser problem");
		} else {
			xw.XMLErrorNotKnownTag("radio");
		}
	}

	if (units == NULL) {
		xw.XMLError("no 'unit' elements found in 'device' element");
	}

	return 0;
}

int TRadio::parseXML(xmlNodePtr node)
{
	unsigned char *c = NULL;
	xmlNodePtr ch;
	
#define NOATR(p, n) \
	{ \
		sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, SC::U2A(p->name).c_str(), xmlGetLineNo(p)); \
			return 1; \
	}
#define NEEDATR(p, n) \
	if (c) free(c); \
	c = xmlGetNoNsProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (xmlChar *)
	if (!strcmp((char *)node->name, "radio")) {
		NEEDATR(node, "id");
		id = SC::U2S(c)[0];
	}
	assert(units == NULL);
	TUnit* u = NULL;
	for (ch = node->children; ch; ch = ch->next)
		if (!strcmp((char *)ch->name, "unit")) {
			if (units == NULL)
				units = u = new TUnit(this);
			else {
				u = u->Append(new TUnit(this));
			}
			assert(u != NULL);
			if (u->parseXML(ch))
				return 1;
		}

	if (c) {
		xmlFree(c);
	}

	if (units == NULL) {
		sz_log(1, "XML file error: no 'unit' elements found in 'device' \
element (line %ld)",
			xmlGetLineNo(node));
		return 1;
	}
	return 0;
#undef NEEDATR
#undef NOATR
#undef X
}

xmlNodePtr TRadio::generateXMLNode(void)
{
#define X (unsigned char *)
	xmlNodePtr r;

	r = xmlNewNode(NULL, X"radio");

	std::wstringstream wss;
	wss << id;

	xmlSetProp(r, X"id", SC::S2U(wss.str()).c_str());
	for (TUnit* u = GetFirstUnit(); u; u = GetNextUnit(u))
		xmlAddChild(r, u->generateXMLNode());
	return r;
#undef X
}

TUnit* TRadio::GetNextUnit(TUnit* u)
{
	if (u == NULL)
		return NULL;
	return u->GetNext();
}

TUnit* TRadio::AddUnit(TUnit* u)
{
	assert (u != NULL);
	if (units == NULL)
		units = u;
	else
		units->Append(u);
	return u;
}

TRadio* TRadio::Append(TRadio* r)
{
	TRadio* t = this;
	while (t->next)
		t = t->next;
	t->next = r;
	return r;
}

int TRadio::GetUnitsCount()
{
	int i = 0;
	for (TUnit* u = GetFirstUnit(); u; u = GetNextUnit(u))
		i++;
	return i;
}

TRadio::~TRadio()
{
	delete next;
	delete units;
}

