#include "params_updater.h"

#include <chrono>
#include <functional>

namespace p = std::placeholders;

#include "global_service.h"

ParamsUpdater::ParamsUpdater( Params& params )
	: params(params)
	, timeout(GlobalService::get_service())
{
	data_updater = std::make_shared<DataUpdater>( this );
}

ParamsUpdater::~ParamsUpdater()
{
	data_updater->parent = NULL;
}

void ParamsUpdater::set_data_feeder( SzbaseWrapper* data_feeder_ )
{
	data_feeder = data_feeder_;

	data_updater->check_szarp_values();
}

void ParamsUpdater::DataUpdater::check_szarp_values( const boost::system::error_code& e )
{
	if( e || !parent ) return;

	using std::chrono::system_clock;
	using namespace boost::posix_time;

	time_t t = system_clock::to_time_t(system_clock::now());
	t = SzbaseWrapper::round( t , PT_SEC10 );

	try {
		for( auto& pname : parent->params )
		{
			auto& name = pname;
			parent->params.param_value_changed(
					name ,
					parent->data_feeder->get_avg( name , t , PT_SEC10 ) );
		}
	} catch( szbase_error& e ) {
		/* TODO: Better error handling (22/05/2014 20:54, jkotur) */
		std::cerr << "Szbase error: " << e.what() << std::endl;
	}

	t = system_clock::to_time_t(system_clock::now());
	t = SzbaseWrapper::round( t , PT_SEC10 );
	t = SzbaseWrapper::next ( t , PT_SEC10 );

	parent->timeout.expires_at( from_time_t(t) + seconds(1));
	parent->timeout.async_wait( std::bind(&ParamsUpdater::DataUpdater::check_szarp_values,shared_from_this(),p::_1) );
}

