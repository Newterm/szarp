#include "params_updater.h"

#include <chrono>
#include <memory>
#include <functional>

namespace p = std::placeholders;

#include <liblog.h>

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
}

ParamsUpdater::Subscription ParamsUpdater::subscribe_param(
		const std::string& name ,
		ProbeType pt ,
		bool update )
{
	auto itr = params.find( name );

	if( itr == params.end() )
		return Subscription();

	Subscription s(
		subscribed_params.insert(
			std::make_pair( 
				std::make_shared<SubKey>( itr , pt ) ,
				0
				) ).first->first );

	if( update )
		data_updater->check_szarp_values();

	return s;
}

ParamsUpdater::Subscription::Subscription()
{
}

ParamsUpdater::Subscription::Subscription( const SharedKey& key )
{
	subset.insert( key );
}

void ParamsUpdater::Subscription::insert( const ParamsUpdater::Subscription& sub )
{
	subset.insert( sub.subset.begin() , sub.subset.end() );
}

void ParamsUpdater::DataUpdater::check_szarp_values(
		const boost::system::error_code& e )
{
	if( e || !parent ) return;

	using std::chrono::system_clock;
	using namespace boost::posix_time;

	time_t t = system_clock::to_time_t(system_clock::now());

	/**
	 * Find minimum probe type to reschedule probes update and remove
	 * unused subscriptions
	 */
	ProbeType min_pt( ProbeType::Type::MAX );
	for( auto itr=parent->subscribed_params.begin() ;
		 itr != parent->subscribed_params.end() ; )
		if( itr->first.use_count() <= 1 ) {
			/** No Subscription object left */
			parent->subscribed_params.erase( itr++ );
		} else {
			min_pt = std::min( (*itr->first).second , min_pt );
			++itr;
		}

	try {
		for( auto itr=parent->subscribed_params.begin() ;
			 itr != parent->subscribed_params.end() ;
			 ++itr )
		{
			auto& name = *(*itr->first).first;
			auto& pt   =  (*itr->first).second;
			auto& last_update = itr->second;

			time_t ptime = SzbaseWrapper::round( t , pt );

			if( last_update == ptime )
				continue;

			itr->second = ptime;

			parent->params.param_value_changed(
					name ,
					parent->data_feeder->get_avg( name , ptime , pt ) ,
					pt );
		}
	} catch( szbase_error& e ) {
		sz_log(0, "Szbase error while updating data: %s", e.what());
	}

	if( parent->subscribed_params.empty() )
		/** Nothing to do -- no need to check for update */
		return;

	t = system_clock::to_time_t(system_clock::now());
	t = SzbaseWrapper::round( t , min_pt );
	t = SzbaseWrapper::next ( t , min_pt );

	parent->timeout.expires_at( from_time_t(t) + seconds(1));
	parent->timeout.async_wait( std::bind(&ParamsUpdater::DataUpdater::check_szarp_values,shared_from_this(),p::_1) );
}

