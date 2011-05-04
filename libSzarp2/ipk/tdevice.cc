/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'device' class implementation.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <string>
#include <dirent.h>
#include <assert.h>

#include "conversion.h"
#include "szarp_config.h"
#include "parcook_cfg.h"
#include "line_cfg.h"
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "liblog.h"

using namespace std;

#define FREE(x)	if (x != NULL) free(x)

int TDevice::parseXML(xmlNodePtr node)
{
#define X (xmlChar *)
	xmlChar *c;
	int i;
	xmlNodePtr ch;
	
	c = xmlGetProp(node, X"daemon");
	if (!c)
		goto inside;
	daemon = SC::U2S(c);
	xmlFree(c);

	c = xmlGetProp(node, X"options");
	if (c) {
		options = SC::U2S(c);
		xmlFree(c);
	}
	c = xmlGetProp(node, X"path");
	if (!c)
		goto inside;
	path = SC::U2S(c);
	xmlFree(c);
	c = xmlGetProp(node, X"speed");
	if (!c)
		goto inside;
	speed = atoi((char*) c);
	xmlFree(c);
	c = xmlGetProp(node, X"stop");
	if (!c)
		goto inside;
	stop = atoi((char*) c);
	xmlFree(c);
	c = xmlGetProp(node, X"protocol");
	if (!c)
		goto inside;
	protocol = atoi((char*)(c));
	xmlFree(c);
inside:
	c = xmlGetProp(node, X"special");
	if (c) {
		special = 1;
		special_value = atoi((char*)c);
		xmlFree(c);
	}
	for (i = 0, ch = node->children; ch; ch = ch->next)
		if (!strcmp((char *)ch->name, "radio"))
			i++;
	if (i <= 0) {
		radios = new TRadio(this);
		assert (radios != NULL);
		return radios->parseXML(node);
	}
	if (special) {
		sz_log(1, "XML file error - cannot set attribute 'special' for \
device with radio modems (line %ld)",
			xmlGetLineNo(node));
		return 1;
	}
	radios = NULL;
	TRadio* r = NULL;
	for (i = 0, ch = node->children; ch; ch = ch->next)
		if (!strcmp((char *)ch->name, "radio")) {
			if (radios == NULL) {
				r = radios = new TRadio(this);
			} else {
				r = r->Append(new TRadio(this));
			}
			assert(r != NULL);
			if (r->parseXML(ch))
				return 1;
		}
	return 0;
#undef X 
}


int TDevice::parseXML(xmlTextReaderPtr reader)
{
//TODO: remove all printf
printf("name device: parseXML\n");

	xmlChar *attr = NULL;
	xmlChar *name = NULL;

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
//TODO: check return value - 0 or 1 - in all files 
#define NEXTTAG if(name) xmlFree(name);\
	if (xmlTextReaderRead(reader) != 1) \
	return 1; \
	goto begin_process_tdevice;
#define XMLERROR(STR) sz_log(1,"XML file error: %s (line,%d)", STR, xmlTextReaderGetParserLineNumber(reader)); \
	xmlFree(attr);

	NEEDATTR("daemon")
	daemon = SC::U2S(attr);
	DELATTR;

	NEEDATTR("path")
	path = SC::U2S(attr);
	DELATTR;

	IFATTR("speed") {
		speed = atoi((char*) attr);
		DELATTR;
	}

	IFATTR("stop") {
		stop = atoi((char*) attr);
		DELATTR;
	}

	IFATTR("protocol") {
		protocol = atoi((char*) attr);
		DELATTR;
	}

	IFATTR("options") {
		options = SC::U2S(attr);
		DELATTR;
	}


	NEXTTAG

begin_process_tdevice:

	name = xmlTextReaderName(reader);
	IFNAME("#text") {
		NEXTTAG
	}

	IFNAME("unit") {
		IFBEGINTAG {
			radios = new TRadio(this);
			assert (radios != NULL);
			return radios->parseXML(reader);
		}
		NEXTTAG
	} else
	IFNAME("device") {
	}
	else {
		printf("ERROR: not know name = %s\n",name);
		assert(name == NULL && "not know name");
	}

printf("name device parseXML END\n");

#undef IFNAME
#undef NEEDATTR
#undef IFATTR
#undef DELATTR
#undef IFBEGINTAG
#undef IFENDTAG
#undef NEXTTAG
#undef XMLERROR

	return 0;
}

TUnit* TDevice::searchUnitById(const wchar_t id, int uniq)
{
	TUnit *u = NULL;
	TRadio *r;
	
	for (r = GetFirstRadio(); r; r = GetNextRadio(r)) {
		for (u = r->GetFirstUnit(); u; u = r->GetNextUnit(u))
			if (u->GetId() == id)
				goto out;
	}
out:
	if (u == NULL)
		return NULL;
	if (uniq == 0)
		return u;
	for (; r; r = GetNextRadio(r))
		for (TUnit* u2 = r->GetNextUnit(u); u2; u2 = r->GetNextUnit(u2))
			if (u2->GetId() == id)
				return NULL;
	return u;
}

TParam *TDevice::getParamByNum(int num)
{
	TParam* p = GetFirstRadio()->GetFirstUnit()->GetFirstParam()->
			GetNthParam(num);
	if (p == NULL)
		return NULL;
	if (p->GetDevice() == this)
		return p;
	return NULL;
}

xmlNodePtr TDevice::generateXMLNode(void)
{
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define BUF X(buffer)
	char buffer[10];
	xmlNodePtr r;
	
	r = xmlNewNode(NULL, (unsigned char*)"device");
	if (!daemon.empty())
		xmlSetProp(r, X"daemon", SC::S2U(daemon).c_str());
	if (!path.empty())
		xmlSetProp(r, X"path", SC::S2U(path).c_str());
	if (speed > 0) {
		ITOA(speed);
		xmlSetProp(r, X"speed", BUF);
	}
	if (protocol >= 0) {
		ITOA(protocol);
		xmlSetProp(r, X"protocol", BUF);
	}
	if (special) {
		ITOA(special_value);
		xmlSetProp(r, X"special", BUF);
	}
	if (!options.empty()) 
		xmlSetProp(r, X"options", SC::S2U(options).c_str());
	if (GetFirstRadio()->GetId())
		for (TRadio* rd = GetFirstRadio(); rd; rd = GetNextRadio(rd)) 
			xmlAddChild(r, rd->generateXMLNode());
	else
		for (TUnit* u = GetFirstRadio()->GetFirstUnit(); u;
				u = GetFirstRadio()->GetNextUnit(u))
			xmlAddChild(r, u->generateXMLNode());
	return r;
#undef X
#undef ITOA
#undef BUF
}

TRadio* TDevice::AddRadio(TRadio *r)
{
	assert (r != NULL);
	if (radios == NULL)
		radios = r;
	else
		radios->Append(r);
	return r;
}

TRadio* TDevice::GetNextRadio(TRadio* r)
{
	if (r == NULL)
		return NULL;
	return r->GetNext();
}

TDevice* TDevice::Append(TDevice* d)
{
	TDevice* t = this;
	while (t->next)
		t = t->next;
	t->next = d;
	return d;
}

int TDevice::GetNum()
{
	int i;
	TDevice* d;
	TSzarpConfig* s = GetSzarpConfig();
	assert (s != NULL);
	for (i = 0, d = s->GetFirstDevice(); d && (d != this); i++,
			d = s->GetNextDevice(d));
	assert(d != NULL);
	return i;
}

int TDevice::GetParamsCount()
{
	int i = 0;
	for (TRadio* r = GetFirstRadio(); r; r = GetNextRadio(r))
	for (TUnit* u = r->GetFirstUnit(); u; u = r->GetNextUnit(u))
		i += u->GetParamsCount();
	return i;
}

int TDevice::GetRadiosCount()
{
	int i = 0;
	for (TRadio* r = GetFirstRadio(); r; r = GetNextRadio(r))
		i++;
	return i;
}

TDevice::~TDevice()
{
	delete next;
	delete radios;
}

