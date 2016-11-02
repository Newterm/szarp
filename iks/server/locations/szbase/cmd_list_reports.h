#ifndef __SZBASE_CMD_LIST_REPORTS_H__
#define __SZBASE_CMD_LIST_REPORTS_H__

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "data/vars.h"

#include "utils/ptree.h"

class ListReportsRcv : public Command {
public:
	ListReportsRcv( Vars& vars , Protocol& prot )
		: vars(vars)
	{
		(void)prot;
		set_next( std::bind(&ListReportsRcv::parse_command,this,std::placeholders::_1) );
	}

	void parse_command( const std::string& tag )
	{
		namespace bp = boost::property_tree;

		bp::ptree out;
		bp::ptree params;

		auto reports = vars.get_sets().list_reports();
		auto cend = key_iterator<Sets::SetsMap>(reports.cend());
		for (auto i = key_iterator<Sets::SetsMap>(reports.cbegin()); i != cend; ++i) {
			params.push_back( std::make_pair( "" , bp::ptree(*i) ) );
		}

		out.add_child( "reports" , params );

		apply( ptree_to_json( out , false ) );
	}

protected:
	Vars& vars;
};

#endif /* end of include guard: __SZBASE_CMD_LIST_SETS_H__ */


