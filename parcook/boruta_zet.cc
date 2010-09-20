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
/*
 * 
 * Darek Marcinkiewicz <reksio@newterm.pl>
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdexcept>
#include <sstream>
#include <sys/types.h>
#include <event.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "liblog.h"
#include "xmlutils.h"
#include "ipchandler.h"
#include "tokens.h"
#include "borutadmn.h"

#define SENDER_CHECKSUM_PARAM 299

class zet_proto_impl {
	enum PLC_TYPE { ZET, SK } m_plc_type;
	char m_id;
	short *m_read;
	short *m_send;
	size_t m_read_count;
	size_t m_send_count;
	std::vector<bool> m_send_no_data;
	std::vector<char> m_buffer;
	size_t m_data_in_buffer;
	struct event m_timer;
	void set_no_data();
	void start_timer();
	void stop_timer();
protected:
	virtual void driver_finished_job() = 0;
	virtual void terminate_connection() = 0;
	virtual struct event_base* get_event_base() = 0;
public:
	void send_query(struct bufferevent* bufev);
	virtual void connection_error(struct bufferevent *bufev);
	virtual void data_ready(struct bufferevent* bufev, int fd);
	virtual void scheduled(struct bufferevent* bufev, int fd);
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send);
	static void timeout_cb(int fd, short event, void *zet_proto_impl);
};

class zet_proto_tcp : public zet_proto_impl, public tcp_client_driver {
protected:
	virtual void driver_finished_job();
	virtual void terminate_connection();
	struct event_base* get_event_base();
public:
	virtual void connection_error(struct bufferevent *bufev);
	virtual void scheduled(struct bufferevent* bufev, int fd);
	virtual void data_ready(struct bufferevent* bufev, int fd);
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send);
};

class zet_proto_serial : public zet_proto_impl, public serial_client_driver {
protected:
	virtual void driver_finished_job();
	virtual void terminate_connection();
	struct event_base* get_event_base();
public:
	virtual void connection_error(struct bufferevent *bufev);
	virtual void scheduled(struct bufferevent* bufev, int fd);
	virtual void data_ready(struct bufferevent* bufev, int fd);
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration& spc);
};

void zet_proto_impl::set_no_data() {
	for (size_t i = 0; i < m_read_count; i++)
		m_read[i] = SZARP_NO_DATA;
}

void zet_proto_impl::start_timer() {
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	evtimer_add(&m_timer, &tv); 
}

void zet_proto_impl::stop_timer() {
	event_del(&m_timer);
}

void zet_proto_impl::send_query(struct bufferevent* bufev) {
	std::stringstream ss;
	ss << "\x11\x02P" << m_id << "\x03";
	bool sending_data = false;
	unsigned short sender_checksum = 0;
	for (size_t i = 0; i < m_send_count; i++) { 
		short value = m_send[i];
		if (value == SZARP_NO_DATA && m_send_no_data[i] == false) 
			continue;
		sending_data = true;
		m_send[i] = SZARP_NO_DATA;
		ss << i << "," << value << ",";
		sender_checksum += i + value;
		sender_checksum &= 0xffff;
	}
	if (sending_data)
		ss << SENDER_CHECKSUM_PARAM << "," << sender_checksum;
	ss << "\x03\n";
	bufferevent_write(bufev, (void*)ss.str().c_str(), ss.str().size());
	m_data_in_buffer = 0;
	m_buffer.resize(1000);
	start_timer();
}

void zet_proto_impl::connection_error(struct bufferevent *bufev) {
	set_no_data();
	stop_timer();
}

void zet_proto_impl::data_ready(struct bufferevent* bufev, int fd) {
	size_t ret;
	if (m_data_in_buffer + 1 > m_buffer.size() / 2)
		m_buffer.resize(m_buffer.size() * 2);
	ret = bufferevent_read(bufev, &m_buffer.at(m_data_in_buffer), m_buffer.size() - m_data_in_buffer - 1);
	m_data_in_buffer += ret;
	try {
		if (m_buffer.at(m_data_in_buffer - 1) != '\r'
				|| m_buffer.at(m_data_in_buffer - 2) != '\r'
				|| (m_plc_type == SK
					&& m_buffer.at(m_data_in_buffer - 3) != '\r'))
			return;
	} catch (std::out_of_range) {
		return;
	}
	if (m_plc_type == SK)
		m_buffer.at(m_data_in_buffer - 1) = '\0';
	else
		m_buffer.at(m_data_in_buffer) = '\0';
	char **toks;
	int tokc = 0;
	tokenize_d(&m_buffer[0], &toks, &tokc, "\r");
	if (tokc < 3) {
		tokenize_d(NULL, &toks, &tokc, NULL);
		dolog(5, "Unable to parse response from ZET/SK, it is invalid");
		stop_timer();
		set_no_data();
		return;
	}
	size_t params_count = tokc - 5;
	if (params_count != m_read_count) {
		dolog(5, "Invalid number of values received, expected: %zu, got: %zu", m_read_count, params_count);
		tokenize_d(NULL, &toks, &tokc, NULL);
		stop_timer();
		set_no_data();
		return;
	}
	char id = toks[2][0];
	if (id != m_id) {
		dolog(5, "Invalid id in response, expected: %c, got: %c", m_id, id);
		tokenize_d(NULL, &toks, &tokc, NULL);
		set_no_data();
		stop_timer();
		return;
	}
	int checksum = 0;
	for (size_t j = 0; j < m_data_in_buffer; j++)
		checksum += (uint) m_buffer[j];
	/* Checksum without checksum ;-) and last empty lines */
	for (char *c = toks[tokc-1]; *c; c++)
		checksum -= (uint) *c;
	for (char *c = &m_buffer[m_data_in_buffer - 1]; *c == '\r'; c--) 
		checksum -= (uint) *c;
	if (checksum != atoi(toks[tokc-1])) {
		dolog(4, "ZET/SK driver, wrong checksum");
		tokenize_d(NULL, &toks, &tokc, NULL);
		set_no_data();
		stop_timer();
		return;
	}
	for (size_t i = 0; i < m_read_count; i++)
		m_read[i] = atoi(toks[i+4]);
	tokenize_d(NULL, &toks, &tokc, NULL);
	stop_timer();
	driver_finished_job();
}


void zet_proto_impl::scheduled(struct bufferevent* bufev, int fd) {
	set_no_data();
	send_query(bufev);
}

int zet_proto_impl::configure(TUnit* unit, xmlNodePtr node, short* read, short *send) {
	m_id = unit->GetId();
	m_read_count = unit->GetParamsCount();
	m_send_count = unit->GetSendParamsCount();
	m_read = read;
	m_send = send;
	std::string plc;
	if (get_xml_extra_prop(node, "plc", plc))
		return 1;
	if (plc == "sk") {
		m_plc_type = SK;
	} else if (plc == "zet") {
		m_plc_type = ZET;
	} else {
		dolog(0, "Invalid value of plc attribute %s, line:%ld", plc.c_str(), xmlGetLineNo(node));
		return 1;
	}
	TSendParam *sp = unit->GetFirstSendParam();
	for (size_t i = 0; i < m_send_count; i++, sp = sp->GetNext())
		m_send_no_data.push_back(sp->GetSendNoData());
	evtimer_set(&m_timer, timeout_cb, this);
	event_base_set(get_event_base(), &m_timer);
	return 0;
}

void zet_proto_impl::timeout_cb(int fd, short event, void *_zet_proto_impl) {
	zet_proto_impl* z = (zet_proto_impl*) _zet_proto_impl;
	z->set_no_data();
	z->stop_timer();
	z->driver_finished_job();
}

void zet_proto_tcp::driver_finished_job() {
	m_manager->driver_finished_job(this);
}

void zet_proto_tcp::terminate_connection() {
	m_manager->terminate_connection(this);
}

struct event_base* zet_proto_tcp::get_event_base() {
	return m_event_base;
}

void zet_proto_tcp::connection_error(struct bufferevent *bufev) {
	zet_proto_impl::connection_error(bufev);
}

void zet_proto_tcp::scheduled(struct bufferevent* bufev, int fd) {
	zet_proto_impl::scheduled(bufev, fd);
}

void zet_proto_tcp::data_ready(struct bufferevent* bufev, int fd) {
	zet_proto_impl::data_ready(bufev, fd);
}

int zet_proto_tcp::configure(TUnit* unit, xmlNodePtr node, short* read, short *send) {
	return zet_proto_impl::configure(unit, node, read, send);
}

void zet_proto_serial::driver_finished_job() {
	m_manager->driver_finished_job(this);
}

void zet_proto_serial::terminate_connection() {
	m_manager->terminate_connection(this);
}

struct event_base* zet_proto_serial::get_event_base() {
	return m_event_base;
}

void zet_proto_serial::connection_error(struct bufferevent *bufev) {
	zet_proto_impl::connection_error(bufev);
}

void zet_proto_serial::scheduled(struct bufferevent* bufev, int fd) {
	zet_proto_impl::scheduled(bufev, fd);
}

void zet_proto_serial::data_ready(struct bufferevent* bufev, int fd) {
	zet_proto_impl::data_ready(bufev, fd);
}

int zet_proto_serial::configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration& spc) {
	return zet_proto_impl::configure(unit, node, read, send);
}

serial_client_driver* create_zet_serial_client() {
	return new zet_proto_serial();
}

tcp_client_driver* create_zet_tcp_client() {
	return new zet_proto_tcp();
}
