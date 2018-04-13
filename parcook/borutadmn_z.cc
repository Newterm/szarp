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

#include <iostream>

#include <libxml/parser.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "conversion.h"
#include "liblog.h"
#include "libpar.h"
#include "xmlutils.h"
#include "tokens.h"
#include "borutadmn_z.h"

#include "serialport.h"
#include "serialportconf.h"

bool g_debug = false;

static const time_t RECONNECT_ATTEMPT_DELAY = 10;

/*implementation*/

template <typename T>
using bopt = boost::optional<T>;

bopt<SerialPortConfiguration> get_serial_conf(TUnit* unit) {
	SerialPortConfiguration conf;
	
	if (auto path = unit->getOptAttribute<std::string>("extra:path")) {
		conf.path = *path;
	}

	int speed = unit->getAttribute("extra:speed", 9600);
	conf.speed = speed;

	std::string parity = unit->getAttribute<std::string>("extra:parity", "");
	if (parity.empty()) {
		dolog(10, "Serial port configuration, parity not specified, assuming no parity");
		conf.parity = SerialPortConfiguration::NONE;
	} else if (parity == "none") {
		dolog(10, "Serial port configuration, none parity");
		conf.parity = SerialPortConfiguration::NONE;
	} else if (parity == "even") {
		dolog(10, "Serial port configuration, even parity");
		conf.parity = SerialPortConfiguration::EVEN;
	} else if (parity == "odd") {
		dolog(10, "Serial port configuration, odd parity");
		conf.parity = SerialPortConfiguration::ODD;
	} else {
		dolog(1, "Unsupported parity %s, confiugration invalid!!!", parity.c_str());
		return boost::none;	
	}

	int stop_bits = unit->getAttribute<int>("extra:stopbits", 1);
	if (stop_bits != 1 && stop_bits != 2) {
		dolog(1, "Unsupported number of stop bits %i, confiugration invalid!!!", stop_bits);
		return boost::none;
	}

	conf.stop_bits = stop_bits;

	int char_size = unit->getAttribute<int>("extra:char_size", 8);
	switch (char_size) {
	case 5:
		conf.char_size = SerialPortConfiguration::CS_5;
		break;
	case 6:
		conf.char_size = SerialPortConfiguration::CS_6;
		break;
	case 7:
		conf.char_size = SerialPortConfiguration::CS_7;
		break;
	case 8:
		conf.char_size = SerialPortConfiguration::CS_8;
		break;
	default:
		dolog(1, "Unsupported char size %i, confiugration invalid!!!", char_size);
		return boost::none;
	}

	dolog(6, "Configured %i char size", conf.GetCharSizeInt());

	return conf;
}

TcpServerConnection::~TcpServerConnection() {
	bufferevent_free(m_bufferevent);
	m_bufferevent = NULL;

	close(m_fd);
	m_fd = -1;
}

void TcpServerConnection::InitConnection(int fd) {
	m_fd = fd;
}

void TcpServerConnection::Open() {
	m_bufferevent = bufferevent_socket_new(m_event_base, m_fd,
			BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
	if (m_bufferevent == NULL)
		throw TcpConnectionException("Couldn't create bufferevent");

	bufferevent_setcb(m_bufferevent, ReadDataCallback, 
			NULL, ErrorCallback, this);

	bufferevent_enable(m_bufferevent, EV_READ | EV_WRITE | EV_PERSIST);

	OpenFinished();
}

void TcpServerConnection::Close() {
	handler->CloseConnection(this);
}

bool TcpServerConnection::Ready() const { return m_fd >= 0 && m_bufferevent != NULL; }

void TcpServerConnection::SetConfiguration(const SerialPortConfiguration& serial_conf) {
	SetConfigurationFinished();
}

void TcpServerConnection::ReadData(struct bufferevent *bufev)
{
	std::vector<unsigned char> data;
	unsigned char c;
	while (bufferevent_read(bufev, &c, 1) == 1) {
		data.push_back(c);
	}

	BaseConnection::ReadData(data);
}

void TcpServerConnection::WriteData(const void* data, size_t size) {
	if (m_bufferevent == NULL) {
		throw TcpConnectionException("Connection is currently unavailable");
	}
	int ret = bufferevent_write(m_bufferevent, data, size);
	if (ret < 0) {
		throw TcpConnectionException("Write data failed.");
	}
}


bool TcpServerConnectionHandler::ip_is_allowed(struct sockaddr_in in_s_addr) {
	return m_allowed_ips.size() == 0 || m_allowed_ips.count(in_s_addr.sin_addr.s_addr) != 0;
}

void TcpServerConnectionHandler::ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) {
	for (auto* listener : m_listeners) {
		listener->ReadData(conn, data);
	}
}

void TcpServerConnectionHandler::ReadError(const BaseConnection *conn, short int event) {
	CloseConnection(conn);
}

void TcpServerConnectionHandler::CloseConnection(const BaseConnection* conn) {
	auto _conn = const_cast<BaseConnection*>(conn);
	auto it = std::find_if(m_connections.begin(), m_connections.end(), [_conn](const std::unique_ptr<BaseConnection>& other){ return other.get() == _conn; });

	if (it != m_connections.end())
		m_connections.erase(it);
}

void TcpServerConnectionHandler::Init(TUnit* unit) {
	int port;
	if (auto port_opt = unit->getOptAttribute<int>("extra:tcp-port"))
		port = *port_opt;
	else
		throw TcpConnectionException("No port given in unit configuration!");

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);
	m_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	ConfigureAllowedIps(unit);
}

void TcpServerConnectionHandler::ConfigureAllowedIps(TUnit* unit) {
	std::string ips_allowed = unit->getAttribute<std::string>("extra:tcp-allowed", "");;
	std::stringstream ss(ips_allowed);
	std::string ip_allowed;
	while (ss >> ip_allowed) {
		struct in_addr ip;
		if (inet_aton(ip_allowed.c_str(), &ip)) {
			// m_log.log(1, "incorrect IP address '%s'", ip_allowed.c_str());
			m_allowed_ips.insert(ip.s_addr);
		} else {
			// m_log.log(5, "IP address '%s' allowed", ip_allowed.c_str());
			continue;
		}
	}
}

void TcpServerConnectionHandler::Open() {
	if (m_fd >= 0)
		return;
	m_fd = start_listening();
	if (m_fd < 0)
		return;
	event_set(&_event, m_fd, EV_READ | EV_PERSIST, connection_accepted_cb, this);
	event_add(&_event, NULL);
	event_base_set(m_event_base, &_event);
}

void TcpServerConnectionHandler::Close() {
	close(m_fd);
	m_fd = -1;

	for (auto& conn: m_connections)
		conn->Close();
}

bool TcpServerConnectionHandler::Ready() const {
	return m_fd != -1;
}

boost::optional<int> TcpServerConnectionHandler::accept_socket(int in_fd) {
	struct sockaddr_in addr;
	socklen_t size = sizeof(addr);

	int fd = -1;
	do {
		fd = accept(in_fd, (struct sockaddr*) &addr, &size);
		if (fd != -1) break;
		if (errno == EINTR)
			continue;
		else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ECONNABORTED) 
			return boost::none;
		else
			return boost::none;
	} while (true);

	if (!ip_is_allowed(addr)) {
		close(fd);
		return boost::none;
	}

	return fd;
}


int TcpServerConnectionHandler::start_listening() {
	m_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (m_fd == -1) {
		return -1;
	}

	int on = 1;
	if(-1 == setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		Close();
		return -1;
	}

	if (-1 == bind(m_fd, (struct sockaddr *)&m_addr, sizeof(m_addr))) {
		Close();
		return -1;
	}

	if (-1 == listen(m_fd, 1)) {
		Close();
		return -1;
	}

	return m_fd;
}

void TcpServerConnectionHandler::AcceptConnection(int _fd) {
	std::unique_ptr<TcpServerConnection> conn(new TcpServerConnection(this, m_event_base));

	if (auto s = accept_socket(_fd)) {
		conn->InitConnection(*s);
		conn->AddListener(this);

		try {
			conn->Open();
			m_connections.push_back(std::move(conn));
		} catch(const TcpConnectionException& e) {
			// log?
		}
	}
}

int bc_manager::configure(TUnit *unit, size_t read, size_t send) {
	auto evbase = m_boruta->get_event_base();

	auto mode = conn_factory::get_mode(unit);
	auto medium = conn_factory::get_medium(unit);
	SerialPortConfiguration conf;
	if (auto opt_conf = get_serial_conf(unit)) {
		conf = *opt_conf;
	} else {
		return 1;
	}

	driver_iface* driver;

	std::string log_header_str = "";

	auto proto = unit->getAttribute<std::string>("extra:proto", "modbus");
	if (proto == "modbus")
		log_header_str += "modbus ";
	else if (proto == "fc")
		log_header_str += "fc ";

	// configure logger
	if (mode == conn_factory::mode::server)
		log_header_str += "server ";
	else
		log_header_str += "client ";

	if (medium == conn_factory::medium::tcp) {
		log_header_str += "TCP at ";
		log_header_str += unit->getAttribute<std::string>("extra:tcp-address", "");
		log_header_str += ":";
		log_header_str += unit->getAttribute<std::string>("extra:tcp-port", "");
	} else {
		log_header_str += "serial at ";
		log_header_str += unit->getAttribute<std::string>("extra:path", "");
	}

	slog logger = std::make_shared<boruta_logger>(log_header_str);
	if (proto == "modbus") {
		if (medium == conn_factory::medium::tcp) {
			if (mode == conn_factory::mode::server) {
				auto conn = new TcpServerConnectionHandler(evbase);
				conn->Init(unit);
				driver = create_bc_modbus_tcp_server(conn, m_boruta, logger);
			} else if (mode == conn_factory::mode::client) {
				auto conn = new TcpConnection(evbase);
				conn->Init(unit);

				if (unit->getAttribute<bool>("extra:use_tcp_2_serial_proxy", false))
					driver = create_bc_modbus_serial_client(conn, m_boruta, logger);
				else
					driver = create_bc_modbus_tcp_client(conn, m_boruta, logger);
			}
		} else if (medium == conn_factory::medium::serial) {
			auto conn = new SerialPort(evbase);
			conn->Init(conf.path);

			if (mode == conn_factory::mode::client) {
				driver = create_bc_modbus_serial_client(conn, m_boruta, logger);
			} else if (mode == conn_factory::mode::server) {
				driver = create_bc_modbus_serial_server(conn, m_boruta, logger);
			}

			conn->Open();
			conn->SetConfiguration(conf);
		}
	} else if (proto == "fc") {
		auto conn = new SerialPort(evbase);
		conn->Init(conf.path);
		conn->Open();
		conn->SetConfiguration(conf);

		driver = create_fc_serial_client(conn, m_boruta, logger);
	}

	if (driver->configure(unit, read, send, conf)) {
		delete driver;
		return 1;
	}

	conns.insert(driver);
	return 0;
}

int boruta_daemon::configure_ipc() {
	char* sub_conn_address = libpar_getpar("parhub", "sub_conn_addr", 1);
	char* pub_conn_address = libpar_getpar("parhub", "pub_conn_addr", 1);

	try {
		m_zmq = new zmqhandler(m_cfg, m_zmq_ctx, pub_conn_address, sub_conn_address);
		dolog(10, "ZMQ initialized successfully");
	} catch (zmq::error_t& e) {
		dolog(1, "ZMQ initialization failed, %s", e.what());
		return 1;
	}

	free(sub_conn_address);
	free(pub_conn_address);

	int sock = m_zmq->subsocket();
	if (sock >= 0) {
		event_base_set(m_event_base, &m_subsock_event);

		event_set(&m_subsock_event, sock, EV_READ | EV_PERSIST, subscribe_callback, this);
		event_add(&m_subsock_event, NULL);

		subscribe_callback(sock, EV_READ, this);
	}

	return 0;
}

int boruta_daemon::configure_events() {
	m_event_base = (struct event_base*) event_init();
	if (!m_event_base)
		return 1;
	m_evdns_base = evdns_base_new(m_event_base, 0);
	if (!m_evdns_base)
		return 2;
	evdns_base_resolv_conf_parse(m_evdns_base, DNS_OPTIONS_ALL, "/etc/resolv.conf");
	evtimer_set(&m_timer, cycle_timer_callback, this);
	event_base_set(m_event_base, &m_timer);
	return 0;
}

int boruta_daemon::configure_units() {
	int i;
	TUnit* u;
	size_t read = 0;
	size_t send = 0;
	for (i = 0, u = m_cfg->GetDevice()->GetFirstUnit(); u; ++i, u = u->GetNext()) {
		if (m_drivers_manager.configure(u, read, send))
			return 1;
		read += u->GetParamsCount();
		send += u->GetSendParamsCount(true);
	}
	return 0;
}

int boruta_daemon::configure_cycle_freq() {
	int duration = m_cfg->GetDevice()->getAttribute<int>("extra:cycle_duration", 10000);
	m_cycle.tv_sec = duration / 1000;
	m_cycle.tv_usec = (duration % 1000) * 1000;

	return 0;
}

boruta_daemon::boruta_daemon() : m_drivers_manager(this), m_zmq_ctx(1) {}

struct event_base* boruta_daemon::get_event_base() {
	return m_event_base;
}

struct evdns_base* boruta_daemon::get_evdns_base() {
	return m_evdns_base;
}

zmqhandler* boruta_daemon::get_zmq() {
	return m_zmq;
}

int boruta_daemon::configure(int *argc, char *argv[]) {
	if (int ret = configure_events())
		return ret;

	m_cfg = new DaemonConfig("borutadmn");
	if (m_cfg->Load(argc, argv))
		return 101;
	g_debug = m_cfg->GetDiagno() || m_cfg->GetSingle();

	if (configure_ipc())
		return 102;
	if (configure_units())
		return 103;
	if (configure_cycle_freq())
		return 104;
	return 0;
}

void boruta_daemon::go() {
	cycle_timer_callback(-1, 0, this);
	event_base_dispatch(m_event_base);
}

void boruta_daemon::cycle_timer_callback(int fd, short event, void* daemon) {
	boruta_daemon* b = (boruta_daemon*) daemon;

	b->m_drivers_manager.finished_cycle();

	b->m_zmq->publish();

	b->m_drivers_manager.starting_new_cycle();

	evtimer_add(&b->m_timer, &b->m_cycle); 
}

void boruta_daemon::subscribe_callback(int fd, short event, void* daemon) {
	boruta_daemon* b = (boruta_daemon*)daemon; 
	b->m_zmq->receive();
}

int main(int argc, char *argv[]) {
	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);
	boruta_daemon daemon;
	if (int ret = daemon.configure(&argc, argv)) {
		sz_log(0, "Error configuring daemon, exiting.");
		return ret;
	}

	signal(SIGPIPE, SIG_IGN);
	dolog(2, "Starting Boruta Daemon");
	daemon.go();
	return 200;
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


void boruta_logger::log(int level, const char * fmt, ...) {
	va_list fmt_args;
	va_start(fmt_args, fmt);
	vlog(level, fmt, fmt_args);
	va_end(fmt_args);
}

void boruta_logger::vlog(int level, const char * fmt, va_list fmt_args) {
	char *l;
	if (vasprintf(&l, fmt, fmt_args) != -1) {
		::dolog(level, "%s: %s", header.c_str(), l);
		free(l);
	} else {
		::dolog(2, "%s: Error in formatting log message", header.c_str());
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

unsigned char value2char(unsigned char c) {
	if (c <= 9)
		return '0' + c;
	else
		return 'A' + c - 10;
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

void to_ascii(unsigned char c, unsigned char& c1, unsigned char &c2) {
	c1 = value2char(c >> 4);
	c2 = value2char(c & 0xf);
}

}
