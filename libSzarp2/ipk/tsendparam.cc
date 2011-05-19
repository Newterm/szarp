/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'send' class implementation.
 *
 * $Id$
 */

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

#define FREE(x)	if (x != NULL) free(x)

xmlNodePtr TSendParam::generateXMLNode(void)
{
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define BUF X(buffer)
	xmlNodePtr r;
	char buffer[10];

	r = xmlNewNode(NULL, X"send");
	if (!configured)
		return r;
	if (!paramName.empty()) 
		xmlSetProp(r, X"param", SC::S2U(paramName).c_str());
	else {
		ITOA(value);
		xmlSetProp(r, X"value", BUF);
	}
	switch (type) {
		case (PROBE) :
			xmlSetProp(r, X"type", X"probe");
			break;
		case (MIN) :
			xmlSetProp(r, X"type", X"min");
			break;
		case (MIN10) :
			xmlSetProp(r, X"type", X"min10");
			break;
		case (HOUR) :
			xmlSetProp(r, X"type", X"hour");
			break;
		case (DAY) :
			xmlSetProp(r, X"type", X"day");
			break;
	}
	ITOA(repeat);
	xmlSetProp(r, X"repeat", BUF);
	if (sendNoData)
		xmlSetProp(r, X"send_no_data", X"1");
	return r;
#undef X
#undef ITOA
#undef BUF
}


int TSendParam::parseXML(xmlTextReaderPtr reader) {

//TODO: remove all printf
printf("name send: parseXML\n");

#define X (unsigned char *)

	XMLWrapper xw(reader);
	if (xw.IsEmptyTag() && !xw.HasAttr())
		return 0;

	const char* need_attr_send[] = { "repeat" , "type", 0 };
	if (!xw.AreValidAttr(need_attr_send)) {
//TODO: check it: ommit all tree?
		return 1;
	}

	bool isValue = false;
	bool isParam = false;

	for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
		const xmlChar *attr = xw.GetAttr();
		const xmlChar *attr_name = xw.GetAttrName();

		if (xw.IsAttr("param")) {
			isParam = true;
			paramName = SC::U2S(attr).c_str();
		} else
		if (xw.IsAttr("value")) {
			isValue = true;
			value = atoi((const char*) attr);
		} else
		if (xw.IsAttr("repeat")) {
			repeat = atoi((const char*) attr);
		} else
		if (xw.IsAttr("send_no_data")) {
			sendNoData = 1;
		} else
		if (xw.IsAttr("type")) {
			if (!xmlStrcmp(attr, X"probe"))
				type = PROBE;
			else if (!xmlStrcmp(attr, X"min"))
				type = MIN;
			else if (!xmlStrcmp(attr, X"min10"))
				type = MIN10;
			else if (!xmlStrcmp(attr, X"hour"))
				type = HOUR;
			else if (!xmlStrcmp(attr, X"day"))
				type = DAY;
			else {
				xw.XMLError("Unknown value for attribute 'type' in element <send>");
				return 1;
			}
		} else {
//TODO: make it better
			printf("ERROR: not known attr: %s\n",attr_name);
//			assert(attr == NULL && "not known attr");
		}
	}

	if (isParam || isValue)
		configured = 1;

printf("name send parseXML END\n");

#undef X

	return 0;
}

int TSendParam::parseXML(xmlNodePtr node)
{

	unsigned char *c = NULL;

#define NOATR(p, n) \
	{ \
		sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, SC::U2A(p->name).c_str(), xmlGetLineNo(p)); \
			return 1; \
	}
#define NEEDATR(p, n) \
	if (c) free(c); \
	c = xmlGetNoNsProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (unsigned char *)
	
	unsigned char* _pn = xmlGetNoNsProp(node, X"param");
	if (_pn == NULL) {
		c = xmlGetNoNsProp(node, X"value");
		if (c == NULL) 
			return 0;
		value = atoi((char*)c);
		free(c);
		c = NULL;
	} else {
		paramName = SC::U2S(_pn).c_str();
		xmlFree(_pn);
	}
	configured = 1;
	NEEDATR(node, "repeat");
	repeat = atoi((char*)c);
	free(c);
	c = NULL;
	if ((c = xmlGetNoNsProp(node, X"send_no_data"))) {
		sendNoData = 1;
	}
	free(c);
	c = NULL;
	NEEDATR(node, "type");
	if (!xmlStrcmp(c, X"probe"))
		type = PROBE;
	else if (!xmlStrcmp(c, X"min"))
		type = MIN;
	else if (!xmlStrcmp(c, X"min10"))
		type = MIN10;
	else if (!xmlStrcmp(c, X"hour"))
		type = HOUR;
	else if (!xmlStrcmp(c, X"day"))
		type = DAY;
	else {
		sz_log(1, "Unknown value for attribute 'type' in element <send> \
in line %ld", xmlGetLineNo(node));
		free(c);
		return 1;
	}
	free(c);
	return 0;
#undef NEEDATR
#undef NOATR
#undef X
}

TSendParam* TSendParam::Append(TSendParam* s)
{
	TSendParam* t = this;
	while (t->next)
		t = t->next;
	t->next = s;
	return s;
}

TSendParam* TSendParam::GetNthParam(int n)
{
	assert (n >= 0);
	TSendParam* s = this;
	for (int i = 0; (i < n) && s; i++)
		s = s->GetNext();
	return s;
}

void TSendParam::Configure(const std::wstring& paramName, int value, int repeat, 
		TProbeType type, int sendNoData)
{
	this->paramName = paramName;
	this->value = value;
	this->repeat = repeat;
	this->type = type;
	this->sendNoData = sendNoData;
	configured = 1;
}

TSendParam::~TSendParam()
{
	delete next;
}

