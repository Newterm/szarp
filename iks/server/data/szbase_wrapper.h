#ifndef __DATA_SZBASE_WRAPPER_H__
#define __DATA_SZBASE_WRAPPER_H__

#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "szbase/szbdate.h"

#include "utils/exception.h"

class SzbaseWrapper {
public:
	typedef SZARP_PROBE_TYPE ProbeType;

	static bool init( const std::string& szarp_dir );
	static bool is_initialized() { return initialized; }

	static time_t next( time_t t , ProbeType pt , int num = 1 );

	SzbaseWrapper( const std::string& base );

	/**
	 * Gets average from exact given time with specified probe
	 */
	double get_avg( const std::string& param , time_t time , ProbeType type ) const
		throw( szbase_init_error, szbase_get_value_error );

	/**
	 * Gets average of probes for exact given time gap
	 */
	double get_avg( const std::string& param , time_t start , time_t end ) const
		throw( szbase_init_error, szbase_get_value_error );

private:
	static boost::filesystem::path szarp_dir;
	static bool initialized;

	std::string base_name;

};

#endif /* end of include guard: __DATA_SZBASE_WRAPPER_H__ */

