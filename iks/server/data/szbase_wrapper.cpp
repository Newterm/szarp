#include "szbase_wrapper.h"
#include "sz4/exceptions.h"

#include <boost/lexical_cast.hpp>

#include <conversion.h>

SzbaseObserverImpl::SzbaseObserverImpl( TParam* param , sz4::base* base , std::function<void( void )> callback )
									  : param( param ) , base( base ) , callback( callback )
{
	base->register_observer( this , std::vector<TParam*>{ param } );
}

void SzbaseObserverImpl::param_data_changed( TParam* )
{
	callback();
}

SzbaseObserverImpl::~SzbaseObserverImpl()
{
	base->deregister_observer( this , std::vector<TParam*>{ param } );
}


bool SzbaseWrapper::initialized = false;
boost::filesystem::path SzbaseWrapper::szarp_dir;
sz4::base* SzbaseWrapper::base;

bool SzbaseWrapper::init( const std::string& _szarp_dir )
{
	if( initialized )
		return boost::filesystem::path(_szarp_dir) == szarp_dir;

	szarp_dir = _szarp_dir;

	IPKContainer::Init( szarp_dir.wstring(), szarp_dir.wstring(), L"pl_PL" );

	base = new sz4::base( szarp_dir.wstring() , IPKContainer::GetObject() );

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

time_t SzbaseWrapper::get_latest(
			const std::string& param ,
			ProbeType type ) const
	throw( szbase_init_error, szbase_get_value_error )
{
	if( !SzbaseWrapper::is_initialized() )
		throw szbase_init_error("Szbase not initialized");

	TParam* tparam = IPKContainer::GetObject()->GetParam( convert_string( base_name + ":" + param ) );
	if( !tparam )
		throw szbase_get_value_error( "Cannot get latest time of param " + param + ", param not found" );

	unsigned t;
	try {
		base->get_last_time( tparam, t );
	} catch( sz4::exception& e) {
		throw szbase_get_value_error( "Cannot get latest time of param " + param + ": " + e.what() );
	}
	
	/**
	 * Round by hand because search returns probes rounded to 
	 * either 10min or 10sec, not to exact pt
	 */
	return t == unsigned( -1 ) ? -1 : round( t , type );
}

double SzbaseWrapper::get_avg(
			const std::string& param ,
			time_t time ,
			ProbeType type ) const
	throw( szbase_init_error, szbase_get_value_error )
{
	if( !SzbaseWrapper::is_initialized() )
		throw szbase_init_error("Szbase not initialized");

	TParam* tparam = IPKContainer::GetObject()->GetParam( convert_string( base_name + ":" + param ) );
	if( !tparam )
		throw szbase_get_value_error( "Cannot get value from param " + param + ", param not found" );

	sz4::weighted_sum<double, unsigned> sum;
	try {
		base->get_weighted_sum( tparam ,
                                unsigned( time ) ,
                                unsigned( next( time , type, 1 ) ) ,
                                type.get_szarp_pt() , 
                                sum );
	} catch( sz4::exception& e ) {
		throw szbase_get_value_error( "Cannot get value from param " + param + ": " + e.what() );
	}

	return sum.avg();
}

SzbaseObserverToken SzbaseWrapper::register_observer( const std::string& param , std::function<void( void )> callback )
	throw( szbase_init_error, szbase_error )
{
	TParam* tparam = IPKContainer::GetObject()->GetParam( convert_string( base_name + ":" + param ) );
	if( !tparam )
		throw szbase_error( "Cannot get value from param " + param + ", param not found" );

	return std::make_shared<SzbaseObserverImpl>( tparam , base , callback );
}

time_t SzbaseWrapper::next( time_t t , ProbeType pt , int num )
{
	return szb_move_time( t , num , pt.get_szarp_pt() , pt.get_len() );
}

time_t SzbaseWrapper::round( time_t t , ProbeType pt )
{
	return szb_round_time( t , pt.get_szarp_pt() , pt.get_len() );
}

