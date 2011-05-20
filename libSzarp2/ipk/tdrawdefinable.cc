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


TParam* TDrawdefinable::parseXML(xmlTextReaderPtr reader, TSzarpConfig *tszarp)
{

printf("name drawdefinable xmlParser\n");

	TParam *params = NULL, *p = NULL;

	XMLWrapper xw(reader,true);

	const char* ignored_tags[] = { "#text", "#comment", 0 };
	xw.SetIgnoredTags(ignored_tags);

	xw.NextTag();

	for (;;) {
		if(xw.IsTag("param")) {
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
		if(xw.IsTag("drawdefinable")) {
			break;
		} else {
			const xmlChar* name = xw.GetTagName();
			printf("ERROR<drawdefinable>: not known name: %s\n",name);
			assert(0 == 1 && "not know name");
		}
	}

printf("name drawdefinable parseXML END\n");

return params;

}
