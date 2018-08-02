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
 * Krzysztof Goliñski <krzgol@newterm.pl>
 *
 * IPK 'TDrawdefinable' class implementation.
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
#include "liblog.h"

using namespace std;

TParam* TDrawdefinable::parseXML(xmlTextReaderPtr reader, TSzarpConfig *tszarp)
{

	TParam *params = NULL, *p = NULL;
	XMLWrapper xw(reader,true);
	xw.NextTag();

	for (;;) {
		if (xw.IsTag("param")) {
			if (xw.IsBeginTag()) {
				if (params == NULL) {
					params = p = new TParam(NULL,tszarp);
				} else {
					p = p->Append(new TParam(NULL,tszarp));
				}
				if (p->parseXML(reader))
					return NULL;
			}
			xw.NextTag();
		} else
		if (xw.IsTag("drawdefinable")) {
			break;
		} else {
			xw.XMLErrorNotKnownTag("drawdefinable");
		}
	}

return params;

}
