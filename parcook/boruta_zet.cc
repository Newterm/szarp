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
/*
 Daemon description block.

 @description_start

 @class 4

 @devices This is a borutadmn subdriver for ZET protocol, used by Z-Elektronik, SK2000 and SK4000 PLC units,
 produced for Praterm company. It's a simple ASCII protocol with unit addressing and CRC checksum, used on
 serial lines or over TCP.
 @devices.pl Sterownik do demona borutadmn, obs³uguj±cy protokó³ ZET, u¿ywany do komunikacji z produkowanymi
 dla firmy Praterm regulatorami Z-Elektronik, SK2000 i SK4000. Protokó³ ZET jest prostym protoko³em typu ASCII
 z adresowaniem jednostek i sprawdzaniem sumy kontrolnej CRC, przeznaczonym do transmisji po ³±czu szeregowym
 lub za pomoc± transmisji TCP.

 @protocol ZET over serial line or ethernet/serial converter.
 @protocol.pl Protokó³ ZET na linii szeregowej RS232/485 lub konwerterze ethernet/RS232.

 @config Driver is configured as a unit subelement of device element in params.xml. See example for allowed
 attributes.
 @config.pl Sterownik jest konfigurowany w pliku params.xml, w podelemencie unit elementu device. Opis
 dodatkowych atrybutów XML znajduje siê w przyk³adzie poni¿ej.

 @config_example
<device 
	xmlns:extra="http://www.praterm.com.pl/SZARP/ipk-extra"
	daemon="/opt/szarp/bin/borutadmn" 
	path="/dev/null"
		ignored, should be /dev/null
	speed="9600"
		ignored
	>

	<unit 
		id="1"
			ZET unit identifier
		type="1"
			ignored, should be 1
	       	subtype="1" 
			ignored, should be 1
		bufsize="1"
			average buffer size, at least 1
		extra:proto="zet" 
			protocol name, denoting boruta driver to use, should be "zet" for this driver
		extra:mode="client" 
			unit working mode, for this driver must be "client"
		extra:plc="zet"
			PLC type, must be "zet" (for Z-Elektronik) or "sk" (for SK2000/SK4000)
		extra:medium="serial"
			data transmission medium, may be "serial" (for RS232) or "tcp" (for TCP)
		extra:tcp-address="172.18.2.2"
		extra:tcp-port="23"
			IP address and port to connect to, required if medium is set to "tcp"
		extra:path="/dev/ttyS0"
			path to serial port, required if medium is set to "serial"
		extra:speed="19200"
			optional serial port speed in bps (for medium "serial"), default is 9600, allowed values
			are also 300, 600, 1200, 2400, 4800, 19200, 38400; all other values result in
			port speed 9600
		extra:parity="even"
			optional serial port parity (for medium "serial"), default is "none", other
			allowed values are "odd" and "even"
		extra:stopbits="1"
			optional serial port stop bits number (for medium "serial"), default is 1, other
			allowed value is 2
	>
		<param .../>
			number and order of parameters corresponds to number and order of parametrs in PLC
		...
	...
      </unit>
 </device>

 @config_example.pl
<device 
	xmlns:extra="http://www.praterm.com.pl/SZARP/ipk-extra"
	daemon="/opt/szarp/bin/borutadmn" 
	path="/dev/null"
		ignorowany, zaleca siê ustawienie /dev/null
	speed="9600"
		ignorowany
	>

	<unit 
		id="1"
			identyfikator sterownika
		type="1"
			ignorowany, powinno byæ "1"
	       	subtype="1" 
			ignorowany, powinno byæ "1"
		bufsize="1"
			wielko¶æ bufora u¶redniania, 1 lub wiêcej
		extra:proto="zet" 
			nazwa protoko³u, u¿ywana przez Borutê do ustalenia u¿ywanego sterownika, dla
			tego sterownika musi byæ "zet"
		extra:mode="client" 
			tryb pracy jednostki, dla tego sterownika powinien byæ "client"
		extra:plc="zet"
			typ regulatora - "zet" dla Z-Elektronika lub "sk" dla SK2000/SK4000
		extra:medium="serial"
			medium transmisyjne, "serial" dla RS232, "tcp" dla konwertera ethernet/RS232
		extra:tcp-address="172.18.2.2"
		extra:tcp-port="23"
			adres i port IP do którego siê ³±czymy, u¿ywany dla medium "tcp"
		extra:path="/dev/ttyS0"
			¶cie¿ka do portu szeregowego dla medium "serial"
		extra:speed="19200"
			opcjonalna prêdko¶æ portu szeregowego w bps dla medium "serial", domy¶lna to 9600,
			inne mo¿liwe warto¶ci to 300, 600, 1200, 2400, 4800, 19200, 38400; ustawienie innej
			warto¶ci spowoduje przyjêcie prêdko¶ci 9600
		extra:parity="even"
			opcjonalna parzysto¶æ portu dla medium "serial", mo¿liwe warto¶ci to "none" (domy¶lna),
			"odd" i "even"
		extra:stopbits="1"
			opcjonalna liczba bitów stopu dla medium "serial", 1 (domy¶lnie) lub 2
	>
		<param .../>
			Ilo¶æ i kolejno¶æ parametrów odpowiada ilo¶ci i kolejno¶ci parametrów w sterowniku.
		...
      </unit>
	...
 </device>

 @description_end

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
	size_t m_timeout_count;
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
	virtual void starting_new_cycle();
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
	virtual void starting_new_cycle();
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
	virtual void starting_new_cycle();
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
	tv.tv_sec = 10;
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

void zet_proto_impl::starting_new_cycle() {
}

void zet_proto_impl::connection_error(struct bufferevent *bufev) {
	set_no_data();
	stop_timer();
}

void zet_proto_impl::data_ready(struct bufferevent* bufev, int fd) {
	size_t ret;
	m_timeout_count = 0;
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

	stop_timer();
	try {
		m_buffer.at(m_data_in_buffer) = '\0';
		char **toks;
		int tokc = 0;
		tokenize_d(&m_buffer[0], &toks, &tokc, "\r");
		if (tokc < 3) {
			tokenize_d(NULL, &toks, &tokc, NULL);
			dolog(5, "Unable to parse response from ZET/SK, it is invalid");
			throw std::invalid_argument("Wrong response format");
		}
		size_t params_count = tokc - 5;
		if (params_count != m_read_count) {
			dolog(5, "Invalid number of values received, expected: %zu, got: %zu", m_read_count, params_count);
			tokenize_d(NULL, &toks, &tokc, NULL);
			throw std::invalid_argument("Wrong number or values");
		}
		char id = toks[2][0];
		if (id != m_id) {
			dolog(5, "Invalid id in response, expected: %c, got: %c", m_id, id);
			tokenize_d(NULL, &toks, &tokc, NULL);
			throw std::invalid_argument("Wrong id in response");
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
			throw std::invalid_argument("Wrong checksum");
		}
		for (size_t i = 0; i < m_read_count; i++)
			m_read[i] = atoi(toks[i+4]);
		tokenize_d(NULL, &toks, &tokc, NULL);
	} catch (const std::invalid_argument&) {
		set_no_data();
	}
	driver_finished_job();
}


void zet_proto_impl::scheduled(struct bufferevent* bufev, int fd) {
	send_query(bufev);
}

int zet_proto_impl::configure(TUnit* unit, xmlNodePtr node, short* read, short *send) {
	m_id = unit->GetId();
	m_read_count = unit->GetParamsCount();
	m_send_count = unit->GetSendParamsCount();
	m_read = read;
	m_send = send;
	m_timeout_count = 0;
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
	if (++z->m_timeout_count > 5) {
		z->m_timeout_count = 0;
		z->set_no_data();
		z->terminate_connection();
	} else {
		z->set_no_data();
		z->driver_finished_job();
	}
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

void zet_proto_tcp::starting_new_cycle() {
	zet_proto_impl::starting_new_cycle();
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

void zet_proto_serial::starting_new_cycle() {
	zet_proto_impl::starting_new_cycle();
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
