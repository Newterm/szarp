/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'draw' class implementation.
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

printf("name defined xmlParser\n");

#define IFNAME(N) if (xmlStrEqual( name , (unsigned char*) N ) )
#define NEEDATTR(ATT) attr = xmlTextReaderGetAttribute(reader, (unsigned char*) ATT); \
	if (attr == NULL) { \
		sz_log(1, "XML parsing error: expected '%s' (line %d)", ATT, xmlTextReaderGetParserLineNumber(reader)); \
		return 1; \
	}
#define IFATTR(ATT) attr = xmlTextReaderGetAttribute(reader, (unsigned char*) ATT); if (attr != NULL)
#define DELATTR xmlFree(attr)
#define IFBEGINTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
#define IFENDTAG if (xmlTextReaderNodeType(reader) == 15)
#define NEXTTAG if(name) xmlFree(name);\
	if (xmlTextReaderRead(reader) != 1) \
	return NULL; \
	goto begin_process_tdefined;
#define XMLERROR(STR) sz_log(1,"XML file error: %s (line,%d)", STR, xmlTextReaderGetParserLineNumber(reader));

	xmlChar *name = NULL;
	TParam *params = NULL, *p = NULL;

	NEXTTAG

begin_process_tdefined:

	name = xmlTextReaderName(reader);
	if (name == NULL)
		name = xmlStrdup(BAD_CAST "--");

	IFNAME("#text") {
		NEXTTAG
	} else
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
	IFNAME("defined") {

	} else {
		printf("ERROR: not known name: %s\n",name);
		assert(0 == 1 && "not know name");
	}


#undef IFNAME
#undef NEEDATTR
#undef IFATTR
#undef DELATTR
#undef IFBEGINTAG
#undef IFENDTAG
#undef NEXTTAG
#undef XMLERROR

printf("name  defined parseXML END\n");

return params;

}
