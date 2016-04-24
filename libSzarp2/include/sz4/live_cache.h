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

namespace zmq {
class socket_t;
class contex_t;
}

namespace sz4
{

class generic_live_block {
	virtual void process_live_value(szarp::ParamValue* value);
};

template<class value_type, class time_type> class live_block : public generic_live_block {
	std::deque<std::pair<value_type, time_type>> m_block;
	std::mutex m_lock;
	std::atomic<live_values_observer*> m_observer;
		
	time_difference<time_type>::type m_retention;

	time_type get_time(szarp::ParamValue* value);
	value_type get_value(szarp::ParamValue* value);
public:
	live_block(time_difference<second_time_t>::type retention);

	virtual void process_live_value(szarp::ParamValue* value);
	void set_observer(live_values_observer* observer);
};

struct generic_live_entry_builder;

class live_cache
{
	std::unique_ptr<zmq::context_t> m_context;

	std::vector<std::string> m_addrs;
	std::vector<std::unique_ptr<zmq::socket_t> > m_socks;

	std::vector<std::vector<generic_live_entry*>> m_cache;

	std::vector<unsigned> m_config_id_map;
public:	
	template<class entry_builder = generic_live_entry_builder>
	live_cache(
		std::vector<std::pair<std::string, TSzarpConfig*>> configuration,
		time_difference<second_time_t>::type& cache_duration
	);

	void register_cache_monitor(TParam *, live_cache_monitor*);

	void start();

};

}
#endif
