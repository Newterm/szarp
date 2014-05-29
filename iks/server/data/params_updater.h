#ifndef __DATA_PARAMS_UPDATER_H__
#define __DATA_PARAMS_UPDATER_H__

#include <set>

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
	typedef std::shared_ptr<Params::iterator> SharedIterator;
	typedef std::set<SharedIterator> SharedSubscription;

public:
	class Subscription;

	ParamsUpdater( Params& params );
	~ParamsUpdater();

	/** TODO: change SzbaseWrapper to generic DataFeeder class */
	void set_data_feeder( SzbaseWrapper* data_feeder = NULL );

	Subscription subscribe_param( const std::string& name , bool update = true );
	template<class Container>
	Subscription subscribe_params( const Container& names , bool update = true )
	{
		Subscription s;
		for( auto itr=names.begin() ; itr!=names.end() ; ++itr )
			s.insert( subscribe_param( *itr , false ) );

		if( update )
			data_updater->check_szarp_values();

		return s;
	}

protected:
	Params& params;
	SzbaseWrapper* data_feeder;

	SharedSubscription subscribed_params;

public:
	class Subscription {
		friend class ParamsUpdater;
	public:
		Subscription();

		bool empty() const { return subset.empty(); }
		void cancel() { subset.clear(); }

	protected:
		Subscription( const SharedIterator& itr );

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

