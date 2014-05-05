#include "ptree.h"

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace ba = boost::algorithm;
namespace bp = boost::property_tree;

namespace boost { namespace property_tree { namespace json_parser {
template<>
std::basic_string<char> create_escapes(const std::basic_string<char> &s)
{
	std::basic_string<char> result;
	std::basic_string<char>::const_iterator b = s.begin();
	std::basic_string<char>::const_iterator e = s.end();
	while (b != e)
	{
		// This assumes an ASCII superset. But so does everything in PTree.
		// We escape everything outside ASCII, because this code can't
		// handle high unicode characters.
		if (*b == 0x20 || *b == 0x21 || (*b >= 0x23 && *b <= 0x2E) ||
			(*b >= 0x30 && *b <= 0x5B) || (*b >= 0x5D && *b <= 0xFF)  //it fails here because char are signed
			|| (*b >= -0x80 && *b < 0 ) ) // this will pass UTF-8 signed chars
			result += *b;
		else if (*b == char('\b')) result += char('\\'), result += char('b');
		else if (*b == char('\f')) result += char('\\'), result += char('f');
		else if (*b == char('\n')) result += char('\\'), result += char('n');
		else if (*b == char('\r')) result += char('\\'), result += char('r');
		else if (*b == char('/')) result += char('\\'), result += char('/');
		else if (*b == char('"'))  result += char('\\'), result += char('"');
		else if (*b == char('\\')) result += char('\\'), result += char('\\');
		else
		{
			const char *hexdigits = "0123456789ABCDEF";
			typedef make_unsigned<char>::type UCh;
			unsigned long u = (std::min)(static_cast<unsigned long>(
											 static_cast<UCh>(*b)),
										 0xFFFFul);
			int d1 = u / 4096; u -= d1 * 4096;
			int d2 = u / 256; u -= d2 * 256;
			int d3 = u / 16; u -= d3 * 16;
			int d4 = u;
			result += char('\\'); result += char('u');
			result += char(hexdigits[d1]); result += char(hexdigits[d2]);
			result += char(hexdigits[d3]); result += char(hexdigits[d4]);
		}
		++b;
	}
	return result;
}
} } }

void fold_xmlattr( bp::ptree& ptree )
{
	auto attr = ptree.find( "<xmlattr>" );

	if( attr != ptree.not_found() ) {
		for( auto itr = attr->second.begin() ; itr != attr->second.end() ; ++itr )
			ptree.put_child( "@" + itr->first , itr->second );
		ptree.erase( attr->first );
	}

	for( auto itr = ptree.begin() ; itr != ptree.end() ; ++itr )
		if( itr->first != "<xmlattr>" && itr->first.front() != '@' )
			fold_xmlattr( itr->second );
}

void unfold_xmlattr( bp::ptree& ptree )
{
	for( auto itr = ptree.begin() ; itr != ptree.end() ; ++itr )
		if( ba::starts_with( itr->first , "@" ) )
			ptree.put_child( "<xmlattr>." + itr->first.substr(1) , itr->second );

	for( auto itr = ptree.begin() ; itr != ptree.end() ; ++itr )
		if( ba::starts_with( itr->first , "@" ) )
			ptree.erase( itr->first );

	for( auto itr = ptree.begin() ; itr != ptree.end() ; ++itr )
		if( itr->first != "<xmlattr>" )
			unfold_xmlattr( itr->second );
}

std::string ptree_to_json( bp::ptree& ptree , bool pretty )
{
	std::stringstream ss;

	ptree_to_json( ss , ptree , pretty );

	auto s = ss.str();

	if( !pretty ) /** terminating ending \n sign */
		s.pop_back();
	
	return s;
}

void ptree_to_json( std::ostream& stream ,
                    bp::ptree& ptree ,
                   	bool pretty )
{
	bp::json_parser::write_json( stream , ptree , pretty );
}

std::string ptree_to_xml( bp::ptree& ptree , bool pretty )
{
	std::stringstream ss;

	ptree_to_xml( ptree , pretty );

	auto s = ss.str();

	if( !pretty ) /** terminating ending \n sign */
		s.pop_back();
	
	return s;
}

void ptree_to_xml( std::ostream& stream ,
                   bp::ptree& ptree ,
                   bool pretty )
{
	auto settings = pretty ?
		bp::xml_writer_make_settings(' ') :
		bp::xml_writer_make_settings(' ', 4);

	bp::xml_parser::write_xml( stream , ptree , settings );
}

void ptree_foreach(
	boost::property_tree::ptree& ptree ,
	std::function<void (bp::ptree::value_type&)> func )
{
	ptree_foreach_if( ptree , [func] ( bp::ptree::value_type& arg ) -> bool {
		func( arg );
		return true;
	} );
}

void ptree_foreach_parent(
	boost::property_tree::ptree& ptree ,
	std::function<void (bp::ptree::value_type& ,
	                    bp::ptree& )> func )
{
	ptree_foreach_if_parent( ptree , [func] ( bp::ptree::value_type& arg , bp::ptree& parent ) -> bool {
		func( arg , parent );
		return true;
	} );
}

void ptree_foreach_if(
	boost::property_tree::ptree& ptree ,
	std::function<bool (boost::property_tree::ptree::value_type&)> func )
{
	for( auto& c : ptree )
		if( func( c ) )
			ptree_foreach_if( c.second , func );

}

void ptree_foreach_if_parent(
	boost::property_tree::ptree& ptree ,
	std::function<bool (boost::property_tree::ptree::value_type& ,
	                    boost::property_tree::ptree& )> func )
{
	for( auto& c : ptree )
		if( func( c , ptree ) )
			ptree_foreach_if_parent( c.second , func );
}

