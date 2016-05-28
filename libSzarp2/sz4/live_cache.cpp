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

#include <memory>
#include <future>

#ifndef MINGW32
#include <zmq.hpp>
#endif

#include "protobuf/paramsvalues.pb.h"
#include "sz4/block.h"
#include "sz4/defs.h"
#include "szarp_config.h"
#include "sz4/live_observer.h"
#include "sz4/live_cache.h"
#include "sz4/factory.h"
#include "sz4/live_cache_templ.h"


namespace sz4 {

void live_cache::process_msg(szarp::ParamsValues* values, size_t sock_no) {
#ifndef MINGW32
	auto cache = m_cache[sock_no];
	for (int i = 0; i < values->param_values_size(); i++) {
		szarp::ParamValue* value = values->mutable_param_values(i);

		size_t param_no = value->param_no();
		if (param_no < cache.size())
			cache[param_no]->process_live_value(value);
	}
#endif
}

void live_cache::process_socket(size_t sock_no) {
#ifndef MINGW32
	do {
		zmq::message_t msg;
		if (!m_socks[sock_no]->recv(&msg, ZMQ_NOBLOCK))
			return;

		szarp::ParamsValues values;
		if (values.ParseFromArray(msg.data(), msg.size()))
			process_msg(&values, sock_no);
		else	
			/* XXX: */;

	} while (true);
#endif
}

void live_cache::start() {
#ifndef MINGW32
	std::promise<void> promise;
	auto future = promise.get_future();
	m_thread = std::thread(&live_cache::run, this, std::move(promise));
	future.wait();
#endif
}

void live_cache::run(std::promise<void> promise) {
#ifndef MINGW32
	for (auto& url : m_urls) {
		auto sock = std::unique_ptr<zmq::socket_t>(new zmq::socket_t(*m_context, ZMQ_SUB));
		sock->connect(url.c_str());
		sock->setsockopt(ZMQ_SUBSCRIBE, "", 0);
		int zero = 0;
		sock->setsockopt(ZMQ_LINGER, &zero, sizeof(zero));
		m_socks.push_back(std::move(sock));
	}

	std::vector<zmq::pollitem_t> polls;

	for (auto& sock : m_socks) {
		zmq::pollitem_t poll;
		poll.socket = *sock;
		poll.events = ZMQ_POLLIN;

		polls.push_back(poll);
	}

	for (size_t i = 0; i < polls.size(); i++) 
		process_socket(i);

	promise.set_value();

	zmq::pollitem_t poll;
	poll.socket = nullptr;
	poll.fd = m_cmd_sock[0];
	poll.events = ZMQ_POLLIN;
	polls.push_back(poll);

	do {
		int rc = zmq::poll(&polls[0], polls.size(), -1);
		if (rc == -1)
			continue;

		for (size_t i = 0; i < polls.size() - 1; i++) 
			if (polls[i].revents & ZMQ_POLLIN)
				process_socket(i);

		if (polls.back().revents & ZMQ_POLLIN)
			return;
	} while (true);
#endif
}

void live_cache::register_cache_observer(TParam *param, live_values_observer* observer) {
#ifndef MINGW32
	auto i = std::find(m_config_id_map.begin(), m_config_id_map.end(), param->GetConfigId());
	if (i == m_config_id_map.end())
		return;

	size_t idx = std::distance(m_config_id_map.begin(), i);
	if (m_cache.at(idx).size() < param->GetParamId())
		return;

	auto block = m_cache[idx][param->GetParamId()];
	block->set_observer(observer);

	observer->set_live_block(block);
#endif
}


live_cache::~live_cache() {
#ifndef MINGW32
	write(m_cmd_sock[1], "a", 1);

	m_thread.join();

	close(m_cmd_sock[0]);
	close(m_cmd_sock[1]);
#endif
}

template
live_cache::live_cache<generic_live_block_builder>
	(const live_cache_config &c, zmq::context_t* context);

template class live_block<short, sz4::second_time_t>;
template class live_block<double, sz4::second_time_t>;
template class live_block<float, sz4::second_time_t>;
template class live_block<int, sz4::second_time_t>;

template class live_block<short, sz4::nanosecond_time_t>;
template class live_block<double, sz4::nanosecond_time_t>;
template class live_block<float, sz4::nanosecond_time_t>;
template class live_block<int, sz4::nanosecond_time_t>;

}
