#ifndef CONFIGINFO__H__
#define CONFIGINFO__H__

#include <string>
#include <boost/property_tree/ptree.hpp>

class TUnit;

namespace basedmn {

namespace pt = boost::property_tree;

struct IPCInfo {
	std::string GetParcookPath() const { return parcook_path; }
	std::string GetLinexPath() const { return linex_path; }

	std::string parcook_path;
	std::string linex_path;
};

struct UnitInfo {
	size_t GetSenderMsgType() const { return msg_type; }
	size_t GetParamsCount() const { return params_count; }
	size_t GetSendParamsCount() const { return sends_count; }

	size_t msg_type{ 257L };
	size_t params_count{ 0 };
	size_t sends_count{ 0 };

	pt::ptree attrs;

	UnitInfo(const pt::ptree&);
	UnitInfo(const TUnit*);
};

}


#endif
