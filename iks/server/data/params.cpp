#include "params.h"

#include <cmath>
#include <chrono>
#include <functional>

namespace p = std::placeholders;

#include <boost/format.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::format;
namespace bp = boost::property_tree;

#include "global_service.h"

#include "utils/ptree.h"

Params::Params()
	: timeout(GlobalService::get_service())
{
}

void Params::set_data_feeder( SzbaseWrapper* data_feeder_ )
{
	data_feeder = data_feeder_;
}

std::shared_ptr<const Param> Params::get_param( const std::string& name ) const
{
	auto itr = params.find(name);
	if( itr == params.end() )
		return std::shared_ptr<const Param>();
	else
		return itr->second;
}

void Params::from_params_file( const std::string& path ) throw(xml_parse_error)
{
	bp::ptree params_doc;
	bp::read_xml(path, params_doc, bp::xml_parser::trim_whitespace|bp::xml_parser::no_comments );

	from_params_xml( params_doc );
}

void Params::create_param( bp::ptree::value_type& p , const std::string& type )
{
	if( p.first == "param" ) {
		auto param = std::make_shared<Param>( type );
		param->from_params_xml( p.second );
		params[ param->get_name() ] = std::move(param);
	}
}

void Params::from_params_xml( bp::ptree& doc ) throw(xml_parse_error)
{
	fold_xmlattr( doc );

	auto& params_ptree = doc.get_child("params");
	for( auto id=params_ptree.begin() ; id!=params_ptree.end() ; ++id )
		ptree_foreach( id->second ,
			std::bind(
				&Params::create_param, this,
				std::placeholders::_1, id->first ) );

}

void Params::param_value_changed( const std::string& name , double value )
{
	param_value_changed( iterator(params.find(name)) , value );
}

void Params::param_value_changed( iterator itr , double value )
{
	if( itr.itr == params.end() ) {
		std::cerr << "Value changed of undefined param" << std::endl;
		return;
	}

	using std::isnan;

	auto pvalue = itr.itr->second->get_value();
	if( value == pvalue || (isnan(value) && isnan(pvalue)) )
		/** If values are equal even if both are NaNs do nothing */
		return;

	itr.itr->second->set_value( value );
	emit_value_changed( itr.itr->second );
}

void Params::request_param_value( const std::string& name ,
                                  double value ,
								  const std::string& pin ) const
{
	auto itr = params.find(name);

	if( itr == params.end() ) {
		std::cerr << "Value requested of undefined param" << std::endl;
		return;
	}

	emit_request_value(
			std::make_shared<ParamRequest>( ParamRequest{ itr->second , value , pin } ) );
}

bool Params::subscribe_param( const std::string& name , bool update )
{
	if( !data_updater ) {
		data_updater = std::make_shared<DataUpdater>( *this );
	}

	auto itr = params.find(name);
	if( itr == params.end() )
		return false;

	subscription.insert( name );

	if( update )
		data_updater->check_szarp_values();

	return true;
}

void Params::subscription_clear()
{
	/** Remove subscription and turn updater off */
	data_updater = std::shared_ptr<DataUpdater>();
	timeout.cancel();
	subscription.clear();
}

void Params::DataUpdater::check_szarp_values( const boost::system::error_code& e )
{
	std::cerr << "check_szarp_values" << std::endl;

	if( e ) return;

	using std::chrono::system_clock;
	using namespace boost::posix_time;

	time_t t = system_clock::to_time_t(system_clock::now());
	t = SzbaseWrapper::round( t , PT_SEC10 );

	try {
		for( auto itr=params.subscription.begin() ;
		     itr!=params.subscription.end() ;
		     ++itr )
		{
			auto v = params.data_feeder->get_avg( *itr , t , PT_SEC10 );
			std::cerr << *itr << " " << v << std::endl;
			params.param_value_changed( *itr , v );
		}
	} catch( szbase_error& e ) {
		/* TODO: Better error handling (22/05/2014 20:54, jkotur) */
		std::cerr << "Szbase error: " << e.what() << std::endl;
	}

	t = system_clock::to_time_t(system_clock::now());
	t = SzbaseWrapper::round( t , PT_SEC10 );
	t = SzbaseWrapper::next ( t , PT_SEC10 );

	params.timeout.expires_at( from_time_t(t) + seconds(1));
	params.timeout.async_wait( std::bind(&Params::DataUpdater::check_szarp_values,shared_from_this(),p::_1) );
}

