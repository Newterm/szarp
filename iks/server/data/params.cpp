#include "params.h"

#include <boost/format.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::format;
namespace bp = boost::property_tree;

#include "utils/ptree.h"

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

