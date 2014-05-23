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

typedef float f32_t;
typedef unsigned timestamp_t;
typedef boost::archive::iterators::base64_from_binary<
	boost::archive::iterators::transform_width<const char *, 6, 8> >
		base64_enc;

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

		SzbaseWrapper::ProbeType pt;

		     if( probe_type == "10s" )
			pt = PT_SEC10;
		else if( probe_type == "10m" )
			pt = PT_MIN10;
		else if( probe_type == "1h" )
			pt = PT_HOUR;
		else if( probe_type == "8h" )
			pt = PT_HOUR8;
		else if( probe_type == "1d" )
			pt = PT_DAY;
		else if( probe_type == "1w" )
			pt = PT_WEEK;
		else if( probe_type == "1M" )
			pt = PT_MONTH;
		else if( probe_type == "1Y" )
			pt = PT_YEAR;
		else {
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
		std::string out;

		try {
			get_data( probes , tbeg , tend , pt , name );
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

		std::copy( base64_enc((char*)(probes.data()+nb)) ,
		           base64_enc((char*)(probes.data()+ne)) ,
				   std::back_inserter(out) );

		boost::property_tree::ptree ptree;
		ptree.add("start", SzbaseWrapper::next(tbeg,pt,nb));
		ptree.add("end"  , SzbaseWrapper::next(tbeg,pt,ne));
		ptree.add("data", out);
		apply( ptree_to_json( ptree , false ) );
	}

	timestamp_t get_data(
			std::vector<f32_t>& out ,
			timestamp_t beg , timestamp_t end , SzbaseWrapper::ProbeType pt ,
			const std::string& param )
	{
		timestamp_t t;

		for( t=beg ; t<end ; t=SzbaseWrapper::next(t,pt) )
			out.push_back( vars.get_szbase()->get_avg( param , t , pt ) );

		return t;
	}

	Vars& vars;
};

#endif /* end of include guard: __SERVER_CMD_GET_HISTORY_H__ */

