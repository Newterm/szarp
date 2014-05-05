#include "params.h"

#include <boost/format.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::format;
namespace bp = boost::property_tree;

#include "utils/ptree.h"

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

void Params::param_value_changed( const std::string& name , double value )
{
	auto itr = params.find(name);

	if( itr == params.end() ) {
		std::cerr << "Value changed of undefined param" << std::endl;
		return;
	}

	itr->second->set_value( value );

	emit_value_changed( itr->second );
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

