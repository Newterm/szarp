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

#include "live_cache.h"


namespace sz4 {

void live_cache::process_msg(szarp::ParamsValues* values, size_t sock_no) {
	auto cache = m_cache[sock_no];
	for (int i = 0; values->param_values_size(); i++) {
		szarp::ParamsValues& value = values->param_values(i);

		size_t param_no = value.param_no();
		if (param_no < cache.size())
			cache[param_no]->process_live_value(&value);
	}
}

void live_cache::process_socket(size_t sock_no) {
	unsigned int event;
	size_t event_size = sizeof(event);	

	m_socks[i]->getsockopt(ZMQ_EVENTS, &event, &event_size);
	while (event & ZMQ_POLLIN) {
		szarp::ParamsValues values;

		zmq::message_t msg;
		m_sub_sock.recv(&msg);

		if (values.ParseFromArray(msg.data(), msg.size()))
			process_msg(&values, sock_no);
		else	
			/* XXX: */;

		m_socks[i]->getsockopt(ZMQ_EVENTS, &event, &event_size);
	}
}

void live_cache::run() {
	std::vector<zmq::poll_item_t> polls;

	for (auto& url : urls) {
		auto sock = std::make_unique<zmq::socket_t>(*m_context);
		sock->connect(url);
		m_socks.push_back(std::move(sock));

		zmq::poll_item_t poll;
		poll.socket = *sock;
		poll.events = ZMQ_POLLIN;

		polls.push_back(poll);
	}

	do {
		int rc = zmq::poll(&polls[0], polls.size(), -1);
		if (rc == -1)
			continue;

		for (size_t i = 0; i < polls.size(); i++) 
			if (polls[i].revents & ZMQ_POLLIN)
				process_socket(i);
	} while (true);

}

void live_cache::start() {
	std::thread(std::bind(&live_cache::run, this));
}

template class live_block<short, sz4::second_time_t, base_types>;
template class live_block<double, sz4::second_time_t, base_types>;
template class live_block<float, sz4::second_time_t, base_types>;
template class live_block<int, sz4::second_time_t, base_types>;

template class live_block<short, sz4::nanosecond_time_t, base_types>;
template class live_block<double, sz4::nanosecond_time_t, base_types>;
template class live_block<float, sz4::nanosecond_time_t, base_types>;
template class live_block<int, sz4::nanosecond_time_t, base_types>;

template live_cache::live_cache<generic_live_entry_builder>(
		std::vector<std::pair<std::string, TSzarpConfig*>> configuration,
		time_difference<second_time_t>::type& cache_duration);

}
