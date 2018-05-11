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

#include "tcpconn.h"
#include "tcpserverconn.h"
#include "serialport.h"
#include "serialportconf.h"

#include "cfgdealer_handler.h"

struct event_base* EventBaseHolder::evbase;

static const time_t RECONNECT_ATTEMPT_DELAY = 10;

/*implementation*/

namespace szarp {

template class time_spec<util::_ms>;
template class time_spec<util::_us>;

template <typename ts>
time_spec<ts> operator+(const time_spec<ts>& ms1, const time_spec<ts>& ms2) {
	time_spec<ts> m = ms1;
	m += ms2;
	return m;
}

template <typename ts>
time_spec<ts> operator-(const time_spec<ts>& ms1, const time_spec<ts>& ms2) {
	time_spec<ts> m = ms1;
	m -= ms2;
	return m;
}

template time_spec<util::_ms> operator-(const time_spec<util::_ms>&, const time_spec<util::_ms>&);
template time_spec<util::_us> operator-(const time_spec<util::_us>&, const time_spec<util::_us>&);

sz4::second_time_t time_now() {
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return sz4::second_time_t(time.tv_sec);
}

template <>
sz4::nanosecond_time_t time_now() {
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return sz4::nanosecond_time_t(time.tv_sec, time.tv_nsec);
}

} // ns szarp

Scheduler::Scheduler() {
	evtimer_set(&m_timer, timer_callback, this);
	event_base_set(EventBaseHolder::evbase, &m_timer);
}

void Scheduler::set_callback(Callback* _callback) {
	callback = _callback;
}

void Scheduler::schedule() {
	evtimer_add(&m_timer, &tv);
}

void Scheduler::reschedule() {
	cancel();
	schedule();
}

void Scheduler::cancel() {
	event_del(&m_timer);
}

void Scheduler::timer_callback(int fd, short event, void *obj) {
	try {
		auto *sched = (Scheduler*) obj;
		(*sched->callback)();
	} catch(const std::exception& e) {
		sz_log(2, "Exception caught in timer callback: %s", e.what());
	}
}

CycleScheduler::CycleScheduler(cycle_callback_handler* handler): scheduler() {
	scheduler.set_callback(this);
	cycle_handler = handler;
}

void CycleScheduler::configure(UnitInfo* unit) {
	auto cycle_timeout = unit->getAttribute<int>("ms_cycle_period", 10000);
	scheduler.set_timeout(szarp::ms(cycle_timeout));
	scheduler.schedule();
}

void CycleScheduler::operator()() {
	std::lock_guard<std::mutex> cycle_guard(cycle_mutex);
	scheduler.schedule();
	cycle_handler->start_cycle();
}


template <typename T>
using bopt = boost::optional<T>;

bopt<SerialPortConfiguration> get_serial_conf(UnitInfo* unit) {
	SerialPortConfiguration conf;
	
	if (auto path = unit->getOptAttribute<std::string>("extra:path")) {
		conf.path = *path;
	}

	int speed = unit->getAttribute("extra:speed", 9600);
	conf.speed = speed;

	std::string parity = unit->getAttribute<std::string>("extra:parity", "");
	if (parity.empty()) {
		sz_log(10, "Serial port configuration, parity not specified, assuming no parity");
		conf.parity = SerialPortConfiguration::NONE;
	} else if (parity == "none") {
		sz_log(10, "Serial port configuration, none parity");
		conf.parity = SerialPortConfiguration::NONE;
	} else if (parity == "even") {
		sz_log(10, "Serial port configuration, even parity");
		conf.parity = SerialPortConfiguration::EVEN;
	} else if (parity == "odd") {
		sz_log(10, "Serial port configuration, odd parity");
		conf.parity = SerialPortConfiguration::ODD;
	} else {
		sz_log(1, "Unsupported parity %s, confiugration invalid!!!", parity.c_str());
		return boost::none;	
	}

	int stop_bits = unit->getAttribute<int>("extra:stopbits", 1);
	if (stop_bits != 1 && stop_bits != 2) {
		sz_log(1, "Unsupported number of stop bits %i, confiugration invalid!!!", stop_bits);
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
		sz_log(1, "Unsupported char size %i, confiugration invalid!!!", char_size);
		return boost::none;
	}

	sz_log(6, "Configured %i char size", conf.GetCharSizeInt());

	return conf;
}


enum class Proto {modbus, fc};
enum class Mode {client, server};
enum class Medium {tcp, serial};

struct driver_factory {
	std::map<Proto, std::string> proto_names = {
		{Proto::modbus, "modbus"},
		{Proto::fc, "fc"}
	};

	std::map<Mode, std::string> mode_names = {
		{Mode::client, "client"},
		{Mode::server, "server"}
	};

	std::map<Medium, std::string> medium_names = {
		{Medium::serial, "serial"},
		{Medium::tcp, "tcp"}
	};

	Proto get_proto(UnitInfo* unit) {
		std::string proto_opt = unit->getAttribute<std::string>("extra:proto", "modbus");
		if (proto_opt == "modbus") {
			return Proto::modbus;
		} else if (proto_opt == "fc") {
			return Proto::fc;
		} else {
			throw std::runtime_error("Invalid proto option (can be modbus or fc)");
		}
	}

	Mode get_mode(UnitInfo* unit) {
		std::string mode_opt = unit->getAttribute<std::string>("extra:mode", "client");
		if (mode_opt == "client") {
			return Mode::client;
		} else if (mode_opt == "server") {
			return Mode::server;
		} else {
			throw std::runtime_error("Invalid mode option (can be client or server)");
		}
	}

	Medium get_medium(UnitInfo* unit) {
		std::string medium_opt = unit->getAttribute<std::string>("extra:medium", "tcp");
		if (medium_opt == "tcp") {
			return Medium::tcp;
		} else if (medium_opt == "serial") {
			return Medium::serial;
		} else {
			throw std::runtime_error("Invalid medium option (can be tcp or serial)");
		}
	}

	std::map<std::tuple<Mode, Medium>, std::function<BaseConnection*(UnitInfo*)>> conn_map = {
		{{Mode::client, Medium::serial}, [](UnitInfo* unit){ return BaseConnFactory::create_from_unit<SerialPort>(EventBaseHolder::evbase, unit); } },
		{{Mode::client, Medium::tcp}, [](UnitInfo* unit){ return BaseConnFactory::create_from_unit<TcpConnection>(EventBaseHolder::evbase, unit); } },
		{{Mode::server, Medium::serial}, [](UnitInfo* unit){ return BaseConnFactory::create_from_unit<SerialPort>(EventBaseHolder::evbase, unit); } },
		{{Mode::server, Medium::tcp}, [](UnitInfo* unit){ return BaseConnFactory::create_from_unit<TcpServerConnectionHandler>(unit); } },
	};

	std::map<Medium, std::function<std::string(UnitInfo*)>> addr_str_map = {
		{Medium::tcp, [](UnitInfo* unit){
			return unit->getAttribute<std::string>("extra:tcp-address", "0.0.0.0")
				   + std::string(":") + unit->getAttribute<std::string>("extra:tcp-port", "0"); }},
		{Medium::serial, [](UnitInfo* unit){ return unit->getAttribute<std::string>("extra:path", "/dev/null"); }},
	};


	std::map<std::tuple<Proto, Mode, Medium>, std::function<driver_iface*(UnitInfo*, BaseConnection*, boruta_daemon*, slog)>> driver_map = {
		{{Proto::fc, Mode::client, Medium::serial}, [](UnitInfo*, BaseConnection* conn, boruta_daemon* boruta, slog logger){
			return create_fc_serial_client(conn, boruta, logger); }},
		{{Proto::fc, Mode::client, Medium::tcp}, [](UnitInfo*, BaseConnection* conn, boruta_daemon* boruta, slog logger){
			return create_fc_serial_client(conn, boruta, logger); }},
		{{Proto::modbus, Mode::server, Medium::serial}, [](UnitInfo*, BaseConnection* conn, boruta_daemon* boruta, slog logger){
			return create_bc_modbus_driver(conn, boruta, logger); }},
		{{Proto::modbus, Mode::server, Medium::tcp}, [](UnitInfo*, BaseConnection* conn, boruta_daemon* boruta, slog logger){
			return create_bc_modbus_driver(conn, boruta, logger); }},
		{{Proto::modbus, Mode::client, Medium::serial}, [](UnitInfo*, BaseConnection* conn, boruta_daemon* boruta, slog logger){
			return create_bc_modbus_driver(conn, boruta, logger); }},
		{{Proto::modbus, Mode::client, Medium::tcp}, [](UnitInfo* unit, BaseConnection* conn, boruta_daemon* boruta, slog logger){
			if (unit->getAttribute<bool>("extra:use_tcp_2_serial_proxy", false))
				return create_bc_modbus_driver(conn, boruta, logger);
			else
				return create_bc_modbus_tcp_client(conn, boruta, logger);
		}}
	};

	driver_iface* create(UnitInfo* unit, boruta_daemon* boruta, size_t read, size_t send) {
		auto proto = get_proto(unit);
		auto mode = get_mode(unit);
		auto medium = get_medium(unit);

		auto conn_f = conn_map.find({mode, medium});
		auto addr_str_f = addr_str_map.find({medium});
		auto driver_f = driver_map.find({proto, mode, medium});

		if (conn_f == conn_map.end())
			throw std::runtime_error("Unknown connection type");
		if (addr_str_f == addr_str_map.end())
			throw std::runtime_error("Unknown connection type");
		if (driver_f == driver_map.end())
			throw std::runtime_error("Unknown connection type");

		std::string log_header = proto_names[proto];
		log_header += " ";
		log_header += medium_names[medium];
		log_header += " ";
		log_header += mode_names[mode];
		log_header += " at ";
		log_header += addr_str_f->second(unit);
		slog logger = std::make_shared<boruta_logger>(log_header);

		BaseConnection* conn = conn_f->second(unit);
		conn->Open();

		auto driver = driver_f->second(unit, conn, boruta, logger);

		SerialPortConfiguration conf;
		if (auto conf_opt = get_serial_conf(unit))
			conf = *conf_opt;
		else
			throw std::runtime_error("Unable to get serial port configuration!");

		conn->SetConfiguration(conf);

		if (driver->configure(unit, read, send, conf)) {
			throw std::runtime_error("Unable to configure driver!");
		}

		return driver;
	}
} factory; // Global! Do not store any state!

int boruta_daemon::configure_ipc(const ArgsManager& args_mgr, DaemonConfigInfo& cfg) {
	bopt<char*> sub_conn_address = args_mgr.get<char*>("parhub", "sub_conn_addr");
	bopt<char*> pub_conn_address = args_mgr.get<char*>("parhub", "pub_conn_addr");

	if (!sub_conn_address || !pub_conn_address) {
		sz_log(0, "Could not get parhub's address from configuration, exiting!");
		return 1;
	}

	try {
		m_zmq = new zmqhandler(&cfg, m_zmq_ctx, *pub_conn_address, *sub_conn_address);
		sz_log(10, "ZMQ initialized successfully");
	} catch (zmq::error_t& e) {
		sz_log(0, "ZMQ initialization failed, %s", e.what());
		return 1;
	}

	free(*sub_conn_address);
	free(*pub_conn_address);

	return 0;
}

int boruta_daemon::configure_units(const ArgsManager&, DaemonConfigInfo& cfg) {
	size_t read = 0;
	size_t send = 0;
	for (auto unit: cfg.GetUnits()) {
		auto driver = factory.create(unit, this, read, send);
		if (!driver)
			return 1;

		conns.insert(std::unique_ptr<driver_iface>(driver));
		read += unit->GetParamsCount();
		send += unit->GetSendParamsCount(true);
	}
	return 0;
}

void boruta_daemon::configure_timer(const ArgsManager&, DaemonConfigInfo& cfg) {
	m_timer.set_callback(new FnPtrScheduler([this](){ cycle_timer_callback(); }));
	int duration = cfg.GetDeviceInfo()->getAttribute<int>("extra:cycle_duration", 10000);
	m_timer.set_timeout(szarp::ms(duration));
}

boruta_daemon::boruta_daemon(): m_zmq_ctx(1) {}

non_owning_ptr<zmqhandler> boruta_daemon::get_zmq() {
	return m_zmq;
}

int boruta_daemon::configure(int *argc, char *argv[]) {
	ArgsManager args_mgr("borutadmn_z");
	args_mgr.parse(*argc, argv, DefaultArgs(), DaemonArgs());
	args_mgr.initLibpar();

	DaemonConfigInfo* cfg;

	if (args_mgr.has("use-cfgdealer")) {
		szlog::init(args_mgr, "borutadmn");
		cfg = new ConfigDealerHandler(args_mgr);
	} else {
		auto d_cfg = new DaemonConfig("borutadmn_z");
		if (d_cfg->Load(args_mgr))
			return 101;
		cfg = d_cfg;
	}

	if (configure_ipc(args_mgr, *cfg))
		return 102;
	if (configure_units(args_mgr, *cfg))
		return 103;

	configure_timer(args_mgr, *cfg);
	return 0;
}

void boruta_daemon::go() {
	cycle_timer_callback();
	event_base_dispatch(EventBaseHolder::evbase);
}

void boruta_daemon::cycle_timer_callback() {
	m_zmq->receive();
	m_zmq->publish();
	m_timer.schedule();
}

int main(int argc, char *argv[]) {
	EventBaseHolder::Initialize();

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);
	boruta_daemon daemon;
	try {
		if (int ret = daemon.configure(&argc, argv)) {
			sz_log(0, "Error configuring daemon, exiting.");
			return ret;
		}
	} catch (const std::exception& e) {
		sz_log(0, "Error configuring daemon %s, exiting.", e.what());
		return 1;
	}

	signal(SIGPIPE, SIG_IGN);
	sz_log(2, "Starting Boruta Daemon");
	daemon.go();
	return 200;
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
		sz_log(level, "%s: %s", header.c_str(), l);
		free(l);
	} else {
		sz_log(2, "%s: Error in formatting log message", header.c_str());
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
