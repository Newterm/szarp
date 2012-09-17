#include <map>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#include "szbase/szbparamobserver.h"

class TParam;
class TestObserver : public SzbParamObserver { 
	boost::mutex m_mutex;
	boost::condition m_cond;
	size_t m_counter;
public:
	TestObserver();
	void reset_counter();
	std::map<TParam*, int> map;
	void param_data_changed(TParam* param, const std::string& path);
	bool wait_for(size_t events, int seconds = 5);
};

