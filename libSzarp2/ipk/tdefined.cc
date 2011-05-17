/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 * Krzysztof Goliñski <krzgol@newterm.pl>
 *
 * IPK 'TDefined' class implementation.
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


TParam* TDefined::parseXML(xmlTextReaderPtr reader, TSzarpConfig *tszarp)
{
//TODO: remove
printf("name defined xmlParser\n");

	TParam *params = NULL, *p = NULL;

	XMLWrapper xw(reader);

	const char* ignored_tags[] = { "#text", "#comment", 0 };
	xw.SetIgnoredTags(ignored_tags);

	if (!xw.NextTag())
		return NULL;

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
		if (xw.IsTag("defined")) {
			return params;
		} else {
			printf("ERROR<defined>: not known name: %s\n",xw.GetTagName());
			assert(0 == 1 && "not know name");
		}
	}

//TODO: remove
printf("name  defined parseXML END\n");

return params;

}
