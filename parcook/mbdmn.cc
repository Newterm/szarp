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
 * Darek Marcinkiewicz <reksio@dalkiaterm.pl>
 * 
 * $Id: mbtcpdmn.cc 106 2009-09-12 13:27:27Z isl $
 */

/*
 Daemon description block.

 @description_start

 @class 4

 @devices All devices or SCADA systems (or other software) using Modbus protocol.
 @devices.pl Wszystkie urz±dzenia i systemy SCADA (lub inne oprogramowanie) wykorzystuj±ce protokó³ Modbus.

 @protocol Modbus RTU using serial line or Modbus TCP/IP.
 @protocol.pl Modbus RTU (po³±czenie szeregowe RS232/RS485) lub Modbus TCP/IP.

 @config Daemon is configured in params.xml, all attributes from 'modbus' namespace not explicity
 described as optional are required.
 @config.pl Sterownik jest konfigurowany w pliku params.xml, wszystkie atrybuty z przestrzeni nazw
 'modbus', nie opisane jako opcjonalne, s± wymagane.

 @comment This daemon is to replace older mbrtudmn and mbtcpdmn daemons.
 @comment.pl Ten sterownik zastêpuje starsze sterowniki mbrtudmn i mbtcpdmn.

 @config_example
<device 
     xmlns:modbus="http://www.praterm.com.pl/SZARP/ipk-extra"
     daemon="/opt/szarp/bin/mbdmn" 
     path="/dev/null"
     modbus:daemon-mode=
 		allowed modes are 'tcp-server' and 'tcp-client' and 'serial-client' and 'serial-server'
     modbud:tcp-port="502"
		TCP port we are listenning on/connecting to (server/client)
     modbus:tcp-allowed="192.9.200.201 192.9.200.202"
		(optional) list of allowed clients IP addresses for server mode, if empty all
		addresses are allowed
     modbus:tcp-address="192.9.200.201"
 		server IP address (required in client mode)
     modbus:tcp-keepalive="yes"
 		should we set TCP Keep-Alive options? "yes" or "no"
     modbus:tcp-timeout="32"
		(optional) connection timeout in seconds, after timeout expires, connection
		is closed; default empty value means no timeout
     modbus:nodata-timeout="15"
		(optional) timeout (in seconds) to set data to 'NO_DATA' if data is not available,
		default is 20 seconds
     modbus:nodata-value="-1"
 		(optional) value to send instead of 'NO_DATA', default is 0
     modbus:FloatOrder="msblsb"
 		(optional) registers order for 4 bytes (2 registers) float order - "msblsb"
		(default) or "lsbmsb"; values names are a little misleading, it shoud be 
		msw/lsw (most/less significant word) not msb/lsb (most/less significant byte),
		but it's left like this for compatibility with Modbus RTU driver configuration
      >
      <unit id="1" modbus:id="49">
		modbus:id is optional Modbus unit identifier - number from 0 to 255; default is to
		use IPK id attribute, but parsed as a character value and not a number; in this 
		example both definitions id="1" and modbud:id="49" give tha same value, because ASCII
		code of "1" is 49
              <param
			Read value using ReadHoldingRegisters (0x03) Modbus function              
                      name="..."
                      ...
                      modbus:address="0x03"
 	                      	modbus register number, starting from 0
                      modbus:val_type="integer">
 	                      	register value type, 'integer' (2 bytes, 1 register) or float (4 bytes,
	                       	2 registers)
                      ...
              </param>
              <param
                      name="..."
                      ...
                      modbus:address="0x04"
                      modbus:val_type="float"
			modbus:val_op="msblsb"
			modbus:val_op="LSW"
				(optional) operator for converting data from float to 2 bytes integer;
				default is 'NONE' (simple conversion to short int), other values
				are 'LSW' and 'MSW' - converting to 4 bytes long and getting less/more
				significant word; in this case there should be 2 parameters with the
				same register address and different val_op attributes - LSW and MSW.
				(useful when param value is stored as 2B, but we multiply it by large prec)
			modbus:combined="LSW">
				(optional) operator for sending a combined param using its lsw and msw parts,
				values are 'LSW' and 'MSW', there should be 2 parameters (lsw and msw) with the
				same register address and different combined attributes - LSW and MSW.
				Additionally, if a matching defined combined param is found, its precision is used.
				...
              </param>
              ...
              <send 
              		Sending value using WriteMultipleRegisters (0x10) Modbus function
                      param="..." 
                      type="min"
                      modbus:address="0x1f"
                      modbus:val_type="float">
                      ...
              </send>
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
#include <event.h>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <deque>
#include <set>

#include <libxml/parser.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "conversion.h"
#include "ipchandler.h"
#include "liblog.h"
#include "xmlutils.h"
#include "tokens.h"
#include "custom_assert.h"

const unsigned short int MB_DEFAULT_PORT = 502;
const unsigned char MB_ERROR_CODE = 0x80;

/** Modbus exception codes. */
const unsigned char MB_ILLEGAL_FUNCTION  = 0x01;
const unsigned char MB_ILLEGAL_DATA_ADDRESS = 0x02;
const unsigned char MB_ILLEGAL_DATA_VALUE = 0x03;
const unsigned char MB_SLAVE_DEVICE_FAILURE = 0x04;
const unsigned char MB_ACKNOWLEDGE = 0x05;
const unsigned char MB_SLAVE_DEVICE_BUSY = 0x06;
const unsigned char MB_MEMORY_PARITY_ERROR  = 0x08;
const unsigned char MB_GATEWAY_PATH_UNAVAILABLE = 0x0A;
const unsigned char MB_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND  = 0x0B;

/** Modbus public function codes. */
const unsigned char MB_F_RHR = 0x03;		/* Read Holding registers */
const unsigned char MB_F_RIR = 0x04;		/* Read Input registers */
const unsigned char MB_F_WSR = 0x06;		/* Write single register */
const unsigned char MB_F_WMR = 0x10;		/* Write multiple registers */

bool g_debug = false;

enum REGISTER_TYPE {
	HOLDING_REGISTER,
	INPUT_REGISTER
};

struct PDU {
	unsigned char func_code;
	std::vector<unsigned char> data;
};

struct TCPADU {
	unsigned short trans_id;
	unsigned short proto_id;
	unsigned short length;
	unsigned char unit_id;
	PDU pdu;
};

struct SDU {
	unsigned char unit_id;
	PDU pdu;
};

//maps regisers values to parcook short values
class parcook_modbus_val_op {
public:
	virtual unsigned short val() = 0;
};

//maps values from sender to parcook registers
class sender_modbus_val_op {
protected:
	float m_nodata_value;
public:
	sender_modbus_val_op(float no_data) : m_nodata_value(no_data) {}	
	virtual void set_val(short val, time_t current_time) = 0;
};

class modbus_register;
typedef std::map<unsigned short, modbus_register*> RMAP;
typedef std::map<unsigned char, RMAP> URMAP;
typedef std::set<std::pair<unsigned char, std::pair<REGISTER_TYPE, unsigned short> > > RSET;

class modbus_daemon;
class modbus_register {
	modbus_daemon *m_modbus_daemon;
	unsigned short m_val;
	time_t m_mod_time;
public:
	modbus_register(modbus_daemon *daemon);
	void set_val(unsigned short val, time_t time);
	unsigned short get_val(bool &valid);
	unsigned short get_val();
};

class modbus_daemon {
protected:
	DaemonConfig *m_cfg;
	IPCHandler *m_ipc;
	URMAP m_registers;

	std::vector<parcook_modbus_val_op*> m_parcook_ops;
	std::vector<sender_modbus_val_op*> m_sender_ops;
	std::vector<modbus_register*> m_send_map;

	RSET m_received;
	RSET m_sent;

	struct event_base* m_event_base;
	struct event m_timer;

	enum FLOAT_ORDER { LSWMSW, MSWLSW } m_float_order;

	time_t m_current_time;
	time_t m_expiration_time;
	float m_nodata_value;

	bool process_request(unsigned char unit, PDU &pdu);
	bool perform_read_holding_regs(RMAP &unit, PDU &pdu);
	bool perform_write_registers(RMAP &unit, PDU &pdu);
	bool create_error_response(unsigned char error, PDU &pdu);
	void consume_read_regs_response(unsigned char& unit, unsigned short start_addr, unsigned short regs_count, PDU &pdu);
	void consume_write_regs_response(unsigned char& unit, unsigned short start_addr, unsigned short regs_count, PDU &pdu);
	int configure_unit(TUnit* u, xmlXPathContextPtr xp_ctx);
	const char* error_string(const unsigned char& error);
public:
	modbus_daemon();
	virtual ~modbus_daemon() {};
	struct event_base* get_event_base();
	bool register_val_expired(time_t time);
	virtual int configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx);
	virtual int initialize();

	void to_parcook();
	void from_sender();

	void go();

	virtual void timer_event();

	static void timer_callback(int fd, short event, void* daemon);
};

struct serial_port_configuration {
	std::string path;
	enum PARITY {
		NONE,
		ODD,
		EVEN
	} parity;
	int stop_bits;
	int speed;
	int delay_between_chars;
	int read_timeout;
};

class tcp_connection_handler {
public:
	virtual void frame_parsed(TCPADU &adu, struct bufferevent* bufev) = 0;
};

class tcp_parser {
	enum { TR, TR2, PR, PR2, L, L2, U_ID, FUNC, DATA } m_state;
	size_t m_payload_size;

	tcp_connection_handler* m_tcp_handler;
	TCPADU m_adu;
public:
	tcp_parser(tcp_connection_handler *tcp_handler);
	void send_adu(unsigned short trans_id, unsigned char unit_id, PDU &pdu, struct bufferevent *bufev);
	void read_data(struct bufferevent *bufev);
	void reset();
};

struct tcp_connection {
	int fd;
	struct sockaddr_in addr;
	struct bufferevent* bufev;
	tcp_parser *parser;
	tcp_connection(int _fd, struct sockaddr_in _addr, struct bufferevent *_bufev, tcp_parser* _parser) : 
		fd(_fd), addr(_addr), bufev(_bufev), parser(_parser) {}
	tcp_connection() {}
};

class tcp_server : public modbus_daemon, public tcp_connection_handler {
	std::vector<struct in_addr> m_allowed_ips;
	unsigned short m_listen_port;
	int m_listen_fd;
	struct event m_listen_event;
	std::map<struct bufferevent*, tcp_connection*> m_connections;

	bool ip_allowed(struct sockaddr_in* addr);
public:
	virtual void frame_parsed(TCPADU &adu, struct bufferevent* bufev);
	virtual int configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx);
	virtual int initialize();

	void accept_connection();
	void connection_error(struct bufferevent* bufev);
	void connection_read(struct bufferevent* bufev);

	static void connection_read_cb(struct bufferevent *ev, void* tcp_server);
	static void connection_error_cb(struct bufferevent *ev, short event, void* tcp_server);
	static void connection_accepted_cb(int fd, short event, void* tcp_server);
};

class client_connection;
class modbus_client : public modbus_daemon {
protected:
	RSET::iterator m_received_iterator;
	RSET::iterator m_sent_iterator;

	PDU m_pdu;

	unsigned short m_start_addr;
	unsigned short m_regs_count;
	unsigned char m_unit;
	REGISTER_TYPE m_register_type;

	enum {
		AWAITING_CONNECTION,
		SENDING_READ_QUERY,
		SENDING_WRITE_QUERY,
		IDLE,
	} m_state;

	bool m_connected;

	client_connection *m_connection;

	time_t m_last_activity;

	void reset_cycle();
	void start_cycle();
	void send_next_query();
	void send_write_query();
	void send_read_query();
	void next_query();
	void send_query();
	void find_continous_reg_block(RSET::iterator &i, RSET &regs);
public:
	modbus_client(client_connection *client);
	virtual int configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx);
	void connection_failed();
	void connection_ready();
	void pdu_received(unsigned char u, PDU &pdu);
	virtual int initialize();
	virtual void timeout();
	virtual void timer_event();
};

class client_connection {
protected: 
	modbus_client* m_modbus_client;
public:
	void set_modbus_client(modbus_client *_modbus_client) { m_modbus_client = _modbus_client; }
	virtual void send_pdu(unsigned char u, PDU &pdu) = 0;
	virtual int configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx) = 0;
	virtual void connect() = 0;
	virtual void disconnect() = 0;
};

class tcp_client_connection : public client_connection, public tcp_connection_handler {
	unsigned short m_port;
	unsigned short m_trans_id;
	tcp_connection m_connection;
	bool m_connect_waiting;
public:
	int configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx);
	virtual void connect();
	virtual void disconnect();
	virtual void frame_parsed(TCPADU &adu, struct bufferevent* bufev);
	virtual void send_pdu(unsigned char u, PDU &pdu);

	void connection_read();
	void connection_error();
	void connection_write();

	static void connection_read_cb(struct bufferevent *ev, void* client);
	static void connection_write_cb(struct bufferevent *ev, void* client);
	static void connection_error_cb(struct bufferevent *ev, short event, void* client);
};

class serial_connection_handler {
public:
	virtual void frame_parsed(SDU &adu, struct bufferevent* bufev) = 0;
	virtual void timeout() = 0;
	virtual struct event_base* get_event_base() = 0;
	virtual struct bufferevent* get_buffer_event() = 0;
};

class serial_parser {
protected:
	serial_connection_handler *m_serial_handler;
	struct event m_read_timer;
	SDU m_sdu;
	bool m_read_timer_started;
	int m_timeout;

	int m_delay_between_chars;
	std::deque<unsigned char> m_output_buffer;
	struct event m_write_timer;
	bool m_write_timer_started;

	void start_read_timer();
	void stop_read_timer();

	void start_write_timer();
	void stop_write_timer();

	virtual void read_timer_event() = 0;
	virtual void write_timer_event();
public:
	serial_parser(serial_connection_handler *serial_handler, int delay_between_chars, int read_timeout);
	virtual void send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev) = 0;
	virtual void read_data(struct bufferevent *bufev) = 0;
	virtual void write_finished(struct bufferevent *bufev);
	virtual void reset() = 0;

	static void read_timer_callback(int fd, short event, void* serial_parser);
	static void write_timer_callback(int fd, short event, void* serial_parser);
};

class serial_rtu_parser : public serial_parser {
	enum { ADDR, FUNC_CODE, DATA } m_state;
	size_t m_data_read;

	bool check_crc();

	static void calculate_crc_table();
	static unsigned short crc16[256];
	static bool crc_table_calculated;

	unsigned short update_crc(unsigned short crc, unsigned char c);

	virtual void read_timer_event();
public:
	serial_rtu_parser(serial_connection_handler *serial_handler, int delay_between_chars, int speed, int read_timeout);
	void read_data(struct bufferevent *bufev);
	void send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev);
	void reset();

	static const int max_frame_size = 256;
};

#if 0 
#error to be implemented
class serial_ascii_parser {
public:
	serial_ascii_parser(serial_connection_handler *serial_handler) : serial_parser(serial_handler) {}
	void read_data(struct bufferevent *bufev);
	void send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev);
	void reset();
};
#endif


unsigned short serial_rtu_parser::crc16[256] = {};
bool serial_rtu_parser::crc_table_calculated = false;

class serial_server : public modbus_daemon, public serial_connection_handler {
	int m_fd;
	serial_port_configuration m_spc;
	serial_rtu_parser* m_parser;
	struct bufferevent *m_bufev;

	void close_connection();
	bool open_connection();
public:
	serial_server();
	virtual int configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx);
	virtual int initialize();
	virtual void frame_parsed(SDU &sdu, struct bufferevent* bufev);
	virtual struct bufferevent* get_buffer_event();

	void connection_error(struct bufferevent* bufev);
	void connection_read(struct bufferevent* bufev);
	void connection_write(struct bufferevent* bufev);

	virtual void timeout();
	virtual struct event_base* get_event_base();

	static void connection_read_cb(struct bufferevent *ev, void* tcp_server);
	static void connection_write_cb(struct bufferevent *ev, void* client);
	static void connection_error_cb(struct bufferevent *ev, short event, void* server);
};

class serial_client_connection : public client_connection, public serial_connection_handler {
	serial_port_configuration m_spc;
	int m_fd;
	struct bufferevent* m_bufev;
	serial_rtu_parser *m_parser;
public:
	virtual int configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx);
	virtual void send_pdu(unsigned char u, PDU &pdu);
	virtual void frame_parsed(SDU& sdu, struct bufferevent* bufev);
	virtual struct bufferevent* get_buffer_event();

	virtual void timeout();
	virtual void connect();
	virtual void disconnect();

	void connection_error(struct bufferevent* bufev);
	void connection_read(struct bufferevent* bufev);
	void connection_write(struct bufferevent* bufev);

	virtual struct event_base* get_event_base();

	static void connection_read_cb(struct bufferevent *ev, void* client);
	static void connection_write_cb(struct bufferevent *ev, void* client);
	static void connection_error_cb(struct bufferevent *ev, short event, void* client);
};

class short_parcook_modbus_val_op : public parcook_modbus_val_op {
	modbus_register* m_reg;
public:
	short_parcook_modbus_val_op(modbus_register *reg);
	virtual unsigned short val();
};

class bcd_parcook_modbus_val_op : public parcook_modbus_val_op {
	modbus_register* m_reg;
public:
	bcd_parcook_modbus_val_op(modbus_register *reg);
	virtual unsigned short val();
};

template<class T> class long_parcook_modbus_val_op : public parcook_modbus_val_op {
	modbus_register *m_reg_lsw;
	modbus_register *m_reg_msw;
	int m_prec;
	bool m_lsw;
public:
	long_parcook_modbus_val_op(modbus_register *reg_lsw, modbus_register *reg_msw, int prec, bool lsw);
	virtual unsigned short val();
};

class short_sender_modbus_val_op : public sender_modbus_val_op {
	modbus_register *m_reg;
public:
	short_sender_modbus_val_op(float no_data, modbus_register *reg);
	virtual void set_val(short val, time_t time);
};

/** Receives a short value (as almost all other sender_modbus_val_ops)
 *	and outputs it as a modbus float */
class float_sender_modbus_val_op : public sender_modbus_val_op {
protected:
	modbus_register *m_reg_lsw;
	modbus_register *m_reg_msw;
	int m_prec;
public:
	float_sender_modbus_val_op(float no_data, modbus_register *reg_lsw, modbus_register *reg_msw, int prec);
	virtual void set_val(short val, time_t time);
};

/** Common storage class for two instances float_to_float_sender_modbus_val_op */
class float_to_float_storage {
	unsigned short m_last_v[2];
public:
	void set_val(bool lsw, short val);
	unsigned short get_val(bool lsw) const;
	unsigned short* get_raw();
};

/** Receives a float (4B) value from two szbase params */
class float_to_float_sender_modbus_val_op : public float_sender_modbus_val_op {
	bool m_lsw;
	float_to_float_storage *m_last_v;
public:
	float_to_float_sender_modbus_val_op(float no_data,
		modbus_register *reg_lsw, modbus_register *reg_msw, int prec,
		bool lsw, float_to_float_storage *last_v);
	virtual void set_val(short val, time_t time);
};


void dolog(int level, const char * fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

xmlChar* get_device_node_prop(xmlXPathContextPtr xp_ctx, const char* prop) {
	xmlChar *c;
	char *e;
	const int ret = asprintf(&e, "./@modbus:%s", prop);
	(void)ret;
	ASSERT(e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx, false);
	free(e);
	return c;
}

int get_tcp_port(xmlXPathContextPtr xp_ctx, unsigned short* port) {
	int ret = 0;
	xmlChar* tcp_port = get_device_node_prop(xp_ctx, "tcp-port");
	if (tcp_port == NULL) {
		dolog(10, "Unspecified tcp port, assuming default port: %hu", MB_DEFAULT_PORT);
		*port = 512;
	} else {
		try {
			*port = boost::lexical_cast<unsigned short>((char*) tcp_port);
		} catch (boost::bad_lexical_cast &e) {
			dolog(10, "Invalid port value %s, value should be number between 1 and 65535", tcp_port);
			return 1;
		} 
	}
	xmlFree(tcp_port);
	return ret;
}

template<class T> long_parcook_modbus_val_op<T>::long_parcook_modbus_val_op(
		modbus_register* reg_lsw,
		modbus_register* reg_msw,
		int prec,
		bool lsw) :
	m_reg_lsw(reg_lsw), m_reg_msw(reg_msw), m_prec(prec), m_lsw(lsw)
{}


short_parcook_modbus_val_op::short_parcook_modbus_val_op(modbus_register *reg) :
	m_reg(reg) 
{}

unsigned short short_parcook_modbus_val_op::val() {
	bool valid;
	unsigned short v = m_reg->get_val(valid);
	if (!valid)
		return SZARP_NO_DATA;
	return v;
}

bcd_parcook_modbus_val_op::bcd_parcook_modbus_val_op(modbus_register *reg) :
	m_reg(reg) 
{}

unsigned short bcd_parcook_modbus_val_op::val() {
	bool valid;
	unsigned short val = m_reg->get_val(valid);
	if (!valid)
		return SZARP_NO_DATA;

	return (val & 0xf)
		+ ((val >> 4) & 0xf) * 10
		+ ((val >> 8) & 0xf) * 100
		+ (val >> 12) * 1000;
}

template<class T> unsigned short long_parcook_modbus_val_op<T>::val() {
	uint16_t v2[2];
	bool valid;

	v2[1] = m_reg_msw->get_val(valid);
	if (!valid) {
		dolog(10, "Int value, msw invalid, no_data");
		return SZARP_NO_DATA;
	}
	v2[0] = m_reg_lsw->get_val(valid);
	if (!valid) {
		dolog(10, "Int value, lsw invalid, no_data");
		return SZARP_NO_DATA;
	}

	int32_t iv = *((int32_t*)v2) * m_prec; 
	uint32_t* pv = (uint32_t*) &iv;

	dolog(10, "Int value: %d, unsigned int: %u", iv, *pv);

	if (m_lsw)
		return *pv & 0xffff;
	else
		return *pv >> 16;
}

template<> unsigned short long_parcook_modbus_val_op<float>::val() {
	unsigned short v2[2];
	bool valid;

	v2[0] = m_reg_msw->get_val(valid);
	if (!valid) {
		dolog(10, "Float value, msw invalid, no_data");
		return SZARP_NO_DATA;
	}
	v2[1] = m_reg_lsw->get_val(valid);
	if (!valid) {
		dolog(10, "Float value, lsw invalid, no_data");
		return SZARP_NO_DATA;
	}

	float* f = (float*) v2;
	int iv = *f * m_prec;	
	unsigned int* pv = (unsigned int*) &iv;

	dolog(10, "Float value: %f, int: %d, unsigned int: %u", *f, iv, *pv);
	if (m_lsw)
		return *pv & 0xffff;
	else
		return *pv >> 16;
}

short_sender_modbus_val_op::short_sender_modbus_val_op(float no_data, modbus_register *reg) : sender_modbus_val_op(no_data), m_reg(reg) {}

void short_sender_modbus_val_op::set_val(short val, time_t time) {
	if (val == SZARP_NO_DATA)
		m_reg->set_val(m_nodata_value, time);
	else
		m_reg->set_val(val, time);
}

float_sender_modbus_val_op::float_sender_modbus_val_op(float no_data, modbus_register *reg_lsw, modbus_register *reg_msw, int prec) :
	sender_modbus_val_op(no_data), m_reg_lsw(reg_lsw), m_reg_msw(reg_msw), m_prec(prec) {}

void float_sender_modbus_val_op::set_val(short val, time_t time) {
	unsigned short iv[2];
	float fval;
	if (val == SZARP_NO_DATA)
		fval = m_nodata_value;
	else
		fval = float(val) / m_prec;
	memcpy(iv, &fval, sizeof(float));

	m_reg_msw->set_val(iv[0], time);
	m_reg_lsw->set_val(iv[1], time);
}

void float_to_float_storage::set_val(bool lsw, short val) {
	if (lsw)
		m_last_v[0] = val;
	else
		m_last_v[1] = val;
}

unsigned short float_to_float_storage::get_val(bool lsw) const {
	if (lsw)
		return m_last_v[0];
	else
		return m_last_v[1];
}

unsigned short* float_to_float_storage::get_raw() {
	return m_last_v;
}

float_to_float_sender_modbus_val_op::float_to_float_sender_modbus_val_op(
		float no_data, modbus_register *reg_lsw, modbus_register *reg_msw,
		int prec, bool lsw, float_to_float_storage *last_v) :
	float_sender_modbus_val_op(no_data, reg_lsw, reg_msw, prec), m_lsw(lsw),
	m_last_v(last_v) {}

void float_to_float_sender_modbus_val_op::set_val(short val, time_t time) {
	uint32_t val32 = 0;
	float fval = 0;
	if (val == SZARP_NO_DATA) {
		// TODO : if only one of the registers is NODATA, behaviour is undefined
		fval = m_nodata_value;
	} else {
		m_last_v->set_val(m_lsw, val);
		// apply precision
		memcpy(&val32, m_last_v->get_raw(), sizeof(uint32_t));
		fval = (float)val32 / m_prec;
	}

	// copy to final output array
	unsigned short iv[2];
	memcpy(iv, &fval, sizeof(float));

	m_reg_msw->set_val(iv[0], time);
	m_reg_lsw->set_val(iv[1], time);
}

modbus_register::modbus_register(modbus_daemon* daemon) : m_modbus_daemon(daemon), m_val(SZARP_NO_DATA), m_mod_time(-1) {}

unsigned short modbus_register::get_val(bool &valid) {
	if (m_mod_time < 0) {
		sz_log(10, "m_mod_time < 0");
		valid = false;
	}
	else
	{
		valid = m_modbus_daemon->register_val_expired(m_mod_time);
		sz_log(10, "valid (from register_val_expired) %d", valid);
	}
	return m_val;
}

unsigned short modbus_register::get_val() {
	return m_val;
}

void modbus_register::set_val(unsigned short val, time_t time) {
	m_val = val;
	m_mod_time = time;
	dolog(9, "set_val to %d, time: %s", val, asctime(localtime(&time)));
}

modbus_daemon::modbus_daemon() {
	m_event_base = (struct event_base*) event_init();
}

void modbus_daemon::go() {
	timer_event();
	event_base_dispatch(m_event_base);
}

bool modbus_daemon::register_val_expired(time_t time) {
	if (m_expiration_time == 0)
		return true;
	else
		return time >= m_current_time - m_expiration_time;
}

bool modbus_daemon::process_request(unsigned char unit, PDU &pdu) {
	URMAP::iterator i = m_registers.find(unit);
	if (i == m_registers.end()) {
		dolog(1, "Received request to unit %d, not such unit in configuration, ignoring", (int) unit);
		return false;
	}

	try {

		switch (pdu.func_code) {
			case MB_F_RHR:
				return perform_read_holding_regs(i->second, pdu);
			case MB_F_WSR:
			case MB_F_WMR:
				return perform_write_registers(i->second, pdu);
			default:
				return create_error_response(MB_ILLEGAL_FUNCTION, pdu);
		}

	} catch (std::out_of_range) {	//message was distorted
		dolog(1, "Error while processing request - there is either a bug or sth was very wrong with request format");
		return false;
	}

}

void modbus_daemon::timer_event() {
	dolog(5, "Timer click, doing ipc data exchange");
	m_current_time = time(NULL);

	from_sender();
	to_parcook();

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	evtimer_add(&m_timer, &tv); 
}

void modbus_daemon::timer_callback(int fd, short event, void* daemon) {
	((modbus_daemon*) daemon)->timer_event();
}

int modbus_daemon::initialize() {

	try {
		auto ipc_ = std::unique_ptr<IPCHandler>(new IPCHandler(m_cfg));
		m_ipc = ipc_.release();
	} catch(...) {
		return 1;
	}

	evtimer_set(&m_timer, timer_callback, this);
	event_base_set(m_event_base, &m_timer);

	return 0;
}

const char* modbus_daemon::error_string(const unsigned char& error) {
	switch (error) {
		case MB_ILLEGAL_FUNCTION:
			return "ILLEGAL FUNCTION";
		case MB_ILLEGAL_DATA_ADDRESS:
			return "ILLEGAL DATA ADDRESS";
		case MB_ILLEGAL_DATA_VALUE:
			return "ILLEGAL DATA VALUE";
		case MB_SLAVE_DEVICE_FAILURE:
			return "SLAVE DEVCE FAILURE";
		case MB_ACKNOWLEDGE:
			return "ACKNOWLEDGE";
		case MB_SLAVE_DEVICE_BUSY:
			return "SLAVE DEVICE FAILURE";
		case MB_MEMORY_PARITY_ERROR:
			return "MEMORY PARITY ERROR";
		case MB_GATEWAY_PATH_UNAVAILABLE:
			return "GATEWAY PATH UNAVAILABLE";
		case MB_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND:
			return "GATEWAY TARGET DEVICE FAILED TO RESPOND";
		default:
			return "UNKNOWN/UNSUPPORTED ERROR CODE";

	}
}

void modbus_daemon::consume_read_regs_response(unsigned char& uid, unsigned short start_addr, unsigned short regs_count, PDU &pdu) {
	dolog(5, "Consuming read holding register response unit_id: %d, address: %hu, registers count: %hu", (int) uid, start_addr, regs_count);
	if (pdu.func_code & MB_ERROR_CODE) {
		dolog(1, "Exception received in response to read holding registers command, unit_id: %d, address: %hu, count: %hu",
		       	(int)uid, start_addr, regs_count);
		dolog(1, "Error is: %s(%d)", error_string(pdu.data.at(0)), (int)pdu.data.at(0));
		return;
	} 

	URMAP::iterator i = m_registers.find(uid);
	ASSERT(i != m_registers.end());
	RMAP& unit = i->second;

	size_t data_index = 1;

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++, data_index += 2) {
		RMAP::iterator j = unit.find(addr);
		unsigned short v = (pdu.data.at(data_index) << 8) | pdu.data.at(data_index + 1);
		dolog(7, "Setting register unit_id: %d, address: %zu, value: %hu", (int) uid, addr, v);
		j->second->set_val(v, m_current_time);
	}


}

void modbus_daemon::consume_write_regs_response(unsigned char& unit, unsigned short start_addr, unsigned short regs_count, PDU &pdu) { 
	dolog(5, "Consuming write holding register response unit_id: %d, address: %hu, registers count: %hu", (int) unit, start_addr, regs_count);
	if (pdu.func_code & MB_ERROR_CODE) {
		dolog(1, "Exception received in response to write holding registers command, unit_id: %d, address: %hu, count: %hu",
		       	(int)unit, start_addr, regs_count);
		dolog(1, "Error is: %s(%d)", error_string(pdu.data.at(0)), (int)pdu.data.at(0));
	} else {
		dolog(5, "Write holding register command poisitively acknowledged");
	}
}

bool modbus_daemon::perform_write_registers(RMAP &unit, PDU &pdu) {
	std::vector<unsigned char>& d = pdu.data;

	unsigned short start_addr;
	unsigned short regs_count;
	size_t data_index;


	start_addr = (d.at(0) << 8) | d.at(1);

	if (pdu.func_code == MB_F_WMR) {
		regs_count = (d.at(2) << 8) | d.at(3);
		data_index = 5;
	} else {
		regs_count = 1;
		data_index = 2;
	}

	dolog(7, "Processing write holding request registers start_addr: %hu, regs_count:%hu", start_addr, regs_count);

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++, data_index += 2) {
		RMAP::iterator j = unit.find(addr);
		if (j == unit.end())
			return create_error_response(MB_ILLEGAL_DATA_ADDRESS, pdu);

		unsigned short v = (d.at(data_index) << 8) | d.at(data_index + 1);

		dolog(7, "Setting register: %zu value: %hu", addr, v);
		j->second->set_val(v, m_current_time);	
	}

	if (pdu.func_code == MB_F_WMR)
		d.resize(4);

	dolog(7, "Request processed successfully");

	return true;
}

bool modbus_daemon::perform_read_holding_regs(RMAP &unit, PDU &pdu) {
	std::vector<unsigned char>& d = pdu.data;
	unsigned short start_addr = (d.at(0) << 8) | d.at(1);
	unsigned short regs_count = (d.at(2) << 8) | d.at(3);

	dolog(7, "Responding to read holding registers request start_addr: %hu, regs_count:%hu", start_addr, regs_count);

	d.resize(1);
	d.at(0) = 2 * regs_count;

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++) {
		RMAP::iterator j = unit.find(addr);
		if (j == unit.end())
			return create_error_response(MB_ILLEGAL_DATA_ADDRESS, pdu);

		unsigned short v = j->second->get_val();
		d.push_back(v >> 8);
		d.push_back(v & 0xff);
		dolog(7, "Sending register: %zu, value: %hu", addr, v);
	}

	dolog(7, "Request processed sucessfully.");

	return true;
}

bool modbus_daemon::create_error_response(unsigned char error, PDU &pdu) {
	pdu.func_code |= MB_ERROR_CODE;
	pdu.data.resize(1);
	pdu.data[0] = error;

	dolog(7, "Sending error response: %s", error_string(error));

	return true;
}

int modbus_daemon::configure_unit(TUnit* u, xmlXPathContextPtr xp_ctx) {
	int i, j;
	TParam *p;
	TSendParam *sp;
	char *c;

	unsigned char id;
	c = (char*) xmlGetNsProp(xp_ctx->node, BAD_CAST("id"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (c != NULL) {
		id = atoi(c);
		xmlFree(c);
	} else {
		unsigned char _id = SC::A2U(u->getAttribute("id"))[0];
		switch (_id) {
			case L'0'...L'9':
				id = _id - L'0';
				break;
			case L'a'...L'z':
				id = (_id - L'a') + 10;
				break;
			case L'A'...L'Z':
				id = (_id - L'A') + 10;
				break;
			default:
				id = _id;
				break;
		}
	}

	std::map<unsigned short, float_to_float_storage*> addr_to_storage;
	for (p = u->GetFirstParam(), sp = u->GetFirstSendParam(), i = 0, j = 0;
			i < u->GetParamsCount() + u->GetSendParamsCount();
			i++, j++) {

		char *expr;

		if (i == u->GetParamsCount())
			j = 0;
		bool send = j != i;

		const int ret = asprintf(&expr, ".//ipk:%s[position()=%d]", i < u->GetParamsCount() ? "param" : "send", j + 1);
		(void)ret;
		xmlNodePtr node = uxmlXPathGetNode(BAD_CAST expr, xp_ctx, false);
		ASSERT(node);
		free(expr);

		c = (char*) xmlGetNsProp(node, BAD_CAST("address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (c == NULL) {
			dolog(0, "Missing address attribute in param element at line: %ld", xmlGetLineNo(node));
			return 1;
		}

		unsigned short addr;
		char *e;	
		long l = strtol((char*)c, &e, 0);
		if (*e != 0 || l < 0 || l > 65535) {
			dolog(0, "Invalid address attribute value: %s(line %ld), between 0 and 65535", c, xmlGetLineNo(node));
			return 1;
		} 
		xmlFree(c);
		addr = l;

		REGISTER_TYPE rt;
		c = (char*) xmlGetNsProp(node, BAD_CAST("register_type"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (c == NULL || !strcmp(c, "holding_register"))
			rt = HOLDING_REGISTER;
		else if (!strcmp(c, "input_register"))
			rt = INPUT_REGISTER;
		else {
			dolog(0, "Unsupported register type, line %ld, should be either input_register or holding_register", xmlGetLineNo(node));
			return 1;
		}
		xmlFree(c);


		c = (char*) xmlGetNsProp(node, BAD_CAST("val_type"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (c == NULL) {
			dolog(2, "Missing val_type attribute value, line %ld", xmlGetLineNo(node));
			return 1;
		}

		TParam *param; 
		if (send) {
			param = m_cfg->GetIPK()->getParamByName(sp->GetParamName());
			if (param == NULL) {
				dolog(0, "parameter with name '%ls' not found (send parameter %d)",
						sp->GetParamName().c_str(), j + 1);
				return 1;
			}
		} else
			param = p;

		sender_modbus_val_op *vos;
		parcook_modbus_val_op *vop;
		bool two_regs = false;
		unsigned short other_reg = 0;
		if (!strcmp(c, "integer")) {
			m_registers[id][addr] = new modbus_register(this);
			if (send) 
				vos = new short_sender_modbus_val_op(m_nodata_value, m_registers[id][addr]);
			else 
				vop = new short_parcook_modbus_val_op(m_registers[id][addr]);
			dolog(7, "Param %s mapped to unit: %lc, register %hu, value type: integer", SC::S2A(param->GetName()).c_str(), u->GetId(), addr);
		} else if (!strcmp(c, "bcd")) {
			m_registers[id][addr] = new modbus_register(this);
			if (send) {
				dolog(0, "Unsupported bcd value type for send param no. %d", j + 1);
				return 1;
			}
			vop = new bcd_parcook_modbus_val_op(m_registers[id][addr]);
			dolog(7, "Param %s mapped to unit: %lc, register %hu, value type: bcd", SC::S2A(param->GetName()).c_str(), u->GetId(), addr);
		} else if (!strcmp(c, "long") || !strcmp(c, "float")) {
			xmlChar* c2 = xmlGetNsProp(node, BAD_CAST("val_op"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));

			FLOAT_ORDER float_order = m_float_order;

			xmlChar* c3 = xmlGetNsProp(node, BAD_CAST("FloatOrder"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
			if (c3 != NULL) {
				if (!xmlStrcmp(c3, BAD_CAST "msblsb")) {
					dolog(5, "Setting mswlsw for next param");
					float_order = MSWLSW;
				} else if (!xmlStrcmp(c3, BAD_CAST "lsbmsb")) {
					dolog(5, "Setting lswmsw for next param");
					float_order = LSWMSW;
				} else {
					dolog(0, "Invalid float order specification: %s, %ld", (char*)c, xmlGetLineNo(node));
					return 1;
				}
			}
			xmlFree(c3);

			unsigned short msw, lsw;
			bool is_lsw;
			if (c2 == NULL)  {
				if (float_order == MSWLSW) {
					msw = addr;
					other_reg = lsw = addr + 1;
				} else {
					other_reg = msw = addr + 1;
					lsw = addr;
				}
				is_lsw = true;
			} else if (!xmlStrcmp(c2, BAD_CAST "MSW")) {
				if (float_order == MSWLSW) {
					msw = addr;
					lsw = addr + 1;
				} else {
					msw = addr + 1;
					lsw = addr;
				}
				other_reg = lsw;
				is_lsw = false;
			} else if (!xmlStrcmp(c2, BAD_CAST "LSW")) {
				if (float_order == LSWMSW) {
					lsw = addr;
					msw = addr + 1;
				} else {
					lsw = addr + 1;
					msw = addr;
				}
				other_reg = msw;
				is_lsw = true;
			} else {
				dolog(0, "Unsupported val_op attribute value - %s, line %ld", (char*) c2, xmlGetLineNo(node));
				return 1;
			}
			xmlFree(c2);

			if (m_registers[id].find(lsw) == m_registers[id].end()) 
				m_registers[id][lsw] = new modbus_register(this);
			if (m_registers[id].find(msw) == m_registers[id].end())
				m_registers[id][msw] = new modbus_register(this);

			int xml_prec = param->GetPrec();

			xmlChar* combined_str = xmlGetNsProp(node, BAD_CAST("combined"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
			bool is_combined = false;
			bool combined_lsw = false;
			if (combined_str != NULL) {
				is_combined = true;
				if (!xmlStrcmp(combined_str, BAD_CAST "MSW")) {
					//combined_lsw = false;
				} else if (!xmlStrcmp(combined_str, BAD_CAST "LSW")) {
					combined_lsw = true;
				} else {
					dolog(0, "Unsupported combined attribute value - %s, line %ld", (char*) combined_str, xmlGetLineNo(node));
					return 1;
				}

				// check if combined param is present in SZARP and load prec from it
				std::wstring combined_name = sp->GetParamName();
				std::wstring::size_type pos = std::string::npos;
				const std::vector<std::wstring> word_endings {L" lsw", L" msw", L" LSW", L" MSW"};
				const int word_ending_length = word_endings.at(0).size();
				for (auto it = word_endings.begin(); it != word_endings.end(); ++it) {
					pos = combined_name.find(*it);
					if (pos != std::wstring::npos)
						break;
				}
				if (pos != std::wstring::npos) {
					combined_name.erase(pos, word_ending_length);
					TParam *combined_param = m_cfg->GetIPK()->getParamByName(combined_name);
					if (combined_param == NULL) {
						dolog(7, "combined parameter with name '%s' not found (send parameter %d), will use prec from %s",
								SC::S2A(combined_name).c_str(), j + 1, SC::S2A(sp->GetParamName()).c_str());
					} else {
						xml_prec = combined_param->GetPrec();
						dolog(7, "Loaded prec from combined param %s : %d", SC::S2A(combined_name).c_str(), xml_prec);
					}
				}
			}
			xmlFree(combined_str);

			int prec = exp10(xml_prec);
			if (!strcmp(c, "float"))  {
				if (!send) {
					vop = new long_parcook_modbus_val_op<float>(m_registers[id][lsw], m_registers[id][msw], prec, is_lsw);
					dolog(7, "Parcook param %s no(%zu), mapped to unit: %lc, register %hu, value type: float, params holds %s part, lsw: %hu, msw: %hu",
					SC::S2A(param->GetName()).c_str(), m_parcook_ops.size(), u->GetId(), addr, is_lsw ? "lsw" : "msw", lsw, msw);
				} else if (is_combined) {
					if (addr_to_storage.find(addr) == addr_to_storage.end()) {
						addr_to_storage[addr] = new float_to_float_storage();
					}
					vos = new float_to_float_sender_modbus_val_op(m_nodata_value, m_registers[id][lsw], m_registers[id][msw],
						prec, combined_lsw, addr_to_storage.at(addr));
					dolog(7, "Sender param %s no(%zu), mapped to unit: %lc, register %hu, value type: float (combined), params holds %s part(output) and %s part (input), lsw: %hu, msw: %hu",
					SC::S2A(param->GetName()).c_str(), m_sender_ops.size(), u->GetId(), addr, is_lsw ? "lsw" : "msw", combined_lsw ? "lsw" : "msw", lsw, msw);
				} else {
					vos = new float_sender_modbus_val_op(m_nodata_value, m_registers[id][lsw], m_registers[id][msw], prec);
					dolog(7, "Sender param %s no(%zu), mapped to unit: %lc, register %hu, value type: float, params holds %s part",
						SC::S2A(param->GetName()).c_str(), m_sender_ops.size(), u->GetId(), addr, is_lsw ? "lsw" : "msw");
				}
			} else {
				if (!send) {
					vop = new long_parcook_modbus_val_op<unsigned int>(m_registers[id][lsw], m_registers[id][msw], prec, is_lsw);
					dolog(7, "Parcook param %s no(%zu), mapped to unit: %lc, register %hu, value type: long, params holds %s part, lsw: %hu, msw: %hu",
						SC::S2A(param->GetName()).c_str(), m_parcook_ops.size(), u->GetId(), addr, is_lsw ? "lsw" : "msw", lsw, msw);
				} else {
					dolog(0, "Unsupported long value type for send param no. %d, exiting!", j + 1);
					return 1;
				}
			}
			two_regs = true;
		}
		xmlFree(c);

		if (!send) {
			p = p->GetNext();
			m_parcook_ops.push_back(vop);
			m_received.insert(std::make_pair(id, std::make_pair(rt, addr)));
			if (two_regs)
				m_received.insert(std::make_pair(id, std::make_pair(rt, other_reg)));
		} else {
			sp = sp->GetNext();
			m_sender_ops.push_back(vos);
			m_send_map.push_back(m_registers[id][addr]);
			m_sent.insert(std::make_pair(id, std::make_pair(rt, addr)));
		}
	}

	return 0;
}

int modbus_daemon::configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx) {

	m_cfg = cfg;

	xmlChar* c = get_device_node_prop(xp_ctx, "FloatOrder");
	if (c == NULL) {
		dolog(5, "Float order not specified, assuming msblsb");
		m_float_order = MSWLSW;
	} else if (!xmlStrcmp(c, BAD_CAST "msblsb")) {
		dolog(5, "Setting mswlsw float order");
		m_float_order = MSWLSW;
	} else if (!xmlStrcmp(c, BAD_CAST "lsbmsb")) {
		dolog(5, "Setting lswmsw float order");
		m_float_order = LSWMSW;
	} else {
		dolog(0, "Invalid float order specification: %s", c);
		return 1;
	}
	xmlFree(c);

	c = get_device_node_prop(xp_ctx, "nodata-timeout");
	if (c == NULL) {
		dolog(5, "no-data timeout not specified, assuming no data expiration");
		m_expiration_time = 0;
	} else {
		try {
			m_expiration_time = boost::lexical_cast<time_t>((char*) c);
		} catch (boost::bad_lexical_cast) {
			dolog(0, "Invalid value for nodata-timeout %s(line: %ld), exiting", c, xmlGetLineNo(xp_ctx->node));
			return 1;
		}
	}
	xmlFree(c);

	c = get_device_node_prop(xp_ctx, "nodata-value");
	if (c == NULL) {
		dolog(9, "Nodata value not given, using 0");
		m_nodata_value = 0.0;
	} else {
		char *e;
		m_nodata_value = strtof((char*)c , &e);
		if (*e) {
			dolog(0, "Invalid nodata value: %s, configuration invalid", (char*) c);
			return 1;
		}
		dolog(9, "Using %f as nodata value", m_nodata_value);
	}
	xmlFree(c);

	xmlNodePtr device_node = xp_ctx->node;
	int j;
	TUnit *u;
	for (j = 0, u = cfg->GetDevice()->GetFirstUnit();
			u; 
			++j, u = u->GetNext()) {

		char *expr;
		const int ret = asprintf(&expr, ".//ipk:unit[position()=%d]", j + 1);
		(void)ret;
		xmlNodePtr node = uxmlXPathGetNode(BAD_CAST expr, xp_ctx);
		free(expr);

		ASSERT(node);
		xp_ctx->node = node;
		if (configure_unit(u, xp_ctx))
			return 1;
		xp_ctx->node = device_node;
	}
	
	return 0;
}

void modbus_daemon::to_parcook() {
	short *v = m_ipc->m_read;
	size_t j = 0;

	for (std::vector<parcook_modbus_val_op*>::iterator i = m_parcook_ops.begin();
			i != m_parcook_ops.end();
			i++, j++, v++) {
		*v = (*i)->val();
		dolog(9, "Parcook param no %zu set to %hu", j, *v);
	}

	m_ipc->GoParcook();
}

void modbus_daemon::from_sender() {
	short *v = m_ipc->m_send;
	size_t j = 0;

	m_ipc->GoSender();
		
	for (auto i = m_sender_ops.begin(); i != m_sender_ops.end();
			i++, v++, j++) {
		(*i)->set_val(*v, m_current_time);
		dolog(9, "Setting %zu param from sender to %hd, time: %s", j, *v, asctime(localtime(&m_current_time)));
	}
}

struct event_base* modbus_daemon::get_event_base() {
	return m_event_base;
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

tcp_parser::tcp_parser(tcp_connection_handler *tcp_handler) : m_state(TR), m_payload_size(0), m_tcp_handler(tcp_handler)
{ }

void tcp_parser::reset() {
	m_state = TR;
	m_adu.pdu.data.resize(0);
};

void tcp_parser::read_data(struct bufferevent *bufev) {
	unsigned char c;

	while (bufferevent_read(bufev, &c, 1) == 1) switch (m_state) {
		case TR:
			dolog(8, "Started to parse new tcp frame");
			m_adu.pdu.data.resize(0);
			m_adu.trans_id = c;
			m_state = TR2;
			break;
		case TR2:
			m_adu.trans_id |= (c << 8);
			m_state = PR;
			m_adu.trans_id = ntohs(m_adu.trans_id);
			dolog(8, "Transaction id: %hu", m_adu.trans_id);
			break;
		case PR:
			m_adu.proto_id = c;
			m_state = PR2;
			break;
		case PR2:
			m_state = L;
			m_adu.proto_id |= (c << 8);
			m_adu.proto_id = ntohs(m_adu.proto_id);
			dolog(8, "Protocol id: %hu", m_adu.proto_id);
			break;
		case L:
			m_state = L2;
			m_adu.length = c;
			break;
		case L2:
			m_state = U_ID;
			m_adu.length = (c << 8);
			m_adu.length = ntohs(m_adu.length);
			m_payload_size = m_adu.length - 2;
			dolog(8, "Data size: %zu", m_payload_size);
			break;
		case U_ID:
			m_state = FUNC;
			m_adu.unit_id = c;
			dolog(8, "Unit id: %d", (int)m_adu.unit_id);
			break;
		case FUNC:
			m_state = DATA;
			m_adu.pdu.func_code = c;
			dolog(8, "Func code: %d", (int)m_adu.pdu.func_code);
			break;
		case DATA:
			m_adu.pdu.data.push_back(c);
			m_payload_size -= 1;
			if (m_payload_size == 0) {
				m_tcp_handler->frame_parsed(m_adu, bufev);
				m_state = TR;
			}
			break;
		}

}

void tcp_parser::send_adu(unsigned short trans_id, unsigned char unit_id, PDU &pdu, struct bufferevent *bufev) {
	unsigned short proto_id = 0;
	unsigned short length;

	trans_id = htons(trans_id);
	length = htons(2 + pdu.data.size());
	
	bufferevent_write(bufev, &trans_id, sizeof(trans_id));
	bufferevent_write(bufev, &proto_id, sizeof(proto_id));
	bufferevent_write(bufev, &length, sizeof(length));
	bufferevent_write(bufev, &unit_id, sizeof(unit_id));
	bufferevent_write(bufev, &pdu.func_code, sizeof(pdu.func_code));
	bufferevent_write(bufev, &pdu.data[0], pdu.data.size());
}

void tcp_server::connection_accepted_cb(int fd, short event, void* server) {
	((tcp_server*) server)->accept_connection();
}

void tcp_server::connection_error_cb(struct bufferevent *bufev, short event, void* server) {
	((tcp_server*) server)->connection_error(bufev);
}

void tcp_server::connection_read_cb(struct bufferevent *bufev, void* server) {
	((tcp_server*) server)->connection_read(bufev);
}

void tcp_server::connection_read(struct bufferevent* bufev) {
	m_connections[bufev]->parser->read_data(bufev);
}

bool tcp_server::ip_allowed(struct sockaddr_in* addr) {
	if (m_allowed_ips.size() == 0)
		return true;

	for (size_t i = 0; i < m_allowed_ips.size(); i++)
		if (m_allowed_ips[i].s_addr == addr->sin_addr.s_addr)
			return true;

	return false;
}

void tcp_server::accept_connection() {
	struct sockaddr_in addr;
	socklen_t size = sizeof(addr);

do_accept:
	int fd = accept(m_listen_fd, (struct sockaddr*) &addr, &size);
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

	if (!ip_allowed(&addr)) {
		dolog(5, "Connection from: %s rejected, not on allowed list", inet_ntoa(addr.sin_addr));
		close(fd);
		return;
	}

	struct bufferevent* bufev = bufferevent_new(fd,
			connection_read_cb, 
			NULL, 
			connection_error_cb,
			this);

	m_connections[bufev] = new tcp_connection(fd, addr, bufev, new tcp_parser(this));

	bufferevent_base_set(m_event_base, bufev);
	bufferevent_enable(bufev, EV_READ | EV_WRITE | EV_PERSIST);
}

void tcp_server::connection_error(struct bufferevent *bufev) {
	tcp_connection* c = m_connections[bufev];

	dolog(5, "Terminating connection with: %s", inet_ntoa(c->addr.sin_addr));

	delete c->parser;
	close(c->fd);
	m_connections.erase(bufev);
	bufferevent_free(bufev);
	delete c;
}

int tcp_server::configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx) {
	if (modbus_daemon::configure(cfg, xp_ctx))
		return 1;

	xmlChar* c;
	c = get_device_node_prop(xp_ctx, "tcp-allowed");
	if (c != NULL) {
		int tokc = 0;
		char **toks;
		tokenize(SC::U2A(c).c_str(), &toks, &tokc);
		xmlFree(c);
		for (int i = 0; i < tokc; i++) {
			struct in_addr allowed_ip;
			int ret = inet_aton(toks[i], &allowed_ip);
			if (ret == 0) {
				dolog(0, "incorrect IP address '%s'", toks[i]);
				return 1;
			} else {
				dolog(5, "IP address '%s' allowed", toks[i]);
				m_allowed_ips.push_back(allowed_ip);
			}
		}
		tokenize(NULL, &toks, &tokc);
	}
	if (m_allowed_ips.size() == 0) {
		dolog(1, "warning: all IP allowed");
	}

	if (get_tcp_port(xp_ctx, &m_listen_port))
		return 1;

	return 0;
}

int tcp_server::initialize() {
	int ret;
	int on = 1;
	struct sockaddr_in addr;

	if (modbus_daemon::initialize())
		return 1;

	m_listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (m_listen_fd == -1) {
		dolog(0, "socket() failed, errno %d (%s)", errno, strerror(errno));
		return 1;
	}

	ret = setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, 
			&on, sizeof(on));
	if (ret == -1) {
		dolog(0, "setsockopt() failed, errno %d (%s)",
				errno, strerror(errno));
		close(m_listen_fd);
		m_listen_fd = -1;
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_listen_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	for (int i = 0; i < 30; i++) {
		ret = bind(m_listen_fd, (struct sockaddr *)&addr, sizeof(addr));
		if (ret == 0)
			break;
		if (errno == EADDRINUSE) {
			sz_log(1, "bind() failed, errno %d (%s), retrying",
				errno, strerror(errno));
			sleep(1);
			continue;
		}
		break;
	}
	if (ret == -1) {
		dolog(0, "bind() failed, errno %d (%s)",
				errno, strerror(errno));
		close(m_listen_fd);
		m_listen_fd = -1;
		return 1;
	}

	ret = listen(m_listen_fd, 1);
	if (ret == -1) {
		dolog(0, "listen() failed, errno %d (%s)",
				errno, strerror(errno));
		close(m_listen_fd);
		m_listen_fd = -1;
		return 1;
	}

	if (set_nonblock(m_listen_fd)) 
		return 1;

	event_set(&m_listen_event, m_listen_fd, EV_READ | EV_PERSIST, connection_accepted_cb, this);
	event_add(&m_listen_event, NULL);
	event_base_set(m_event_base, &m_listen_event);

	return 0;
}

void tcp_server::frame_parsed(TCPADU &adu, struct bufferevent* bufev) {
	if (process_request(adu.unit_id, adu.pdu) == true) {
		m_connections[bufev]->parser->send_adu(adu.trans_id, adu.unit_id, adu.pdu, bufev);
	}
}

modbus_client::modbus_client(client_connection *connection) {
	m_connection = connection;
	connection->set_modbus_client(this);
	m_state = IDLE;
	m_connected = false;
}

void modbus_client::send_write_query() {
	m_pdu.func_code = MB_F_WMR;	
	m_pdu.data.resize(0);
	m_pdu.data.push_back(m_start_addr >> 8);
	m_pdu.data.push_back(m_start_addr & 0xff);
	m_pdu.data.push_back(m_regs_count >> 8);
	m_pdu.data.push_back(m_regs_count & 0xff);
	m_pdu.data.push_back(m_regs_count * 2);

	dolog(7, "Sending write multiple registers command, start register: %hu, registers count: %hu", m_start_addr, m_regs_count);
	for (size_t i = 0; i < m_regs_count; i++) {
		unsigned short v = m_registers[m_unit][m_start_addr + i]->get_val();
		m_pdu.data.push_back(v >> 8);
		m_pdu.data.push_back(v & 0xff);
		dolog(7, "Sending register: %zu, value: %hu:", m_start_addr + i, v);
	}

	m_connection->send_pdu(m_unit, m_pdu);
}

void modbus_client::send_read_query() {
	dolog(7, "Sending read holding registers command, unit_id: %d,  address: %hu, count: %hu", (int)m_unit, m_start_addr, m_regs_count);
	switch (m_register_type) {
		case INPUT_REGISTER:
			m_pdu.func_code = MB_F_RIR;
			break;
		case HOLDING_REGISTER:
			m_pdu.func_code = MB_F_RHR;
			break;
	}
	m_pdu.data.resize(0);
	m_pdu.data.push_back(m_start_addr >> 8);
	m_pdu.data.push_back(m_start_addr & 0xff);
	m_pdu.data.push_back(m_regs_count >> 8);
	m_pdu.data.push_back(m_regs_count & 0xff);

	m_connection->send_pdu(m_unit, m_pdu);
}

void modbus_client::reset_cycle() {
	m_received_iterator = m_received.begin();
	m_sent_iterator = m_sent.begin();
}

void modbus_client::find_continous_reg_block(RSET::iterator &i, RSET &regs) {
	unsigned short current;
	ASSERT(i != regs.end());

	m_unit = i->first;
	m_start_addr = current = i->second.second;
	m_regs_count = 1;
	m_register_type = i->second.first;
	
	while (++i != regs.end() && m_regs_count < 125) {
		unsigned char c = i->first;
		if (c != m_unit)
			break;
		if (m_register_type != i->second.first)
			break;
		unsigned short addr = i->second.second;
		if (++current != addr)
			break;
		m_regs_count += 1;
	}
}

void modbus_client::next_query() {
	switch (m_state) {
		case AWAITING_CONNECTION:
		case IDLE:
			m_state = SENDING_READ_QUERY;
		case SENDING_READ_QUERY:
			if (m_received_iterator != m_received.end()) {
				find_continous_reg_block(m_received_iterator, m_received);
				break;
			} 
			m_state = SENDING_WRITE_QUERY;
		case SENDING_WRITE_QUERY:
			if (m_sent_iterator != m_sent.end()) {
				find_continous_reg_block(m_sent_iterator, m_sent);
				break;
			}
			m_state = IDLE;
			dolog(7, "Client querying cycle finished.");
	}
}

void modbus_client::send_query() {
	switch (m_state) {
		default:
			return;
		case SENDING_READ_QUERY:
			send_read_query();
			break;
		case SENDING_WRITE_QUERY:
			send_write_query();
			break;
	}
}

void modbus_client::pdu_received(unsigned char u, PDU &pdu) {
	switch (m_state) {
		case SENDING_READ_QUERY:
			try {
				consume_read_regs_response(m_unit, m_start_addr, m_regs_count, pdu);
			} catch (std::out_of_range) {	//message was distorted
				dolog(1, "Error while processing response - there is either a bug or sth was very wrong with request format");
			}
			send_next_query();
			break;
		case SENDING_WRITE_QUERY:
			try {
				consume_write_regs_response(m_unit, m_start_addr, m_regs_count, pdu);
			} catch (std::out_of_range) {	//message was distorted
				dolog(1, "Error while processing response - there is either a bug or sth was very wrong with request format");
			}
			send_next_query();
			break;
		default:
			dolog(10, "Received unexpected response from slave - ignoring it.");
			break;
	}
	m_last_activity = m_current_time;
}

void modbus_client::send_next_query() {

	switch (m_state) {
		case IDLE:
		case SENDING_READ_QUERY:
		case SENDING_WRITE_QUERY:
			next_query();
			send_query();
			break;
		default:
			ASSERT(false);
			break;
	}
}

void modbus_client::start_cycle() {
	dolog(7, "Starting cycle");
	reset_cycle();
	send_next_query();
}

void modbus_client::timer_event() {
	modbus_daemon::timer_event();

	switch (m_state)  {
		case IDLE:
			if (m_connected) {
				start_cycle();
			} else {
				m_state = AWAITING_CONNECTION;
				m_connection->connect();
			}
			return;
		case SENDING_READ_QUERY:
			dolog(2, "New cycle started but we are in read cycle");
			break;
		case SENDING_WRITE_QUERY:
			dolog(2, "New cycle started but we are in read cycle");
			break;
		case AWAITING_CONNECTION:
			dolog(2, "New cycle but we still wait for connection");
			break;
	}

	if (m_last_activity + 20 < m_current_time) {
		dolog(1, "Connection idle for too long, reconnecting");
		m_last_activity = m_current_time;
		m_connection->disconnect();
		m_connected = false;
		m_state = IDLE;
	} else if (m_last_activity + 3 < m_current_time) {
		dolog(1, "Answer did not arrive, sending next query");
		send_next_query();
	}
}

int modbus_client::configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx) {
	if (modbus_daemon::configure(cfg, xp_ctx))
		return 1;
	if (m_connection->configure(cfg, xp_ctx))
		return 1;

	m_last_activity = time(NULL);
	return 0;
}

void modbus_client::connection_failed() {
	dolog(2, "Connection terminated, waiting for cycle to finish and will try again");
	m_connected = false;
	m_state = IDLE;
}

void modbus_client::connection_ready() {
	dolog(7, "Connection ready");

	m_connected = true;

	if (m_state == AWAITING_CONNECTION) {
		m_state = IDLE;
		start_cycle();
	}
}

int modbus_client::initialize() {
	return modbus_daemon::initialize();

	m_state = IDLE;
	return 0;
}

void modbus_client::timeout() {
	switch (m_state) {
		case SENDING_READ_QUERY:
			dolog(1, "Timeout while reading data, unit_id: %d, address: %hu, registers count: %hu, progressing with queries",
					(int)m_unit, m_start_addr, m_regs_count);

			send_next_query();
			break;
		case SENDING_WRITE_QUERY:
			dolog(1, "Timeout while writing data, unit_id: %d, address: %hu, registers count: %hu, progressing with queries",
					(int)m_unit, m_start_addr, m_regs_count);
			send_next_query();
			break;
		default:
			dolog(1, "Received unexpected timeout from connection, ignoring.");
			break;
	}
}

int tcp_client_connection::configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx) {

	xmlChar *c;
	c = get_device_node_prop(xp_ctx, "tcp-address");
	if (c == NULL) {
		dolog(0, "missing server's addres");
		return 1;
	}
	if (inet_aton((char*) c, &m_connection.addr.sin_addr) == 0) {
		dolog(0, "incorrect server's IP address '%s'", (char*) c);
		return 1;
	}
	xmlFree(c);

	if (get_tcp_port(xp_ctx, &m_port))
		return 1;
	
	m_connection.addr.sin_family = AF_INET;
	m_connection.addr.sin_port = htons(m_port);
	m_connection.parser = new tcp_parser(this);
	m_connection.fd = -1;
	return 0;
}

void tcp_client_connection::disconnect() {
	if (m_connection.fd >= 0) {
		bufferevent_free(m_connection.bufev);
		m_connection.bufev = NULL;

		close(m_connection.fd);
		m_connection.fd = -1;
	}
}

void tcp_client_connection::connect() {
	int ret;

	disconnect();
	m_connection.fd = socket(PF_INET, SOCK_STREAM, 0);
	if (m_connection.fd == -1) {
		dolog(1, "Faile to create connection socket, error: %s", strerror(errno));
		m_modbus_client->connection_failed(); 
		return;
	}

	if (set_nonblock(m_connection.fd)) {
		dolog(1, "Failed to set non blocking mode on socket");
		exit(1);
	}

do_connect:
	ret = ::connect(m_connection.fd, (struct sockaddr*) &m_connection.addr, sizeof(m_connection.addr));
	if (ret == 0) {
		m_connect_waiting = false;
		m_modbus_client->connection_ready();
	} else if (errno == EINTR) {
		goto do_connect;
	} else if (errno == EWOULDBLOCK || errno == EINPROGRESS) {
		m_connect_waiting = true;
	} else {
		dolog(0, "Failed to connect: %s", strerror(errno));
		m_modbus_client->connection_failed();
		return;
	}

	m_connection.bufev = bufferevent_new(m_connection.fd, connection_read_cb, connection_write_cb, connection_error_cb, this);
	bufferevent_base_set(m_modbus_client->get_event_base(), m_connection.bufev);
	bufferevent_enable(m_connection.bufev, EV_READ | EV_WRITE | EV_PERSIST);
	m_trans_id = 0;

}

void tcp_client_connection::send_pdu(unsigned char uid, PDU& pdu) {
	m_connection.parser->reset();
	m_connection.parser->send_adu(m_trans_id++, uid, pdu, m_connection.bufev);
}

void tcp_client_connection::frame_parsed(TCPADU &adu, struct bufferevent* bufev) {
	m_modbus_client->pdu_received(adu.unit_id, adu.pdu);
}

void tcp_client_connection::connection_read() {
	m_connection.parser->read_data(m_connection.bufev);
}

void tcp_client_connection::connection_error() {
	disconnect();
	m_modbus_client->connection_failed();
}

void tcp_client_connection::connection_write() {
	if (m_connect_waiting) {
		m_modbus_client->connection_ready();
		m_connect_waiting = false;
	}
}

void tcp_client_connection::connection_read_cb(struct bufferevent *ev, void* client) {
	((tcp_client_connection*) client)->connection_read();
}

void tcp_client_connection::connection_write_cb(struct bufferevent *ev, void* client) {
	((tcp_client_connection*) client)->connection_write();
}

void tcp_client_connection::connection_error_cb(struct bufferevent *ev, short event, void* client) {
	((tcp_client_connection*) client)->connection_error();
}

void serial_rtu_parser::calculate_crc_table() {
	unsigned short crc, c;

	for (size_t i = 0; i < 256; i++) {
		crc = 0;
		c = (unsigned short) i;
		for (size_t j = 0; j < 8; j++) {
			if ((crc ^ c) & 0x0001)
				crc = (crc >> 1) ^ 0xa001;
			else
				crc = crc >> 1;
			c = c >> 1;
		}
		crc16[i] = crc;
	}

	crc_table_calculated = true;
}

unsigned short serial_rtu_parser::update_crc(unsigned short crc, unsigned char c) {
	if (crc_table_calculated == false)
		calculate_crc_table();
	unsigned short tmp;
	tmp = crc ^ c;
	crc = (crc >> 8) ^ crc16[tmp & 0xff];
	return crc;
}

bool serial_rtu_parser::check_crc() {
	if (m_data_read <= 2)
		return false;
	std::vector<unsigned char>& d = m_sdu.pdu.data;
	unsigned short crc = 0xffff;
	crc = update_crc(crc, m_sdu.unit_id);
	crc = update_crc(crc, m_sdu.pdu.func_code);

	for (size_t i = 0; i < m_data_read - 2; i++) 
		crc = update_crc(crc, m_sdu.pdu.data[i]);

	unsigned short frame_crc = d[m_data_read - 2] | (d[m_data_read - 1] << 8);
	dolog(9, "Checking crc, result: %s, unit_id: %d, calculated crc: %hx, frame crc: %hx",
	       	(crc == frame_crc ? "OK" : "ERROR"), m_sdu.unit_id, crc, frame_crc);
#if 0
	if (crc != frame_crc) {
	    char * s = (char *) malloc(sizeof(char) * 3 * m_data_read + 256);
	    if (NULL != s) {
		int pos = 0;
    		pos += sprintf(s, "Frame dump: id: %hhx func: %hhx data: ", m_sdu.unit_id, m_sdu.pdu.func_code);
		for (size_t j = 0; j < m_data_read - 2; j++) {
		    pos += sprintf(s + pos, "%hhx ", m_sdu.pdu.data[j]);
		}
		dolog(10, s);
		free(s);
	    }
	}
#endif
	return crc == frame_crc;
}


serial_parser::serial_parser(serial_connection_handler *serial_handler, int delay_between_chars, int read_timeout) : m_serial_handler(serial_handler), m_read_timer_started(false), m_timeout(read_timeout), m_delay_between_chars(delay_between_chars), m_write_timer_started(false) {
	evtimer_set(&m_read_timer, read_timer_callback, this);
	event_base_set(m_serial_handler->get_event_base(), &m_read_timer);
	if (m_delay_between_chars)
		evtimer_set(&m_write_timer, write_timer_callback, this);
}

void serial_parser::read_timer_callback(int fd, short event, void* parser) {
	((serial_parser*) parser)->read_timer_event();
}

void serial_parser::write_timer_callback(int fd, short event, void* parser) {
	((serial_parser*) parser)->write_timer_event();
}

void serial_parser::write_timer_event() {
	if (m_delay_between_chars && m_output_buffer.size()) {
		unsigned char c = m_output_buffer.front();
		m_output_buffer.pop_front();
		bufferevent_write(m_serial_handler->get_buffer_event(), &c, 1);
		if (m_output_buffer.size())
			start_write_timer();
		else
			stop_write_timer();
	}
	
}

void serial_parser::write_finished(struct bufferevent *bufev) {
	if (m_delay_between_chars && m_output_buffer.size())
		start_write_timer();
}

void serial_parser::stop_write_timer() {
	if (m_write_timer_started) {
		event_del(&m_write_timer);
		m_write_timer_started = false;
	}
}

void serial_parser::start_write_timer() {
	stop_write_timer();

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = m_delay_between_chars;
	evtimer_add(&m_write_timer, &tv);

	m_write_timer_started = true;
}

void serial_parser::stop_read_timer() {
	if (m_read_timer_started) {
		event_del(&m_read_timer);
		m_read_timer_started = false;
	}
}

void serial_parser::start_read_timer() {
	stop_read_timer();

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = m_timeout;
	evtimer_add(&m_read_timer, &tv); 
	m_read_timer_started = true;
}

serial_rtu_parser::serial_rtu_parser(serial_connection_handler *serial_handler, int delay_between_chars, int speed, int read_timeout) : serial_parser(serial_handler, delay_between_chars, read_timeout), m_state(FUNC_CODE) {
	/*according to protocol specification, intra-character
	 * delay cannot exceed 1.5 * (time of transmittion of one character),
	 * we will make it double to be on safe side */
	if (m_timeout == 0) {
		if (speed)
			m_timeout = 5 * 1000000 / (speed / 10);
		else
			m_timeout = 10000;
	}
	dolog(9, "serial_rtu_parser m_timeout: %d", m_timeout);
}

void serial_rtu_parser::reset() {
	m_state = ADDR;	
	stop_write_timer();
	stop_read_timer();
	m_output_buffer.resize(0);
}

void serial_rtu_parser::read_data(struct bufferevent *bufev) {
	size_t r;

	stop_read_timer();
	switch (m_state) {
		case ADDR:
			if (bufferevent_read(bufev, &m_sdu.unit_id, sizeof(m_sdu.unit_id)) == 0) 
				break;
			dolog(9, "Parsing new frame, unit_id: %d", (int) m_sdu.unit_id);
			m_state = FUNC_CODE;
		case FUNC_CODE:
			if (bufferevent_read(bufev, &m_sdu.pdu.func_code, sizeof(m_sdu.pdu.func_code)) == 0) {
				start_read_timer();
				break;
			}
			dolog(9, "\tfunc code: %d", (int) m_sdu.pdu.func_code);
			m_state = DATA;
			m_data_read = 0;
			m_sdu.pdu.data.resize(max_frame_size - 2);
		case DATA:
			r = bufferevent_read(bufev, &m_sdu.pdu.data[m_data_read], m_sdu.pdu.data.size() - m_data_read);
			m_data_read += r;
			if (check_crc()) {
				m_state = ADDR;
				m_sdu.pdu.data.resize(m_data_read - 2);
				m_serial_handler->frame_parsed(m_sdu, bufev);
			} else {
				start_read_timer();
			}
	}

}

void serial_rtu_parser::read_timer_event() {
	reset();
	m_serial_handler->timeout();
}

void serial_rtu_parser::send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev) {
	unsigned short crc = 0xffff;
	crc = update_crc(crc, unit_id);
	crc = update_crc(crc, pdu.func_code);
	for (size_t i = 0; i < pdu.data.size(); i++)
		crc = update_crc(crc, pdu.data[i]);

	unsigned char cl, cm;
	cl = crc & 0xff;
	cm = crc >> 8;

	if (m_delay_between_chars) {
		m_output_buffer.resize(0);
		m_output_buffer.push_back(pdu.func_code);
		m_output_buffer.insert(m_output_buffer.end(), pdu.data.begin(), pdu.data.end());
		m_output_buffer.push_back(cl);
		m_output_buffer.push_back(cm);

		bufferevent_write(bufev, &unit_id, sizeof(unit_id));
	} else {
		bufferevent_write(bufev, &unit_id, sizeof(unit_id));
		bufferevent_write(bufev, &pdu.func_code, sizeof(pdu.func_code));
		bufferevent_write(bufev, &pdu.data[0], pdu.data.size());
		bufferevent_write(bufev, &cl, 1);
		bufferevent_write(bufev, &cm, 1);
	}
}

int get_serial_config(xmlXPathContextPtr xp_ctx, DaemonConfig* cfg, serial_port_configuration &spc) {

	spc.speed = cfg->GetDevice()->getAttribute<int>("speed");
	spc.path = cfg->GetDevice()->getAttribute("path");

	xmlChar* parity = get_device_node_prop(xp_ctx, "parity");
	if (parity == NULL) {
		dolog(10, "Serial port configuration, parity not specified, assuming no parity");
		spc.parity = serial_port_configuration::NONE;
	} else if (!xmlStrcasecmp(parity, BAD_CAST "none")) {
		dolog(10, "Serial port configuration, none parity");
		spc.parity = serial_port_configuration::NONE;
	} else if (!xmlStrcasecmp(parity, BAD_CAST "even")) {
		dolog(10, "Serial port configuration, even parity");
		spc.parity = serial_port_configuration::EVEN;
	} else if (!xmlStrcasecmp(parity, BAD_CAST "odd")) {
		dolog(10, "Serial port configuration, odd parity");
		spc.parity = serial_port_configuration::ODD;
	} else {
		dolog(0, "Unsupported parity %s, confiugration invalid!!!", (char*) parity);
		return 1;	
	}
	xmlFree(parity);

	xmlChar* stop_bits = get_device_node_prop(xp_ctx, "stopbits");
	if (stop_bits == NULL) {
		dolog(10, "Serial port configuration, stop bits not specified, assuming 1 stop bit");
		spc.stop_bits = 1;
	} else if (xmlStrcmp(stop_bits, BAD_CAST "1")) {
		dolog(10, "Serial port configuration, setting one stop bit");
		spc.stop_bits = 1;
	} else if (xmlStrcmp(stop_bits, BAD_CAST "2")) {
		dolog(10, "Serial port configuration, setting two stop bits");
		spc.stop_bits = 2;
	} else {
		dolog(0, "Unsupported number of stop bits %s, confiugration invalid!!!", (char*) stop_bits);
		return 1;
	}
	xmlFree(stop_bits);

	xmlChar* delay_between_chars = get_device_node_prop(xp_ctx, "out-intra-character-delay");
	if (delay_between_chars == NULL) {
		dolog(10, "Serial port configuration, delay between chars not given assuming no delay");
		spc.delay_between_chars = 0;
	} else {
		spc.delay_between_chars = atoi((char*) delay_between_chars) * 1000;
		dolog(10, "Serial port configuration, delay between chars set to %d miliseconds", spc.delay_between_chars);
	}
	xmlFree(delay_between_chars);

	xmlChar* read_timeout = get_device_node_prop(xp_ctx, "read-timeout");
	if (read_timeout == NULL) {
		dolog(10, "Serial port configuration, read timeout not given, will use one based on speed");
		spc.read_timeout = 0;
	} else {
		spc.read_timeout = atoi((char*) read_timeout) * 1000;
		dolog(10, "Serial port configuration, read timeout set to %d miliseconds", spc.read_timeout);
	}
	xmlFree(read_timeout);

	return 0;
}

int open_serial_port(serial_port_configuration& spc) {
	int fd = open(spc.path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK, 0);
	if (fd == -1) {
		dolog(0, "Failed do open port: %s, error: %s", spc.path.c_str(), strerror(errno));
		return -1;
	}
	
	struct termios ti;
	if (tcgetattr(fd, &ti) == -1) {
		dolog(1, "Cannot retrieve port settings, errno:%d (%s)", errno, strerror(errno));
		close(fd);
		return -1;
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
		default:
			ti.c_cflag = B9600;
			dolog(8, "setting port speed to default value 9600");
	}

	if (spc.stop_bits == 2)
		ti.c_cflag |= CSTOPB;

	switch (spc.parity) {
		case serial_port_configuration::EVEN:
			ti.c_cflag |= PARENB;
			break;
		case serial_port_configuration::ODD:
			ti.c_cflag |= PARODD;
			break;
		case serial_port_configuration::NONE:
			break;
	}
			
	ti.c_oflag = 
	ti.c_iflag =
	ti.c_lflag = 0;

	ti.c_cflag |= CS8 | CREAD | CLOCAL ;

	if (tcsetattr(fd, TCSANOW, &ti) == -1) {
		dolog(1,"Cannot set port settings, errno: %d (%s)", errno, strerror(errno));	
		close(fd);
		return -1;
	}

	return fd;
}

int serial_client_connection::configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx) {
	m_fd = -1;

	if (get_serial_config(xp_ctx, cfg, m_spc))
		return 1;
	m_parser = new serial_rtu_parser(this, m_spc.delay_between_chars, m_spc.speed, m_spc.read_timeout);
	return 0;
}

void serial_client_connection::disconnect() {
	if (m_fd >= 0) {
		bufferevent_free(m_bufev);
		close(m_fd);
		m_fd = -1;
	}
}

void serial_client_connection::connect() {
	disconnect();

	m_fd = open_serial_port(m_spc);
	if (m_fd == -1) {
		m_modbus_client->connection_failed();
		return;
	}

	m_parser->reset();

	m_bufev = bufferevent_new(m_fd, connection_read_cb, connection_write_cb, connection_error_cb, this);
	bufferevent_base_set(get_event_base(), m_bufev);
	bufferevent_enable(m_bufev, EV_READ | EV_WRITE | EV_PERSIST);

	m_modbus_client->connection_ready();
}

void serial_client_connection::send_pdu(unsigned char u, PDU &pdu) {
	m_parser->send_sdu(u, pdu, m_bufev);
}

void serial_client_connection::timeout() {
	dolog(1, "Timeout while waiting for valid frame");
	m_modbus_client->timeout();
}

void serial_client_connection::frame_parsed(SDU &sdu, struct bufferevent *bufev) {
	m_modbus_client->pdu_received(sdu.unit_id, sdu.pdu);
}

struct bufferevent* serial_client_connection::get_buffer_event() {
	return m_bufev;
}

struct event_base* serial_client_connection::get_event_base() {
	return m_modbus_client->get_event_base();
}

void serial_client_connection::connection_read(struct bufferevent *bufev) {
	m_parser->read_data(bufev);
}

void serial_client_connection::connection_write(struct bufferevent *bufev) {
	m_parser->write_finished(bufev);
}

void serial_client_connection::connection_error(struct bufferevent *bufev) {
	disconnect();
	m_modbus_client->connection_failed();
}

void serial_client_connection::connection_read_cb(struct bufferevent *ev, void* client) {
	((serial_client_connection*) client)->connection_read(ev);
}

void serial_client_connection::connection_write_cb(struct bufferevent *ev, void* client) {
	((serial_client_connection*) client)->connection_write(ev);
}

void serial_client_connection::connection_error_cb(struct bufferevent *ev, short event, void* client) {
	((serial_client_connection*) client)->connection_error(ev);
}

serial_server::serial_server() {
	m_fd = -1;
	m_parser = new serial_rtu_parser(this, m_spc.delay_between_chars, m_spc.speed, m_spc.read_timeout);
}

int serial_server::configure(DaemonConfig *cfg, xmlXPathContextPtr xp_ctx) {
	if (modbus_daemon::configure(cfg, xp_ctx))
		return 1;

	if (get_serial_config(xp_ctx, cfg, m_spc))
		return 1;

	return 0;
}

void serial_server::close_connection() {
	if (m_fd >= 0) {
		close(m_fd);
		m_fd = -1;
		bufferevent_free(m_bufev);
	}
}

bool serial_server::open_connection() {
	m_fd = open_serial_port(m_spc);
	if (m_fd == -1) {
		dolog(0, "Failed to open serial port, initialization failed. exiting");
		return false;
	}

	m_bufev = bufferevent_new(m_fd, connection_read_cb, connection_write_cb, connection_error_cb, this);
	bufferevent_base_set(m_event_base, m_bufev);
	bufferevent_enable(m_bufev, EV_READ | EV_WRITE | EV_PERSIST);

	m_parser->reset();
	
	return true;
}

int serial_server::initialize() {
	if (modbus_daemon::initialize())
		return 1;
	if (!open_connection())
		return 1;
	return 0;
}

struct event_base* serial_server::get_event_base() {
	return modbus_daemon::get_event_base();
}

void serial_server::timeout() {
	dolog(1, "Received incomplete/invalid frame from server, igonred");
}

void serial_server::frame_parsed(SDU &sdu, struct bufferevent* bufev) {
	if (modbus_daemon::process_request(sdu.unit_id, sdu.pdu))
		m_parser->send_sdu(sdu.unit_id, sdu.pdu, bufev);
}

struct bufferevent* serial_server::get_buffer_event() {
	return m_bufev;
}

void serial_server::connection_read(struct bufferevent *bufev) {
	m_parser->read_data(bufev);
}

void serial_server::connection_write(struct bufferevent *bufev) {
	m_parser->write_finished(bufev);
}

void serial_server::connection_error(struct bufferevent* bufev) {
	dolog(1, "Connection read error, reopening serial port");
	close_connection();
	open_connection();
}

void serial_server::connection_error_cb(struct bufferevent *bufev, short event, void *server) {
	((serial_server*) server)->connection_error(bufev);
}

void serial_server::connection_read_cb(struct bufferevent *bufev, void *server) {
	((serial_server*) server)->connection_read(bufev);
}

void serial_server::connection_write_cb(struct bufferevent *bufev, void *server) {
	((serial_server*) server)->connection_write(bufev);
}

int configure_daemon_mode(xmlXPathContextPtr xp_ctx, DaemonConfig *cfg, modbus_daemon **dmn) {
	int ret = 0;
	xmlChar* mode = get_device_node_prop(xp_ctx, "daemon-mode");
	if (mode == NULL) {
		dolog(0, "Unspecified driver mode, configuration not vaild, exiting!");
		return 1;
	}

	*dmn = NULL;
	dolog(9, "Configuring mode: %s", (char*) mode);
	if (!xmlStrcmp(mode, BAD_CAST "tcp-server")) {
		*dmn = new tcp_server();
	} else if (!xmlStrcmp(mode, BAD_CAST "serial-server")) {
		*dmn = new serial_server();
	} else if (!xmlStrcmp(mode, BAD_CAST "tcp-client")) {
		*dmn = new modbus_client(new tcp_client_connection());
	} else if (!xmlStrcmp(mode, BAD_CAST "serial-client")) {
		*dmn = new modbus_client(new serial_client_connection());
	} else {
		dolog(0, "Unknown mode: %s, exiting", (char*) mode);
		ret = 1;
	}
	xmlFree(mode);

	if (*dmn) {
		ret = (*dmn)->configure(cfg, xp_ctx);
		if (ret)
			delete *dmn;
	}
	return ret;
}

int configure_daemon(modbus_daemon **dmn, DaemonConfig *cfg) {
	int ret;
	xmlDocPtr doc;

	/* get config data */
	ASSERT(cfg != NULL);
	doc = cfg->GetXMLDoc();
	ASSERT(doc != NULL);

	/* prepare xpath */
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(doc);
	ASSERT(xp_ctx != NULL);

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);
	(void)ret;
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "modbus",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	ASSERT(ret == 0);
	(void)ret;

	xp_ctx->node = cfg->GetXMLDevice();

	return configure_daemon_mode(xp_ctx, cfg, dmn);
}

int main(int argc, char *argv[]) {

	DaemonConfig   *cfg;

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);

	cfg = new DaemonConfig("mbdmn");
	ASSERT(cfg != NULL);
	
	if (cfg->Load(&argc, argv))
		return 1;

	signal(SIGPIPE, SIG_IGN);

	g_debug = cfg->GetDiagno() || cfg->GetSingle();

	modbus_daemon *daemon;
	if (configure_daemon(&daemon, cfg))
		return 1;

	if (daemon->initialize())
		return 1;

	dolog(2, "Starting Modbus Daemon");

	daemon->go();

	dolog(0, "Error: daemon's event loop exited - that shouldn't happen!");
	return 1;
}

void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	if (g_debug) {
		char *l;
		va_start(fmt_args, fmt);
		const int ret = vasprintf(&l, fmt, fmt_args);
		(void)ret;
		va_end(fmt_args);

		std::cout << l << std::endl;
		sz_log(level, "%s", l);
		free(l);
	} else {
		va_start(fmt_args, fmt);
		vsz_log(level, fmt, fmt_args);
		va_end(fmt_args);
	}

} 

