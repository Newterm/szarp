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
 * IPK 'unit' class implementation.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <string>
#include <sstream>
#include <dirent.h>
#include <assert.h>

#include "szarp_config.h"
#include "parcook_cfg.h"
#include "conversion.h"
#include "line_cfg.h"
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "liblog.h"

using namespace std;

#define FREE(x)	if (x != NULL) free(x)

int TUnit::parseXML(xmlTextReaderPtr reader)
{
	TAttribHolder::parseXML(reader);
	TParam* p = NULL;
	TSendParam *sp = NULL;
	params = NULL;
	sendParams = NULL;

	XMLWrapper xw(reader);

	const char* need_attr[] = { "id", 0 };
	if (!xw.AreValidAttr(need_attr)) {
		throw XMLWrapperException();
	}


	if (!xw.NextTag())
		return 1;

	for (;;) {
		if (xw.IsTag("param")) {
			if (xw.IsBeginTag() || ( xw.IsEndTag() && !xw.IsEmptyTag()) ) {
				if (params == NULL)
					p = params = GetSzarpConfig()->createParam(this);
				else
					p = p->Append(GetSzarpConfig()->createParam(this));
				if (p->parseXML(reader))
					return 1;
			}
		} else
		if (xw.IsTag("unit")) {
			break;
		} else
		if (xw.IsTag("send")) {
			if (xw.IsBeginTag()) {
				if (sendParams == NULL)
					sp = sendParams = new TSendParam(this);
				else
					sp = sp->Append(new TSendParam(this));
				if (sp->parseXML(reader))
					return 1;
			}
		}
		else {
			xw.XMLErrorNotKnownTag("unit");
		}
		if (!xw.NextTag())
			return 1;
	}

	return 0;
}

int TUnit::parseXML(xmlNodePtr node)
{
	TAttribHolder::parseXML(node);

	TParam* p = NULL;
	TSendParam* sp = NULL;
	params = NULL;
	sendParams = NULL;
	for (auto ch = node->children; ch; ch = ch->next) {
		if (!strcmp((char *)ch->name, "param")) {
			if (params == NULL)
				p = params = GetSzarpConfig()->createParam(this);
			else
				p = p->Append(GetSzarpConfig()->createParam(this));
			if (p->parseXML(ch))
				return 1;
		} else if (!strcmp((char *)ch->name, "send")) {
			if (sendParams == NULL)
				sp = sendParams = new TSendParam(this);
			else
				sp = sp->Append(new TSendParam(this));
			if (sp->parseXML(ch))
				return 1;
		}
	}
	return 0;
}

TDevice* TUnit::GetDevice() const
{
	return parentDevice;
}

SzarpConfigInfo* TUnit::GetSzarpConfig() const
{
	return GetDevice()->GetSzarpConfig();
}

size_t TUnit::GetParamsCount() const
{
	int i;
	TParam* p;
	for (i = 0, p = params; p; i++, p = p->GetNext());
	return i;
}

size_t TUnit::GetSendParamsCount() const
{
	int i;
	TSendParam* s;
	for (i = 0, s = sendParams; s; i++, s = s->GetNext());
	return i;
}

void TUnit::AddParam(TParam* p)
{
	if (params == NULL)
		params = p;
	else
		params->Append(p);
}

void TUnit::AddParam(TSendParam* s)
{
	if (sendParams == NULL)
		sendParams = s;
	else
		sendParams->Append(s);
}

wchar_t TUnit::GetId() const
{
	return SC::A2S(getAttribute("id"))[0];
}


TUnit::~TUnit()
{
	auto next = GetNext();
	if (next) delete next;

	delete params;
	delete sendParams;
}

