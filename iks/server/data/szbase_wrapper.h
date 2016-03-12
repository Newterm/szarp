#ifndef __DATA_SZBASE_WRAPPER_H__
#define __DATA_SZBASE_WRAPPER_H__

#include "szarp_config.h"
#include "szbase/szbdate.h"
#include "sz4/base.h"

#include "utils/exception.h"
#include "data/probe_type.h"

class SzbaseObserverImpl;
typedef std::shared_ptr<SzbaseObserverImpl> SzbaseObserverToken;

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

	static bool init( const std::string& _szarp_dir , size_t base_low_water_mark , size_t base_high_water_mark);
	static bool is_initialized() { return initialized; }

	static time_t next( time_t t , ProbeType pt , int num = 1 );
	static time_t round( time_t , ProbeType pt );

	SzbaseWrapper( const std::string& base );

	/** 
	 * Returns latest probe time in base.
	 *
	 * Throws szbase_get_value_error if param not present in base.
	 */
	time_t get_latest( const std::string& param , ProbeType type ) const
		throw( szbase_init_error, szbase_get_value_error );

	/**
	 * Gets average from exact given time with specified probe
	 */
	double get_avg( const std::string& param , time_t time , ProbeType type ) const
		throw( szbase_init_error, szbase_get_value_error );

	std::string search_data( const std::string& param ,
						     const std::string& from ,
							 const std::string& to ,
							 TimeType time_type ,
							 SearchDir dir ,
							 ProbeType pt
							 ) const
		throw( szbase_init_error, szbase_param_not_found_error, szbase_error );

	std::string get_data( const std::string& param ,
						  const std::string& from ,
						  const std::string& to ,
						  ValueType value_type ,
						  TimeType time_type ,
						  ProbeType pt
						  ) const
		throw( szbase_init_error, szbase_param_not_found_error, szbase_error );

	SzbaseObserverToken register_observer( const std::string& param , std::function<void( void )> )
		throw( szbase_init_error, szbase_param_not_found_error, szbase_error );

	std::string add_param( const std::string& param
						 , const std::string& base
						 , const std::string& formula
						 , const std::string& token
						 , const std::string& type
						 , int prec
						 , unsigned start_time)
		throw( szbase_invalid_name , szbase_formula_invalid_syntax, szbase_init_error );

	void remove_param(const std::string& base , const std::string& param)
		throw( szbase_param_not_found_error, szbase_init_error );

	const std::string& get_base_name() const;
private:
	std::wstring convert_string( const std::string& param ) const;

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

};

class SzbaseObserverImpl : public sz4::param_observer {
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

