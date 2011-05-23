/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 *
 * IPK 'analysisinterval' class implementation.
 *
 * $Id$
 */

extern "C" {
#include <assert.h>
}
#include "liblog.h"
#include "szarp_config.h"


TAnalysisInterval* TAnalysisInterval::parseXML(xmlTextReaderPtr reader) {

	int duration;
	int grate_speed_upper;
	int grate_speed_lower;

	XMLWrapper xw(reader);

	const char* need_attr[] = { "duration", "grate_speed_upper", "grate_speed_lower", 0 };
	if (!xw.AreValidAttr(need_attr)) {
//TODO: check it: ommit all tree?
		return NULL;
	}

	for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {

		const xmlChar* attr = xw.GetAttr();

		if(xw.IsAttr("duration")) {
			if ((sscanf((const char*)attr,"%d",&duration) != 1) || (duration < 0)) {
				xw.XMLError("Invalid 'duration' attribute on element 'interval'");
				return NULL;
			}
		} else
		if(xw.IsAttr("grate_speed_upper")) {
			if ((sscanf((const char*) attr,"%d",&grate_speed_upper) != 1) || (grate_speed_upper< 0)) {
				xw.XMLError("Invalid 'grate_speed_upper' attribute on element 'interval'");
				return NULL;
			}
		} else
		if(xw.IsAttr("grate_speed_lower")) {
			if ((sscanf((const char*) attr, "%d", &grate_speed_lower) != 1) || (grate_speed_lower < 0)) {
				xw.XMLError("Invalid 'grate_speed_lower' attribute on element 'interval'");
				return NULL;
			}
		} else {
			xw.XMLErrorNotKnownAttr();
		}
	}

	return new TAnalysisInterval(grate_speed_lower, grate_speed_upper, duration);

}

TAnalysisInterval* TAnalysisInterval::parseXML(xmlNodePtr node) {
#define NOATR(p, n) \
	{ \
		sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, p->name, xmlGetLineNo(p)); \
			return NULL; \
	}
#define NEEDATR(p, n) \
	if (c) xmlFree(c); \
	c = (char *)xmlGetNoNsProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
	assert(node != NULL);
	char *c = NULL;

	int duration;
	NEEDATR(node, "duration");
	if ((sscanf(c,"%d",&duration) != 1) || (duration < 0)) {
		sz_log(1, "Invalid 'duration' attribute on element 'interval' (line %ld)", 
			xmlGetLineNo(node));
		xmlFree(c);
		return NULL;
	}

	int grate_speed_upper;
	NEEDATR(node, "grate_speed_upper");
	if ((sscanf(c,"%d",&grate_speed_upper) != 1) || (grate_speed_upper< 0)) {
		sz_log(1, "Invalid 'grate_speed_upper' attribute on element 'interval' (line %ld)", 
			xmlGetLineNo(node));
		xmlFree(c);
		return NULL;
	}
	
	int grate_speed_lower;
	NEEDATR(node, "grate_speed_lower");
	if ((sscanf(c, "%d", &grate_speed_lower) != 1) || (grate_speed_lower < 0)) {
		sz_log(1, "Invalid 'grate_speed_lower' attribute on element 'interval' (line %ld)", 
			xmlGetLineNo(node));
		xmlFree(c);
		return NULL;
	}
	xmlFree(c);
	return new TAnalysisInterval(grate_speed_lower, grate_speed_upper, duration);

#undef NEEDATR
#undef NOATR
}

xmlNodePtr TAnalysisInterval::generateXMLNode() {
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define X (const xmlChar*) 
	char buffer[10];
	xmlNodePtr r = xmlNewNode(NULL, X"interval");

	ITOA(duration);	
	xmlSetProp(r, X"duration", X buffer);

	ITOA(grate_speed_lower);
	xmlSetProp(r, X"grate_speed_lower", X buffer);

	ITOA(grate_speed_upper);
	xmlSetProp(r, X"grate_speed_upper", X buffer);

	return r;
#undef ITOA
#undef X
}

TAnalysisInterval* TAnalysisInterval::Append(TAnalysisInterval *interval) {
	if (next == NULL) 
		return next = interval;
	else
		return next->Append(interval);
}

TAnalysisInterval::~TAnalysisInterval() {
	delete next;
}
