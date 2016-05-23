/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#ifndef __LIVE_CACHE_H__
#define __LIVE_CACHE_H__

#include <atomic>
#include <thread>
#include <mutex>
#include <deque>

#include "defs.h"

namespace zmq {
class socket_t;
class context_t;
}

namespace szarp {
class ParamValue;
class ParamsValues;
}

class TSzarpConfig;
class TParam;

namespace sz4
{

class live_values_observer;

class generic_live_block {
public:
	virtual void process_live_value(szarp::ParamValue* value) = 0;
	virtual void set_observer(live_values_observer* observer) = 0;
};

struct live_cache_config {
	std::vector<std::pair<std::string, TSzarpConfig*>> urls;
	typename time_difference<second_time_t>::type retention;
};

template<class value_type, class time_type> class live_block : public generic_live_block {
	std::mutex m_lock;
	std::deque<value_time_pair<value_type,time_type>> m_block;
	std::atomic<live_values_observer*> m_observer;
		
	typename time_difference<time_type>::type m_retention;
	
	time_type m_start_time;
public:
	live_block(time_difference<second_time_t>::type retention);

	std::pair<bool, time_type>
	search_data_left(const time_type& start, const time_type& end, const search_condition& condition);

	time_type search_data_right(const time_type& start, const time_type& end, const search_condition& condition);

	void get_first_time(time_type &t);
	void get_last_time(time_type &t);

	void process_live_value(szarp::ParamValue* value);
	void set_observer(live_values_observer* observer);

	bool get_weighted_sum(const time_type& start, time_type& end,
				weighted_sum<value_type, time_type>& sum);
};

struct generic_live_block_builder;

class live_cache
{
	std::thread m_thread;
	int m_cmd_sock[2];

	std::unique_ptr<zmq::context_t> m_context;

	std::vector<std::string> m_urls;
	std::vector<std::unique_ptr<zmq::socket_t> > m_socks;

	std::vector<std::vector<generic_live_block*>> m_cache;

	std::vector<unsigned> m_config_id_map;

	void process_msg(szarp::ParamsValues* values, size_t sock_no);
	void process_socket(size_t sock_no);

	void start();
	void run();
public:	
	template<class entry_builder = generic_live_block_builder>
	live_cache(const live_cache_config &c, zmq::context_t* context);

	void register_cache_observer(TParam *, live_values_observer*);

	~live_cache();
};

}
#endif
