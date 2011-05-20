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
printf("name script xmlParser\n");

	unsigned char *script = NULL;

	XMLWrapper xw(reader);

	const char* ignored_tags[] = { "#text", "#comment", 0 };
	xw.SetIgnoredTags(ignored_tags);

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
			const xmlChar *name = xw.GetTagName();
			printf("ERROR<script>: not known name: %s\n",name);
			assert(0 == 1 && "not know name");
		}
	}

printf("name script parseXML END\n");

return script;

}
