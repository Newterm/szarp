#include "param.h"

#include <sstream>
#include <cmath>
#include <limits>

#include "utils/ptree.h"

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace bp = boost::property_tree;

Param::Param( const std::string& parent_tag )
	: parent_tag(parent_tag)
{
}

Param::~Param()
{
}

void Param::from_params_xml( const bp::ptree& ptree ) throw(xml_parse_error)
{
	param_desc = ptree;

	name = param_desc.get<std::string>("@name");

	param_desc.erase("@base_ind");
	param_desc.erase("define");

	double min = std::numeric_limits<double>::quiet_NaN();
	double max = std::numeric_limits<double>::quiet_NaN();
	using std::isnan;

	if( param_desc.count("draw") ) {
		auto& draw_desc = param_desc.get_child("draw");
		for( auto ic=draw_desc.begin() ; ic!=draw_desc.end() ; ++ic )
			try {
				if( ic->first == "@min" ) {
					auto m = boost::lexical_cast<double>( ic->second.data() );
					if( isnan(min) || m < min ) min = m;
				}

				if( ic->first == "@max" ) {
					auto m = boost::lexical_cast<double>( ic->second.data() );
					if( isnan(max) || m > max ) max = m;
				}


			} catch( const boost::bad_lexical_cast& ) {
				throw xml_parse_error("Invalid min or max value at param_desc " + name);
			}
	}

	param_desc.erase("draw");
	param_desc.erase("raport");

	param_desc.put("@min",min);
	param_desc.put("@max",max);

	param_desc.put("@type",parent_tag);
}

void Param::to_json( std::ostream& stream , bool pretty ) const
{
	ptree_to_json( stream , param_desc , pretty );
}

std::string Param::to_json( bool pretty ) const
{
	std::stringstream ss;
	to_json( ss , pretty );
	return ss.str();
}

void Param::to_xml( std::ostream& stream , bool pretty ) const
{
	bp::ptree pt;
	pt.put_child( "var" , param_desc );
	unfold_xmlattr( pt );

	ptree_to_xml( stream , pt , pretty );
}

std::string Param::to_xml( bool pretty ) const
{
	std::stringstream ss;
	to_xml( ss , pretty );
	return ss.str();
}

