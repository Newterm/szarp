#include "szbase_wrapper.h"

#include <boost/lexical_cast.hpp>

#include <conversion.h>

bool SzbaseWrapper::initialized = false;

#if BOOST_FILESYSTEM_VERSION == 3
typedef boost::filesystem::path path;
#else
typedef boost::filesystem::wpath path;
#endif

path SzbaseWrapper::szarp_dir;

bool SzbaseWrapper::init( const std::string& _szarp_dir )
{
	std::wstring w_szarp_dir = SC::A2S(_szarp_dir);

	if( initialized )
		return path( w_szarp_dir ) == szarp_dir;

	szarp_dir = w_szarp_dir;

	IPKContainer::Init( w_szarp_dir, w_szarp_dir, L"pl_PL" );
	Szbase::Init( w_szarp_dir , NULL );

	initialized = true;

	return true;
}

SzbaseWrapper::SzbaseWrapper( const std::string& base )
	: base_name(base)
{
	std::wstring wbp( base_name.begin() , base_name.end() );
	IPKContainer::GetObject()->GetConfig( wbp );
}

std::wstring SzbaseWrapper::convert_string( const std::string& str ) const
{
	std::basic_string<unsigned char> ubp( str.begin() , str.end() );
	return SC::U2S( ubp );
}

void SzbaseWrapper::set_prober_address( const std::string& address , unsigned port )
{
	Szbase::GetObject()->SetProberAddress(
			convert_string(base_name) ,
			convert_string(address) ,
			boost::lexical_cast<std::wstring>(port) );
}

void SzbaseWrapper::sync() const
{
	Szbase::GetObject()->NextQuery();
}

double SzbaseWrapper::get_avg(
			const std::string& param ,
			time_t time ,
			ProbeType type ) const
	throw( szbase_init_error, szbase_get_value_error )
{
	sync();
	return get_avg_no_sync( param , time , type );
}

double SzbaseWrapper::get_avg_no_sync(
			const std::string& param ,
			time_t time ,
			ProbeType type ) const
	throw( szbase_init_error, szbase_get_value_error )
{
	if( !SzbaseWrapper::is_initialized() )
		throw szbase_init_error("Szbase not initialized");

	bool is_fixed, ok;
	std::wstring error;
	double val = Szbase::GetObject()->GetValue(
			convert_string( base_name + ":" + param ) ,
			time , type.get_szarp_pt() , type.get_len() ,
			&is_fixed , ok , error );

	if( !ok )
		throw szbase_get_value_error("Cannot get value from param " + param + ": " + SC::S2A(error) );

	return val;
}

double SzbaseWrapper:: get_avg(
			const std::string& param ,
			time_t start ,
			time_t end ) const
	throw( szbase_init_error, szbase_get_value_error )
{
	sync();
	return get_avg_no_sync( param , start , end );
}

double SzbaseWrapper:: get_avg_no_sync(
			const std::string& param ,
			time_t start ,
			time_t end ) const
	throw( szbase_init_error, szbase_get_value_error )
{
	if( !SzbaseWrapper::is_initialized() )
		throw szbase_init_error("Szbase not initialized");

	int len = end - start;

	bool is_fixed, ok;
	std::wstring error;
	double val = Szbase::GetObject()->GetValue(
			convert_string( base_name + ":" + param ) ,
			start , PT_CUSTOM , len ,
			&is_fixed , ok , error );

	if( !ok )
		throw szbase_get_value_error("Cannot get value from param " + param + ": " + SC::S2A(error) );

	return val;
}

time_t SzbaseWrapper::next( time_t t , ProbeType pt , int num )
{
	return szb_move_time( t , num , pt.get_szarp_pt() , pt.get_len() );
}

time_t SzbaseWrapper::round( time_t t , ProbeType pt )
{
	return szb_round_time( t , pt.get_szarp_pt() , pt.get_len() );
}

