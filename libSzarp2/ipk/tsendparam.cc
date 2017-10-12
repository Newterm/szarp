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
 * IPK 'send' class implementation.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
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

int TSendParam::processAttributes() {
	auto type_attr = getAttribute<std::string>("type", "probe");
	if (type_attr == "probe")
		type = PROBE;
	else if (type_attr == "min")
		type = MIN;
	else if (type_attr == "min10")
		type = MIN10;
	else if (type_attr == "hour")
		type = HOUR;
	else if (type_attr == "day")
		type = DAY;
	else {
		throw std::runtime_error(std::string("Invalid probe type in send param") + SC::S2A(paramName));
	}

	configured = hasAttribute("param") || hasAttribute("value");
	return 0;
}

int TSendParam::parseXML(xmlTextReaderPtr reader) {
	if (TAttribHolder::parseXML(reader)) return 1;
	return 0;
}

int TSendParam::parseXML(xmlNodePtr node)
{
	if (TAttribHolder::parseXML(node)) return 1;
	return 0;
}

TSendParam* TSendParam::GetNthParam(int n)
{
	assert (n >= 0);
	TSendParam* s = this;
	for (int i = 0; (i < n) && s; i++)
		s = s->GetNext();
	return s;
}

void TSendParam::Configure(const std::wstring& paramName, int value, int repeat, 
		TProbeType type, int sendNoData)
{
	this->paramName = paramName;
	storeAttribute("value", std::to_string(value));
	storeAttribute("repeat", std::to_string(repeat));
	storeAttribute("type", std::to_string(type));
	storeAttribute("send_no_data", std::to_string(sendNoData));

	TSendParam::processAttributes();
}
