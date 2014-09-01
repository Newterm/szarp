#include "param.h"

#include <sstream>
#include <cmath>
#include <limits>

#include "utils/ptree.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
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
	/**
	 * name, unit and short_name are obligatory
	 */
	try {
		name = ptree.get<std::string>("@name");

		param_desc.put( "@name" , name );
		param_desc.put( "@unit" , ptree.get<std::string>( "@unit" ) );
		param_desc.put( "@short_name" , ptree.get<std::string>( "@short_name" ) );
	} catch( bp::ptree_error& e ) {
		throw xml_parse_error( str( boost::format("Cannot read param \"%s\": %s") % name % e.what() ) );
	}

	auto prec = ptree.get_optional<std::string>( "@prec" );
	param_desc.put( "@prec" , prec ? *prec : "0" );

	auto dname = ptree.get_optional<std::string>( "@draw_name" );
	param_desc.put( "@draw_name" , dname ? *dname : name );

	/**
	 * Find min and max value for this param based on his draw section
	 */
	double min = std::numeric_limits<double>::quiet_NaN();
	double max = std::numeric_limits<double>::quiet_NaN();
	using std::isnan;

	if( ptree.count("draw") )
		for( auto& c : ptree.get_child("draw") )
			try {
				if( c.first == "@min" ) {
					auto m = boost::lexical_cast<double>( c.second.data() );
					if( isnan(min) || m < min ) min = m;
				}

				if( c.first == "@max" ) {
					auto m = boost::lexical_cast<double>( c.second.data() );
					if( isnan(max) || m > max ) max = m;
				}


			} catch( const boost::bad_lexical_cast& ) {
				throw xml_parse_error("Invalid min or max value at param_desc " + name);
			}

	param_desc.put("@min",min);
	param_desc.put("@max",max);

	/**
	 * Add type which is its parent section
	 */
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

bool Param::is_summaric() const
{
	const auto unit = param_desc.get<std::string>( "@unit", "-" );
	if( unit.rfind("/h") != std::string::npos )
		return true;
	if( summaric_units.find( unit ) != summaric_units.end() )
		return true;
	if( param_desc.count("draw") )
		for( const auto& c : param_desc.get_child("draw") ) {
			const auto special = c.second.get<std::string>( "@special", "" );
			if ( special == "hoursum" )
				return true;
		}
	return false;
}

std::string Param::get_summaric_unit() const
{
	auto unit = param_desc.get<std::string>( "@unit", "-" );
	const auto pos = unit.rfind("/h");
	if( pos != std::string::npos )
		return unit.erase(pos);
	if( summaric_units.find( unit ) != summaric_units.end() )
		return unit + "h";
	return unit;
}

double Param::get_value( ProbeType pt ) const
{
	auto itr = values.find( pt );
	if( itr != values.end() )
		return itr->second;

	return std::numeric_limits<double>::quiet_NaN();
}

void Param::set_value( double v , ProbeType pt )
{
	values[ pt ] = v;
}

const std::set<std::string> Param::summaric_units {"MW", "kW"};
