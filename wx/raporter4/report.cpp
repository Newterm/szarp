#include "report.h"

#include <boost/property_tree/xml_parser.hpp>

#include <locale>
#include <map>

namespace ReportParser {

ReportInfo FromXML(boost::property_tree::wptree& pt) {
	auto report_node = pt.get_child(L"report");

	std::wstring name_str = report_node.get<std::wstring>(L"<xmlattr>.title");
	std::wstring config_str = report_node.get<std::wstring>(L"<xmlattr>.source");
	
	ReportInfo report(name_str, config_str);
	

	for (const auto& param: pt.get_child(L"report")) {
		if (param.first != L"param") continue;

		ParamInfo pi = {
			param.second.get<std::wstring>(L"<xmlattr>.name"),
			param.second.get<std::wstring>(L"<xmlattr>.short_name"),
			param.second.get<std::wstring>(L"<xmlattr>.unit"),
			param.second.get(L"<xmlattr>.prec", (unsigned int) 0),
			param.second.get(L"<xmlattr>.description", L"")
		};

		report.params.push_back(pi);
	}

	return report;
}



void ToXML(boost::property_tree::wptree& pt, const ReportInfo& report) {
	boost::property_tree::wptree report_node;

	report_node.put(L"<xmlattr>.title", report.name);
	report_node.put(L"<xmlattr>.source", report.config);

	for (const auto& param: report.params) {
		boost::property_tree::wptree param_node;
		param_node.put(L"<xmlattr>.name", param.name);
		param_node.put(L"<xmlattr>.unit", param.unit);
		param_node.put(L"<xmlattr>.prec", param.prec);
		param_node.put(L"<xmlattr>.short_name", param.short_name);
		param_node.put(L"<xmlattr>.description", param.description);

		report_node.add_child(L"param", param_node);
	}

	pt.add_child(L"report", report_node);
}

ReportInfo FromFile(const std::string& file) {
	boost::property_tree::wptree report;

	read_xml(file, report, 0, std::locale("pl_PL.UTF-8"));
	return FromXML(report);
}


void ToFile(const std::string& file, const ReportInfo& report) {
	boost::property_tree::wptree pt;

	ToXML(pt, report);

	write_xml(file, pt);
}


std::wstring ParseReportElement(const boost::property_tree::wptree& pt, const boost::property_tree::wptree& rt, ParamInfo& pi)
{
	// name is "" if param was not initialized
	if (pi.name == L"") {
		pi.name = pt.get<std::wstring>(L"<xmlattr>.name");
		pi.short_name = pt.get<std::wstring>(L"<xmlattr>.short_name");
		pi.unit = pt.get<std::wstring>(L"<xmlattr>.unit");
		pi.prec = pt.get(L"<xmlattr>.prec", (unsigned int)0);
	}

	// even if param was already initialized we should update description
	pi.description = rt.get(L"<xmlattr>.description", L"");
	return rt.get<std::wstring>(L"<xmlattr>.title");
}

void ParseParamElement(std::map<std::wstring, ReportInfo*>& reports, const boost::property_tree::wptree& pt)
{
	ParamInfo cur_param;

	for (const auto& report: pt) {
		if (report.first != L"raport") continue;

		auto title = ParseReportElement(pt, report.second, cur_param);

		if (!reports.count(title)) {
			reports[title] = new ReportInfo(title);
		}

		reports[title]->params.push_back(cur_param);
	}
}


std::vector<ReportInfo> FromParams(const std::string& params_path, const std::wstring& config) {
	std::map<std::wstring, ReportInfo*> reports;

	boost::property_tree::wptree pt;
	read_xml(params_path, pt, 0, std::locale("pl_PL.UTF-8"));

	for (const auto& device: pt.get_child(L"params")) {
		if (device.first == L"device") {
			for (const auto& unit: device.second) {
				if (unit.first != L"unit") continue;

				for (const auto& param: unit.second) {
					if (param.first != L"param") continue;

					ParseParamElement(reports, param.second);
				} // param
			} // unit
		} // device

		else if (device.first == L"defined" || device.first == L"drawdefinable") {
			for (const auto& param: device.second) {
				if (param.first != L"param") continue;

				ParseParamElement(reports, param.second);
			} // param
		} // defined || drawdefinable
	} // params

	std::vector<ReportInfo> ret_reports;
	for (const auto& report_entry: reports) {
		ret_reports.push_back(*(report_entry.second));
		ret_reports.back().is_system = true;
		ret_reports.back().config = config;
		delete report_entry.second;
	}

	return ret_reports;
}

} // namespace ReportParser

bool operator==(const ParamInfo& l, const ParamInfo& r) {
	return l.name == r.name;
}

bool operator!=(const ParamInfo& l, const ParamInfo& r) {
	return !(l==r);
}

bool operator==(const ReportInfo& l, const ReportInfo& r) {
	return l.name == r.name &&
		   l.config == r.config &&
		   l.is_system == r.is_system &&
		   l.params == r.params;
}

bool operator!=(const ReportInfo& l, const ReportInfo& r) {
	return !(l==r);
}
