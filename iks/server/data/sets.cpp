#include "sets.h"

#include <locale>

#include <boost/format.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace bp = boost::property_tree;
using boost::format;

#include "utils/ptree.h"

std::shared_ptr<const Set> Sets::get_set( const std::string& name ) const
{
	auto itr = sets.find(name);
	return itr == sets.end() ? std::shared_ptr<Set>() : itr->second;
}

void Sets::update_set( const Set& s , const std::string& old_name )
{
	std::string on = old_name.empty() ? s.get_name() : old_name;

	if( !has_set(on) && !s.empty() ) {
		auto ps = std::make_shared<Set>( s );
		sets[ s.get_name() ] = ps;
		emit_set_updated( on , ps ); /** New set */
		return;
	}

	auto itr = sets.find(on);

	if( s.empty() ) {
		/** Set removed */
		sets.erase( itr );
		emit_set_updated( on , std::shared_ptr<Set>() );
		return;
	}

	if( *itr->second != s ) {
		if( s.get_name() != on )
			sets.erase( itr );

		auto ps = std::make_shared<Set>( s );
		sets[ s.get_name() ] = ps;

		/** Set updated */
		emit_set_updated( on , ps );
	}
}

void Sets::from_params_file( const std::string& path ) throw(xml_parse_error)
{
	bp::ptree sets_doc;
	bp::read_xml(path, sets_doc, bp::xml_parser::trim_whitespace|bp::xml_parser::no_comments );

	from_params_xml( sets_doc );
}

void Sets::from_params_xml( boost::property_tree::ptree& ptree  ) throw(xml_parse_error)
{
	std::unordered_map<std::string,bp::ptree> sets_map;

	fold_xmlattr( ptree );

	ptree_foreach_parent( ptree , [&] ( bp::ptree::value_type& d , bp::ptree& parent ) {
		if( d.first == "draw" ) {
			auto name = d.second.get<std::string>("@title");
			bp::ptree child;
			child.put("@name",parent.get<std::string>("@name"));
			child.put("@min",d.second.get<std::string>("@min"));
			child.put("@max",d.second.get<std::string>("@max"));
			auto color = d.second.get_optional<std::string>("@color");
			if( color ) child.put("@graph_color",*color);
			sets_map[name].push_back( std::make_pair( "" , child ) );
		}
	} );

	for( auto& c : sets_map )
	{
		bp::ptree ptree;
		ptree.put("@name",c.first);
		ptree.put_child("params",c.second);

		Set s;
		s.from_json( ptree );
		update_set( s );
	}
}

void Sets::to_file( const std::string& path ) const
{
	auto settings = bp::xml_writer_make_settings(' ', 2);
	bp::xml_parser::write_xml( path , get_xml_ptree() , std::locale() , settings );
}

bp::ptree Sets::get_xml_ptree() const
{
	bp::ptree pt;

	/* TODO: Save only choosen sets (e.g. dont save params.xml sets) (05/05/2014 12:23, jkotur) */
	for( auto& s : sets )
		pt.add_child("sets.set",s.second->get_xml_ptree().get_child("set"));

	return pt;
}

