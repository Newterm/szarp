#ifndef __DATA_PARAMS_UPDATER_H__
#define __DATA_PARAMS_UPDATER_H__

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
public:
	ParamsUpdater( Params& params );
	~ParamsUpdater();

	/** TODO: change SzbaseWrapper to generic DataFeeder class */
	void set_data_feeder( SzbaseWrapper* data_feeder = NULL );

protected:
	Params& params;
	SzbaseWrapper* data_feeder;

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

