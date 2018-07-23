#ifndef __DATA_SZBASE_WRAPPER_H__
#define __DATA_SZBASE_WRAPPER_H__

#include "szarp_config.h"
#include "szbase/szbdate.h"

#include "utils/config.h"
#include "utils/exception.h"
#include "data/probe_type.h"

#include "iks_live_cache.h"

using SzbaseObserverToken = std::shared_ptr<Observer>;

enum class SearchDir {
	LEFT,
	RIGHT
};

enum class TimeType {
	SECOND,
	NANOSECOND
};

enum class ValueType {
	SHORT,
	INT,
	FLOAT,
	DOUBLE
};

class SzbaseWrapper {
public:
	static std::string get_dir()
	{	return szarp_dir.string(); }

	static bool init( const std::string& _szarp_dir , const CfgSections& locs, int live_cache_retention, size_t base_low_water_mark, size_t base_high_water_mark);
	static bool is_initialized() { return initialized; }

	static time_t next( time_t t , ProbeType pt , int num = 1 );
	static time_t round( time_t , ProbeType pt );

	SzbaseWrapper( const std::string& base );

	/** 
	 * Returns latest probe time in base.
	 *
	 * Throws szbase_get_value_error if param not present in base.
	 */
	time_t get_latest( const std::string& param , ProbeType type ) const;

	double get_updated_value( const std::string& param , ProbeType ptype ) const;

	/**
	 * Gets average from exact given time with specified probe
	 */
	double get_avg( const std::string& param , time_t time , ProbeType type ) const;
	double get_live_val(const std::string& param) const { return live_values_holder.get_value(param); }

	std::string search_data( const std::string& param ,
						     const std::string& from ,
							 const std::string& to ,
							 TimeType time_type ,
							 SearchDir dir ,
							 ProbeType pt
							 ) const;

	std::string get_data( const std::string& param ,
						  const std::string& from ,
						  const std::string& to ,
						  ValueType value_type ,
						  TimeType time_type ,
						  ProbeType pt
						  ) const;

	SzbaseObserverToken register_observer( const std::string& param , boost::optional<ProbeType> pt, std::function<void( void )> );

	void deregister_param(const std::string& name);

	const std::string& get_base_name() const;
private:
	std::wstring convert_string( const std::string& param ) const;

	// base -> parhub url
	static std::unordered_map<std::string, std::string> parhub_urls;
	std::unique_ptr<ParhubPoller> live_updater;
	LiveCache live_values_holder;

	static void purge_cache();

	static boost::filesystem::path szarp_dir;
	static bool initialized;
	static sz4::base* base;
	static size_t base_cache_low_water_mark;
	static size_t base_cache_high_water_mark;

	std::string base_name;

public:
	static const size_t BASE_CACHE_LOW_WATER_MARK_DEFAULT;
	static const size_t BASE_CACHE_HIGH_WATER_MARK_DEFAULT;
	static const int BASE_LIVE_CACHE_RETENTION;

};

class SzbaseObserverImpl : public sz4::param_observer, public Observer {
	std::wstring param_name;
	IPKContainer* ipk;
	sz4::base* base;

	std::function<void( void )> callback;
public:
	SzbaseObserverImpl( const std::wstring& param_name
					  , IPKContainer* ipk
					  , sz4::base* base
					  , std::function<void( void )> callback );

	void param_data_changed( TParam* );
	
	~SzbaseObserverImpl();
};	


#endif /* end of include guard: __DATA_SZBASE_WRAPPER_H__ */

