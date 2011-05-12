/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'radio' class implementation.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <sstream>
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

#define FREE(x)	if (x != NULL) free(x)


TUnit* TRadio::unitById(wchar_t id)
{
	for (TUnit* u = GetFirstUnit(); u; u = GetNextUnit(u))
		if (u->GetId() == id)
			return u;
	return NULL;
}

int TRadio::parseXML(xmlTextReaderPtr reader)
{
//TODO: remove printf
	printf("name: radio parseXML\n");

	const xmlChar *attr = NULL;
	const xmlChar *attr_name = NULL;
	const xmlChar *name = NULL;

#define IFNAME(N) if (xmlStrEqual( name , (unsigned char*) N ) )
#define NEEDATTR(ATT) attr = xmlTextReaderGetAttribute(reader, (unsigned char*) ATT); \
	if (attr == NULL) { \
		sz_log(1, "XML parsing error: expected '%s' (line %d)", ATT, xmlTextReaderGetParserLineNumber(reader)); \
		return 1; \
	}
//#define IFATTR(ATT) attr = xmlTextReaderGetAttribute(reader, (unsigned char*) ATT); if (attr != NULL)
#define IFATTR(ATT) if (xmlStrEqual(attr_name, (unsigned char*) ATT) )
#define DELATTR xmlFree(attr)
#define IFBEGINTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
#define IFENDTAG if (xmlTextReaderNodeType(reader) == 15)
#define IFCOMMENT if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_COMMENT)
//TODO: check return value - 0 or 1 - in all files 
#define NEXTTAG if (xmlTextReaderRead(reader) != 1) \
	return 1; \
	goto begin_process_tradio;
#define XMLERROR(STR) sz_log(1,"XML file error: %s (line,%d)", STR, xmlTextReaderGetParserLineNumber(reader));
#define FORALLATTR for (int __atr = xmlTextReaderMoveToFirstAttribute(reader); __atr > 0; __atr =  xmlTextReaderMoveToNextAttribute(reader) )
#define GETATTR attr_name = xmlTextReaderConstLocalName(reader); attr = xmlTextReaderConstValue(reader);
#define CHECKNEEDEDATTR(LIST) \
	if (sizeof(LIST) > 0) { \
		std::set<std::string> __tmpattr(LIST, LIST + (sizeof(LIST) / sizeof(LIST[0]))); \
		FORALLATTR { GETATTR; __tmpattr.erase((const char*) attr_name); } \
		if (__tmpattr.size() > 0) { XMLERRORATTR(__tmpattr.begin()->c_str()); return 1; } \
	}

	assert(units == NULL);
	TUnit* u = NULL;

begin_process_tradio:

	name = xmlTextReaderConstName(reader);
	IFNAME("#text") {
		NEXTTAG
	}
	IFCOMMENT {
		NEXTTAG
	}

	IFNAME("unit") {
		IFBEGINTAG {
			if (units == NULL)
				units = u = new TUnit(this);
			else {
				u = u->Append(new TUnit(this));
			}
			assert(u != NULL);
			if (u->parseXML(reader))
				return 1;
			NEXTTAG
		}
	} else
	IFNAME("device") {
		IFENDTAG {
		} else
			assert(0 ==1  && "workaound parser problem");
	} else {
		printf("ERROR<radio>: not known name = %s\n",name);
		assert(0 == 1 &&"not known tag");
	}

#undef IFNAME
#undef NEEDATTR
#undef IFATTR
#undef DELATTR
#undef IFBEGINTAG
#undef IFENDTAG
#undef IFCOMMENT
#undef NEXTTAG
#undef XMLERROR

printf("name radio parseXML END\n");

	return 0;
}

int TRadio::parseXML(xmlNodePtr node)
{
	unsigned char *c = NULL;
	int i;
	xmlNodePtr ch;
	
#define NOATR(p, n) \
	{ \
		sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, SC::U2A(p->name).c_str(), xmlGetLineNo(p)); \
			return 1; \
	}
#define NEEDATR(p, n) \
	if (c) free(c); \
	c = xmlGetProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (xmlChar *)
	if (!strcmp((char *)node->name, "radio")) {
		NEEDATR(node, "id");
		id = SC::U2S(c)[0];
	}
	assert(units == NULL);
	TUnit* u = NULL;
	for (i = 0, ch = node->children; ch; ch = ch->next)
		if (!strcmp((char *)ch->name, "unit")) {
			if (units == NULL)
				units = u = new TUnit(this);
			else {
				u = u->Append(new TUnit(this));
			}
			assert(u != NULL);
			if (u->parseXML(ch))
				return 1;
		}
	if (units == NULL) {
		sz_log(1, "XML file error: no 'unit' elements found in 'device' \
element (line %ld)",
			xmlGetLineNo(node));
		return 1;
	}
	return 0;
#undef NEEDATR
#undef NOATR
#undef X
}

xmlNodePtr TRadio::generateXMLNode(void)
{
#define X (unsigned char *)
	xmlNodePtr r;

	r = xmlNewNode(NULL, X"radio");

	std::wstringstream wss;
	wss << id;

	xmlSetProp(r, X"id", SC::S2U(wss.str()).c_str());
	for (TUnit* u = GetFirstUnit(); u; u = GetNextUnit(u))
		xmlAddChild(r, u->generateXMLNode());
	return r;
#undef X
}

TUnit* TRadio::GetNextUnit(TUnit* u)
{
	if (u == NULL)
		return NULL;
	return u->GetNext();
}

TUnit* TRadio::AddUnit(TUnit* u)
{
	assert (u != NULL);
	if (units == NULL)
		units = u;
	else
		units->Append(u);
	return u;
}

TRadio* TRadio::Append(TRadio* r)
{
	TRadio* t = this;
	while (t->next)
		t = t->next;
	t->next = r;
	return r;
}

int TRadio::GetUnitsCount()
{
	int i = 0;
	for (TUnit* u = GetFirstUnit(); u; u = GetNextUnit(u))
		i++;
	return i;
}

TRadio::~TRadio()
{
	delete next;
	delete units;
}

