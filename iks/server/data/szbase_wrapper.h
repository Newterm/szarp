#ifndef __DATA_SZBASE_WRAPPER_H__
#define __DATA_SZBASE_WRAPPER_H__

#include <conversion.h>

#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "szbase/szbdate.h"

#include "utils/exception.h"

class SzbaseWrapper {
public:
	typedef SZARP_PROBE_TYPE ProbeType;

	static std::string get_dir()
#if BOOST_FILESYSTEM_VERSION == 3
	{	return szarp_dir.string(); }
#else
	{	return SC::S2A( szarp_dir.string() ); }
#endif

	static bool init( const std::string& szarp_dir );
	static bool is_initialized() { return initialized; }

	static time_t next( time_t t , ProbeType pt , int num = 1 );
	static time_t round( time_t , ProbeType pt );

	SzbaseWrapper( const std::string& base );

	void set_prober_address( const std::string& address , unsigned port );

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
	std::wstring convert_string( const std::string& param ) const;

#if BOOST_FILESYSTEM_VERSION == 3
	static boost::filesystem::path szarp_dir;
#else
	static boost::filesystem::wpath szarp_dir;
#endif

	static bool initialized;

	std::string base_name;

};

#endif /* end of include guard: __DATA_SZBASE_WRAPPER_H__ */

