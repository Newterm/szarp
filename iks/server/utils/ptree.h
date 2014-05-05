#ifndef __UTILS_PTREE_H__
#define __UTILS_PTREE_H__

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#if BOOST_VERSION > 104400
/**
 * This is fix for boost ptree in current version for UTF-8 characters.
 * Comes from:
 *     http://stackoverflow.com/questions/10260688/boostproperty-treejson-parser-and-two-byte-wide-characters
 */
namespace boost { namespace property_tree { namespace json_parser
{
    // Create necessary escape sequences from illegal characters
    template<>
    std::basic_string<char> create_escapes(const std::basic_string<char> &s);
} } }
#endif

void fold_xmlattr( boost::property_tree::ptree& ptree );
void unfold_xmlattr( boost::property_tree::ptree& ptree );

std::string ptree_to_json( const boost::property_tree::ptree& ptree ,
                          bool pretty = false );
void ptree_to_json( std::ostream& stream ,
                    const boost::property_tree::ptree& ptree ,
                    bool pretty = false );

std::string ptree_to_xml( boost::property_tree::ptree& ptree ,
                          bool pretty = false );
void ptree_to_xml( std::ostream& stream ,
                   const boost::property_tree::ptree& ptree ,
                   bool pretty = false );

void ptree_foreach(
	boost::property_tree::ptree& ptree ,
	std::function<void (boost::property_tree::ptree::value_type&)> func );

void ptree_foreach_parent(
	boost::property_tree::ptree& ptree ,
	std::function<void (boost::property_tree::ptree::value_type& ,
	                    boost::property_tree::ptree& )> func );

void ptree_foreach_if(
	boost::property_tree::ptree& ptree ,
	std::function<bool (boost::property_tree::ptree::value_type&)> func );

void ptree_foreach_if_parent(
	boost::property_tree::ptree& ptree ,
	std::function<bool (boost::property_tree::ptree::value_type& ,
	                    boost::property_tree::ptree& )> func );


#endif /* __UTILS_PTREE_H__ */

