/* 
  SZARP: SCADA software 

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
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "liblog.h"

using namespace std;

unsigned char* TScript::parseXML(xmlTextReaderPtr reader)
{

	unsigned char *script = NULL;

	XMLWrapper xw(reader);

	for (;;) {
		if(xw.IsTag("script")) {
			if (xw.IsBeginTag()) {
				script = xmlTextReaderReadString(reader);
				xw.NextTag();
			} else
				break;
		} else
		if(xw.IsTag("#cdata-section")) {
			xw.NextTag();
		} else {
			xw.XMLErrorNotKnownTag("script");
		}
	}

return script;

}
