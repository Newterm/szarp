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
 * Krzysztof Goli�ski <krzgol@newterm.pl>
 *
 * IPK 'TBoilers' class implementation.
 *
 * $Id$
 */

#include <sstream>
 
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

TBoiler* TBoilers::parseXML(xmlTextReaderPtr reader, TSzarpConfig *tszarp)
{

	TBoiler *boilers= NULL, *b = NULL;

	XMLWrapper xw(reader);
	const char* ignored_tags[] = { "#text", "#comment", 0 };
	xw.SetIgnoredTags(ignored_tags);

	xw.NextTag();

	for (;;) {
		if (xw.IsTag("boiler")) {
			if (xw.IsBeginTag()) {
				if (boilers== NULL) {
					boilers = b = TBoiler::parseXML(reader);
					b->SetParent(tszarp);
				} else {
					TBoiler* _tmp = TBoiler::parseXML(reader);
					_tmp->SetParent(tszarp);
					b = b->Append(_tmp);
				}
			}
			xw.NextTag();
		} else
		if (xw.IsTag("boilers")) {
			break;
		} else {
			xw.XMLErrorNotKnownTag("boilers");
		}
	}

	return boilers;

}
