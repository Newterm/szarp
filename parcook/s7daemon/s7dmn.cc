#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <map>
#include <set>
#include <cstdint>
#include <algorithm>

#include "conversion.h"
#include "liblog.h"

#include "../base_daemon.h"

#include "s7dmn.h"
#include "datatypes.h"

S7Daemon::S7Daemon(BaseDaemon& _base_dmn): base_dmn(_base_dmn) {
	_dumpHex = base_dmn.getArgsMgr().has("dumphex");
	ParseConfig(base_dmn.getDaemonCfg());

	base_dmn.setCycleHandler([this](BaseDaemon&){
		Read();
		Transfer();
	});
};

void S7Daemon::ParseConfig(const DaemonConfigInfo &cfg)
{	
	sz_log(5, "S7Dmn::ParseConfig");
	
	_client.Configure(cfg);

	int i = 0;
	for (auto unit: cfg.GetUnits()) {
		for (auto param: unit->GetParams()) {
			_client.ConfigureParam(i, param);
			_pmap.ConfigureParam(i, param);
			i++;
		}

		for (auto param: unit->GetSendParams()) {
			_client.ConfigureSend(i, param);
			_pmap.ConfigureSend(i, param);
			i++;
		}
	}

	/** Sort and merge all available read/write queries */
	_client.BuildQueries();
}

void S7Daemon::Read()
{
	sz_log(5, "S7Dmn::Read");

	/* Fill read queries with data from PLC */
	if( _client.Connect() ) {
		_client.AskAll();
	}
}

void S7Daemon::Transfer()
{
	sz_log(5, "S7Dmn::Transfer");
	base_dmn.getIpc().receive();

	/** Write SZARP DB with values from read queries */ 
	_pmap.clearWriteBuffer();
	_client.ProcessResponse([&](unsigned long int idx, DataBuffer data, bool no_data) {
		if(no_data) { base_dmn.getIpc().setRead(idx, SZARP_NO_DATA); return; }

		_pmap.WriteData(idx, data, [&](unsigned long int pid, int16_t value) {
			base_dmn.getIpc().setRead(pid, value);
		});
	});	
	
	/** 
	 * Assume all write queries have data.
	 * If part of merged query will lack data, whole query will not be send to device.
	 */
	_client.ClearWriteNoDataFlags();

	/** Read SZARP DB and write values to write queries */
	_pmap.clearReadBuffer();
	_client.AccessData([&](unsigned long int idx, DataDescriptor desc) {
		SzInteger send_value = SZARP_NO_DATA;
		return _pmap.ReadData(idx, desc, [&](unsigned long int pid) {
			send_value = base_dmn.getIpc().getSend<int16_t>(pid - base_dmn.getDaemonCfg().GetParamsCount());
			return send_value;
		});
	});

	/** Send write queries to PLC */
	if(_client.Connect()) _client.TellAll();

	if(_dumpHex) _client.DumpAll();

	base_dmn.getIpc().publish();
}


int main(int argc, const char *argv[])
{
	BaseDaemonFactory::Go<S7Daemon>(argc, argv, "s7dmn");
	return 0;
}
