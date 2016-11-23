#include "config.h"

#include <boost/format.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace bp = boost::property_tree;
using boost::format;

void Config::from_file( const std::string& path )
{
	bp::ptree cfg_doc;
	bp::read_xml(path, cfg_doc, bp::xml_parser::trim_whitespace|bp::xml_parser::no_comments );

	from_xml( cfg_doc );
}

void Config::from_xml( const bp::ptree& ptree )
{
	/* TODO: Do we need such config file? (05/05/2014 16:34, jkotur) */
	emit_changed();
}

void Config::from_pairs( const CfgPairs& pairs )
{
	std::copy(pairs.begin(), pairs.end(), std::inserter(cfg, cfg.end()));
	emit_changed();
}

