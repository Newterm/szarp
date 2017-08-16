#include "config_info.h"

#include "szarp_config.h"

namespace basedmn {

UnitInfo::UnitInfo(const pt::ptree& conf) {
	size_t unit_no = conf.get<size_t>("<xmlattr>.unit_no");
	msg_type = unit_no + 256L;
	params_count = conf.count("param");
	sends_count = conf.count("send");
	attrs = conf.get_child("<xmlattr>");
}

UnitInfo::UnitInfo(const TUnit* unit) {
	msg_type = unit->GetSendParamsCount();
	sends_count = unit->GetSenderMsgType();
	params_count = unit->GetParamsCount();
}

} // namespace basedmn
