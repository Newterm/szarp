#include "s7client.h"

#include <boost/lexical_cast.hpp>

#include <limits>

#include "conversion.h"

const int S7Client::_default_rack = 0;
const int S7Client::_default_slot = 2;
const std::string S7Client::_default_address = std::string("127.0.0.1");

void S7Client::Configure(const DaemonConfigInfo& dmn)
{
	auto device = dmn.GetDeviceInfo();
	_address = device->getAttribute<std::string>("extra:address");
	_rack = device->getAttribute<int>("extra:rack", _default_rack);
	_slot = device->getAttribute<int>("extra:slot", _default_slot);

	sz_log(5, "Configured with address: %s, rack: %d, slot: %d", _address.c_str(), _rack, _slot);
}

S7QueryMap::S7Param S7Client::ConfigureNode(unsigned long int idx, TAttribHolder *node)
{
	auto db = node->getAttribute<int>("extra:db");
	auto db_type = node->getAttribute<std::string>("extra:db_type", "db");
	auto address = node->getAttribute<int>("extra:address");
	auto val_type = node->getAttribute<std::string>("extra:val_type");
	
	int number = -1;
	if (val_type.compare(std::string("bit"))==0) {
		/*bit number*/
		number = node->getAttribute<int>("extra:number");

		if (number < 0 || number > 7) {
			throw std::runtime_error("Bit number must be from [0,7]");
		}
	}

	sz_log(7, "ParseParam param db:%d, db_type:%s address:%d, val_type:%s, number:%d",
			db,
			db_type.c_str(),
			address,
			val_type.c_str(),
			number);
	
	return S7QueryMap::S7Param(db,db_type,address,val_type,number,false);
}

void S7Client::ConfigureSend(unsigned long int idx, SendParamInfo *s)
{
	sz_log(7, "S7Client::ConfigureSend");
	
	S7QueryMap::S7Param param = ConfigureNode(idx, s);
	param.s7Write = true;

	if (!_s7qmap.AddQuery(idx, param)) {
		throw std::runtime_error("Could not add query, check previous logs.");
	}
}

void S7Client::ConfigureParam(unsigned long int idx, IPCParamInfo *p)
{
	sz_log(7, "S7Client::ConfigureParam");
	
	S7QueryMap::S7Param param = ConfigureNode(idx, p);
	param.s7Write = false;

	if (!_s7qmap.AddQuery(idx, param)) {
		throw std::runtime_error("Could not add query, check previous logs.");
	}
}

bool S7Client::IsConnected()	
{
	sz_log(10, "S7Client::IsConnected");

	int conn = 0;
	int ret = Cli_GetConnected(_s7client, &conn);
	if( (ret != 0) || (conn == 0) )
		return false;
	return true;
}

bool S7Client::Connect() 
{
	sz_log(10, "S7Client::Connect");

	int ret = Cli_ConnectTo(_s7client, _address.c_str(), _rack, _slot);
	if (ret != 0) {
		sz_log(2, "Cannot connect to: %s, rack: %d, slot: %d", _address.c_str(),_rack,_slot);
		return false;
	}
	return true;
}

bool S7Client::Reconnect() 
{
	sz_log(10, "S7Client::Reconnect");

	int ret = Cli_Connect(_s7client);
	if (ret != 0) {
		sz_log(2, "Cannot connect to: %s", _address.c_str());
		return false;
	}
	return true;
}

bool S7Client::BuildQueries() 
{
	sz_log(10, "S7Client::BuildQueries");

	_s7qmap.Sort();
	_s7qmap.Merge();

	return true;
}

bool S7Client::ClearWriteNoDataFlags()
{
	sz_log(10, "S7Client::ClearWriteNoDataFlags");

	return (_s7qmap.ClearWriteNoDataFlags());
}

bool S7Client::QueryAll()
{
	sz_log(10, "S7Client::QueryAll");

	return (_s7qmap.AskAll(_s7client) ||
		_s7qmap.TellAll(_s7client));
}



bool S7Client::AskAll()
{
	sz_log(10, "S7Client::AskAll");

	return (_s7qmap.AskAll(_s7client));
}

bool S7Client::TellAll()
{
	sz_log(10, "S7Client::TellAll");

	return (_s7qmap.TellAll(_s7client));
}

bool S7Client::DumpAll()
{
	sz_log(10, "S7Client::DumpAll");

	return _s7qmap.DumpAll();
}


