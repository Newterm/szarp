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

#define FREE(x)	if (x != NULL) free(x)

/** String representations of special attributes. */
static const wchar_t* SPECIAL_TYPES_STR[] = {
	L"none",
	L"piedraw",
	L"hoursum",
	L"valve",
	L"rel",
	L"diff" };

TDraw* TDraw::Append(TDraw* d)
{
	if (d == NULL)
		return NULL;
	if (next != NULL)
		return next->Append(d);
	next = d;
	return next;
}

void TDraw::SetSpecial(SPECIAL_TYPES spec)
{
	special = spec;
}

int TDraw::GetCount()
{
	int i = 0;
	TDraw *d;
	for (d = this; d != NULL; d = d->next)
		i++;
	return i;
}

TDraw* TDraw::parseXML(xmlTextReaderPtr reader)
{
printf("name draw xmlParser\n");

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
//TODO: check return value - 0 or 1 - in all files 
#define NEXTTAG if(name) xmlFree(name);\
	if (xmlTextReaderRead(reader) != 1) \
	return 1; \
	goto begin_process_tdraw;
#define XMLERROR(STR) sz_log(1,"XML file error: %s (line,%d)", STR, xmlTextReaderGetParserLineNumber(reader));
#define XMLERRORATTR(ATT) sz_log(1,"XML parsing error: expected attribute '%s' (line: %d)", ATT, xmlTextReaderGetParserLineNumber(reader));
#define FORALLATTR for (int __atr = xmlTextReaderMoveToFirstAttribute(reader); __atr > 0; __atr =  xmlTextReaderMoveToNextAttribute(reader) )
#define GETATTR attr_name = xmlTextReaderConstLocalName(reader); attr = xmlTextReaderConstValue(reader);
#define CHECKNEEDEDATTR(LIST) \
	if (sizeof(LIST) > 0) { \
		std::set<string> __tmpattr(LIST, LIST + (sizeof(LIST) / sizeof(LIST[0]))); \
		FORALLATTR { GETATTR; __tmpattr.erase((const char*) attr_name); } \
		if (__tmpattr.size() > 0) { XMLERRORATTR(__tmpattr.begin()->c_str()); return NULL; } \
	}

#define CONVERT(FROM, TO) { \
	std::wstringstream ss;	\
	ss.imbue(locale("C"));	\
	ss << FROM;		\
	ss >> TO; 		\
	}

#define UNDEFVAL -1234.5

	const xmlChar *attr_name = NULL;
	const xmlChar *attr = NULL;
	xmlChar *name = NULL;
	double p = -1.0, o = -1.0, min = 0.0, max = UNDEFVAL, smin = UNDEFVAL, smax = UNDEFVAL;
	int sc = 0;
	SPECIAL_TYPES sp = NONE;
	std::wstring strw_c, strw_w;

	const char* need_attr[] = { "title" };
	CHECKNEEDEDATTR(need_attr);

	FORALLATTR {
		GETATTR

		IFATTR("title") {
			strw_w = SC::U2S(attr);
		} else
		IFATTR("color") {
			strw_c = SC::U2S(attr);
		} else
		IFATTR("prior") {
			CONVERT(SC::U2S(attr), p);
		} else
		IFATTR("order") {
			CONVERT(SC::U2S(attr), o);
		} else
		IFATTR("max") {
			CONVERT(SC::U2S(attr), max);
		} else
		IFATTR("min") {
			CONVERT(SC::U2S(attr), min);
		} else
		IFATTR("scale") {
			CONVERT(SC::U2S(attr), sc);
		} else
		IFATTR("minscale") {
			CONVERT(SC::U2S(attr), smin);
		} else
		IFATTR("maxscale") {
			CONVERT(SC::U2S(attr),smax);
		} else
		IFATTR("special") {
			for (unsigned i = 0; i < sizeof(SPECIAL_TYPES_STR) / sizeof(wchar_t*); i++)
				if (SC::U2S(attr) == SPECIAL_TYPES_STR[i]) {
					sp = (SPECIAL_TYPES)i;
					break;
			}
		} else {
			printf("not known attr: %s\n",attr_name);
			assert(0 ==1 && "not known attr");
		}
	} // FORALLATTR

	if (max == UNDEFVAL) {
		min = 0.0;
		max = -1.0;
	}
	if (sc > 0)
		if (smax < smin)
			sc = 0;

	TDraw* ret = new TDraw(strw_c, 
			strw_w, 
			p, 
			o, 
			min, 
			max, 
			sc, 
			smin, 
			smax, 
			sp);

//TODO: make me
/*
	for (xmlNodePtr ch = node->children; ch; ch = ch->next) if (!strcmp((const char*) ch->name, "treenode")) {
		TTreeNode* node = new TTreeNode();
		if (node->parseXML(ch)) {
			delete ret;	
			return NULL;
		}
		ret->_tree_nodev.push_back(node);
	}
*/

#undef CONVERT
#undef UNDEFVAL

#undef IFNAME
#undef NEEDATTR
#undef IFATTR
#undef DELATTR
#undef IFBEGINTAG
#undef IFENDTAG
#undef NEXTTAG
#undef XMLERROR
#undef FORALLATTR
#undef GETATTR
#undef CHECKNEEDEDATTR

printf("name draw parseXML END\n");

	return ret;
}

TDraw* TDraw::parseXML(xmlNodePtr node)
{
	unsigned char *ch = NULL;
	unsigned char *w, *c;
	double p, o, min, max, smin = 0, smax = 0;
	int sc;

#define CONVERT(FROM, TO) { \
	std::wstringstream ss;	\
	ss.imbue(locale("C"));	\
	ss << FROM;		\
	ss >> TO; 		\
	}

	SPECIAL_TYPES sp;
	
	w = xmlGetProp(node, (xmlChar*)"title");
	if (w == NULL) {
		sz_log(1, "Attribute 'title' on element 'draw' not found (line %ld)", 
			xmlGetLineNo(node));
		return NULL;
	}

	c = xmlGetProp(node, (xmlChar*)"color");

	ch = xmlGetProp(node, (xmlChar*)"prior");
	if (ch) {
		CONVERT(SC::U2S(ch), p);
		xmlFree(ch);
	} else
		p = -1.0;

	ch = xmlGetProp(node, (xmlChar*)"order");
	if (ch) {
		CONVERT(SC::U2S(ch), o);
		xmlFree(ch);
	} else
		o = -1.0;

	ch = xmlGetProp(node, (xmlChar*)"max");
	if (ch) {
		CONVERT(SC::U2S(ch), max);
		xmlFree(ch);
		ch = xmlGetProp(node, (xmlChar*)"min");
		if (ch) {
			CONVERT(SC::U2S(ch), min);
			xmlFree(ch);
		} else
			min = 0.0;
	} else {
		min = 0.0;
		max = -1.0;
	}

	ch = xmlGetProp(node, (xmlChar*)"scale");
	if (ch) {
		CONVERT(SC::U2S(ch), sc);
		xmlFree(ch);
		if (sc > 0)
			ch = xmlGetProp(node, (xmlChar*)"minscale");
		else
			ch = NULL;
		if (ch) {
			CONVERT(SC::U2S(ch), smin);
			xmlFree(ch);
			ch = xmlGetProp(node, (xmlChar*)"maxscale");
		} 
		if (ch) {
			CONVERT(SC::U2S(ch), smax);
			xmlFree(ch);
			if (smax <= smin)
				sc = 0;
		}
	} else {
		sc = 0;
	}
	
	sp = NONE;
	ch = xmlGetProp(node, (xmlChar*)"special");
	if (ch) {
		for (unsigned i = 0; i < sizeof(SPECIAL_TYPES_STR) / sizeof(wchar_t*); i++)
			if (SC::U2S(ch) == SPECIAL_TYPES_STR[i]) {
				sp = (SPECIAL_TYPES)i;
				break;
			}
		xmlFree(ch);
	}
	TDraw* ret = new TDraw(c == NULL ? std::wstring() : SC::U2S(c), 
			SC::U2S(w), 
			p, 
			o, 
			min, 
			max, 
			sc, 
			smin, 
			smax, 
			sp);

	xmlFree(c);
	xmlFree(w);
	for (xmlNodePtr ch = node->children; ch; ch = ch->next) if (!strcmp((const char*) ch->name, "treenode")) {
		TTreeNode* node = new TTreeNode();
		if (node->parseXML(ch)) {
			delete ret;	
			return NULL;
		}
		ret->_tree_nodev.push_back(node);
	}
	return ret;

#undef CONVERT
}

xmlNodePtr TDraw::GenerateXMLNode()
{
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define DTOA(x) snprintf(buffer, 10, "%g", x)
#define BUF X(buffer)
	xmlNodePtr r;
	char buffer[10];


	r = xmlNewNode(NULL, X"draw");
	xmlSetProp(r, X"title", SC::S2U(window).c_str());

	if (prior > 0) {
		DTOA(prior);
		xmlSetProp(r, X"prior", BUF);
	}

	if (order > 0) {
		DTOA(order);
		xmlSetProp(r, X"order", BUF);
	}

	if (!color.empty())
		xmlSetProp(r, X"color", SC::S2U(color).c_str());

	if (maxVal > minVal) {
		DTOA(minVal);
		xmlSetProp(r, X"min", BUF);
		DTOA(maxVal);
		xmlSetProp(r, X"max", BUF);
	}

	if (scaleProc > 0) {
		ITOA(scaleProc);
		xmlSetProp(r, X"scale", BUF);
		DTOA(scaleMin);
		xmlSetProp(r, X"minscale", BUF);
		DTOA(scaleMax);
		xmlSetProp(r, X"maxscale", BUF);
	}
	
	if (special != NONE)
		xmlSetProp(r, X"special", SC::S2U(SPECIAL_TYPES_STR[special]).c_str());
	for (std::vector<TTreeNode*>::iterator i = _tree_nodev.begin(); i != _tree_nodev.end(); i++)
		xmlAddChild(r, (*i)->generateXML());
	return r;
#undef X
#undef ITOA
#undef BUF
}

TDraw::~TDraw()
{
	for (std::vector<TTreeNode*>::iterator i = _tree_nodev.begin(); i != _tree_nodev.end(); i++)
		delete *i;
	delete next;
}

