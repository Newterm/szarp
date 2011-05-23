/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 * Krzysztof Goliñski <krzgol@newterm.pl>
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
		if(xw.IsTag("boiler")) {
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
		if(xw.IsTag("boilers")) {
			break;
		} else {
			xw.XMLErrorNotKnownTag("boilers");
		}
	}

	return boilers;

}
