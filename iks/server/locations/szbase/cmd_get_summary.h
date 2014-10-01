#ifndef __SERVER_CMD_GET_SUMMARY_H__
#define __SERVER_CMD_GET_SUMMARY_H__

#include <ctime>
#include <iterator>

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include "locations/command.h"
#include "utils/ptree.h"
#include "cmd_common.h"

class GetSummaryRcv : public Command {
public:
	GetSummaryRcv( Vars& vars , Protocol& prot )
		: vars(vars)
	{
		(void)prot;
		set_next( std::bind(&GetSummaryRcv::parse_command,this,std::placeholders::_1) );
	}

	virtual ~GetSummaryRcv()
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

		if( !param->is_summaric() ) {
			fail( ErrorCodes::not_summaric );
			return;
		}

		auto probe_type = tags[0];

		std::unique_ptr<ProbeType> pt;

		if (probe_type == "10s") {
			pt.reset( new ProbeType( probe_type ) );
		} else {
			pt.reset( new ProbeType( "10m" ) );
		}

		timestamp_t tbeg, tend;

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

		int nanCount = 0;
		double accumulator = 0;
		for( auto it = probes.begin(); it != probes.end(); ++it ) {
			if( isnan(*it) ) {
				nanCount++;
			} else {
				accumulator += *it;
			}
		}
		// if better precision is needed use e.g. Kahan algorithm or anything that draw3 uses
		const int divisor = probe_type == "10s" ? 360 : 6;
		const double sum = accumulator / divisor;
		const double notNanPercentage = 100 - ((double)nanCount / probes.size() * 100);
		const auto unit = param->get_summaric_unit();

		boost::property_tree::ptree ptree;
		ptree.add("sum", sum);
		ptree.add("nan", notNanPercentage);
		ptree.add("unit", unit);
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

#endif /* end of include guard: __SERVER_CMD_GET_SUMMARY_H__ */

