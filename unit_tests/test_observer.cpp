#include "test_observer.h"

TestObserver::TestObserver() {
	m_counter = 0;
}

void TestObserver::param_data_changed(TParam* param, const std::string& path) {
	boost::mutex::scoped_lock lock(m_mutex);
	std::map<TParam*, int>::iterator i = map.find(param);
	if (i == map.end())
		map[param] = 1;
	else
		i->second++;
	m_counter += 1;
	m_cond.notify_one();
}

bool TestObserver::wait_for(size_t events, int seconds) {
	boost::mutex::scoped_lock lock(m_mutex);
	while (events != m_counter)
		if (!m_cond.timed_wait(lock, boost::posix_time::seconds(seconds)))
			return false;
	return true;
}

