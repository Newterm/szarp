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
	GetHistoryRcv( Vars& vars )
		: vars(vars)
	{
		set_next( std::bind(&GetHistoryRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetHistoryRcv()
	{
	}

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
		auto s = probe_type[probe_type.size()-1];

		timestamp_t probe_sec;

		try {
			probe_type.erase(probe_type.size()-1);

			probe_sec = boost::lexical_cast<timestamp_t>(probe_type);
		} catch( const boost::bad_lexical_cast& ) {
			fail( ErrorCodes::invalid_timestamp );
			return;
		}

		switch( s ) {
			case 's':
				probe_sec = probe_sec;
				break;
			case 'm':
				probe_sec = probe_sec * 60;
				break;
			case 'h':
				probe_sec = probe_sec * 60 * 60;
				break;
			case 'd':
				probe_sec = probe_sec * 60 * 60 * 24;
				break;
			case 'M':
				/* FIXME: 31 days in month is temporary simplification:)
				 * (31/03/2014 12:29, jkotur) */
				probe_sec = probe_sec * 60 * 60 * 24 * 31;
				break;
			default:
				fail( ErrorCodes::ill_formed );
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
		std::string data;

		tend = gen_data( probes , tbeg , tend , probe_sec , 0 , 100 );
		
		std::copy( base64_enc((char*)probes.data()) ,
		           base64_enc((char*)(probes.data()+probes.size())) ,
				   std::back_inserter(data) );

		boost::property_tree::ptree out;
		out.add("start", tbeg);
		out.add("end", tend);
		out.add("data", data);
		apply( ptree_to_json( out , false ) );
	}

	timestamp_t gen_data(
			std::vector<f32_t>& out ,
			timestamp_t beg , timestamp_t end , timestamp_t step ,
			f32_t min , f32_t max )
	{
		timestamp_t t;

		for( t=beg ; t<=end ; t+=step )
			out.push_back( 666 );

		return t - step;
	}

protected:
	Vars& vars;
};

#endif /* end of include guard: __SERVER_CMD_GET_HISTORY_H__ */

