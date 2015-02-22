#ifndef __SERVER_CMD_GET_HISTORY_H__
#define __SERVER_CMD_GET_HISTORY_H__

#include <ctime>
#include <iterator>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include "locations/command.h"
#include "utils/ptree.h"
#include "cmd_common.h"

class GetHistoryRcv : public Command {
public:
	GetHistoryRcv( Vars& vars , Protocol& prot )
		: vars(vars)
	{
		(void)prot;
		set_next( std::bind(&GetHistoryRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetHistoryRcv()
	{
	}

protected:
	void parse_command( const std::string& data )
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

		if( tags.size() != 3 ) {
			fail( ErrorCodes::ill_formed );
			return;
		}

		auto param = vars.get_params().get_param( name );

		if( !param ) {
			fail( ErrorCodes::unknown_param );
			return;
		}

		auto probe_type = tags[0];

		std::unique_ptr<ProbeType> pt;

		try {
			pt.reset( new ProbeType( probe_type ) );
		} catch( parse_error& e ) {
			fail( ErrorCodes::wrong_probe_type );
			return;
		};

		timestamp_t tbeg , tend;

		try {
			tbeg = boost::lexical_cast<timestamp_t>(tags[1]);
			tend = boost::lexical_cast<timestamp_t>(tags[2]);
		} catch( const boost::bad_lexical_cast& ) {
			fail( ErrorCodes::invalid_timestamp );
			return;
		}

		std::vector<f32_t> probes;

		try {
			probes = get_data( tbeg , tend , *pt , name );
		} catch ( const szbase_error& e ) {
			fail( ErrorCodes::szbase_error , e.what() );
			return;
		}

		using std::isnan;
		
		auto beg = probes.begin();
		auto end = probes.end();

		while( beg!=end && isnan(*beg) ) ++beg;
		while( beg!=end && isnan(*std::prev(end)) ) --end;

		auto nb = std::distance(probes.begin(),beg);
		auto ne = std::distance(probes.begin(),end);

		std::string out;
		std::copy( base64_enc((char*)(probes.data()+nb)) ,
		           base64_enc((char*)(probes.data()+ne)) ,
				   std::back_inserter(out) );

		boost::property_tree::ptree ptree;
		ptree.add("start", SzbaseWrapper::next(tbeg,*pt,nb));
		ptree.add("end"  , SzbaseWrapper::next(tbeg,*pt,ne));
		ptree.add("data", out);
		apply( ptree_to_json( ptree , false ) );
	}

	std::vector<f32_t> get_data(
			timestamp_t beg , timestamp_t end , ProbeType pt ,
			const std::string& param )
	{
		return get_probes(vars, beg, end, pt, param);
	}

	Vars& vars;
};

#endif /* end of include guard: __SERVER_CMD_GET_HISTORY_H__ */

