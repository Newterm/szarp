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
/* $Id$ */


#include <sstream>
#include <iomanip>

#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "svg_generator.h"
#include "conversion.h"
#include "xmlutils.h"
#include "liblog.h"

#define X (const xmlChar*)

namespace {

template<class T> void set_node_prop(xmlNodePtr node, const xmlChar* name, T val) {
	std::wstringstream ws;
	ws << std::fixed;
	ws << val;
	xmlSetProp(node, name, SC::S2U(ws.str()).c_str());
}

}

bool SVGGenerator::GetParamsFromXml() {

	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(m_request_doc);
	xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(X"//param", xpath_ctx);

	if (xpath_obj->nodesetval) for (int i = 0; i < xpath_obj->nodesetval->nodeNr; i++) {
		xmlChar* _pn = xmlGetProp(xpath_obj->nodesetval->nodeTab[i], X"name"); 
		if (_pn == NULL)
			continue;

		m_params_names.push_back(SC::U2S(_pn));
		xmlFree(_pn);

		xmlChar* _color = xmlGetProp(xpath_obj->nodesetval->nodeTab[i], X"color"); 
		std::wstring color;
		if (_color) {
			color = SC::U2S(_color);
			xmlFree(_color);
		}
		m_params_colors.push_back(color);
	}

	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeContext(xpath_ctx);

	return m_params_names.size() != 0;
}

bool SVGGenerator::GetParamsValues() {
	
	for (size_t i = 0; i < m_params_names.size(); i++) {
		m_params_values.push_back(std::vector<double>());

		TParam* param = m_ipk_container->GetParam(m_params_names.at(i));
		if (param == NULL)
			return false;

		m_params.push_back(param);
		
		time_t t = m_start_time;
		int j = 0;
		while (j++ < m_graph_points_no) {
			bool ok, fixed;
			std::wstring error;
			double val;

			val = m_szbase->GetValue(m_params_names.at(i), t, m_probe_type, m_custom_length, &fixed, ok, error);
			if (!ok || std::isnan(val))
				val = 0;

			m_params_values[i].push_back(val);
			t = szb_move_time(t, 1, m_probe_type, m_custom_length);
		}

	}

	return true;
}

bool SVGGenerator::GetGraphTimeParams() {
	xmlChar *_start_time, *_end_time, *_period_type, *_custom_length;

	xmlXPathContextPtr xpath_ctx = xmlXPathNewContext(m_request_doc);

	_start_time = uxmlXPathGetProp(X"//graph/@start_time", xpath_ctx);
	_end_time = uxmlXPathGetProp(X"//graph/@end_time", xpath_ctx);
	_period_type = uxmlXPathGetProp(X"//graph/@period_type", xpath_ctx);
	_custom_length = uxmlXPathGetProp(X"//graph/@custom_length", xpath_ctx);

	time_t t, end_time;
	bool ret = false;

	if (_start_time == NULL || _end_time == NULL || _period_type == NULL) {
		goto end;
		return false;
	}

	m_custom_length = 0;
	if (!xmlStrcmp(_period_type, X"year"))
		m_probe_type = PT_YEAR;
	else if (!xmlStrcmp(_period_type, X"month"))
		m_probe_type = PT_MONTH;
	else if (!xmlStrcmp(_period_type, X"week"))
		m_probe_type = PT_WEEK;
	else if (!xmlStrcmp(_period_type, X"day"))
		m_probe_type = PT_DAY;
	else if (!xmlStrcmp(_period_type, X"hour8"))
		m_probe_type = PT_HOUR8;
	else if (!xmlStrcmp(_period_type, X"hour"))
		m_probe_type = PT_HOUR;
	else if (!xmlStrcmp(_period_type, X"min10"))
		m_probe_type = PT_MIN10;
	else if (!xmlStrcmp(_period_type, X"sec10"))
		m_probe_type = PT_SEC10;
	else if (!xmlStrcmp(_period_type, X"custom")) {
		m_probe_type = PT_CUSTOM;
		if (_custom_length == NULL)
			goto end;
		m_custom_length = atoi((char*)_custom_length);
	}

	m_start_time = atoi((char*)_start_time);
	end_time = atoi((char*)_end_time);
	m_graph_points_no = 1;
	t = m_start_time;
	m_probes_times.push_back(t);
	while (t < end_time) {
		t = szb_move_time(t, 1, m_probe_type, m_custom_length);
		m_probes_times.push_back(t);
		m_graph_points_no++;
	}

	ret = true;
end:
	xmlFree(_start_time); xmlFree(_end_time); xmlFree(_period_type); xmlFree(_custom_length);
	return ret;
}


xmlNodePtr SVGGenerator::GenerateParam(size_t draw_no, xmlDocPtr doc) {
	TParam* param = m_params.at(draw_no);
	if (param == NULL)
		return NULL;

	TDraw* draw = param->GetDraws();
	if (draw == NULL)
		return NULL;

	double dmin = draw->GetMin();
	double dmax = draw->GetMax();

	xmlNodePtr param_node = xmlNewNode(NULL, X"param");
	set_node_prop(param_node, X"min", dmin);
	set_node_prop(param_node, X"max", dmax);
	set_node_prop(param_node, X"prec", param->GetPrec());
	set_node_prop(param_node, X"unit", param->GetUnit());
	set_node_prop(param_node, X"descr", param->GetDrawName());
		
	std::wstring color;
	if (!m_params_colors.at(draw_no).empty())
		color = m_params_colors.at(draw_no);
	else
		color = draw->GetColor();
	set_node_prop(param_node, X"color", color);

	std::wstringstream ps;
	for (int i = 0; i < m_graph_points_no; i++) {
		std::wstringstream ssval;
		ssval << m_params_values.at(draw_no).at(i);
		xmlNodePtr tnode = xmlNewDocText(doc, SC::S2U(ssval.str()).c_str());

		xmlNodePtr value_node = xmlNewNode(NULL, X"value");
		xmlAddChild(value_node, tnode);
		xmlAddChild(param_node, value_node);

	}

	return param_node;
}

xmlNodePtr SVGGenerator::GenerateTimeInfo(xmlDocPtr doc) {
	xmlNodePtr time_node = xmlNewNode(NULL, X"probes_times");
	switch (m_probe_type) {
		case PT_YEAR:
			xmlSetProp(time_node, X"period", X"year");
			break;
		case PT_MONTH:
			xmlSetProp(time_node, X"period", X"month");
			break;
		case PT_WEEK:
			xmlSetProp(time_node, X"period", X"week");
			break;
		case PT_DAY:
			xmlSetProp(time_node, X"period", X"day");
			break;
		case PT_HOUR8:
			xmlSetProp(time_node, X"period", X"hour8");
			break;
		case PT_HOUR:
			xmlSetProp(time_node, X"period", X"hour");
			break;
		case PT_MIN10:
			xmlSetProp(time_node, X"period", X"min10");
			break;
		case PT_SEC10:
			xmlSetProp(time_node, X"period", X"sec10");
			break;
		case PT_SEC:
			xmlSetProp(time_node, X"period", X"sec");
			break;
		case PT_CUSTOM:
			xmlSetProp(time_node, X"period", X"custom");
			set_node_prop(time_node, X"period", m_custom_length);
			break;
		default:
			break;
	}

	for (size_t i = 0; i < m_probes_times.size(); i++) {
		std::wstringstream sst;
		sst << m_probes_times.at(i);
		xmlNodePtr tnode = xmlNewDocText(doc, SC::S2U(sst.str()).c_str());

		xmlNodePtr probe_time_node = xmlNewNode(NULL, X"probe");
		xmlAddChild(probe_time_node, tnode);
		xmlAddChild(time_node, probe_time_node);

	}

	return time_node;
}

xmlDocPtr SVGGenerator::StartDoc() {
	xmlDocPtr doc = xmlNewDoc(X "1.0");

        xmlNodePtr root = xmlNewDocNode(doc, NULL, X "graph", NULL);
	xmlDocSetRootElement(doc, root);

	return doc;
}


xmlDocPtr SVGGenerator::GenerateDoc() {
	xmlDocPtr doc = StartDoc();
	xmlNodePtr root = xmlDocGetRootElement(doc);
	for (size_t i = 0; i < m_params_names.size(); i++) {
		xmlNodePtr param_node = GenerateParam(i, doc);
		if (param_node == NULL) {
			xmlFreeDoc(doc);
			return NULL;
		}
		xmlAddChild(root, param_node);
	}
	xmlAddChild(root, GenerateTimeInfo(doc));
	return doc;
}

xmlChar* SVGGenerator::GenerateSVG(int *code) {
	if (!GetParamsFromXml()) {
		*code = 404;
		sz_log(1, "GenerateSVG: No params given");
		return xmlStrdup(X"No params given");
	}

	if (!GetGraphTimeParams()) {
		*code = 404;
		sz_log(1, "GenerateSVG: Invalid graph time range");
		return xmlStrdup(X"Invalid graph time range");
	}

	if (!GetParamsValues()) {
		*code = 404;
		sz_log(1, "GenerateSVG: Invalid parametrs' names");
		return xmlStrdup(X"Invalid parametrs' names");
	}

	xmlDocPtr doc = GenerateDoc();
	if (doc == NULL) {
		*code = 404;
		sz_log(1, "GenerateSVG: Unable to generate XML");
		return xmlStrdup(X"Unable to generate XML");
	}
	
	xmlChar *rb;
	int size;
	xmlDocDumpFormatMemory(doc, &rb, &size, 1);
	xmlFreeDoc(doc);

	*code = 200;
	return rb;
}

SVGGenerator::SVGGenerator(Szbase *szbase, xmlDocPtr request_doc) : m_szbase(szbase), m_request_doc(request_doc) {
	m_ipk_container = IPKContainer::GetObject();
};
