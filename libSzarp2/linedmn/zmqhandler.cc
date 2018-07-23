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

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include <zmqhandler.h>

#include "sz4/defs.h"

zmqhandler::zmqhandler(DaemonConfigInfo* config,
	zmq::context_t& context,
	const std::string& sub_address,
	const std::string& pub_address)
	:
	m_sub_sock(context, ZMQ_SUB),
	m_pub_sock(context, ZMQ_PUB) {

	m_pubs_idx = config->GetFirstParamIpcInd();

	// Ignore units for sends
	auto param_sent_no = 0;
	for (const auto send_ipc_ind: config->GetSendIpcInds()) {
		m_send_map[send_ipc_ind].insert(param_sent_no++);
	}

	m_send.resize(param_sent_no);

	m_pub_sock.connect(pub_address.c_str());
	if (m_send.size())
		m_sub_sock.connect(sub_address.c_str());

	int zero = 0;
	m_pub_sock.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));
	m_sub_sock.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));

	m_sub_sock.setsockopt(ZMQ_SUBSCRIBE, "", 0);
}

void zmqhandler::process_msg(szarp::ParamsValues& values) {
	for (int i = 0; i < values.param_values_size(); i++) {
		const szarp::ParamValue& param_value = values.param_values(i);

		auto it = m_send_map.find(param_value.param_no());
		if (it == m_send_map.end())
			continue;

		for (const auto s: it->second) {
			m_send[s] = param_value;
		}
	}
}

szarp::ParamValue& zmqhandler::get_value(size_t i) {
	return m_send.at(i);
}

int zmqhandler::subsocket() {
	if (!m_send.size())
		return -1;

	size_t fds;
	int fd;

	fds = sizeof(fd);
	m_sub_sock.getsockopt(ZMQ_FD, &fd, &fds);

	return fd;
}

void delete_str(void *, void *buf) {
	delete (std::string*) buf;
}

void zmqhandler::publish() {
	std::string* buffer = new std::string();
	google::protobuf::io::StringOutputStream stream(buffer);

	m_pubs.SerializeToZeroCopyStream(&stream);
	m_pubs.clear_param_values();

	zmq::message_t msg((void*)buffer->data(), buffer->size(), delete_str, buffer);
	m_pub_sock.send(msg);
}

void zmqhandler::receive() {
	if (!m_send.size())
		return;

	do {
		zmq::message_t msg;
		if (!m_sub_sock.recv(&msg, ZMQ_NOBLOCK))
			return;

		szarp::ParamsValues values;
		if (values.ParseFromArray(msg.data(), msg.size()))
			process_msg(values);
		else	
			/* XXX: */;

	} while (true);
}
