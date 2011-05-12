/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 *
 * IPK 'analysis' class implementation.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

#include "conversion.h"
#include "liblog.h"
#include "szarp_config.h"

const TAnalysis::ParamName TAnalysis::ParamsNames[] = {
    {TAnalysis::ANALYSIS_ENABLE, 			L"analysis_enable"}, 
    {TAnalysis::REGULATOR_WORK_TIME, 			L"regulator_work_time"}, 
    {TAnalysis::COAL_VOLUME, 				L"coal_volume"}, 
    {TAnalysis::ENERGY_COAL_VOLUME_RATIO, 		L"energy_coal_volume_ratio"}, 
    {TAnalysis::LEFT_GRATE_SPEED, 			L"left_grate_speed"}, 
    {TAnalysis::RIGHT_GRATE_SPEED, 			L"right_grate_speed"}, 
    {TAnalysis::MAX_AIR_STREAM, 			L"max_air_stream"}, 
    {TAnalysis::MIN_AIR_STREAM, 			L"min_air_stream"}, 
    {TAnalysis::LEFT_COAL_GATE_HEIGHT, 			L"left_coal_gate_height"}, 
    {TAnalysis::RIGHT_COAL_GATE_HEIGHT, 		L"right_coal_gate_height"}, 
    {TAnalysis::ANALYSIS_STATUS, 			L"analysis_status"}, 
    {TAnalysis::AIR_STREAM, 				L"air_stream"},
    {TAnalysis::AIR_STREAM_RESULT, 			L"air_stream_result"},
    {TAnalysis::INVALID, 				L"-"}
};

const std::wstring& TAnalysis::GetNameForParam(AnalysisParam type) {
	unsigned i;
	for (i = 0; ParamsNames[i].name_id != INVALID; ++i)
		if (ParamsNames[i].name_id == type)
			return ParamsNames[i].name;

	return ParamsNames[i].name;
}	

TAnalysis::AnalysisParam TAnalysis::GetTypeForParamName(const std::wstring &name) {
	for (unsigned i = 0; ParamsNames[i].name != std::wstring(L"-"); ++i) 
		if (ParamsNames[i].name == name)
			return ParamsNames[i].name_id;
	return INVALID;
}	


TAnalysis* TAnalysis::Append(TAnalysis* analysis) {
	if (this->next)
		return next->Append(analysis);
	else
		return next = analysis;
}

xmlNodePtr TAnalysis::generateXMLNode() {
#define X (const xmlChar*)
	xmlNodePtr r;

	r = xmlNewNode(NULL, X"analysis");
	
	std::wstringstream wss;
	wss << boiler_no;
	xmlSetProp(r, X"boiler_no", SC::S2U(wss.str()).c_str());

	const std::wstring& name = GetNameForParam(param);
	if (name.empty()) {
		sz_log(1,"Cannot generete analysis element. Invalid parameter type.");
		xmlFree(r);
		return NULL;
	}

	xmlSetProp(r, X"param_type", SC::S2U(name).c_str());
	
	return r;
#undef X
}

TAnalysis* TAnalysis::parseXML(xmlTextReaderPtr reader) {

	printf("analysis: param parseXML\n");

	const xmlChar *attr_name = NULL;
	const xmlChar *attr = NULL;

#define IFATTR(ATT) if (xmlStrEqual(attr_name, (unsigned char*) ATT) )
#define XMLERROR(STR) sz_log(1,"XML file error: %s (line,%d)", STR, xmlTextReaderGetParserLineNumber(reader));
#define XMLERRORATTR(ATT) sz_log(1,"XML parsing error: expected attribute '%s' (line: %d)", ATT, xmlTextReaderGetParserLineNumber(reader));
#define FORALLATTR for (int __atr = xmlTextReaderMoveToFirstAttribute(reader); __atr > 0; __atr =  xmlTextReaderMoveToNextAttribute(reader) )
#define GETATTR attr_name = xmlTextReaderConstName(reader); attr = xmlTextReaderConstValue(reader);
#define CHECKNEEDEDATTR(LIST) \
	if (sizeof(LIST) > 0) { \
		std::set<std::string> __tmpattr(LIST, LIST + (sizeof(LIST) / sizeof(LIST[0]))); \
		FORALLATTR { GETATTR; __tmpattr.erase((const char*) attr_name); } \
		if (__tmpattr.size() > 0) { XMLERRORATTR(__tmpattr.begin()->c_str()); return NULL; } \
	}

	const char* need_attr_param[] = { "boiler_no","param_type" };
	CHECKNEEDEDATTR(need_attr_param);

	int bnr = 0;
	TAnalysis::AnalysisParam param ; 

	FORALLATTR {
		GETATTR

		IFATTR("boiler_no") {
			bnr = atoi((const char*) attr);
		} else
		IFATTR("param_type") {
			param = GetTypeForParamName(SC::U2S((unsigned char*)attr));
		} else {
			printf("ERROR<analysis>: not known attr:%s\n",attr_name);
		}
	}
	if (param == TAnalysis::INVALID) {
		XMLERROR("Incorrect value of 'param_type' attribute on element 'analysis'");
		return NULL;
	}

	printf("analysis: param parseXML END\n");

#undef IFATTR
#undef XMLERROR
#undef XMLERRORATTR
#undef FORALLATTR
#undef GETATTR
#undef CHECKNEEDEDATTR


	return new TAnalysis(bnr, param);
}

TAnalysis* TAnalysis::parseXML(xmlNodePtr node) {
#define X (const xmlChar*)

	char *ch = NULL;
	
	ch = (char*)xmlGetProp(node, X"boiler_no");

	if (!ch) {
		sz_log(1, "Attribute 'boiler_no' on 'analysis' element not found (line %ld)",
				xmlGetLineNo(node));
		return NULL;
	}
	int boiler_no = atoi(ch);
	xmlFree(ch);

	ch = (char*)xmlGetProp(node, X"param_type");
	if (!ch) {
		sz_log(1, "Attribute 'param_type' on 'analysis' element not found (line %ld)",
				xmlGetLineNo(node));
		return NULL;
	}

	TAnalysis::AnalysisParam param = GetTypeForParamName(SC::U2S((unsigned char*)ch));
	xmlFree(ch);
	if (param == TAnalysis::INVALID) {
		sz_log(1,"Incorrect value of 'param_type' attribute on element 'analysis', line(%ld)",
				xmlGetLineNo(node));
		return NULL;
	}

	return new TAnalysis(boiler_no, param);
#undef X
}

TAnalysis::~TAnalysis() {
	delete next;
}
