#ifndef ZMQHANDLER_H
#define ZMQHANDLER_H
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

#include <unordered_map>
#include <zmq.hpp>

#include "protobuf/paramsvalues.pb.h"
#include "sz4/util.h"

#include "szarp_config.h"
#include "dmncfg.h"

class zmqhandler {
	zmq::socket_t m_sub_sock;
	zmq::socket_t m_pub_sock;

	size_t m_pubs_idx;
	szarp::ParamsValues m_pubs;

	std::vector<szarp::ParamValue> m_send;
	std::unordered_map<size_t, std::set<size_t>> m_send_map;

	void process_msg(szarp::ParamsValues& values);

public:
	zmqhandler(DaemonConfigInfo* config,
		zmq::context_t& context,
		const std::string& sub_address,
		const std::string& pub_address);

	template <class T, class V> void set_value(size_t index, const T& t, const V& value) {
		szarp::ParamValue* param = m_pubs.add_param_values();
		param->set_param_no(index + m_pubs_idx);
		param->set_is_nan(false);
		sz4::pv_set_time(*param, t);
		sz4::pv_set_value(*param, value);
	}

	template <class T> void set_no_data(size_t index, const T& t) {
		szarp::ParamValue* param = m_pubs.add_param_values();
		param->set_param_no(index + m_pubs_idx);
		param->set_is_nan(true);
		sz4::pv_set_time(*param, t);
	}

	szarp::ParamValue& get_value(size_t i);

	template <class VT> sz4::value_time_pair<VT, sz4::nanosecond_time_t> get_send(size_t i, int32_t prec_adj) {
		auto& pv = get_value(i);
		return sz4::cast_param_value<VT>(pv, prec_adj);
	}

	int subsocket();

	void publish();
	void receive();
};

#endif
