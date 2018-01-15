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

#include "szarp_config.h"
#include "dmncfg.h"

class zmqhandler {
	zmq::socket_t m_sub_sock;
	zmq::socket_t m_pub_sock;

	size_t m_pubs_idx;
	szarp::ParamsValues m_pubs;

	std::vector<szarp::ParamValue> m_send;
	std::unordered_map<size_t, size_t> m_send_map;

	void process_msg(szarp::ParamsValues& values);

public:
	zmqhandler(DaemonConfigInfo* config,
		zmq::context_t& context,
		const std::string& sub_address,
		const std::string& pub_address);

	template<class T, class V> void set_value(size_t i, const T& t, const V& value);
	szarp::ParamValue& get_value(size_t i);

	int subsocket();

	void publish();
	void receive();
};

#endif
