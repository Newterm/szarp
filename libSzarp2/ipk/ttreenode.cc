/* 
  SZARP: SCADA software 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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

	int depth = xmlTextReaderDepth(reader);
	XMLWrapper xw(reader);

	bool isEmptyTag = xw.IsEmptyTag();

	const char* need_attr[] = { "name", 0 };
	if (!xw.AreValidAttr(need_attr)) {
		throw XMLWrapperException();
	}

	for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {
		const xmlChar *attr = xw.GetAttr();
		try {
			if (xw.IsAttr("name")) {
				_name = SC::U2S(attr);
			} else
			if (xw.IsAttr("prior")) {
				_prior = boost::lexical_cast<double>(attr);
			} else
			if (xw.IsAttr("draw_prior")) {
				_draw_prior = boost::lexical_cast<double>(attr);
			} else {
				xw.XMLWarningNotKnownAttr();
			}
		} catch (boost::bad_lexical_cast &) {
			xw.XMLErrorWrongAttrValue();
		}
	}

	if (isEmptyTag)
		return 0;

	if (!xw.NextTag())
		return 1;

	if (xw.IsTag("treenode")) {
		if (xw.IsBeginTag()) {
			_parent = new TTreeNode();
			if (_parent->parseXML(reader))
				return 1;
			if (!xw.NextTag())
				return 1;
		}
		if (xw.IsEndTag()) {
			int _tmp = xmlTextReaderDepth(reader);
			assert( _tmp == depth && "treenode recurrent problem");
		}
	}

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
