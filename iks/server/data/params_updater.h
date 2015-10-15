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
	class SubPar;

	typedef std::shared_ptr< SubPar > SubParPtr;
	typedef std::weak_ptr  < SubPar > SubParWeakPtr;

	typedef std::pair< std::string, ProbeType > SubKey;
	typedef std::map< SubKey , SubParWeakPtr > SubParsMap;
	
	friend class SubPar;
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
			s.insert( subscribe_param( *itr , pt , update ) );

		return s;
	}

protected:
	Params& params;
	SzbaseWrapper* data_feeder;

	std::map< SubKey , SubParWeakPtr > subscribed_params;
public:
	class Subscription {
		friend class ParamsUpdater;
	public:
		Subscription();

		bool empty() const { return subset.empty(); }
		void cancel() { subset.clear(); }

	protected:
		std::set<SubParPtr> subset;

		void insert( const Subscription& sub );
		void insert( const SubParPtr& sub );
	};

private:

	class SubPar : public std::enable_shared_from_this< SubPar > {
		std::string pname;
		ProbeType pt;

		ParamsUpdater* parent;

		SzbaseObserverToken token;
	public:
		SubPar( const std::string& pname , ProbeType pt , ParamsUpdater* parent);
		~SubPar();

		bool start_sub();
	
		void update_param();

		static void callback( SubParWeakPtr ptr );
	};

};


#endif /* end of include guard: __DATA_PARAMS_UPDATER_H__ */

