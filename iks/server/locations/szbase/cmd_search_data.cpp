#include <boost/algorithm/string.hpp>

#include "data/szbase_wrapper.h"
#include "utils/tokens.h"
#include "szbase.h"

#include "cmd_search_data.h"

SearchData::SearchData( Vars& vars , Protocol& prot )
	: vars(vars)
{
	(void)prot;
	set_next( std::bind(&SearchData::parse_command,this,std::placeholders::_1) );
}

void SearchData::parse_command( const std::string& data )
{
	namespace balgo = boost::algorithm;

	auto r = find_quotation( data );
	std::string name(r.begin(),r.end());

	auto l = find_after_quotation( data ) ;
	std::vector<std::string> tags;
	balgo::split(
			tags ,
			l ,
			balgo::is_any_of(" "), balgo::token_compress_on );

	if( tags.size() != 4 ) {
		fail( ErrorCodes::ill_formed );
		return;
	}

	ProbeType pt(tags[0]);

	SearchDir dir;
	if( tags[2] == "left" ) {
		dir = SearchDir::LEFT;
	} else if( tags[2] == "right" ) {
		dir = SearchDir::RIGHT;
	} else {
		fail( ErrorCodes::ill_formed );
		return;
	}

	TimeType ttype;
	if( tags[3] == "second_t" ) {
		ttype = TimeType::SECOND;
	} else if( tags[3] == "nanosecond_t" ) {
		ttype = TimeType::NANOSECOND;
	} else {
		fail( ErrorCodes::ill_formed );
		return;
	}
		
	apply( vars.get_szbase()->search_data ( name, tags[4] , tags[5], ttype , dir , pt ) );

}

