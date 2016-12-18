#ifndef __DATA_SZBASE_WRAPPER_H__
#define __DATA_SZBASE_WRAPPER_H__

#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "szbase/szbdate.h"

#include "utils/exception.h"

#include "probe_type.h"

class SzbaseWrapper {
public:
	static std::string get_dir()
	{	return szarp_dir.string(); }

	static bool init( const std::string& szarp_dir );
	static bool is_initialized() { return initialized; }

	static time_t next( time_t t , ProbeType pt , int num = 1 );
	static time_t round( time_t , ProbeType pt );

	SzbaseWrapper( const std::string& base );

	void set_prober_address( const std::string& address , unsigned port );

	/** 
	 * Returns latest probe time in base.
	 *
	 * Throws szbase_get_value_error if param not present in base.
	 */
	time_t get_latest( const std::string& param , ProbeType type ) const;

	/**
	 * Synchronize with data base 
	 */
	void sync() const;

	/**
	 * Gets average from exact given time with specified probe
	 *
	 * Before calling no_sync version you have to call sync by hand.
	 */
	double get_avg( const std::string& param , time_t time , ProbeType type ) const;
	double get_avg_no_sync( const std::string& param , time_t time , ProbeType type ) const;


	/**
	 * Gets average of probes for exact given time gap
	 *
	 * Before calling no_sync version you have to call sync by hand.
	 */
	double get_avg( const std::string& param , time_t start , time_t end ) const;
	double get_avg_no_sync( const std::string& param , time_t start , time_t end ) const;

private:
	std::wstring convert_string( const std::string& param ) const;

	static boost::filesystem::path szarp_dir;
	static bool initialized;

	std::string base_name;

};

#endif /* end of include guard: __DATA_SZBASE_WRAPPER_H__ */

