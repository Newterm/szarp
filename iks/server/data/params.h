#ifndef __DATA_PARAMS_H__
#define __DATA_PARAMS_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

#include "param.h"
#include "szbase_wrapper.h"

#include "utils/exception.h"
#include "utils/iterator.h"
#include "utils/signals.h"

class Params {
	typedef std::unordered_map<std::string,std::shared_ptr<Param>> ParamsMap;
	typedef std::unordered_set<std::string> ParamsSet;

public:
	class iterator : public key_iterator<ParamsMap> {
		friend class Params;
	public:
		iterator( const iterator& itr )
			: key_iterator<ParamsMap>(itr.itr) {}
		iterator( const ParamsMap::const_iterator& itr )
			: key_iterator<ParamsMap>(itr) {}
	};

	Params();

	void from_params_file( const std::string& path ) throw(xml_parse_error);

	iterator begin() const { return iterator(params.cbegin()); }
	iterator end  () const { return iterator(params.cend  ()); }

	bool has_param( const std::string& name ) const
	{	return params.count(name); }

	std::shared_ptr<const Param> get_param( const std::string& name ) const;

	void param_value_changed( iterator itr , double value );
	void param_value_changed( const std::string& name , double value );
	void request_param_value( const std::string& name ,
	                          double value ,
							  const std::string& pin = "" ) const;

	/** TODO: change SzbaseWrapper to generic DataFeeder class */
	void set_data_feeder( SzbaseWrapper* data_feeder = NULL );

	bool subscribe_param( const std::string& name , bool update = true );
	template<class Container> bool subscribe_params( const Container& names , bool update = true )
	{
		bool ret = true;

		for( auto itr=names.begin() ; itr!=names.end() ; ++itr )
			ret &= subscribe_param( *itr , false );

		if( update )
			data_updater->check_szarp_values();

		return ret;
	}
	void subscription_clear();

	slot_connection on_param_value_changed( const sig_param_slot& slot ) const
	{	return emit_value_changed.connect( slot ); }
	slot_connection on_request_param_value( const sig_param_request_slot& slot ) const
	{	return emit_request_value.connect( slot ); }

protected:
	void from_vars_xml( const boost::property_tree::ptree& vars_doc ) throw(xml_parse_error);
	void from_params_xml( boost::property_tree::ptree& params_doc ) throw(xml_parse_error);

	void create_param( boost::property_tree::ptree::value_type& p , const std::string& type );

private:
	ParamsMap params;
	ParamsSet subscription;

	SzbaseWrapper* data_feeder;

	mutable sig_param emit_value_changed;
	mutable sig_param_request emit_request_value;

	class DataUpdater : public std::enable_shared_from_this<DataUpdater> {
	public: 
		DataUpdater( Params& params ) : params(params) {}
		~DataUpdater() { std::cerr << "Die!" << std::endl; }

		void check_szarp_values( const boost::system::error_code& e = boost::system::error_code() );

		Params& params;
	};
	friend class Params::DataUpdater;
	std::shared_ptr<DataUpdater> data_updater;

	boost::asio::deadline_timer timeout;

};

#endif /* end of include guard: __DATA_PARAMS_H__ */

