#include "s7client.h"

#include <boost/lexical_cast.hpp>

#include <limits>

#include "conversion.h"

const int S7Client::_default_rack = 0;
const int S7Client::_default_slot = 2;
const std::string S7Client::_default_address = std::string("127.0.0.1");

bool S7Client::ConfigureFromXml( xmlNodePtr& node )
{
	sz_log(7, "S7Client::ConfigureFromXml");

	xmlChar* prop = xmlGetNsProp(node, BAD_CAST("address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (prop == NULL) {
		sz_log(0, "No attribute address given in line %ld", xmlGetLineNo(node));
		return false;
	}
	_address = std::string((char*)prop);
	xmlFree(prop);	

	prop = xmlGetNsProp(node, BAD_CAST("rack"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (prop == NULL) {
		sz_log(2, "No attribute rack given, assuming default %d (line %ld)", _default_rack, xmlGetLineNo(node));
		_rack = _default_rack;
	}
	else {
		_rack = boost::lexical_cast<int>(prop);
		xmlFree(prop);	
	}

	prop = xmlGetNsProp(node, BAD_CAST("slot"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (prop == NULL) {
		sz_log(2, "No attribute slot given, assuming default %d (line %ld)", _default_slot, xmlGetLineNo(node));
		_slot = _default_slot;
	}
	else {
		_slot = boost::lexical_cast<int>(prop);
		xmlFree(prop);	
	}

	sz_log(5, "Configured with address: %s, rack: %d, slot: %d", _address.c_str(), _rack, _slot);
	return true;
}

S7QueryMap::S7Param S7Client::ConfigureParamFromXml( unsigned long int idx, xmlNodePtr& node )
{
	sz_log(7, "S7Client::ConfigureParamFromXml");

	/* db */
	int db = -1;
	xmlChar* prop = xmlGetNsProp(node, BAD_CAST("db"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == prop) {
		sz_log(2, "No attribute db given for param in line %ld",
			       	xmlGetLineNo(node));
	} else {
		db = boost::lexical_cast<int>((char*)prop);
		xmlFree(prop);
	}

	/* db_type */
	prop = xmlGetNsProp(node, BAD_CAST("db_type"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == prop) {
		sz_log(0, "No attribute db_type given for param in line %ld",
			       	xmlGetLineNo(node));
		return S7QueryMap::S7Param();
	}
	std::string db_type = std::string((char*)prop);
	xmlFree(prop);	

	/* address */
	prop = xmlGetNsProp(node, BAD_CAST("address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == prop) {
		sz_log(0, "No attribute address given for param in line %ld",
			       	xmlGetLineNo(node));
		return  S7QueryMap::S7Param();
	}
	int address = boost::lexical_cast<int>((char*)prop);
	xmlFree(prop);	

	/* val_type */
	prop = xmlGetNsProp(node, BAD_CAST("val_type"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == prop) {
		sz_log(0, "No attribute val_type given for param in line %ld",
			       	xmlGetLineNo(node));
		return S7QueryMap::S7Param();
	}
	std::string val_type = std::string((char*)prop);
	xmlFree(prop);
	
	int number = -1;
	if (val_type.compare(std::string("bit"))==0) {
		/*bit number*/
		prop = xmlGetNsProp(node, BAD_CAST("number"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (NULL == prop) {
			sz_log(0, "No attribute number given for param in line %ld",
					xmlGetLineNo(node));
			return S7QueryMap::S7Param();
		}
		number = boost::lexical_cast<int>((char*)prop);
		xmlFree(prop);
		if (number < 0 || number > 7) {
			sz_log(0, "Bit number must be from [0,7]");
			return S7QueryMap::S7Param();
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

bool S7Client::ConfigureParamFromXml( unsigned long int idx, TSendParam* s,  xmlNodePtr& node )
{
	sz_log(7, "S7Client::ConfigureParamFromXml(TSendParam)");
	
	S7QueryMap::S7Param param = ConfigureParamFromXml(idx,node);
	param.s7Write = true;

	return _s7qmap.AddQuery(idx, param);
}

bool S7Client::ConfigureParamFromXml( unsigned long int idx, TParam* p,  xmlNodePtr& node )
{
	sz_log(7, "S7Client::ConfigureParamFromXml(TParam)");
	
	S7QueryMap::S7Param param = ConfigureParamFromXml(idx,node);
	param.s7Write = false;

	return _s7qmap.AddQuery(idx, param);
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


