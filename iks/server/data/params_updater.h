#ifndef __DATA_PARAMS_UPDATER_H__
#define __DATA_PARAMS_UPDATER_H__

#include <set>
#include <unordered_map>

#include "params.h"
#include "szbase_wrapper.h"

/**
 * Class that keeps needed params value up-to-date.
 *
 * Because of performance reason one can subscribe to chosen params and
 * those params values would be updated. Other params may have invalid
 * values.
 */
class ParamsUpdater {
	typedef std::pair<Params::iterator,ProbeType> SubKey;
	typedef std::shared_ptr<SubKey> SharedKey;
	typedef std::set<SharedKey> SharedSubscription;
	typedef std::unordered_map<SharedKey,time_t> SubscribedParams;

public:
	class Subscription;

	ParamsUpdater( Params& params );
	~ParamsUpdater();

	/** TODO: change SzbaseWrapper to generic DataFeeder class */
	void set_data_feeder( SzbaseWrapper* data_feeder = NULL );

	Subscription subscribe_param( const std::string& name , ProbeType pt , bool update = true );
	template<class Container>
	Subscription subscribe_params( const Container& names , ProbeType pt , bool update = true )
	{
		Subscription s;
		for( auto itr=names.begin() ; itr!=names.end() ; ++itr )
			s.insert( subscribe_param( *itr , pt , false ) );

		if( update )
			data_updater->check_szarp_values();

		return s;
	}

protected:
	Params& params;
	SzbaseWrapper* data_feeder;

	SubscribedParams subscribed_params;

public:
	class Subscription {
		friend class ParamsUpdater;
	public:
		Subscription();

		bool empty() const { return subset.empty(); }
		void cancel() { subset.clear(); }

	protected:
		Subscription( const SharedKey& itr );

		void insert( const Subscription& sub );

	private:
		SharedSubscription subset;
	};

private:
	class DataUpdater : public std::enable_shared_from_this<DataUpdater> {
	public: 
		DataUpdater( ParamsUpdater* parent ) : parent(parent) {}
		~DataUpdater() {}

		void check_szarp_values( const boost::system::error_code& e = boost::system::error_code() );

		ParamsUpdater* parent;
	};
	std::shared_ptr<DataUpdater> data_updater;

	boost::asio::deadline_timer timeout;

};


#endif /* end of include guard: __DATA_PARAMS_UPDATER_H__ */

