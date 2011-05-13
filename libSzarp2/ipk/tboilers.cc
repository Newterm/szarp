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

	printf("name boilers xmlParser\n");

#define IFNAME(N) if (xmlStrEqual( name , (unsigned char*) N ) )
#define IFBEGINTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
#define IFCOMMENT if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_COMMENT)
#define NEXTTAG if (xmlTextReaderRead(reader) != 1) \
	return NULL; \
	goto begin_process_tboilers;

	const xmlChar *name = NULL;
	TBoiler *boilers= NULL, *b = NULL;

	NEXTTAG

begin_process_tboilers:

	name = xmlTextReaderConstLocalName(reader);
	if (name == NULL)
		return NULL;

	IFNAME("#text") {
		NEXTTAG
	}
	IFCOMMENT {
		NEXTTAG
	}

	IFNAME("boiler") {
		IFBEGINTAG {
			if (boilers== NULL) {
				boilers = b = TBoiler::parseXML(reader);
				b->SetParent(tszarp);
			} else {
				TBoiler* _tmp = TBoiler::parseXML(reader);
				_tmp->SetParent(tszarp);
				b = b->Append(_tmp);
			}
//			if (b->parseXML(reader))
//				return NULL;
		}
		NEXTTAG
	} else
	IFNAME("boilers") {

	} else {
		printf("ERROR<boilers>: not known name: %s\n",name);
		assert(0 == 1 && "not know name");
	}


#undef IFNAME
#undef IFBEGINTAG
#undef IFCOMMENT
#undef NEXTTAG

	printf("name boilers parseXML END\n");

	return boilers;

}
