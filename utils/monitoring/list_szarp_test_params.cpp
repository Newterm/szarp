#include <iostream>
#include <string>
#include <vector>
#include "argsmgr.h"
#include "szarp_config.h"
#include "config_info.h"
#include "conversion.h"


class CheckDaemonArgs: public DefaultArgs {
public:
	po::options_description get_options() const override {
		po::options_description desc{"Options"};
		desc.add_options()
			("help,h", "Print this help messages")
			("version,V", "Output version")
			("base,b", po::value<std::string>()->required(), "Base to read configuration from")
			("override,D", po::value<std::vector<std::string>>()->multitoken(), "Override default arguments (-Dparam=val)");
		
		return desc;
	}
};

TParam* selectParam(TDevice* device) {
	TParam* selParam = nullptr;

	auto firstUnit = device->GetFirstUnit();
	
	for (auto param = firstUnit->GetFirstParam(); param != nullptr; param = param->GetNext()) {
		if (selParam == nullptr) {
			selParam = param;
		}
		if (param->hasAttribute("test_param") && param->getAttribute<std::string>("test_param") == "y") {
			selParam = param;
			break;
		}
	}
	return selParam;
}

std::vector<TParam*> getSelectedParams(TDevice* device) {
	std::vector<TParam*> selectedParams;

	for (;device != nullptr; device = device->GetNext()) {
		std::string device_daemon = device->getAttribute("daemon");
		
		/* check if daemon is not fake */
		if (device_daemon.find("dummy") != std::string::npos
		|| device_daemon.find("fake") != std::string::npos
		|| device_daemon.find("false") != std::string::npos
		|| device_daemon.find("test") != std::string::npos) {
			continue;
		}

		/* select params from devices */
		if (selectParam(device) != nullptr) {
			selectedParams.push_back(selectParam(device));
		}
	}
	return selectedParams;
}

int main(int argc, char* argv[]){
	ArgsManager args_mgr("check_szarp_daemon");
	args_mgr.parse(argc, argv, CheckDaemonArgs());
	args_mgr.initLibpar();

	boost::optional<std::string> base;
	if (!(base = args_mgr.get<std::string>("base"))) {
	        throw std::runtime_error("base is required!");
	}

	std::wstring base_name = SC::L2S(*base);
	std::wstring base_path = L"/opt/szarp/";
	base_path.append(base_name).append(L"/config/params.xml");
	
	TSzarpConfig sc;
	sc.loadXML(base_path, base_name);	

	std::vector<TParam*> selectedParams = getSelectedParams(sc.GetFirstDevice());

	/* print selected params */
	for (unsigned int i = 0; i < selectedParams.size(); i++) {
		std::cout<<selectedParams.at(i)->getAttribute<std::string>("name")<<std::endl;
	}
}
