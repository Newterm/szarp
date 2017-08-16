#ifndef CONFIGDEALERHANDLER__H__
#define CONFIGDEALERHANDLER__H__

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "cmdlineparser.h"
#include "../libSzarp2/include/config_info.h"

namespace basedmn {

namespace pt = boost::property_tree;

class ConfigDealerHandler {
	struct DeviceInfo {
		std::vector<UnitInfo> units;

		size_t first_ipc_param_ind{ 0 };
		size_t device_no{ 1 };

		size_t params_count{ 0 };
		size_t sends_count{ 0 };

		pt::ptree attrs;

		DeviceInfo(const pt::ptree& conf, size_t _device_no);
		DeviceInfo() {}
	};

public:
	ConfigDealerHandler(const basedmn::ArgsManager&);

	std::string GetPrintableDeviceXMLString() const {
		std::ostringstream oss;
		pt::xml_parser::write_xml(oss, conf_tree);
		return std::move(oss.str());
	}

	bool GetSingle() const { return single; }

	size_t GetParamsCount() const { return device.params_count; }

	size_t GetSendsCount() const { return device.sends_count; }

	size_t GetLineNumber() const { return device.device_no; }

	const std::string& GetIPKPath() const { return ipk_path; }


	struct timeval GetDeviceTimeval() const {
		struct timeval tv;
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		return tv;
	}

	const ConfigDealerHandler& GetIPK() const { return *this; }

	size_t GetDevice() const { return device.device_no; }

	size_t GetFirstParamIpcInd(size_t) const { return device.first_ipc_param_ind; }

	std::vector<size_t> GetSendIpcInds(size_t device_no) const { return std::move(std::vector<size_t>{}); }

	const std::vector<UnitInfo>& GetUnits() const { return device.units; }

	const IPCInfo& GetIPCInfo() const { return ipc_info; }

private:
	bool single;

	std::string cfgdealer_address;
	std::string ipk_path;

	DeviceInfo device;
	IPCInfo ipc_info;

	pt::ptree conf_tree;

};

} // namespace basedmn

#endif
