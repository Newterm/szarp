/* 
  SZARP: SCADA software 
*/

#include <sstream>

#include "szarp_config.h"
#include "liblog.h"
#include "conversion.h"

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

TTreeNode::TTreeNode() : _parent(NULL), _prior(-1), _draw_prior(-1) {}

int TTreeNode::parseXML(xmlNodePtr node) {
	xmlNodePtr ch;
	unsigned char *c = NULL;
#define NOATR(p, n) \
	{ \
	sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, SC::U2A(p->name).c_str(), xmlGetLineNo(p)); \
			return 1; \
	}
#define NEEDATR(p, n) \
	if (c) xmlFree(c); \
	c = xmlGetProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (xmlChar*)

	NEEDATR(node, "name"); 
	_name = SC::U2S(c);
	c = xmlGetProp(node, X "prior");
	if (c) {
		std::wstringstream ss;
		ss.imbue(std::locale("C"));
		ss << SC::U2S(c);
		ss >> _prior;
		xmlFree(c);
	}
	c = xmlGetProp(node, X "draw_prior");
	if (c) {
		std::wstringstream ss;
		ss.imbue(std::locale("C"));
		ss << SC::U2S(c);
		ss >> _draw_prior;
	}
    	for (ch = node->children; ch; ch = ch->next)
		if (!strcmp((char*) ch->name, "treenode"))
			break;
	if (ch) {
		_parent = new TTreeNode();
		if (_parent->parseXML(ch))
			return 1;
	}
	return 0;
#undef X
#undef NEEDATR
#undef NOATR
}

int TTreeNode::parseXML(xmlTextReaderPtr reader) {

	printf("name treenode xmlParser\n");

	const xmlChar *attr_name = NULL;
	const xmlChar *attr = NULL;
	const xmlChar *name = NULL;

#define IFNAME(N) if (xmlStrEqual( name , (unsigned char*) N ) )
#define IFATTR(ATT) if (xmlStrEqual(attr_name, (unsigned char*) ATT) )
#define IFBEGINTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
#define IFENDTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT)
#define IFCOMMENT if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_COMMENT)
//TODO: check return value - 0 or 1 - in all files 
#define NEXTTAG if (xmlTextReaderRead(reader) != 1) \
	return NULL; \
	goto begin_process_ttreenode;
#define XMLERROR(STR) sz_log(1,"XML file error: %s (line,%d)", STR, xmlTextReaderGetParserLineNumber(reader));
#define XMLERRORATTR(ATT) sz_log(1,"XML parsing error: expected attribute '%s' (line: %d)", ATT, xmlTextReaderGetParserLineNumber(reader));
#define FORALLATTR for (int __atr = xmlTextReaderMoveToFirstAttribute(reader); __atr > 0; __atr =  xmlTextReaderMoveToNextAttribute(reader) )
#define GETATTR attr_name = xmlTextReaderConstLocalName(reader); attr = xmlTextReaderConstValue(reader);
#define CHECKNEEDEDATTR(LIST) \
	if (sizeof(LIST) > 0) { \
		std::set<std::string> __tmpattr(LIST, LIST + (sizeof(LIST) / sizeof(LIST[0]))); \
		FORALLATTR { GETATTR; __tmpattr.erase((const char*) attr_name); } \
		if (__tmpattr.size() > 0) { XMLERRORATTR(__tmpattr.begin()->c_str()); return NULL; } \
	}

#define CONVERT(FROM, TO) { \
	std::wstringstream ss;	\
	ss.imbue(std::locale("C"));	\
	ss << FROM;		\
	ss >> TO; 		\
	}

	int depth = xmlTextReaderDepth(reader);

	bool isEmptyTag = false;
	if (xmlTextReaderIsEmptyElement(reader)) {
		isEmptyTag = true;
	}

	const char* need_attr[] = { "name" };
	CHECKNEEDEDATTR(need_attr);

	FORALLATTR {
		GETATTR

		IFATTR("name") {
			_name = SC::U2S(attr);
		} else
		IFATTR("prior") {
			CONVERT(SC::U2S(attr), _prior);
		} else
		IFATTR("draw_prior") {
			CONVERT(SC::U2S(attr), _draw_prior);
		} else {
		printf("<treenode>: not known attr:%s\n",name);
		}
	}

	if (isEmptyTag)
		return 0;

	NEXTTAG

begin_process_ttreenode:

	name = xmlTextReaderConstLocalName(reader);
	IFNAME("#text") {
		NEXTTAG
	}
	IFCOMMENT {
		NEXTTAG
	}

	IFNAME("treenode") {
		IFBEGINTAG {
			_parent = new TTreeNode();
			if (_parent->parseXML(reader))
				return 1;
			NEXTTAG
		}
		IFENDTAG {
			int _tmp = xmlTextReaderDepth(reader);
			assert( _tmp == depth);
		}
	}

#undef CONVERT

#undef IFNAME
#undef IFATTR
#undef IFBEGINTAG
#undef IFENDTAG
#undef IFCOMMENT
#undef NEXTTAG
#undef XMLERROR
#undef FORALLATTR
#undef GETATTR
#undef CHECKNEEDEDATTR

	printf("name treenode parseXML END\n");
	return 0;
}

xmlNodePtr TTreeNode::generateXML() {
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 50, "%f", x)
	char buffer[50];
	xmlNodePtr r;
	r = xmlNewNode(NULL, (unsigned char *) "treenode");
	xmlSetProp(r, X "name", SC::S2U(_name).c_str());
	if (_prior >= 0) {
		ITOA(_prior);
		xmlSetProp(r, X "prior", X buffer);
	}
	if (_draw_prior >= 0) {
		ITOA(_draw_prior);
		xmlSetProp(r, X "draw_prior", X buffer);
	}
	if (_parent)
		xmlAddChild(r, _parent->generateXML());
	return r;
}

TTreeNode::~TTreeNode() {
	delete _parent;
}
