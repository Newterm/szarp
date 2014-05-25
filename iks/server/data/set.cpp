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
			auto pt = ic->second;

			upgrade_option( pt , "@bg_color" , "@background_color" );
			upgrade_option( pt , "@color"    , "@graph_color" );

			convert_colour( pt , "@background_color" );
			convert_colour( pt , "@graph_color" );

			pt.erase("@order");

			params.insert(
				ParamId { name , ic->second.get<double>("@order") , pt } );
		}

	set_desc.erase( "param" );

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

	/** Convert orders to doubles and find maximal order value */
	double max_order = 0;
	for( auto ic=pdesc.begin() ; ic!=pdesc.end() ; ++ic )
	{
		auto so = ic->second.get_optional<double>("@order");
		if( !so )
			continue;

		try {
			double o = boost::lexical_cast<double>(*so);
			ic->second.put("@order",o);

			max_order = std::max( max_order , o );
		} catch( boost::bad_lexical_cast& e ) {
			std::cerr << "Invalid order in param "
					  << ic->second.get<std::string>("@name") << std::endl;

			ic->second.erase("@order");
		}
	}

	/** Put params without order at the end */
	int i = 0;
	for( auto ic=pdesc.begin() ; ic!=pdesc.end() ; ++ic )
	{
		auto o = ic->second.get_optional<double>("@order");
		if( !o ) ic->second.put("@order",max_order+ ++i);
	}

	for( auto ic=pdesc.begin() ; ic!=pdesc.end() ; ++ic )
	{
		bp::ptree pt = ic->second;

		pt.erase("@order");

		params.insert(
				ParamId {
					ic->second.get<std::string>("@name") ,
					ic->second.get<double>("@order") ,
					pt } );
	}
	set_desc.erase("params");

	update_hash();
}

boost::property_tree::ptree Set::get_json_ptree() const
{
	bp::ptree desc = set_desc;
	bp::ptree params_desc;

	for( auto in=params.begin() ; in!=params.end() ; ++in )
		params_desc.push_back( std::make_pair( "" , in->desc ) );

	desc.put_child( "params" , std::move(params_desc) );

	return desc;
}

void Set::to_json( std::ostream& stream , bool pretty ) const
{
	ptree_to_json( stream , get_json_ptree() , pretty );
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
	auto& pset = pt.put_child( "set" , get_json_ptree() );

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
	std::hash<ParamsMap::value_type::first_type> hash_fn;

	hash = 0;
	/** Because xor is associative there is no need to care about order */
	for( auto in=params.begin() ; in!=params.end() ; ++in )
		hash ^= hash_fn(in->first);

	set_desc.put( "@hash" , hash );
}

bool operator==( const Set& a , const Set& b )
{
	auto ai = a.params.begin();
	auto bi = b.params.begin();
	for(  ; ai!=a.params.end() && bi!=b.params.end() ; ++ai , ++bi )
		if( ai->desc != bi->desc )
			return false;

	return a.name     == b.name
	    && a.set_desc == b.set_desc
	    && a.hash     == b.hash
	    && a.order    == b.order;
}

bool operator!=( const Set& a , const Set& b )
{	return !(a==b); }

