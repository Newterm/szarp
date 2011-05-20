/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 *
 * IPK 'boiler' class implementation.
 *
 * $Id$
 */

extern "C" {
#include <assert.h>
}
#include "liblog.h"
#include "szarp_config.h"
#include "conversion.h"

const TBoiler::BoilerTypeName TBoiler::BoilerTypesNames[] = {
    {TBoiler::INVALID, 	L"-"}, 
    {TBoiler::WR1_25, 	L"WR-1,25"}, 
    {TBoiler::WR2_5, 	L"WR-2,5"}, 
    {TBoiler::WR5, 	L"WR-5"}, 
    {TBoiler::WR10, 	L"WR-10"}, 
    {TBoiler::WR25, 	L"WR-25"}
};

const std::wstring& TBoiler::GetNameForBoilerType(BoilerType type) {
	for (unsigned i = 0; i < sizeof(BoilerTypesNames)/sizeof(BoilerTypeName); ++i) 
		if (BoilerTypesNames[i].type_id == type)
			return BoilerTypesNames[i].name;
	return BoilerTypesNames[0].name;
}

TBoiler::BoilerType TBoiler::GetTypeForBoilerName(const std::wstring& name) {
	for (unsigned i = 0; i < sizeof(BoilerTypesNames)/sizeof(BoilerTypeName); ++i) 
		if (BoilerTypesNames[i].name == name)
			return BoilerTypesNames[i].type_id;
	return INVALID;
}	

TBoiler* TBoiler::Append(TBoiler* b) {
		if (next == NULL)
			return next = b;
		else
			return next->Append(b);
}

TParam* TBoiler::GetParam(TAnalysis::AnalysisParam param_type) {
	assert(parent != NULL);

	TParam* param;
	for (param = parent->GetFirstParam(); param;
		param = parent->GetNextParam(param)) {

		TAnalysis* analysis;

		for (analysis = param->GetFirstAnalysis();
				analysis;
				analysis = analysis->GetNext()) {
			if (analysis->GetBoilerNo() == boiler_no
				&& analysis->GetParam() == param_type)
				return param;
		}
	}

	return NULL;
}

TBoiler* TBoiler::parseXML(xmlTextReaderPtr reader) {
//TODO: remove 
printf("name boiler: parseXML\n");

	TBoiler *boiler = NULL;
	int boiler_no;
	float grate_speed;
	float coal_gate_height;
	BoilerType boiler_type;

	XMLWrapper xw(reader);

	const char* need_attr[] = { "boiler_no" , "grate_speed", "coal_gate_height", "boiler_type", 0};
	if (!xw.AreValidAttr(need_attr)) {
//TODO: check it: ommit all tree?
		return NULL;
	}

	for (bool isAttr = xw.IsFirstAttr(); isAttr == true; isAttr = xw.IsNextAttr()) {

		const xmlChar* attr = xw.GetAttr();

		if (xw.IsAttr("boiler_no")) {
			if ((sscanf((const char*)attr,"%d",&boiler_no) != 1) || (boiler_no < 0)) {
				xw.XMLError("Invalid 'boiler_no' attribute on element 'boiler'");
				return NULL;
			}
		} else
		if (xw.IsAttr("grate_speed")) {
			if ((sscanf((const char*)attr,"%f",&grate_speed) != 1) || (grate_speed < 0)) {
				xw.XMLError("Invalid 'grate_speed' attribute on element 'boiler'");
				return NULL;
			}
		} else
		if (xw.IsAttr("coal_gate_height")) {
			if ((sscanf((const char*)attr,"%f",&coal_gate_height) != 1) || (coal_gate_height < 0)) {
				xw.XMLError("Invalid 'max_coal_gate_height_change' attribute on element 'boiler'");
				return NULL;
			}
		} else
		if (xw.IsAttr("boiler_type")) {
			boiler_type = GetTypeForBoilerName(SC::U2S((const unsigned char*)attr));
			if (boiler_type == INVALID ) {
				xw.XMLError("Invalid 'boiler_type' attribute on element 'boiler'");
				return NULL;
			}
		} else {
//TODO: make it xmlerrorattr or sth simmilar
			printf("<boiler> not known attr: %s\n",xw.GetAttrName());
		}

	}

	boiler = new TBoiler(boiler_no, grate_speed, coal_gate_height, boiler_type);

	xw.NextTag();

	for (;;) {
		if (xw.IsTag("interval")) {
			if (xw.IsBeginTag()) {
				TAnalysisInterval *interval = TAnalysisInterval::parseXML(reader);
				if (!interval) {
					delete boiler;
					return NULL;
				}
				boiler->AddInterval(interval);
			}
			xw.NextTag();
		} else
		if (xw.IsTag("boiler")) {
			return boiler;
		}
		else {
			xw.XMLErrorNotKnownTag("boiler");
			assert(xw.GetTagName() == NULL && "not know name");
		}
	}

printf("name boiler parseXML END\n");

	return boiler;
}

TBoiler* TBoiler::parseXML(xmlNodePtr node) {

#define NOATR(p, n) \
	{ \
		sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, SC::U2A(p->name).c_str(), xmlGetLineNo(p)); \
			return NULL; \
	}
#define NEEDATR(p, n) \
	if (c) xmlFree(c); \
	c = (char *)xmlGetNoNsProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);

	char *c = NULL;
	TBoiler *boiler = NULL;

	int boiler_no;
	NEEDATR(node, "boiler_no");
	if ((sscanf(c,"%d",&boiler_no) != 1) || (boiler_no < 0)) {
		sz_log(1, "Invalid 'boiler_no' attribute on element 'boiler' (line %ld)", 
			xmlGetLineNo(node));
		xmlFree(c);
		return NULL;
	}

	float grate_speed;
	NEEDATR(node, "grate_speed");
	if ((sscanf(c,"%f",&grate_speed) != 1) || (grate_speed < 0)) {
		sz_log(1, "Invalid 'grate_speed' attribute on element 'boiler' (line %ld)", 
			xmlGetLineNo(node));
		xmlFree(c);
		return NULL;
	}

	float coal_gate_height;
	NEEDATR(node, "coal_gate_height");
	if ((sscanf(c,"%f",&coal_gate_height) != 1) || (coal_gate_height < 0)) {
		sz_log(1, "Invalid 'max_coal_gate_height_change' attribute on element 'boiler' (line %ld)", 
			xmlGetLineNo(node));
		xmlFree(c);
		return NULL;
	}

	NEEDATR(node, "boiler_type");
	BoilerType boiler_type = GetTypeForBoilerName(SC::U2S((unsigned char*)c));
	if (boiler_type == INVALID ) {
		sz_log(1, "Invalid 'boiler_type' attribute on element 'boiler' (line %ld)", 
			xmlGetLineNo(node));
		xmlFree(c);
		return NULL;
	}
	xmlFree(c);

	boiler = new TBoiler(boiler_no, grate_speed, coal_gate_height,
			boiler_type);
			
	xmlNodePtr ch;
	for (ch = node->children; ch; ch = ch->next) 
		if (!strcmp((char *)ch->name, "interval")) {
			TAnalysisInterval *interval = TAnalysisInterval::parseXML(ch);
			if (!interval)
				goto failure;
			boiler->AddInterval(interval);
		}
	return boiler;

failure:
	delete boiler;
	return NULL;

#undef NEEDATR
#undef NOATR
}

xmlNodePtr TBoiler::generateXMLNode() {
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define FTOA(x) snprintf(buffer, 10, "%.2f", x)
#define X (const xmlChar*)
	char buffer[10];
	xmlNodePtr r = xmlNewNode(NULL, X"boiler");

	ITOA(boiler_no);
	xmlSetProp(r, X"boiler_no", SC::A2U(buffer).c_str());

	FTOA(grate_speed);
	xmlSetProp(r, X"grate_speed", SC::A2U(buffer).c_str());

	FTOA(coal_gate_height);
	xmlSetProp(r, X"coal_gate_height", SC::A2U(buffer).c_str());

	const std::wstring & boiler_type_name = GetNameForBoilerType(boiler_type);
	if (boiler_type_name.empty()) {
		sz_log(1,"Cannot generate xml node for a boiler (%d), unrecognized boiler type",boiler_no);
		goto failure;
	}	

	xmlSetProp(r, X"boiler_type", SC::S2U(boiler_type_name).c_str());

	for (TAnalysisInterval *in = intervals; in; in = in->GetNext()) {
		xmlNodePtr node = in->generateXMLNode();
		if (node == NULL) 
			goto failure;
		xmlAddChild(r, node);
	}

	return r;

failure:
	xmlFree(r);
	return NULL;
#undef ITOA
#undef FTOA
}

TAnalysisInterval* TBoiler::AddInterval(TAnalysisInterval* interval) {
	if (intervals == NULL) 
		return intervals = interval;
	else
		return intervals->Append(interval);
}

TBoiler::~TBoiler() {
	delete next;
	delete intervals;
}

