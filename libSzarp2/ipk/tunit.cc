/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'unit' class implementation.
 *
 * $Id$
 */

#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <string>
#include <sstream>
#include <dirent.h>
#include <assert.h>

#include "szarp_config.h"
#include "parcook_cfg.h"
#include "conversion.h"
#include "line_cfg.h"
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "liblog.h"

using namespace std;

#define FREE(x)	if (x != NULL) free(x)

int TUnit::parseXML(xmlNodePtr node)
{
	unsigned char* c = NULL;
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
	NEEDATR(node, "id");
	if (xmlStrlen(c) != 1) {
		sz_log(1, "XML file error: attribute 'id' should be one ASCII\
 character (line %ld)", 
 			xmlGetLineNo(node));
		return 1;
	}
	id = SC::U2S(c)[0];
	NEEDATR(node, "type");
	type = atoi((char*)c);
	NEEDATR(node, "subtype");
	subtype = atoi((char*)c);
	NEEDATR(node, "bufsize");
	bufsize = atoi((char*)c);
	free(c);
	c = xmlGetProp(node, (xmlChar *) "name");
	if (c) {
		name = SC::U2S(c);
		free(c);
	}
	
	assert (params == NULL);
	assert (sendParams == NULL);
	TParam* p = NULL;
	TSendParam* sp = NULL;
	for (ch = node->children; ch; ch = ch->next) {
		if (!strcmp((char *)ch->name, "param")) {
			if (params == NULL)
				p = params = new TParam(this);
			else
				p = p->Append(new TParam(this));
			if (p->parseXML(ch))
				return 1;
		} else if (!strcmp((char *)ch->name, "send")) {
			if (sendParams == NULL)
				sp = sendParams = new TSendParam(this);
			else
				sp = sp->Append(new TSendParam(this));
			if (sp->parseXML(ch))
				return 1;
		}
	}
	return 0;
#undef NEEDATR
#undef NOATR
#undef X
}

xmlNodePtr TUnit::generateXMLNode(void)
{
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define BUF X(buffer)
	char buffer[10];
	xmlNodePtr r;

	std::wstringstream wss;
	wss << id;

	r = xmlNewNode(NULL, X"unit");
	sprintf(buffer, "%s", SC::S2U(wss.str()).c_str());
	xmlSetProp(r, X"id", BUF);
	ITOA(type);
	xmlSetProp(r, X"type", BUF);
	ITOA(subtype);
	xmlSetProp(r, X"subtype", BUF);
	ITOA(bufsize);
	xmlSetProp(r, X"bufsize", BUF);

	if (!name.empty())
		xmlSetProp(r, X"name", SC::S2U(name).c_str());

	for (TParam* p = params; p; p = p->GetNext()) {
		xmlAddChild(r, p->generateXMLNode());
	}
	for (TSendParam* sp = GetFirstSendParam(); sp; sp =
			GetNextSendParam(sp))
		xmlAddChild(r, sp->generateXMLNode());
	return r;
#undef X
#undef ITOA
#undef BUF
}

TDevice* TUnit::GetDevice() const
{
	return parentRadio->GetDevice();
}

TSzarpConfig* TUnit::GetSzarpConfig() const
{
	return GetDevice()->GetSzarpConfig();
}

TSendParam* TUnit::GetNextSendParam(TSendParam* s)
{
	if (s == NULL)
		return NULL;
	return s->GetNext();
}

TUnit* TUnit::Append(TUnit* u)
{
	TUnit* t = this;
	while (t->next)
		t = t->next;
	t->next = u;
	return u;
}

int TUnit::GetParamsCount()
{
	int i;
	TParam* p;
	for (i = 0, p = params; p; i++, p = p->GetNext());
	return i;
}

int TUnit::GetSendParamsCount()
{
	int i;
	TSendParam* s;
	for (i = 0, s = sendParams; s; i++, s = s->GetNext());
	return i;
}

void TUnit::AddParam(TParam* p)
{
	if (params == NULL)
		params = p;
	else
		params->Append(p);
}

void TUnit::AddParam(TSendParam* s)
{
	if (sendParams == NULL)
		sendParams = s;
	else
		sendParams->Append(s);
}

TParam* TUnit::GetNextParam(TParam* p)
{
	if (p == NULL)
		return NULL;
	return p->GetNext();
}

const std::wstring& TUnit::GetUnitName() 
{
	return name;
}

TUnit::~TUnit()
{
	delete next;
	delete params;
	delete sendParams;
}

