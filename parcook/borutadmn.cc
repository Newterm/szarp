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

 @devices This is a generic daemon that does not implement any particular protocol. Instead it relies on its
  components - subdrivers - to provide support for particular protocols. Currently drivers are available for modbus, 
  zet and fp2100 protocols. 
 @devices.pl Boruta jest uniwersalnym demonem, który sam w sobie nie implementuje ¿adnego z protoko³ów.
  Obs³ug± w³a¶ciwych protoko³ów zajmuj± siê sterowniki bêd±ce modu³ami boruty. Obecnie dostêpne s± sterowniki
  dla protoko³ów Modbus, ZET i FP210.

 @protocol Modbus RTU/ASCII, Modbus TCP, ZET and FP210
 @protocol.pl Modbus RTU/ASCII, Modbus TCP, ZET i FP210

 @config Daemon is configured in params.xml. Each unit subelement of device describes one driver. Please consult
 descriptions of particular drivers for configuration details.
 @config.pl Sterownik jest konfigurowany w pliku params.xml. Ka¿dy podelement unit zawiera konfiguracjê jednego
 sterownika. Szczegó³y konfiguracji znajdziesz w opisach poszczególnych sterowników.

 @config_example
<device 
	xmlns:extra="http://www.praterm.com.pl/SZARP/ipk-extra"
	daemon="/opt/szarp/bin/borutadmn" 
	path="/dev/null"
	>

	<unit id="1"
		extra:mode="client"
			this unit working mode possible values are 'client' and 'server'
		extra:medium="serial"
			data transmittion medium, may be either serial (line) or tcp
		extra:proto="modbus"
			protocol to use for this unit
		in case of serial line, following self-explanatory attributes are also supported:
		extra:path, extra:speed, extra:parity, extra:stopbits, extra:char_size (all but path are not required, 
			they have defaults which are 9600, N, 1, 8)
		in case of tcp client mode following attributes are required:
		extra:tcp-address, extra:tcp-port
		in case of tcp server mode one need to specify extra:tcp-port attribute
	>
	...
      </unit>
 </device>

 @description_end

*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <deque>
#include <set>

#include <libxml/parser.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "conversion.h"
#include "liblog.h"
#include "xmlutils.h"
#include "tokens.h"
#include "borutadmn.h"

bool g_debug = false;

/*implementation*/

std::string sock_addr_to_string(struct sockaddr_in& addr) {
	return inet_ntoa(addr.sin_addr);
}

int set_nonblock(int fd) {

	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		dolog(0, "set_nonblock: Unable to get socket flags");
		return -1;
	}

	flags |= O_NONBLOCK;
	flags = fcntl(fd, F_SETFL, flags);
	if (flags == -1) {
		dolog(0, "set_nonblock: Unable to set socket flags");
		return -1;
	}

	return 0;
}

bool operator==(const sockaddr_in &s1, const sockaddr_in &s2) {
	return s1.sin_addr.s_addr == s2.sin_addr.s_addr && s1.sin_port == s2.sin_port;
}

int get_serial_port_config(xmlNodePtr node, serial_port_configuration &spc) {
	std::string path;
	if (get_xml_extra_prop(node, "path", path))
		return 1;
	spc.path = path;

	int speed = 0;
	get_xml_extra_prop(node, "speed", speed, true);
	if (speed == 0) {
		dolog(10, "Speed not specified, assuming value 9600");
		speed = 9600;
	}
	dolog(10, "Serial port configuration, speed: %d", speed);
	spc.speed = speed;

	std::string parity;
	get_xml_extra_prop(node, "parity", parity, true);
	if (parity.empty()) {
		dolog(10, "Serial port configuration, parity not specified, assuming no parity");
		spc.parity = serial_port_configuration::NONE;
	} else if (parity == "none") {
		dolog(10, "Serial port configuration, none parity");
		spc.parity = serial_port_configuration::NONE;
	} else if (parity == "even") {
		dolog(10, "Serial port configuration, even parity");
		spc.parity = serial_port_configuration::EVEN;
	} else if (parity == "odd") {
		dolog(10, "Serial port configuration, odd parity");
		spc.parity = serial_port_configuration::ODD;
	} else {
		dolog(0, "Unsupported parity %s, confiugration invalid (line %ld)!!!", parity.c_str(), xmlGetLineNo(node));
		return 1;	
	}

	std::string stop_bits;
	get_xml_extra_prop(node, "stopbits", stop_bits, true);
	if (stop_bits.empty()) {
		dolog(10, "Serial port configuration, stop bits not specified, assuming 1 stop bit");
		spc.stop_bits = 1;
	} else if (stop_bits == "1") {
		dolog(10, "Serial port configuration, setting one stop bit");
		spc.stop_bits = 1;
	} else if (stop_bits == "2") {
		dolog(10, "Serial port configuration, setting two stop bits");
		spc.stop_bits = 2;
	} else {
		dolog(0, "Unsupported number of stop bits %s, confiugration invalid (line %ld)!!!", stop_bits.c_str(), xmlGetLineNo(node));
		return 1;
	}

	std::string char_size;
	get_xml_extra_prop(node, "char_size", char_size, true);
	if (char_size.empty()) {
		dolog(10, "Serial port configuration, char size not specified, assuming 8 bit char size");
		spc.char_size = serial_port_configuration::CS_8;
	} else if (char_size == "8") {
		dolog(10, "Serial port configuration, setting 8 bit char size");
		spc.char_size = serial_port_configuration::CS_8;
	} else if (char_size == "7") {
		dolog(10, "Serial port configuration, setting 7 bit char size");
		spc.char_size = serial_port_configuration::CS_7;
	} else if (char_size == "6") {
		dolog(10, "Serial port configuration, setting 6 bit char size");
		spc.char_size = serial_port_configuration::CS_6;
	} else {
		dolog(0, "Unsupported char size %s, confiugration invalid (line %ld)!!!", char_size.c_str(), xmlGetLineNo(node));
		return 1;
	}
	return 0;
}

int set_serial_port_settings(int fd, serial_port_configuration &spc) {
	struct termios ti;
	if (tcgetattr(fd, &ti) == -1) {
		dolog(1, "Cannot retrieve port settings, errno:%d (%s)", errno, strerror(errno));
		return 1;
	}

	dolog(8, "setting port speed to %d", spc.speed);
	switch (spc.speed) {
		case 300:
			ti.c_cflag = B300;
			break;
		case 600:
			ti.c_cflag = B600;
			break;
		case 1200:
			ti.c_cflag = B1200;
			break;
		case 2400:
			ti.c_cflag = B2400;
			break;
		case 4800:
			ti.c_cflag = B4800;
			break;
		case 9600:
			ti.c_cflag = B9600;
			break;
		case 19200:
			ti.c_cflag = B19200;
			break;
		case 38400:
			ti.c_cflag = B38400;
			break;
		case 115200:
			ti.c_cflag = B115200;
			break;
		default:
			ti.c_cflag = B9600;
			dolog(8, "setting port speed to default value 9600");
	}

	if (spc.stop_bits == 2)
		ti.c_cflag |= CSTOPB;

	switch (spc.parity) {
		case serial_port_configuration::ODD:
			ti.c_cflag |= PARODD;
		case serial_port_configuration::EVEN:
			ti.c_cflag |= PARENB;
			break;
		case serial_port_configuration::NONE:
			break;
	}
			
	ti.c_oflag = 
	ti.c_iflag =
	ti.c_lflag = 0;

	ti.c_cflag |= CREAD | CLOCAL ;

	switch (spc.char_size) {
		case serial_port_configuration::CS_8:
			ti.c_cflag |= CS8;
			break;
		case serial_port_configuration::CS_7:
			ti.c_cflag |= CS7;
			break;
		case serial_port_configuration::CS_6:
			ti.c_cflag |= CS6;
			break;
	}

	if (tcsetattr(fd, TCSANOW, &ti) == -1) {
		dolog(1,"Cannot set port settings, errno: %d (%s)", errno, strerror(errno));	
		return -1;
	}
	return 0;
}

void boruta_driver::set_event_base(struct event_base* ev_base)
{
	m_event_base = ev_base;
}

const std::pair<size_t, size_t> & client_driver::id() {
	return m_id;
}

void client_driver::set_id(std::pair<size_t, size_t> id) {
	m_id = id;
}

void client_driver::set_manager(client_manager* manager) {
	m_manager = manager;
}

void client_driver::finished_cycle() {
	return;
}

void client_driver::starting_new_cycle() {
	return;
}

size_t& server_driver::id() {
	return m_id;
}

tcp_proxy_2_serial_client::tcp_proxy_2_serial_client(serial_client_driver* _serial_client) : m_serial_client(_serial_client) { }

const std::pair<size_t, size_t> & tcp_proxy_2_serial_client::id() {
	return m_serial_client->id();
}

void tcp_proxy_2_serial_client::set_id(std::pair<size_t, size_t> id) {
	m_serial_client->set_id(id);
}

void tcp_proxy_2_serial_client::set_manager(client_manager* manager) {
	m_serial_client->set_manager(manager);
}

void tcp_proxy_2_serial_client::set_event_base(struct event_base* ev_base) {
	m_serial_client->set_event_base(ev_base);
}

void tcp_proxy_2_serial_client::connection_error(struct bufferevent *bufev) {
	m_serial_client->connection_error(bufev);
}

void tcp_proxy_2_serial_client::scheduled(struct bufferevent* bufev, int fd) {
	m_serial_client->scheduled(bufev, fd);
}

void tcp_proxy_2_serial_client::data_ready(struct bufferevent* bufev, int fd) {
	m_serial_client->data_ready(bufev, fd);
}

void tcp_proxy_2_serial_client::finished_cycle() {
	m_serial_client->finished_cycle();
}

void tcp_proxy_2_serial_client::starting_new_cycle() {
	m_serial_client->starting_new_cycle();
}

int tcp_proxy_2_serial_client::configure(TUnit* unit, xmlNodePtr node, short* read, short *send) {
	serial_port_configuration spc;
	if (get_serial_port_config(node, spc)) {
		dolog(1, "tcp_proxy_2_serial_client: failed to get serial port settings for tcp_proxy_2_serial_client");
		return 1;
	}
	return m_serial_client->configure(unit, node, read, send, spc);
}

tcp_proxy_2_serial_client::~tcp_proxy_2_serial_client() {
	delete m_serial_client;
}


void serial_server_driver::set_manager(serial_server_manager* manager) {
	m_manager = manager;
}

void tcp_server_driver::set_manager(tcp_server_manager* manager) {
	m_manager = manager;
}

protocols::protocols() {
	m_tcp_client_factories["zet"] = create_zet_tcp_client;
	m_serial_client_factories["zet"] = create_zet_serial_client;
	m_tcp_client_factories["modbus"] = create_modbus_tcp_client;
	m_serial_client_factories["modbus"] = create_modbus_serial_client;
	m_tcp_server_factories["modbus"] = create_modbus_tcp_server;
	m_serial_server_factories["modbus"] = create_modbus_serial_server;
	m_serial_client_factories["fp210"] = create_fp210_serial_client;
	m_serial_client_factories["lumel"] = create_lumel_serial_client;
	m_tcp_client_factories["wmtp"] = create_wmtp_tcp_client;
}

std::string protocols::get_proto_name(xmlNodePtr node) {
	std::string ret;
	get_xml_extra_prop(node, "proto", ret);
	return ret;
}

tcp_client_driver* protocols::create_tcp_client_driver(xmlNodePtr node) {
	std::string proto = get_proto_name(node);
	if (proto.empty())
		return NULL;
	std::string use_tcp_2_serial_proxy;
	if (get_xml_extra_prop(node, "use_tcp_2_serial_proxy", use_tcp_2_serial_proxy, true))
		return NULL;

	if (use_tcp_2_serial_proxy != "yes") {
		tcp_client_factories_table::iterator i = m_tcp_client_factories.find(proto);
		if (i == m_tcp_client_factories.end()) {
			dolog(0, "No driver defined for proto %s and tcp client role", proto.c_str());
			return NULL;
		}
		return i->second();
	} else {
		serial_client_factories_table::iterator i = m_serial_client_factories.find(proto);
		if (i == m_serial_client_factories.end()) {
			dolog(0, "No driver defined for proto %s and serial client role", proto.c_str());
			return NULL;
		}
		return new tcp_proxy_2_serial_client(i->second());
	}
}

serial_client_driver* protocols::create_serial_client_driver(xmlNodePtr node) {
	std::string proto = get_proto_name(node);
	if (proto.empty())
		return NULL;
	serial_client_factories_table::iterator i = m_serial_client_factories.find(proto);
	if (i == m_serial_client_factories.end()) {
		dolog(0, "No driver defined for proto %s and serial client role", proto.c_str());
		return NULL;
	}
	return i->second();
}

tcp_server_driver* protocols::create_tcp_server_driver(xmlNodePtr node) {
	std::string proto = get_proto_name(node);
	if (proto.empty())
		return NULL;
	tcp_server_factories_table::iterator i = m_tcp_server_factories.find(proto);
	if (i == m_tcp_server_factories.end()) {
		dolog(0, "No driver defined for proto %s and tcp server role", proto.c_str());
		return NULL;
	}
	return i->second();
}

serial_server_driver* protocols::create_serial_server_driver(xmlNodePtr node) {
	std::string proto = get_proto_name(node);
	if (proto.empty())
		return NULL;
	serial_server_factories_table::iterator i = m_serial_server_factories.find(proto);
	if (i == m_serial_server_factories.end()) {
		dolog(0, "No driver defined for proto %s and serial server role", proto.c_str());
		return NULL;
	}
	return i->second();
}

serial_connection::serial_connection(size_t _conn_no, serial_connection_manager *_manager) :
	conn_no(_conn_no), manager(_manager), fd(-1), bufev(NULL) {}

int serial_connection::open_connection(const std::string& path, struct event_base* ev_base) {
	fd = open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
	if (fd == -1) {
		dolog(0, "Failed do open port: %s, error: %s", path.c_str(), strerror(errno));
		return 1;
	}
	bufev = bufferevent_new(fd, connection_read_cb, NULL, connection_error_cb, this);
	bufferevent_base_set(ev_base, bufev);
	bufferevent_enable(bufev, EV_READ | EV_WRITE | EV_PERSIST);
	return 0;
}

void serial_connection::close_connection() {
	if (bufev) {
		bufferevent_free(bufev);
		bufev = NULL;
	}
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}	
}

void serial_connection::connection_read_cb(struct bufferevent *ev, void* _connection) {
	serial_connection* c = (serial_connection*) _connection;
	c->manager->connection_read_cb(c);
}

void serial_connection::connection_error_cb(struct bufferevent *ev, short event, void* _connection) {
	serial_connection* c = (serial_connection*) _connection;
	c->manager->connection_error_cb(c);
}

void client_manager::finished_cycle() {
	dolog(10, "client_manager::finished_cycle");
	for (std::vector<std::vector<client_driver*> >::iterator i = m_connection_client_map.begin();
			i != m_connection_client_map.end();
			i++) 
		for (std::vector<client_driver*>::iterator j = i->begin();
				j != i->end();
				j++)
			(*j)->finished_cycle();
}

void client_manager::starting_new_cycle() {
	dolog(10, "client_manager, starting new cycle");
	for (std::vector<std::vector<client_driver*> >::iterator i = m_connection_client_map.begin();
			i != m_connection_client_map.end();
			i++) 
		for (std::vector<client_driver*>::iterator j = i->begin();
				j != i->end();
				j++)
			(*j)->starting_new_cycle();
	for (size_t connection = 0; connection < m_connection_client_map.size(); connection++) {
		size_t& client = m_current_client.at(connection);
		if (do_get_connection_state(connection) == NOT_CONNECTED) {
			if (do_establish_connection(connection)) {
				for (size_t c = 0; c < m_connection_client_map.at(connection).size(); c++)
					m_connection_client_map.at(connection).at(c)->connection_error(NULL);
				continue;
			}
		}
		if (do_get_connection_state(connection) == CONNECTED) {
			if (client == m_connection_client_map.at(connection).size()) {
				client = 0;
				do_schedule(connection, client);
			}
		}
	}
}

void client_manager::driver_finished_job(client_driver *driver) {
	size_t connection = driver->id().first;
	size_t& client = m_current_client.at(connection);
	if (driver->id().second != client) {
		dolog(0, "Boruta was notfied by client number %zu (connection %zu) that it has finished it's job!", client, connection);
		dolog(0, "But this driver in not a current driver for this connection, THIS IS VERY, VERY WRONG (BUGGY DRIVER?)");
		return;
	}
	if (++client < m_connection_client_map.at(connection).size())
		do_schedule(connection, client);
}

void client_manager::terminate_connection(client_driver *driver) {
	size_t connection = driver->id().first;
	size_t& client = m_current_client.at(connection);
	if (driver->id().second != client) {
		dolog(0, "Boruta core was requested by driver number %zu (connection %zu) to terminate connection!", client, connection);
		dolog(0, "But this driver in not a current driver for this connection, THIS IS VERY, VERY WRONG (BUGGY DRIVER?)");
		dolog(0, "Request ignored");
		return;
	}
	do_terminate_connection(connection);
	client += 1;
	if (client < m_connection_client_map.at(connection).size()) {
		if (do_establish_connection(connection))
			return;
		do_schedule(connection, client);
	}
}

void client_manager::connection_read_cb(size_t connection, struct bufferevent *bufev, int fd) { 
	if (m_current_client.at(connection) < m_connection_client_map.at(connection).size()) {
		client_driver *d = m_connection_client_map.at(connection).at(m_current_client.at(connection));
		d->data_ready(bufev, fd);
	} else {
		dolog(1, "Received data in client_manager when for this connection (no. %zu) no driver is active. Terminating connection", connection);
		do_terminate_connection(connection);
	}
}

void client_manager::connection_error_cb(size_t connection) {
	size_t &current_client = m_current_client.at(connection);
	if (m_current_client.at(connection) >= m_connection_client_map.at(connection).size()) {
		do_terminate_connection(connection);
		return;
	}
	client_driver *d = m_connection_client_map.at(connection).at(current_client);
	d->connection_error(do_get_connection_buf(connection));
	do_terminate_connection(connection);
	size_t clients_count = m_connection_client_map.at(connection).size();
	current_client += 1;
	if (current_client == clients_count)
		return;

	if (!do_establish_connection(connection)) {
		if (do_get_connection_state(connection) == CONNECTED)
			do_schedule(connection, current_client);	
	} else {
		     for (size_t i = current_client; i < m_connection_client_map.at(connection).size(); i++)
			m_connection_client_map.at(connection).at(i)->connection_error(do_get_connection_buf(connection));
	}
}

void client_manager::connection_established_cb(size_t connection) {
	size_t &current_client = m_current_client.at(connection);
	if (current_client == m_connection_client_map.at(connection).size())
		current_client = 0;
	do_schedule(connection, current_client);
}

tcp_client_manager::tcp_connection::tcp_connection(tcp_client_manager *_manager, size_t _addr_no, std::string _address) : state(NOT_CONNECTED), fd(-1), bufev(NULL), conn_no(_addr_no), manager(_manager), address(_address) {}

int tcp_client_manager::configure_tcp_address(xmlNodePtr node, struct sockaddr_in &addr) {
	std::string address;
	if (get_xml_extra_prop(node, "tcp-address", address))
		return 1;
	if (!inet_aton(address.c_str(), &addr.sin_addr)) {
		dolog(0, "Invalid tcp-address value(%s) in line %ld, exiting", address.c_str(), xmlGetLineNo(node));
		return 1;
	}
	int port;
	if (get_xml_extra_prop(node, "tcp-port", port))
		return 1;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	return 0;
}

void tcp_client_manager::close_connection(tcp_connection &c) {
	if (c.bufev) {
		bufferevent_free(c.bufev);
		c.bufev = NULL;
	}
	if (c.fd >= 0) {
		close(c.fd);
		c.fd = -1;
	}
	c.state = NOT_CONNECTED;
}

int tcp_client_manager::open_connection(tcp_connection &c, struct sockaddr_in& addr) {
	c.fd = socket(PF_INET, SOCK_STREAM, 0);
	assert(c.fd >= 0);
	if (set_nonblock(c.fd)) {
		dolog(1, "Failed to set non blocking mode on socket");
		close_connection(c);
		return 1;
	}
do_connect:
	int ret = connect(c.fd, (struct sockaddr*) &addr, sizeof(addr));
	if (ret == 0) {
		c.state = CONNECTED;
	} else if (errno == EINTR) {
		goto do_connect;
	} else if (errno == EWOULDBLOCK || errno == EINPROGRESS) {
		c.state = CONNECTING;
	} else {
		dolog(0, "Failed to connect: %s", strerror(errno));
		close_connection(c);
		return 1;
	}
	c.bufev = bufferevent_new(c.fd, connection_read_cb, connection_write_cb, connection_error_cb, &c);
	bufferevent_base_set(m_boruta->get_event_base(), c.bufev);
	bufferevent_enable(c.bufev, EV_READ | EV_WRITE | EV_PERSIST);
	return 0;
}

CONNECTION_STATE tcp_client_manager::do_get_connection_state(size_t conn_no) {
	return m_tcp_connections.at(conn_no).state;
}

struct bufferevent* tcp_client_manager::do_get_connection_buf(size_t conn_no) {
	return m_tcp_connections.at(conn_no).bufev;
}

void tcp_client_manager::do_terminate_connection(size_t conn_no) {
	tcp_connection &c = m_tcp_connections.at(conn_no);
	dolog(8, "tcp_client_manager::terminating connection: %s", c.address.c_str());
	close_connection(c);
}

int tcp_client_manager::do_establish_connection(size_t conn_no) {
	tcp_connection &c = m_tcp_connections.at(conn_no);
	dolog(8, "tcp_client_manager::connecting to address: %s", c.address.c_str());
	if (open_connection(c, m_addresses.at(conn_no)))
		return 1;
	if (c.state == CONNECTED)
		dolog(8, "tcp_client_manager::connection with address: %s established", c.address.c_str());
	return c.state == NOT_CONNECTED ? 1 : 0;
}

void tcp_client_manager::do_schedule(size_t conn_no, size_t client_no) {
	tcp_connection& c = m_tcp_connections.at(conn_no);
	dolog(8, "tcp_client_manager::scheduling client %zu for address %s", client_no, c.address.c_str());
	m_connection_client_map.at(conn_no).at(client_no)->scheduled(c.bufev, c.fd);
}

int tcp_client_manager::configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols& _protocols) {
	struct sockaddr_in addr;
	if (configure_tcp_address(node, addr))
		return 1;
	tcp_client_driver* driver = _protocols.create_tcp_client_driver(node);
	if (driver == NULL)
		return 1;
	driver->set_manager(this);
	driver->set_event_base(m_boruta->get_event_base());
	if (driver->configure(unit, node, read, send)) {
		delete driver;
		return 1;
	}
	size_t i;
	for (i = 0; i < m_addresses.size(); i++)
		if (addr == m_addresses[i])
			break;
	if (i == m_addresses.size()) {
		i = m_addresses.size();
		m_addresses.push_back(addr);
		m_connection_client_map.push_back(std::vector<client_driver*>());
		m_tcp_connections.push_back(tcp_connection(this, i, sock_addr_to_string(addr)));
	}
	driver->set_id(std::make_pair(i, m_connection_client_map[i].size()));
	m_connection_client_map[i].push_back(driver);
	return 0;
}

int tcp_client_manager::initialize() {
	m_current_client.resize(m_connection_client_map.size());
	for (size_t i = 0; i < m_connection_client_map.size(); i++)
		m_current_client.at(i) = m_connection_client_map.at(i).size();
	return 0;
}

void tcp_client_manager::connection_read_cb(struct bufferevent *ev, void* _tcp_connection) {
	tcp_connection* c = (tcp_connection*) _tcp_connection;
	tcp_client_manager* t = c->manager;
	t->client_manager::connection_read_cb(c->conn_no, c->bufev, c->fd);
}

void tcp_client_manager::connection_write_cb(struct bufferevent *ev, void* _tcp_connection) {
	tcp_connection* c = (tcp_connection*) _tcp_connection;
	if (c->state != CONNECTING)
		return;
	tcp_client_manager* t = c->manager;
	c->state = CONNECTED;
	dolog(8, "tcp_client_manager: connection with address: %s established", c->address.c_str());
	t->connection_established_cb(c->conn_no);
}
 
void tcp_client_manager::connection_error_cb(struct bufferevent *ev, short event, void* _tcp_connection) {
	tcp_connection* c = (tcp_connection*) _tcp_connection;
	tcp_client_manager* t = c->manager;
	dolog(8, "tcp_client_manager: connection with address: %s error", c->address.c_str());
	t->client_manager::connection_error_cb(c->conn_no);
}

CONNECTION_STATE serial_client_manager::do_get_connection_state(size_t conn_no) {
	return m_serial_connections.at(conn_no).fd >= 0 ? CONNECTED : NOT_CONNECTED;
}

struct bufferevent* serial_client_manager::do_get_connection_buf(size_t conn_no) {
	return m_serial_connections.at(conn_no).bufev;
}

void serial_client_manager::do_terminate_connection(size_t conn_no) {
	m_serial_connections.at(conn_no).close_connection();
}

int serial_client_manager::do_establish_connection(size_t conn_no) {
	const std::string path = m_configurations.at(conn_no).at(0).path;
	return m_serial_connections.at(conn_no).open_connection(path, m_boruta->get_event_base());
}

void serial_client_manager::do_schedule(size_t conn_no, size_t client_no) {
	serial_connection& sc = m_serial_connections.at(conn_no);
	set_serial_port_settings(sc.fd, m_configurations.at(conn_no).at(client_no));
	m_connection_client_map.at(conn_no).at(client_no)->scheduled(
			m_serial_connections.at(conn_no).bufev,
			m_serial_connections.at(conn_no).fd);
}

int serial_client_manager::configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols &_protocols) {
	serial_port_configuration spc;
	if (get_serial_port_config(node, spc))
		return 1;
	serial_client_driver* driver = _protocols.create_serial_client_driver(node);
	if (driver == NULL)
		return 1;
	driver->set_manager(this);
	driver->set_event_base(m_boruta->get_event_base());
	if (driver->configure(unit, node, read, send, spc)) {
		delete driver;
		return 1;
	}
	std::map<std::string, size_t>::iterator i = m_ports_client_no_map.find(spc.path);
	size_t j;	
	if (i == m_ports_client_no_map.end()) {
		j = m_connection_client_map.size();
		m_connection_client_map.push_back(std::vector<client_driver*>());
		m_configurations.resize(m_configurations.size() + 1);
		m_ports_client_no_map[spc.path] = j;
		m_serial_connections.push_back(serial_connection(j, this));
	} else {
		j = i->second;
	}
	m_connection_client_map.at(j).push_back(driver);
	m_configurations.at(j).push_back(spc);
	driver->set_id(std::make_pair(j, m_connection_client_map.at(j).size() - 1));
	return 0;
}

int serial_client_manager::initialize() {
	m_current_client.resize(m_connection_client_map.size());
	for (size_t i = 0; i < m_connection_client_map.size(); i++)
		m_current_client.at(i) = m_connection_client_map.at(i).size();
	return 0;
}

void serial_client_manager::connection_read_cb(serial_connection* c) {
	client_manager::connection_read_cb(c->conn_no, c->bufev, c->fd);
}

void serial_client_manager::connection_error_cb(serial_connection* c) {
	client_manager::connection_error_cb(c->conn_no);		
}

int serial_server_manager::configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols &_protocols) {
	serial_port_configuration spc;
	if (get_serial_port_config(node, spc))
		return 1;
	serial_server_driver* driver = _protocols.create_serial_server_driver(node);
	if (driver == NULL)
		return 1;
	driver->set_manager(this);
	driver->set_event_base(m_boruta->get_event_base());
	if (driver->configure(unit, node, read, send, spc)) {
		delete driver;
		return 1;
	}
	driver->id() = m_drivers.size();
	m_drivers.push_back(driver);
	m_connections.push_back(serial_connection(m_connections.size(), this));
	m_configurations.push_back(spc);
	return 0;
}

int serial_server_manager::initialize() {
	return 0;
}

void serial_server_manager::starting_new_cycle() {
	for (size_t i = 0; i < m_connections.size(); i++)
		if (m_connections[i].fd < 0) {
			if (m_connections[i].open_connection(m_configurations.at(i).path, m_boruta->get_event_base()))
				m_connections[i].close_connection();
			else
				set_serial_port_settings(m_connections[i].fd, m_configurations.at(i));
		}
	for (std::vector<serial_server_driver*>::iterator i = m_drivers.begin();
			i != m_drivers.end();
			i++)
		(*i)->starting_new_cycle();
}

void serial_server_manager::finished_cycle() {
	for (std::vector<serial_server_driver*>::iterator i = m_drivers.begin();
			i != m_drivers.end();
			i++)
		(*i)->finished_cycle();
}

void serial_server_manager::restart_connection_of_driver(serial_server_driver* driver) {
	std::vector<serial_server_driver*>::iterator i = std::find(m_drivers.begin(), m_drivers.end(), driver);		
	assert(i != m_drivers.end());
	m_connections.at(std::distance(i, m_drivers.end())).close_connection();
}

void serial_server_manager::connection_read_cb(serial_connection *c) {
	m_drivers.at(c->conn_no)->data_ready(c->bufev);
}

void serial_server_manager::connection_error_cb(serial_connection *c) {
	m_drivers.at(c->conn_no)->connection_error(c->bufev);
	m_connections.at(c->conn_no).close_connection();
}

tcp_server_manager::listen_port::listen_port(tcp_server_manager* _manager, int _port, int _serv_no) :
	manager(_manager), port(_port), fd(-1), serv_no(_serv_no) {}

int tcp_server_manager::start_listening_on_port(int port) {
	int ret, fd, on = 1;
	struct sockaddr_in addr;
	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		dolog(0, "socket() failed, errno %d (%s)", errno, strerror(errno));
		return -1;
	}
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 
			&on, sizeof(on));
	if (ret == -1) {
		dolog(0, "setsockopt() failed, errno %d (%s)",
				errno, strerror(errno));
		close(fd);
		return -1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	if (ret == 0) {
		return fd;
	} else {
		dolog(1, "bind() failed, errno %d (%s), will retry",
			errno, strerror(errno));
		close(fd);
		return -1;
	}
}

void tcp_server_manager::close_connection(struct bufferevent* bufev) {
	connection &c = m_connections[bufev];
	close(c.fd);
	m_connections.erase(bufev);
	bufferevent_free(bufev);
}

int tcp_server_manager::configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols &_protocols) {
	tcp_server_driver* driver = _protocols.create_tcp_server_driver(node);
	if (driver == NULL)		
		return 1;
	driver->set_manager(this);
	driver->set_event_base(m_boruta->get_event_base());
	if (driver->configure(unit, node, read, send)) {
		delete driver;
		return 1;
	}
	int port;
	if (!get_xml_extra_prop(node, "port", port))
		return 1;
	m_listen_ports.push_back(listen_port(this, port, m_drivers.size()));
	driver->id() = m_drivers.size();
	m_drivers.push_back(driver);
	return 0;
}

int tcp_server_manager::initialize() {
	return 0;
}

void tcp_server_manager::starting_new_cycle() {
	for (std::vector<listen_port>::iterator i = m_listen_ports.begin(); i != m_listen_ports.end(); i++) {
		if (i->fd >= 0)
			continue;
		i->fd = start_listening_on_port(i->port);
		if (i->fd < 0)
			continue;
		event_set(&i->_event, i->fd, EV_READ | EV_PERSIST, connection_accepted_cb, &(*i));
		event_add(&i->_event, NULL);
		event_base_set(m_boruta->get_event_base(), &i->_event);
	}
}

void tcp_server_manager::finished_cycle() {
	for (std::vector<tcp_server_driver*>::iterator i = m_drivers.begin();
			i != m_drivers.end();
			i++)
		(*i)->finished_cycle();
}

void tcp_server_manager::connection_read_cb(struct bufferevent *bufev, void* _manager) {
	tcp_server_manager *m = (tcp_server_manager*) _manager;
	m->m_drivers.at(m->m_connections[bufev].serv_no)->data_ready(bufev);
}

void tcp_server_manager::connection_error_cb(struct bufferevent *bufev, short event, void* _manager) {
	tcp_server_manager *m = (tcp_server_manager*) _manager;
	connection &c = m->m_connections[bufev];
	m->m_drivers.at(c.serv_no)->connection_error(bufev);
	m->close_connection(bufev);
}

void tcp_server_manager::connection_accepted_cb(int _fd, short event, void* _listen_port) {
	struct sockaddr_in addr;
	socklen_t size = sizeof(addr);
	listen_port* p = (listen_port*) _listen_port;
	tcp_server_manager *m = p->manager;
do_accept:
	int fd = accept(_fd, (struct sockaddr*) &addr, &size);
	if (fd == -1) {
		if (errno == EINTR)
			goto do_accept;
		else if (errno == EAGAIN || errno == EWOULDBLOCK) 
			return;
		else if (errno == ECONNABORTED)
			return;
		else {
			dolog(0, "Accept error(%s), terminating application", strerror(errno));
			exit(1);
		}
	}
	dolog(5, "Connection from: %s", inet_ntoa(addr.sin_addr));
	connection c;
	c.serv_no = p->serv_no;
	c.fd = fd;
	c.bufev = bufferevent_new(fd,
			connection_read_cb, 
			NULL, 
			connection_error_cb,
			m);
	bufferevent_base_set(m->m_boruta->get_event_base(), c.bufev);
	bufferevent_enable(c.bufev, EV_READ | EV_WRITE | EV_PERSIST);
	m->m_connections[c.bufev] = c;
	if (m->m_drivers.at(p->serv_no)->connection_accepted(c.bufev, fd, &addr))
		m->close_connection(c.bufev);
}

int boruta_daemon::configure_ipc() {
	m_ipc = new IPCHandler(m_cfg);
	if (!m_cfg->GetSingle()) {
		if (m_ipc->Init())
			return 1;
		dolog(10, "IPC initialized successfully");
	} else {
		dolog(10, "Single mode, ipc not intialized!!!");
	}
	return 0;
}

int boruta_daemon::configure_events() {
	m_event_base = (struct event_base*) event_init();
	evtimer_set(&m_timer, cycle_timer_callback, this);
	event_base_set(m_event_base, &m_timer);
	return 0;
}

int boruta_daemon::configure_units() {
	int i, ret;
	TUnit* u;
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(m_cfg->GetXMLDoc());
	xp_ctx->node = m_cfg->GetXMLDevice();
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
		SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "boruta",
		BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert(ret == 0);
	short *read = m_ipc->m_read;
	short *send = m_ipc->m_send;
	protocols _protocols;
	for (i = 0, u = m_cfg->GetDevice()->GetFirstRadio()->GetFirstUnit();
			u;
			++i, u = u->GetNext()) {
		std::stringstream ss;
		ss << "./ipk:unit[position()=" << i + 1 << "]";
		xmlNodePtr node = uxmlXPathGetNode((const xmlChar*) ss.str().c_str(), xp_ctx);
		std::string mode;
		if (get_xml_extra_prop(node, "mode", mode))
			return 1;
		bool server;
		if (mode == "server")
			server = true;	
		else if (mode == "client")
			server = false;	
                else {
			dolog(0, "Unknown unit mode: %s, failed to configure daemon", mode.c_str());
			return 1;
		}
		std::string medium;
		if (get_xml_extra_prop(node, "medium", medium))
			return 1;
		if (medium == "tcp") {
			if (server)
				ret = m_tcp_server_mgr.configure(u, node, read, send, _protocols);
			else
				ret = m_tcp_client_mgr.configure(u, node, read, send, _protocols);
			if (ret)
				return 1;
		} else if (medium == "serial") {
			if (server)
				ret = m_serial_server_mgr.configure(u, node, read, send, _protocols);
			else
				ret = m_serial_client_mgr.configure(u, node, read, send, _protocols);
			if (ret)
				return 1;
		} else {
			dolog(0, "Unknown connection type: %s, failed to configure daemon", medium.c_str());
			return 1;
		}
		read += u->GetParamsCount();
		send += u->GetSendParamsCount();
	}
	xmlXPathFreeContext(xp_ctx);
	return 0;
}


boruta_daemon::boruta_daemon() : m_tcp_client_mgr(this), m_tcp_server_mgr(this), m_serial_client_mgr(this), m_serial_server_mgr(this) {}

struct event_base* boruta_daemon::get_event_base() {
	return m_event_base;
}

int boruta_daemon::configure(int *argc, char *argv[]) {
	if (configure_events())
		return 1;

	m_cfg = new DaemonConfig("borutadmn");
	if (m_cfg->Load(argc, argv, 0, NULL, 0, m_event_base))
		return 1;
	g_debug = m_cfg->GetDiagno() || m_cfg->GetSingle();

	if (configure_ipc())
		return 1;
	if (configure_units())
		return 1;
	return 0;
}

void boruta_daemon::go() {
	if (!m_cfg->GetSingle()) for (int i = 0; i < m_ipc->m_params_count; i++)
		m_ipc->m_read[i] = SZARP_NO_DATA;

	if (m_tcp_server_mgr.initialize())
		return;
	if (m_serial_server_mgr.initialize())
		return;
	if (m_tcp_client_mgr.initialize())
		return;
	if (m_serial_client_mgr.initialize())
		return;
	cycle_timer_callback(-1, 0, this);
	event_base_dispatch(m_event_base);
}

void boruta_daemon::cycle_timer_callback(int fd, short event, void* daemon) {
	boruta_daemon* b = (boruta_daemon*) daemon;
	b->m_tcp_client_mgr.finished_cycle();
	b->m_serial_client_mgr.finished_cycle();
	b->m_tcp_server_mgr.finished_cycle();
	b->m_serial_server_mgr.finished_cycle();
	b->m_ipc->GoParcook();
	b->m_ipc->GoSender();
	b->m_tcp_client_mgr.starting_new_cycle();
	b->m_tcp_server_mgr.starting_new_cycle();
	b->m_serial_client_mgr.starting_new_cycle();
	b->m_serial_server_mgr.starting_new_cycle();
	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	evtimer_add(&b->m_timer, &tv); 
}

int main(int argc, char *argv[]) {
	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);
	boruta_daemon daemon;
	if (daemon.configure(&argc, argv))
		return 1;
	signal(SIGPIPE, SIG_IGN);
	dolog(2, "Starting Boruta Daemon");
	daemon.go();
	dolog(0, "Error: daemon's event loop exited - that shouldn't happen!");
	return 1;
}

void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	if (g_debug) {
		char *l;
		va_start(fmt_args, fmt);
		if (vasprintf(&l, fmt, fmt_args) != -1) {
			std::cout << l << std::endl;
			sz_log(level, "%s", l);
			free(l);
		} else {
			std::cout << "Logging error " << strerror(errno) << std::endl;
		}
		va_end(fmt_args);

	} else {
		va_start(fmt_args, fmt);
		vsz_log(level, fmt, fmt_args);
		va_end(fmt_args);
	}

} 

namespace ascii {

int char2value(unsigned char c, unsigned char &o) {
	if (c >= '0' && c <= '9')
		o |= (c - '0');
	else if (c >= 'A' && c <= 'F')
		o |= (c - 'A' + 10);
	else
		return 1;
	return 0;
}

int from_ascii(unsigned char c1, unsigned char c2, unsigned char &c) {
	c = 0;
	if (char2value(c1, c))
		return 1;
	c <<= 4;
	if (char2value(c2, c))
		return 1;
	return 0;
}

unsigned char value2char(unsigned char c) {
	if (c <= 9)
		return '0' + c;
	else
		return 'A' + c - 10;
}

void to_ascii(unsigned char c, unsigned char& c1, unsigned char &c2) {
	c1 = value2char(c >> 4);
	c2 = value2char(c & 0xf);
}

}
