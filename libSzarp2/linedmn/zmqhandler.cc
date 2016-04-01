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

namespace {

void set_param_value(szarp::ParamValue* value, const short& v) {
	value->set_int_value(v);
}

void set_param_value(szarp::ParamValue* value, const int& v) {
	value->set_int_value(v);
}

void set_param_value(szarp::ParamValue* value, const float& v) {
	value->set_float_value(v);

}

void set_param_value(szarp::ParamValue* value, const double& v) {
	value->set_double_value(v);
}

void set_param_time(szarp::ParamValue* value, const sz4::second_time_t& t) {
	value->set_time(t);
}

void set_param_time(szarp::ParamValue* value, const sz4::nanosecond_time_t& t) {
	value->set_time(t.second);
	value->set_nanotime(t.nanosecond);
}

}

void zmqhandler::process_msg(szarp::ParamsValues& values) {
	for (int i = 0; i < values.param_values_size(); i++) {
		const szarp::ParamValue& param_value = values.param_values(i);

		auto it = m_send_map.find(param_value.param_no());
		if (it == m_send_map.end())
			continue;

		m_send[it->second] = param_value;
	}
}

zmqhandler::zmqhandler(
		TSzarpConfig* config,
		TDevice *device,
		zmq::context_t& context,
		const std::string& sub_address,
		const std::string& pub_address)
		:
		m_sub_sock(context, ZMQ_SUB),
		m_pub_sock(context, ZMQ_PUB) {

	for (TUnit* u = device->GetFirstRadio()->GetFirstUnit(); u; u = u->GetNext()) {
		if (TParam* p = u->GetFirstParam())
			m_pubs_idx = p->GetIpcInd();

		size_t send_param_count = 0;
		for (TSendParam *s = u->GetFirstSendParam(); s; s = u->GetNextSendParam(s), send_param_count++) {
			const std::wstring& param_name = s->GetParamName();
			if (param_name.empty())
				continue;

			TParam* param = config->getParamByName(param_name);
			assert(param);

			m_send_map[param->GetIpcInd()] = send_param_count;
		}

		m_send.resize(send_param_count);
	}

	m_pub_sock.connect(pub_address.c_str());
	if (m_send.size())
		m_sub_sock.connect(sub_address.c_str());
}

template<class T, class V> void zmqhandler::set_value(size_t index, const T& t, const V& value) {
	szarp::ParamValue* param = m_pubs.add_param_values();

	param->set_param_no(index + m_pubs_idx);
	set_param_time(param, t);
	set_param_value(param, value);
}

template void zmqhandler::set_value(size_t index, const sz4::second_time_t& t, const short& v);
template void zmqhandler::set_value(size_t index, const sz4::second_time_t& t, const int& v);
template void zmqhandler::set_value(size_t index, const sz4::second_time_t& t, const float& v);
template void zmqhandler::set_value(size_t index, const sz4::second_time_t& t, const double& v);
template void zmqhandler::set_value(size_t index, const time_t& t, const short& v);
template void zmqhandler::set_value(size_t index, const time_t& t, const int& v);
template void zmqhandler::set_value(size_t index, const time_t& t, const float& v);
template void zmqhandler::set_value(size_t index, const time_t& t, const double& v);
template void zmqhandler::set_value(size_t index, const sz4::nanosecond_time_t& t, const short& v);
template void zmqhandler::set_value(size_t index, const sz4::nanosecond_time_t& t, const int& v);
template void zmqhandler::set_value(size_t index, const sz4::nanosecond_time_t& t, const float& v);
template void zmqhandler::set_value(size_t index, const sz4::nanosecond_time_t& t, const double& v);

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

	unsigned int event;
	size_t event_size = sizeof(event);	
	m_sub_sock.getsockopt(ZMQ_EVENTS, &event, &event_size);
	while (event & ZMQ_POLLIN) {
		szarp::ParamsValues values;
		zmq::message_t msg;
		m_sub_sock.recv(&msg);

		if (values.ParseFromArray(msg.data(), msg.size()))
			process_msg(values);

		m_sub_sock.getsockopt(ZMQ_EVENTS, &event, &event_size);
	}
}

