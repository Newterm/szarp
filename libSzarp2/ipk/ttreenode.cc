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
	c = xmlGetNoNsProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (xmlChar*)

	NEEDATR(node, "name"); 
	_name = SC::U2S(c);
	xmlFree(c);
	c = NULL;

	c = xmlGetNoNsProp(node, X "prior");
	if (c) {
		std::wstringstream ss;
		ss.imbue(std::locale("C"));
		ss << SC::U2S(c);
		ss >> _prior;
		xmlFree(c);
	}
	c = xmlGetNoNsProp(node, X "draw_prior");
	if (c) {
		std::wstringstream ss;
		ss.imbue(std::locale("C"));
		ss << SC::U2S(c);
		ss >> _draw_prior;
		xmlFree(c);
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

#define CONVERT(FROM, TO) { \
	std::wstringstream ss;	\
	ss.imbue(std::locale("C"));	\
	ss << FROM;		\
	ss >> TO; 		\
	}

	int depth = xmlTextReaderDepth(reader);
	XMLWrapper xw(reader);

	bool isEmptyTag = xw.IsEmptyTag();

	const char* need_attr[] = { "name", 0 };
	if (!xw.AreValidAttr(need_attr)) {
//TODO: check it: ommit all tree?
		return 1;
	}

	const char* ignored_tags[] = { "#text", "#comment", 0 };
	xw.SetIgnoredTags(ignored_tags);

	for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
		const xmlChar *attr = xw.GetAttr();
		const xmlChar *name = xw.GetTagName();

		if (xw.IsAttr("name")) {
			_name = SC::U2S(attr);
		} else
		if (xw.IsAttr("prior")) {
			CONVERT(SC::U2S(attr), _prior);
		} else
		if (xw.IsAttr("draw_prior")) {
			CONVERT(SC::U2S(attr), _draw_prior);
		} else {
		printf("<treenode>: not known attr:%s\n",name);
		}
	}

	if (isEmptyTag)
		return 0;

	xw.NextTag();

	if(xw.IsTag("treenode")) {
		if (xw.IsBeginTag()) {
			_parent = new TTreeNode();
			if (_parent->parseXML(reader))
				return 1;
			xw.NextTag();
		}
		if (xw.IsEndTag()) {
			int _tmp = xmlTextReaderDepth(reader);
			assert( _tmp == depth);
		}
	}

#undef CONVERT

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
