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

		wchar_t GetId() const { return SC::A2S(getAttribute<std::string>("id", "1"))[0]; }
		int GetUnitNo() const { return unit_no; }

		std::vector<IPCParamInfo*> GetParams() const { return params; }
		std::vector<SendParamInfo*> GetSendParams() const { return sends; }

	private:
		size_t unit_no;

		std::vector<IPCParamInfo*> params;
		std::vector<SendParamInfo*> sends;
	};

public:
	ConfigDealerHandler(const ArgsManager&);

private:
	void parseArgs(const ArgsManager&);
	void parseDevice(const boost::property_tree::wptree&);
	void parseSentParam(const boost::property_tree::wptree&);

public:

	IPCParamInfo* GetParamToSend(const std::wstring& name) { return sent_params[name]; }

	virtual std::vector<UnitInfo*> GetUnits() const { return units; }
	virtual const IPCInfo& GetIPCInfo() const { return ipc_info; }
	virtual bool GetSingle() const { return single; }
	virtual std::string GetIPKPath() const { return IPKPath; }
	virtual int GetLineNumber() const { return device_no; }

	virtual size_t GetParamsCount() const { 
		size_t count = 0;
		for (const auto& u: units) {
			count += u->GetParamsCount();
		}
		return count;
	}

	virtual size_t GetSendsCount() const {
		size_t count = 0;
		for (const auto& u: units) {
			count += u->GetSendParamsCount();
		}
		return count;
	}

	virtual size_t GetFirstParamIpcInd() const { return ipc_offset; }
	virtual std::vector<size_t> GetSendIpcInds() const { return sends_inds; }

	virtual std::string GetPrintableDeviceXMLString() const { return device_xml; }

	// TODO
	virtual timeval GetDeviceTimeval() const { return timeval{10, 0}; }

protected:
	IPCInfo ipc_info;

	std::vector<UnitInfo*> units;
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
