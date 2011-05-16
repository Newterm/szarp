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

#define IFNAME(N) if (xmlStrEqual( name , (unsigned char*) N ) )
#define IFBEGINTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
#define NEXTTAG if (xmlTextReaderRead(reader) != 1) \
	return NULL; \
	goto begin_process_tscript;

	const xmlChar *name = NULL;
	unsigned char *script = NULL;

//	NEXTTAG

begin_process_tscript:

	name = xmlTextReaderConstName(reader);
	if (name == NULL)
		return NULL;

	IFNAME("#text") {
		NEXTTAG
	} else
	IFNAME("script") {
		IFBEGINTAG {
			script = xmlTextReaderReadString(reader);
			NEXTTAG
		}
	} else
	IFNAME("#cdata-section") {
//		script = (unsigned char*) xmlTextReaderValue(reader);
		NEXTTAG
	} else {
		printf("ERROR<script>: not known name: %s\n",name);
		assert(0 == 1 && "not know name");
	}


#undef IFNAME
#undef IFBEGINTAG
#undef NEXTTAG

printf("name script parseXML END\n");

return script;

}
