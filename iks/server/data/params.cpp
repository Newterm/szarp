#include "params.h"

#include <cmath>

#include <boost/format.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::format;
namespace bp = boost::property_tree;

#include <liblog.h>

#include "utils/ptree.h"

Params::Params()
{
}

std::shared_ptr<const Param> Params::get_param( const std::string& name ) const
{
	auto itr = params.find(name);
	return itr == params.end() ? std::shared_ptr<const Param>() : itr->second;
}

void Params::from_params_file( const std::string& path ) throw(xml_parse_error)
{
	bp::ptree params_doc;
	bp::read_xml(path, params_doc, bp::xml_parser::trim_whitespace|bp::xml_parser::no_comments );

	from_params_xml( params_doc );
}

void Params::from_params_xml( bp::ptree& doc ) throw(xml_parse_error)
{
	fold_xmlattr( doc );

	for( auto& d : doc.get_child("params") )
		ptree_foreach( d.second , [&] ( bp::ptree::value_type& p ) {
			if( p.first == "param" ) {
				auto param = std::make_shared<Param>( d.first );
				param->from_params_xml( p.second );
				params.emplace( param->get_name(), std::move(param) );
			}
		} );
}

void Params::param_value_changed( const std::string& name , double value , ProbeType pt )
{
	param_value_changed( iterator(params.find(name)) , value , pt );
}

void Params::param_value_changed( iterator itr , double value , ProbeType pt )
{
	if( itr.itr == params.end() ) {
		sz_log(1, "Value changed of undefined param %s", itr->c_str() );
		return;
	}

	using std::isnan;

	auto pvalue = itr.itr->second->get_value( pt );
	if( pt.get_type() == ProbeType::Type::LIVE
	 && (value == pvalue || (isnan(value) && isnan(pvalue))) )
		/**
		 * With LIVE probes if values are equal even
		 * if both are NaNs do nothing. For other probe
		 * types always send update.
		 */
		return;

	itr.itr->second->set_value( value , pt );
	emit_value_changed( itr.itr->second , value , pt );
}

void Params::request_param_value( const std::string& name ,
                                  double value ,
								  const std::string& pin ) const
{
	auto itr = params.find(name);

	if( itr == params.end() ) {
		sz_log(1, "Value requested of undefined param %s", name.c_str());
		return;
	}

	emit_request_value(
			std::make_shared<ParamRequest>( ParamRequest{ itr->second , value , pin } ) );
}

