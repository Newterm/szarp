#include "set.h"

#include <functional>
#include <limits>

namespace p = std::placeholders;

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace bp = boost::property_tree;

#include <liblog.h>

#include "utils/ptree.h"
#include "utils/colors.h"

Set::Set()
	: order(std::numeric_limits<double>::quiet_NaN())
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
	for( auto& c : set_desc )
		if( c.first == "param" ) {
			auto name = c.second.get<std::string>("@name");
			auto pt = c.second;

			upgrade_option( pt , "@bg_color" , "@background_color" );
			upgrade_option( pt , "@color"    , "@graph_color" );

			convert_colour( pt , "@background_color" );
			convert_colour( pt , "@graph_color" );

			pt.erase("@order");

			params.insert(
				ParamId { name , c.second.get<double>("@order") , pt } );
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
	if( in.front() == '#' )
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
			sz_log(0, "Invalid order in param %s" ,
				ic->second.get<std::string>("@name").c_str());

			ic->second.erase("@order");
		}
	}

	convert_color_names_to_hex();

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

	generate_colors_like_draw3();

	update_hash();
}

void Set::convert_color_names_to_hex()
{
	auto& pdesc = set_desc.get_child("params");

	/** Convert color names to hex codes */
	for( auto ic=pdesc.begin() ; ic!=pdesc.end() ; ++ic )
	{
		auto oc = ic->second.get_optional<std::string>("@graph_color");
		if( !oc ) continue;

		auto itr = Colors::name_to_hex.find(*oc);

		if( itr != Colors::name_to_hex.end() )
			/** Put converted color */
			ic->second.put( "@graph_color" , itr->second );
		else
			/** Else normalize to lower hex */
			ic->second.put(
					"@graph_color" ,
					boost::algorithm::to_lower_copy(*oc) );
	}
}

void Set::assign_color( ParamId& p , const std::string& color )
{
	p.desc.put("@graph_color",color);
}

void Set::generate_colors_like_draw3()
{
	/** Generate colors if not specified */
	std::unordered_set<std::string> used_colors;
	for( auto ic=params.begin() ; ic!=params.end() ; ++ic )
	{
		auto oc = ic->desc.get_optional<std::string>("@graph_color");
		if( !oc ) continue;
		used_colors.insert( *oc );
	}

	/**
	 * This algorithm is used by draw3 to generate colors if not
	 * specified.
	 *
	 * Colors are taken from predefined table of 12 colors in order
	 * unless this color was already used in other parameter. If so this
	 * color is omitted. This works great but only for 12 colors. Above
	 * this * number we don't check if color was used and simply assign
	 * colors from predefined list.
	 */
	int def = Colors::draw3_defaults.size();
	int cur = 0;
	for( auto ic=params.begin() ; ic!=params.end() ; ++ic )
	{
		auto oc = ic->desc.get_optional<std::string>("@graph_color");
		if( oc ) continue;
		std::string color;
		do {
			color = Colors::draw3_defaults[cur++];
			cur %= Colors::draw3_defaults.size();
		} while( def > 0 && used_colors.count(color) );
		params.modify( ic , std::bind(&Set::assign_color,this,p::_1,std::ref(color)) );
	}
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
	str.pop_back(); // remove terminating \n sign
	return str;
}

boost::property_tree::ptree Set::get_xml_ptree() const
{
	bp::ptree pt;
	auto& pset = pt.put_child( getElementName() , get_json_ptree() );

	for( auto& c : pset.get_child("params") )
		pset.add_child("param",c.second);
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

