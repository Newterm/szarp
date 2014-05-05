#ifndef __DATA_CONFIG_H__
#define __DATA_CONFIG_H__

#include <string>
#include <unordered_map>

#include <boost/property_tree/ptree.hpp>

#include "utils/signals.h"
#include "utils/exception.h"

class Config {
	typedef std::unordered_map<std::string,std::string> CfgMap;

public:

	void from_file( const std::string& path ) throw(xml_parse_error);
	void from_xml( const boost::property_tree::ptree& cfg_ptree ) throw(xml_parse_error);

	CfgMap::const_iterator begin() const { return cfg.cbegin(); }
	CfgMap::const_iterator end  () const { return cfg.cend  (); }

	bool has( const std::string& val ) const
	{	return cfg.count(val); }

	const std::string& get( const std::string& val )
	{	return cfg[val]; }

	const std::string& get( const std::string& val , const std::string& def ) const
	{
		auto itr = cfg.find(val);
		return itr == cfg.end() ? def : itr->second;
	}

	connection on_changed( const sig_void_slot& slot ) const
	{	return emit_changed.connect( slot ); }

private:
	CfgMap cfg;

	mutable sig_void emit_changed;
};

#endif /* end of include guard: __DATA_CONFIG_H__ */

