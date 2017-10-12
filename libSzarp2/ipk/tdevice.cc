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
#include "libpar.h"


using namespace std;

TDevice::TDevice(TSzarpConfig *parent):
	parentSzarpConfig(parent),
	units(NULL)
{}
#define FREE(x)	if (x != NULL) free(x)

int TDevice::parseXML(xmlNodePtr node)
{
	TAttribHolder::parseXML(node);

	units = NULL;
	TUnit* u = NULL;
	for (auto ch = node->children; ch; ch = ch->next)
		if (!strcmp((char *)ch->name, "unit")) {
			if (units == NULL) {
				u = units = parentSzarpConfig->createUnit(this);
			} else {
				u = u->Append(parentSzarpConfig->createUnit(this));
			}
			assert(u != NULL);
			u->parseXML(ch);
		}
	return 0;
}


int TDevice::parseXML(xmlTextReaderPtr reader)
{
	TAttribHolder::parseXML(reader);
	TUnit* u = NULL;
	units = NULL;

	XMLWrapper xw(reader);

	const char* need_attr[] = { "daemon", 0 };
	if (!xw.AreValidAttr(need_attr)) {
		throw XMLWrapperException();
	}

	if (!xw.NextTag())
		throw std::runtime_error("Unexpected end of tags in device");

	for (;;) {
		if (xw.IsTag("unit")) {
			if (xw.IsBeginTag()) {
				if (units == NULL) {
					u = units = parentSzarpConfig->createUnit(this);
				} else {
					u = u->Append(parentSzarpConfig->createUnit(this));
				}
				assert (u != NULL);
				u->parseXML(reader);
			}
		} else
		if (xw.IsTag("device")) {
			break;
		}
		else {
			xw.XMLErrorNotKnownTag("device");
		}

		if (!xw.NextTag())
			throw std::runtime_error("Unexpected end of tags in device");
	}

	return 0;
}

TParam *TDevice::getParamByNum(int num)
{
	TParam* p = GetFirstUnit()->GetFirstParam()->
			GetNthParam(num);
	if (p == NULL)
		return NULL;
	if (p->GetDevice() == this)
		return p;
	return NULL;
}

TUnit* TDevice::AddUnit(TUnit* u)
{
	assert (u != NULL);
	if (units == NULL) {
		units = u;
	} else {
		units->Append(u);
	}
	return u;
}

int TDevice::GetUnitsCount()
{
	int i = 0;
	for (TUnit* u = GetFirstUnit(); u; u = u->GetNext())
		i++;
	return i;
}

int TDevice::GetParamsCount()
{
	int i = 0;
	for (auto u = GetFirstUnit(); u != nullptr; u = u->GetNext())
		i += u->GetParamsCount();
	return i;
}

TDevice::~TDevice()
{
	delete units;
}

