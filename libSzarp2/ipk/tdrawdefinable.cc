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

#define IFNAME(N) if (xmlStrEqual( name , (unsigned char*) N ) )
#define IFBEGINTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
#define IFCOMMENT if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_COMMENT)
#define NEXTTAG if (xmlTextReaderRead(reader) != 1) \
	return NULL; \
	goto begin_process_tdrawdef;

	const xmlChar *name = NULL;
	TParam *params = NULL, *p = NULL;

	NEXTTAG

begin_process_tdrawdef:

	name = xmlTextReaderConstLocalName(reader);
	if (name == NULL)
		return NULL;

	IFNAME("#text") {
		NEXTTAG
	}
	IFCOMMENT {
		NEXTTAG
	}

	IFNAME("param") {
		IFBEGINTAG {
			if (params == NULL) {
				params = p = new TParam(NULL,tszarp);
			} else {
				p = p->Append(new TParam(NULL,tszarp));
			}
			if (p->parseXML(reader))
				return NULL;
		}
		NEXTTAG
	} else
	IFNAME("drawdefinable") {

	} else {
		printf("ERROR<drawdefinable>: not known name: %s\n",name);
		assert(0 == 1 && "not know name");
	}


#undef IFNAME
#undef IFBEGINTAG
#undef IFCOMMENT
#undef NEXTTAG

printf("name drawdefinable parseXML END\n");

return params;

}
