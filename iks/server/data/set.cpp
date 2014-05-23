#include "set.h"

#include <functional>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace bp = boost::property_tree;

#include "utils/ptree.h"

Set::Set()
{
}

Set::~Set()
{
}

void Set::from_xml( const bp::ptree& ptree )
{
	set_desc = ptree;

	bp::ptree params_desc;

	fold_xmlattr( set_desc );

	name = set_desc.get<std::string>("@name");

	/**
	 * Convert obsolete color format to new one
	 */
	convert_colour( set_desc , "@background_color" );
	convert_colour( set_desc , "@background_text_color" );
	convert_colour( set_desc , "@text_color" );

	convert_float( set_desc , "@spacing_vertical"   );
	convert_float( set_desc , "@spacing_horizontal" );

	params.clear();
	for( auto ic=set_desc.begin() ; ic!=set_desc.end() ; ++ic )
		if( ic->first == "param" ) {
			auto name = ic->second.get<std::string>("@name");

			upgrade_option( ic->second , "@bg_color" , "@background_color" );
			upgrade_option( ic->second , "@color"    , "@graph_color" );

			convert_colour( ic->second , "@background_color" );
			convert_colour( ic->second , "@graph_color" );

			params.insert( name );

			params_desc.push_back( std::make_pair( "" , ic->second ) );
		}

	set_desc.erase( "param" );
	set_desc.put_child( "params" , std::move(params_desc) );

	update_hash();
}

void Set::upgrade_option( bp::ptree& ptree , const std::string& prev , const std::string& curr )
{
	auto opt = ptree.get_optional<std::string>( prev );
	if( opt ) {
		ptree.put( curr ,*opt);
		ptree.erase( prev );
	}
}

void Set::convert_float( bp::ptree& ptree , const std::string& name )
{
	auto value = ptree.get_optional<std::string>(name);
	if( !value ) return;
	std::replace( value->begin() , value->end() , ',' , '.' );
	ptree.put( name , value );
}

void Set::convert_colour( bp::ptree& ptree , const std::string& name )
{
	auto value = ptree.get_optional<std::string>(name);
	if( !value ) return;
	auto out = convert_colour( *value );
	ptree.put( name , out );
}

std::string Set::convert_colour( const std::string& in )
{
	/** no need to convert */
	if( in[0] == '#' )
		return in;

	unsigned int uint;
	try {
		uint = boost::lexical_cast<unsigned int>( in );
	} catch( const boost::bad_lexical_cast& ) {
		return "#000000"; /** invalid convertion, return black */
	}

	uint &= 0xffffff; /** remove alpha byte */

	return str( boost::format("#%06x") % uint );
}

void Set::from_json( const bp::ptree& ptree )
{
	/* TODO: Verify ptree (19/03/2014 20:48, jkotur) */
	set_desc = ptree;

	params.clear();
	name = set_desc.get<std::string>("@name");
	auto& pdesc = set_desc.get_child("params");
	for( auto ic=pdesc.begin() ; ic!=pdesc.end() ; ++ic )
		params.insert( ic->second.get<std::string>("@name") );

	update_hash();
}

void Set::to_json( std::ostream& stream , bool pretty ) const
{
	ptree_to_json( stream , set_desc , pretty );
}

std::string Set::to_json( bool pretty ) const
{
	std::stringstream ss;
	to_json( ss , pretty );
	auto str = ss.str();
	str.erase(str.size()-1); // remove terminating \n sign
	return str;
}

boost::property_tree::ptree Set::get_xml_ptree() const
{
	bp::ptree pt;
	auto& pset = pt.put_child( "set" , set_desc );

	auto& pdesc = pset.get_child("params");
	for( auto ic=pdesc.begin() ; ic!=pdesc.end() ; ++ic )
		pset.add_child("param",ic->second);
	pset.erase("params");
	pset.erase("@hash");

	unfold_xmlattr( pt );

	return pt;
}

void Set::to_xml( std::ostream& stream , bool pretty ) const
{
	ptree_to_xml( stream , get_xml_ptree() , pretty );
}

std::string Set::to_xml( bool pretty ) const
{
	std::stringstream ss;
	to_xml( ss , pretty );
	return ss.str();
}

void Set::update_hash()
{
	std::hash<ParamsMap::value_type> hash_fn;

	hash = 0;
	/** Because xor is associative there is no need to care about order */
	for( auto in=params.begin() ; in!=params.end() ; ++in )
		hash ^= hash_fn(*in);

	set_desc.put( "@hash" , hash );
}

bool operator==( const Set& a , const Set& b )
{
	return a.name     == b.name
	    && a.set_desc == b.set_desc;
}

bool operator!=( const Set& a , const Set& b )
{	return !(a==b); }

