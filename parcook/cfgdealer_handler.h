#ifndef CONFIGDEALERHANDLER__H__
#define CONFIGDEALERHANDLER__H__

#include "../libSzarp2/include/config_info.h"

#include "../libSzarp/include/argsmgr.h"
#include "dmncfg.h"

#include <sys/time.h>
#include <unordered_map>

namespace pt = boost::property_tree;

class ConfigDealerHandler: public DaemonConfigInfo {
	class CParam: public IPCParamInfo {
		unsigned int ipc_ind;

	public:
		CParam(unsigned _ind): ipc_ind(_ind) {}
		CParam(const boost::property_tree::wptree&, size_t);
		unsigned int GetIpcInd() const override { return ipc_ind; }

	};

	class CSend: public SendParamInfo {
		ConfigDealerHandler* parent;
		std::wstring sent_param_name;
		
	public:
		CSend(ConfigDealerHandler* _parent, const boost::property_tree::wptree&);

		IPCParamInfo* GetParamToSend() const {
			if (sent_param_name.empty()) {
				return nullptr;
			} else {
				if (parent == nullptr) return nullptr;
				return parent->GetParamToSend(sent_param_name);
			}
		}

		const std::wstring& GetParamName() const override {
			return sent_param_name;
		}

	};

	class CUnit: public UnitInfo {
	public:
		CUnit(ConfigDealerHandler* parent, const boost::property_tree::wptree&, size_t inds_offset);

		size_t GetSenderMsgType() const { return unit_no + 256L; }
		size_t GetParamsCount() const { return params.size(); }
		size_t GetSendParamsCount() const { return sends.size(); }

		SzarpConfigInfo* GetSzarpConfig() const { return nullptr; }

		wchar_t GetId() const { return SC::L2S(getAttribute<std::string>("id", "1"))[0]; }
		int GetUnitNo() const { return unit_no; }

		std::vector<IPCParamInfo*> GetParams() const { return params; }
		std::vector<SendParamInfo*> GetSendParams() const { return sends; }

	private:
		size_t unit_no;

		std::vector<IPCParamInfo*> params;
		std::vector<SendParamInfo*> sends;
	};

	class CDevice: public DeviceInfo {};

public:
	ConfigDealerHandler(const ArgsManager&);

private:
	void parseArgs(const ArgsManager&);
	void parseDevice(const boost::property_tree::wptree&);
	void parseSentParam(const boost::property_tree::wptree&);

public:

	IPCParamInfo* GetParamToSend(const std::wstring& name) { return sent_params[name]; }

	std::vector<UnitInfo*> GetUnits() const override { return units; }
	DeviceInfo* GetDeviceInfo() const override { return device; }
	const IPCInfo& GetIPCInfo() const override { return ipc_info; }
	bool GetSingle() const override { return single; }
	std::string GetIPKPath() const override { return IPKPath; }
	int GetLineNumber() const override { return device_no; }

	size_t GetParamsCount() const override { 
		size_t count = 0;
		for (const auto& u: units) {
			count += u->GetParamsCount();
		}
		return count;
	}

	size_t GetSendsCount() const override {
		size_t count = 0;
		for (const auto& u: units) {
			count += u->GetSendParamsCount();
		}
		return count;
	}

	size_t GetFirstParamIpcInd() const override { return ipc_offset; }
	std::vector<size_t> GetSendIpcInds() const override { return sends_inds; }

	std::string GetPrintableDeviceXMLString() const override { return device_xml; }

	// TODO
	timeval GetDeviceTimeval() const override { return timeval{10, 0}; }

protected:
	IPCInfo ipc_info;

	std::vector<UnitInfo*> units;
	DeviceInfo* device;
	std::vector<size_t> sends_inds;
	size_t params_count;
	std::unordered_map<std::wstring, IPCParamInfo*> sent_params;

	size_t ipc_offset;
	std::string device_xml;

	// ArgsMgr info
	size_t device_no;
	std::string IPKPath;
	bool single;
};

#endif
