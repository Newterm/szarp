#ifndef _REPORT_H_
#define _REPORT_H_

#include <boost/property_tree/ptree.hpp>
#include <vector>

// TODO: switch to wstring
struct ParamInfo {
	std::wstring name;
	std::wstring short_name;
	std::wstring unit;
	unsigned int prec;
	std::wstring description;
};

bool operator==(const ParamInfo& l, const ParamInfo&r);
bool operator!=(const ParamInfo& l, const ParamInfo&r);


struct ReportInfo {
	std::wstring name = L"";
	std::wstring config = L"";
	bool is_system = {false};

	std::vector<ParamInfo> params = std::vector<ParamInfo>();

	ReportInfo(const std::wstring& name = L"", const std::wstring& config = L"", bool is_system = false): name(name), config(config), params() {}
};

bool operator==(const ReportInfo& l, const ReportInfo&r);
bool operator!=(const ReportInfo& l, const ReportInfo&r);

struct ServerInfoHolder {
	std::string server;
	std::string port;
	std::wstring base;
};


namespace ReportParser {

ReportInfo FromFile(const std::string&);
void ToFile(const std::string&, const ReportInfo&);

ReportInfo FromXML(boost::property_tree::wptree&);
void ToXML(boost::property_tree::wptree&, const ReportInfo&);

std::vector<ReportInfo> FromParams(const std::string&, const std::wstring&);

}

#endif
