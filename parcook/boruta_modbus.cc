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
#include <set>
#include <deque>
#include <sys/types.h>
#include <event.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "liblog.h"
#include "xmlutils.h"
#include "conversion.h"
#include "ipchandler.h"
#include "tokens.h"
#include "borutadmn.h"


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

class modbus_unit;
class modbus_register {
	modbus_unit *m_modbus_unit;
	unsigned short m_val;
	time_t m_mod_time;
public:
	modbus_register(modbus_unit *daemon);
	void set_val(unsigned short val, time_t time);
	unsigned short get_val(bool &valid);
	unsigned short get_val();
};

class modbus_unit {
protected:
	URMAP m_registers;

	RSET m_received;
	RSET m_sent;

	std::vector<short*> m_reads;
	std::vector<size_t> m_reads_count;
	std::vector<short*> m_sends;
	std::vector<size_t> m_sends_count;

	std::vector<parcook_modbus_val_op*> m_parcook_ops;
	std::vector<sender_modbus_val_op*> m_sender_ops;
	std::vector<modbus_register*> m_send_map;

	enum FLOAT_ORDER { LSWMSW, MSWLSW } m_float_order;

	time_t m_current_time;
	time_t m_expiration_time;
	float m_nodata_value;

	bool process_request(unsigned char unit, PDU &pdu);
	int configure_unit(TUnit* u, xmlXPathContextPtr xp_ctx);
	const char* error_string(const unsigned char& error);
	void consume_read_regs_response(unsigned char& unit, unsigned short start_addr, unsigned short regs_count, PDU &pdu);
	void consume_write_regs_response(unsigned char& unit, unsigned short start_addr, unsigned short regs_count, PDU &pdu);
	bool perform_write_registers(RMAP &unit, PDU &pdu);
	bool perform_read_holding_regs(RMAP &unit, PDU &pdu);
	bool create_error_response(unsigned char error, PDU &pdu);
	void to_parcook();
	void from_sender();
public:
	bool register_val_expired(time_t time);
	virtual int initialize();
	int configure_unit(TUnit* u, xmlNodePtr node);
	int configure(TUnit *unit, xmlNodePtr node, short *read, short *send);
	void starting_new_cycle();
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

class tcp_server : public modbus_unit, public tcp_connection_handler, public tcp_server_driver {
	std::vector<struct in_addr> m_allowed_ips;
	std::map<struct bufferevent*, tcp_parser*> m_parsers;

	bool ip_allowed(struct sockaddr_in* addr);
public:
	virtual void frame_parsed(TCPADU &adu, struct bufferevent* bufev);
	virtual int configure(TUnit* unit, xmlNodePtr node, short *read, short *send);
	virtual int initialize();
	virtual void connection_error(struct bufferevent* bufev);
	virtual void data_ready(struct bufferevent* bufev);
	virtual int connection_accepted(struct bufferevent* bufev, int socket, struct sockaddr_in *addr);
	virtual void starting_new_cycle();
};

class modbus_client : public modbus_unit {
protected:
	RSET::iterator m_received_iterator;
	RSET::iterator m_sent_iterator;

	PDU m_pdu;

	unsigned short m_start_addr;
	unsigned short m_regs_count;
	unsigned char m_unit;
	REGISTER_TYPE m_register_type;

	enum {
		READ_QUERY_SENT,
		WRITE_QUERY_SENT,
		IDLE,
	} m_state;

	time_t m_last_activity;

protected:
	void reset_cycle();
	void start_cycle();
	void send_next_query();
	void send_write_query();
	void send_read_query();
	void next_query();
	void send_query();
	void find_continous_reg_block(RSET::iterator &i, RSET &regs);
	void timeout();

	virtual void send_pdu(unsigned char unit, PDU &pdu) = 0;
	virtual void cycle_finished() = 0;
	virtual void terminate_connection() = 0;

public:
	void pdu_received(unsigned char u, PDU &pdu);
	virtual int initialize();
};

class modbus_tcp_client : public tcp_connection_handler, public tcp_client_driver, public modbus_client {
	tcp_parser *m_parser;
	struct bufferevent* m_bufev; 
	unsigned char m_trans_id;
protected:
	virtual void send_pdu(unsigned char unit, PDU &pdu);
	virtual void cycle_finished();
	virtual void terminate_connection();
public:
	virtual void frame_parsed(TCPADU &adu, struct bufferevent* bufev);
	virtual void connection_error(struct bufferevent *bufev);
	virtual void scheduled(struct bufferevent* bufev, int fd);
	virtual void data_ready(struct bufferevent* bufev, int fd);
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send);
	virtual void timeout();
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
	serial_parser(serial_connection_handler *serial_handler);
	int configure(xmlNodePtr node, serial_port_configuration &spc);
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
	serial_rtu_parser(serial_connection_handler *serial_handler);
	void read_data(struct bufferevent *bufev);
	void send_sdu(unsigned char unit_id, PDU &pdu, struct bufferevent *bufev);
	void reset();

	static const int max_frame_size = 256;
};


class modbus_serial_client : public serial_connection_handler, public serial_client_driver, public modbus_client {
	serial_parser *m_parser;
	struct bufferevent* m_bufev;
protected:
	virtual void send_pdu(unsigned char unit, PDU &pdu);
	virtual void cycle_finished();
	virtual void terminate_connection();
public:

	virtual void connection_error(struct bufferevent *bufev);
	virtual void scheduled(struct bufferevent* bufev, int fd);
	virtual void data_ready(struct bufferevent* bufev, int fd);
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration &spc);
	virtual void frame_parsed(SDU &adu, struct bufferevent* bufev);
	virtual void timeout();
	virtual struct event_base* get_event_base();
	virtual struct bufferevent* get_buffer_event();
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

class serial_server : public modbus_unit, public serial_connection_handler, public serial_server_driver {
	serial_rtu_parser* m_parser;
	struct bufferevent *m_bufev;

public:
	virtual int configure(TUnit *unit, xmlNodePtr node, short *read, short *send, serial_port_configuration &spc);
	virtual int initialize();
	virtual void frame_parsed(SDU &sdu, struct bufferevent* bufev);
	virtual struct bufferevent* get_buffer_event();

	virtual void connection_error(struct bufferevent* bufev);
	virtual void data_ready(struct bufferevent* bufev);
	virtual struct event_base* get_event_base();
	virtual void timeout();
	virtual void starting_new_cycle();
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

class float_sender_modbus_val_op : public sender_modbus_val_op {
	modbus_register *m_reg_lsw;
	modbus_register *m_reg_msw;
	int m_prec;
public:
	float_sender_modbus_val_op(float no_data, modbus_register *reg_lsw, modbus_register *reg_msw, int prec);
	virtual void set_val(short val, time_t time);
};


//implementation

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

modbus_register::modbus_register(modbus_unit* unit) : m_modbus_unit(unit), m_val(SZARP_NO_DATA), m_mod_time(-1) {}

unsigned short modbus_register::get_val(bool &valid) {
	if (m_mod_time < 0)
		valid = false;
	else
		valid = m_modbus_unit->register_val_expired(m_mod_time);
	return m_val;
}

unsigned short modbus_register::get_val() {
	return m_val;
}

void modbus_register::set_val(unsigned short val, time_t time) {
	m_val = val;
	m_mod_time = time;
}

bool modbus_unit::process_request(unsigned char unit, PDU &pdu) {
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

const char* modbus_unit::error_string(const unsigned char& error) {
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

void modbus_unit::consume_read_regs_response(unsigned char& uid, unsigned short start_addr, unsigned short regs_count, PDU &pdu) {
	dolog(5, "Consuming read holing register response unit_id: %d, address: %hu, registers count: %hu", (int) uid, start_addr, regs_count);
	if (pdu.func_code & MB_ERROR_CODE) {
		dolog(1, "Exception received in response to read holding registers command, unit_id: %d, address: %hu, count: %hu",
		       	(int)uid, start_addr, regs_count);
		dolog(1, "Error is: %s(%d)", error_string(pdu.data.at(0)), (int)pdu.data.at(0));
		return;
	} 

	URMAP::iterator i = m_registers.find(uid);
	assert(i != m_registers.end());
	RMAP& unit = i->second;

	size_t data_index = 1;

	for (size_t addr = start_addr; addr < start_addr + regs_count; addr++, data_index += 2) {
		RMAP::iterator j = unit.find(addr);
		unsigned short v = (pdu.data.at(data_index) << 8) | pdu.data.at(data_index + 1);
		dolog(7, "Setting register unit_id: %d, address: %hu, value: %hu", (int) uid, addr, v);
		j->second->set_val(v, m_current_time);
	}


}

void modbus_unit::consume_write_regs_response(unsigned char& unit, unsigned short start_addr, unsigned short regs_count, PDU &pdu) { 
	dolog(5, "Consuming write holding register response unit_id: %d, address: %hu, registers count: %hu", (int) unit, start_addr, regs_count);
	if (pdu.func_code & MB_ERROR_CODE) {
		dolog(1, "Exception received in response to write holding registers command, unit_id: %d, address: %hu, count: %hu",
		       	(int)unit, start_addr, regs_count);
		dolog(1, "Error is: %s(%d)", error_string(pdu.data.at(0)), (int)pdu.data.at(0));
	} else {
		dolog(5, "Write holding register command poisitively acknowledged");
	}
}

bool modbus_unit::perform_write_registers(RMAP &unit, PDU &pdu) {
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

		dolog(7, "Setting register: %hu value: %hu", addr, v);
		j->second->set_val(v, m_current_time);	
	}

	if (pdu.func_code == MB_F_WMR)
		d.resize(4);

	dolog(7, "Request processed successfully");

	return true;
}

bool modbus_unit::perform_read_holding_regs(RMAP &unit, PDU &pdu) {
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
		dolog(7, "Sending register: %hu, value: %hu", addr, v);
	}

	dolog(7, "Request processed sucessfully.");

	return true;
}

bool modbus_unit::create_error_response(unsigned char error, PDU &pdu) {
	pdu.func_code |= MB_ERROR_CODE;
	pdu.data.resize(1);
	pdu.data[0] = error;

	dolog(7, "Sending error response: %s", error_string(error));

	return true;
}

void modbus_unit::to_parcook() {
	std::vector<short*>::iterator i = m_reads.begin();
	std::vector<size_t>::iterator j = m_reads_count.begin();
	std::vector<parcook_modbus_val_op*>::iterator k = m_parcook_ops.begin();
	for (; i != m_reads.end(); i++, j++) {
		for (size_t m = 0; m < *j; m++, k++) {
			(*i)[m] = (*k)->val();
			dolog(9, "Parcook param no %zu set to %hu", m, (*i)[m]);
		}

	}
}

void modbus_unit::from_sender() {
	std::vector<short*>::iterator i = m_sends.begin();
	std::vector<size_t>::iterator j = m_sends_count.begin();
	std::vector<sender_modbus_val_op*>::iterator k = m_sender_ops.begin();
	for (; i != m_sends.end(); i++, j++)
		for (size_t m = 0; m < *j; m++, k++) {
			(*k)->set_val((*i)[m], m_current_time);
			dolog(9, "Setting %zu param from sender to %hd", m, (*i)[m]);
		}
}

bool modbus_unit::register_val_expired(time_t time) {
	if (m_expiration_time == 0)
		return true;
	else
		return time >= m_current_time - m_expiration_time;
}

int modbus_unit::configure_unit(TUnit* u, xmlNodePtr node) {
	int i, j;
	TParam *p;
	TSendParam *sp;
	char *c;

	unsigned char id = 0;
	get_xml_extra_prop(node, "id", id, true);
	if (!id)
		switch (u->GetId()) {
			case L'0'...L'9':
				id = u->GetId() - L'0';
				break;
			case L'a'...L'z':
				id = (u->GetId() - L'a') + 10;
				break;
			case L'A'...L'Z':
				id = (u->GetId() - L'A') + 10;
				break;
			default:
				id = u->GetId();
				break;
		}

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(node->doc);
	xp_ctx->node = node;
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "modbus", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	for (p = u->GetFirstParam(), sp = u->GetFirstSendParam(), i = 0, j = 0; i < u->GetParamsCount() + u->GetSendParamsCount(); i++, j++) {
		char *expr;
		if (i == u->GetParamsCount())
			j = 0;
		bool send = j != i;

	        asprintf(&expr, ".//ipk:%s[position()=%d]", i < u->GetParamsCount() ? "param" : "send", j + 1);
		xmlNodePtr node = uxmlXPathGetNode(BAD_CAST expr, xp_ctx);
		assert(node);
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

		std::string val_type;	
		if (get_xml_extra_prop(node, "val_type", val_type))
			return 1;

		TParam *param; 
		if (send) {
			param = u->GetSzarpConfig()->getParamByName(sp->GetParamName());
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
		if (val_type == "integer") {
			m_registers[id][addr] = new modbus_register(this);
			if (send) 
				vos = new short_sender_modbus_val_op(m_nodata_value, m_registers[id][addr]);
			else 
				vop = new short_parcook_modbus_val_op(m_registers[id][addr]);
			dolog(7, "Param %s mapped to unit: %lc, register %hu, value type: integer", SC::S2A(param->GetName()).c_str(), u->GetId(), addr);
		} else if (val_type == "bcd") {
			m_registers[id][addr] = new modbus_register(this);
			if (send) {
				dolog(0, "Unsupported bcd value type for send param no. %d", j + 1);
				return 1;
			}
			vop = new bcd_parcook_modbus_val_op(m_registers[id][addr]);
			dolog(7, "Param %s mapped to unit: %lc, register %hu, value type: bcd", SC::S2A(param->GetName()).c_str(), u->GetId(), addr);
		} else if (val_type == "long" || val_type == "float") {
			std::string val_op;
			get_xml_extra_prop(node, "val_op", val_op, false);

			FLOAT_ORDER float_order = m_float_order;

			std::string _float_order;
			get_xml_extra_prop(node, "FloatOrder", _float_order, false);
			if (!_float_order.empty()) {
				if (_float_order == "msblsb") {
					dolog(5, "Setting mswlsw for next param");
					float_order = MSWLSW;
				} else if (_float_order == "lsbmsb") {
					dolog(5, "Setting lswmsw for next param");
					float_order = LSWMSW;
				} else {
					dolog(0, "Invalid float order specification: %s, %ld", _float_order.c_str(), xmlGetLineNo(node));
					return 1;
				}
			}

			unsigned short msw, lsw;
			bool is_lsw;
			if (val_op.empty())  {
				if (float_order == MSWLSW) {
					msw = addr;
					other_reg = lsw = addr + 1;
				} else {
					other_reg = msw = addr + 1;
					lsw = addr;
				}
				is_lsw = true;
			} else if (val_op == "MSW") {
				if (float_order == MSWLSW) {
					msw = addr;
					lsw = addr + 1;
				} else {
					msw = addr + 1;
					lsw = addr;
				}
				other_reg = lsw;
				is_lsw = false;
			} else if (val_op == "LSW") {
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
				dolog(0, "Unsupported val_op attribute value - %s, line %ld", val_op.c_str(), xmlGetLineNo(node));
				return 1;
			}

			if (m_registers[id].find(lsw) == m_registers[id].end()) 
				m_registers[id][lsw] = new modbus_register(this);
			if (m_registers[id].find(msw) == m_registers[id].end())
				m_registers[id][msw] = new modbus_register(this);

			int prec = exp10(param->GetPrec());
			if (val_type == "float")  {
				if (!send) {
					vop = new long_parcook_modbus_val_op<float>(m_registers[id][lsw], m_registers[id][msw], prec, is_lsw);
					dolog(7, "Parcook param %s no(%zu), mapped to unit: %lc, register %hu, value type: float, params holds %s part, lsw: %hu, msw: %hu",
						SC::S2A(param->GetName()).c_str(), m_parcook_ops.size(), u->GetId(), addr, is_lsw ? "lsw" : "msw", lsw, msw);
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
	xmlXPathFreeContext(xp_ctx);
	return 0;
}

int modbus_unit::configure(TUnit *unit, xmlNodePtr node, short *read, short *send) {

	std::string float_order;
	get_xml_extra_prop(node, "FloatOrder", float_order, true);
	if (float_order.empty()) {
		dolog(5, "Float order not specified, assuming msblsb");
		m_float_order = MSWLSW;
	} else if (float_order == "msblsb") {
		dolog(5, "Setting mswlsw float order");
		m_float_order = MSWLSW;
	} else if (float_order == "lsbmsb") {
		dolog(5, "Setting lswmsw float order");
		m_float_order = LSWMSW;
	} else {
		dolog(0, "Invalid float order specification: %s", float_order.c_str());
		return 1;
	}

	m_expiration_time = 0;
	if (get_xml_extra_prop(node, "nodata-timeout", m_expiration_time, false)) {
		dolog(5, "invalid no-data timeout specified, error");
		return 1;
	}
	if (m_expiration_time) {
		dolog(5, "no-data timeout not specified (or 0), assuming no data expiration");
		m_expiration_time = 0;
	} 

	m_nodata_value = 0;
	if (get_xml_extra_prop(node, "nodata-value", m_nodata_value, true)) {
		dolog(5, "invalid nodata-value, not a float");
		return 1;
	}
	dolog(9, "No data value set to: %f", m_nodata_value);

	if (configure_unit(unit, node))
		return 1;

	m_reads.push_back(read);
	m_reads_count.push_back(unit->GetParamsCount());
	m_sends.push_back(send);
	m_sends_count.push_back(unit->GetSendParamsCount());

	return 0;
}

void modbus_unit::starting_new_cycle() {
	dolog(5, "Timer click, doing ipc data exchange");
	m_current_time = time(NULL);
	from_sender();
	to_parcook();
}

int modbus_unit::initialize() {
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
			dolog(8, "Data size: %hu", m_payload_size);
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

bool tcp_server::ip_allowed(struct sockaddr_in* addr) {
	if (m_allowed_ips.size() == 0)
		return true;
	for (size_t i = 0; i < m_allowed_ips.size(); i++)
		if (m_allowed_ips[i].s_addr == addr->sin_addr.s_addr)
			return true;

	return false;
}


void tcp_server::frame_parsed(TCPADU &adu, struct bufferevent* bufev) {
	if (process_request(adu.unit_id, adu.pdu) == true)
		m_parsers[bufev]->send_adu(adu.trans_id, adu.unit_id, adu.pdu, bufev);
}

int tcp_server::configure(TUnit* unit, xmlNodePtr node, short *read, short *send) {
	if (modbus_unit::configure(unit, node, read, send))
		return 1;
	std::string ips_allowed;
	get_xml_extra_prop(node, "tcp-allowed", ips_allowed, true);
	std::stringstream ss(ips_allowed);
	std::string ip_allowed;
	while (ss >> ip_allowed) {
		struct in_addr ip;
		int ret = inet_aton(ip_allowed.c_str(), &ip);
		if (ret == 0) {
			dolog(0, "incorrect IP address '%s'", ip_allowed.c_str());
			return 1;
		} else {
			dolog(5, "IP address '%s' allowed", ip_allowed.c_str());
			m_allowed_ips.push_back(ip);
		}
	}
	if (m_allowed_ips.size() == 0)
		dolog(1, "warning: all IP allowed");	
	return 0;
}

int tcp_server::initialize() {
	return 0;
}
	
void tcp_server::data_ready(struct bufferevent* bufev) {
	m_parsers[bufev]->read_data(bufev);
}

int tcp_server::connection_accepted(struct bufferevent* bufev, int socket, struct sockaddr_in *addr) {
	if (!ip_allowed(addr))
		return 1;
	m_parsers[bufev] = new tcp_parser(this);
	return 0;
}

void tcp_server::starting_new_cycle() {
	modbus_unit::starting_new_cycle();
}

void tcp_server::connection_error(struct bufferevent *bufev) {
	tcp_parser* parser = m_parsers[bufev];
	dolog(5, "Terminating connection");
	m_parsers.erase(bufev);
	delete parser;
}

void modbus_client::reset_cycle() {
	m_received_iterator = m_received.begin();
	m_sent_iterator = m_sent.begin();
}

void modbus_client::start_cycle() {
	dolog(7, "Starting cycle");
	send_next_query();
}

void modbus_client::send_next_query() {

	switch (m_state) {
		case IDLE:
		case READ_QUERY_SENT:
		case WRITE_QUERY_SENT:
			next_query();
			send_query();
			break;
		default:
			assert(false);
			break;
	}
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
		dolog(7, "Sending register: %hu, value: %hu:", m_start_addr + i, v);
	}

	send_pdu(m_unit, m_pdu);
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

	send_pdu(m_unit, m_pdu);
}

void modbus_client::next_query() {
	switch (m_state) {
		case IDLE:
			m_state = READ_QUERY_SENT;
		case READ_QUERY_SENT:
			if (m_received_iterator != m_received.end()) {
				find_continous_reg_block(m_received_iterator, m_received);
				break;
			} 
			m_state = WRITE_QUERY_SENT;
		case WRITE_QUERY_SENT:
			if (m_sent_iterator != m_sent.end()) {
				find_continous_reg_block(m_sent_iterator, m_sent);
				break;
			}
			m_state = IDLE;
			reset_cycle();
			cycle_finished();
			break;
	}
}

void modbus_client::send_query() {
	switch (m_state) {
		default:
			return;
		case READ_QUERY_SENT:
			send_read_query();
			break;
		case WRITE_QUERY_SENT:
			send_write_query();
			break;
	}
}

void modbus_client::timeout() {
	switch (m_state) {
		case READ_QUERY_SENT:
			dolog(1, "Timeout while reading data, unit_id: %d, address: %hu, registers count: %hu, progressing with queries",
					(int)m_unit, m_start_addr, m_regs_count);

			send_next_query();
			break;
		case WRITE_QUERY_SENT:
			dolog(1, "Timeout while writing data, unit_id: %d, address: %hu, registers count: %hu, progressing with queries",
					(int)m_unit, m_start_addr, m_regs_count);
			send_next_query();
			break;
		default:
			dolog(1, "Received unexpected timeout from connection, ignoring.");
			break;
	}
}


void modbus_client::find_continous_reg_block(RSET::iterator &i, RSET &regs) {
	unsigned short current;
	assert(i != regs.end());

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

void modbus_client::pdu_received(unsigned char u, PDU &pdu) {
	switch (m_state) {
		case READ_QUERY_SENT:
			try {
				consume_read_regs_response(m_unit, m_start_addr, m_regs_count, pdu);
			} catch (std::out_of_range) {	//message was distorted
				dolog(1, "Error while processing response - there is either a bug or sth was very wrong with request format");
			}
			send_next_query();
			break;
		case WRITE_QUERY_SENT:
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

#if 0
void modbus_client::starting_new_cycle() {
	modbus_unit::startig_new_cycle();
	switch (m_state)  {
		case IDLE:
			return;
		case READ_QUERY_SENT:
			dolog(2, "New cycle started but we are in read cycle");
			break;
		case WRITE_QUERY_SENT:
			dolog(2, "New cycle started but we are in read cycle");
			break;
	}

	if (m_last_activity + 30 < m_current_time) {
		dolog(1, "Connection idle for too long, terminating");
		m_last_activity = m_current_time;
		terminate_connection();
	} else if (m_last_activity + 10 < m_current_time) {
		dolog(1, "Answer did not arrive, sending next query");
		send_next_query();
	}
}
#endif

int modbus_client::initialize() {
	return 0;
}

void modbus_tcp_client::send_pdu(unsigned char unit, PDU &pdu) {
	m_parser->send_adu(m_trans_id, unit, pdu, m_bufev);
	m_trans_id++;
}

void modbus_tcp_client::cycle_finished() {
	m_manager->driver_finished_job(this);
}

void modbus_tcp_client::terminate_connection() {
	m_manager->terminate_connection(this);
}

void modbus_tcp_client::frame_parsed(TCPADU &adu, struct bufferevent* bufev) {
	pdu_received(adu.unit_id, adu.pdu);
}

void modbus_tcp_client::connection_error(struct bufferevent *bufev) {
	m_parser->reset();
}

void modbus_tcp_client::scheduled(struct bufferevent* bufev, int fd) {
	m_bufev = bufev;
	start_cycle();
}

void modbus_tcp_client::data_ready(struct bufferevent* bufev, int fd) {
	m_parser->read_data(bufev);
}

int modbus_tcp_client::configure(TUnit* unit, xmlNodePtr node, short* read, short *send) {
	if (modbus_unit::configure(unit, node, read, send))
		return 1;
	m_trans_id = 0;
	m_parser = new tcp_parser(this);
	return 0;
}

void modbus_tcp_client::timeout() {
	modbus_client::timeout();
}

void modbus_serial_client::send_pdu(unsigned char unit, PDU &pdu) {
	m_parser->send_sdu(unit, pdu, m_bufev);
}

void modbus_serial_client::cycle_finished() {
	m_manager->driver_finished_job(this);
}

void modbus_serial_client::terminate_connection() {
	m_manager->terminate_connection(this);
}

void modbus_serial_client::connection_error(struct bufferevent *bufev) {
	m_parser->reset();
}

void modbus_serial_client::scheduled(struct bufferevent* bufev, int fd) {
	m_bufev = bufev;
	start_cycle();
}

void modbus_serial_client::data_ready(struct bufferevent* bufev, int fd) {
	m_parser->read_data(bufev);
}

int modbus_serial_client::configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration &spc) {
	if (modbus_unit::configure(unit, node, read, send))
		return 1;
	if (m_parser->configure(node, spc))
		return 1;
	return 0;
}

void modbus_serial_client::frame_parsed(SDU &sdu, struct bufferevent* bufev) {
	pdu_received(sdu.unit_id, sdu.pdu);
}

void modbus_serial_client::timeout() {
	modbus_client::timeout();
}

struct event_base* modbus_serial_client::get_event_base() {
	return m_event_base;
}

struct bufferevent* modbus_serial_client::get_buffer_event() {
	return m_bufev;
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
	dolog(9, "Checking crc, result: %s, unit_id: %d, caluclated crc: %hx, frame crc: %hx",
	       	(crc == frame_crc ? "OK" : "ERROR"), m_sdu.unit_id, crc, frame_crc);
	return crc == frame_crc;
}

serial_parser::serial_parser(serial_connection_handler *serial_handler) : m_serial_handler(serial_handler), m_write_timer_started(false) {
	evtimer_set(&m_read_timer, read_timer_callback, this);
	event_base_set(m_serial_handler->get_event_base(), &m_read_timer);
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

void serial_parser::read_timer_callback(int fd, short event, void* parser) {
	((serial_parser*) parser)->read_timer_event();
}

void serial_parser::write_timer_callback(int fd, short event, void* parser) {
	((serial_parser*) parser)->write_timer_event();
}

int serial_parser::configure(xmlNodePtr node, serial_port_configuration &spc) {
	int delay_between_chars = 0;
	get_xml_extra_prop(node, "out-intra-character-delay", delay_between_chars, true);
	if (delay_between_chars == 0)
		dolog(10, "Serial port configuration, delay between chars not given (or 0) assuming no delay");
	else
		dolog(10, "Serial port configuration, delay between chars set to %d miliseconds", delay_between_chars);

	int read_timeout = 0;
	get_xml_extra_prop(node, "read-timeout", read_timeout, true);
	if (read_timeout == 0)
		dolog(10, "Serial port configuration, read timeout not given (or 0), will use one based on speed");
	else
		dolog(10, "Serial port configuration, read timeout set to %d miliseconds", read_timeout);
	/*according to protocol specification, intra-character
	 * delay cannot exceed 1.5 * (time of transmittion of one character),
	 * we will make it double to be on safe side */
	if (m_timeout == 0) {
		if (spc.speed)
			m_timeout = 5 * 1000000 / (spc.speed / 10);
		else
			m_timeout = 10000;
	}
	dolog(9, "serial_parser m_timeout: %d", m_timeout);
	if (m_delay_between_chars)
		evtimer_set(&m_write_timer, write_timer_callback, this);
	return 0;
}

serial_rtu_parser::serial_rtu_parser(serial_connection_handler *serial_handler) : serial_parser(serial_handler), m_state(FUNC_CODE) {
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

int serial_server::configure(TUnit *unit, xmlNodePtr node, short *read, short *send, serial_port_configuration &spc) {
	if (modbus_unit::configure(unit, node, read, send))
		return 1;
	m_parser = new serial_rtu_parser(this);
	if (m_parser->configure(node, spc))
		return 1;
	return 0;
}

int serial_server::initialize() {
	return 0;
}

void serial_server::frame_parsed(SDU &sdu, struct bufferevent* bufev) {
	if (process_request(sdu.unit_id, sdu.pdu) == true)
		m_parser->send_sdu(sdu.unit_id, sdu.pdu, bufev);
}

void serial_server::connection_error(struct bufferevent* bufev) {
	m_bufev = NULL;
	m_parser->reset();
}

void serial_server::data_ready(struct bufferevent* bufev) {
	m_bufev = bufev;
	m_parser->read_data(bufev);	
}

void serial_server::timeout() {
	m_parser->reset();
}

void serial_server::starting_new_cycle() {
	modbus_unit::starting_new_cycle();
}

struct event_base* serial_server::get_event_base() {
	return m_event_base;
}

struct bufferevent* serial_server::get_buffer_event() {
	return m_bufev;
}

serial_client_driver* create_modbus_serial_client() {
	return new modbus_serial_client();
}

tcp_client_driver* create_modbus_tcp_client() {
	return new modbus_tcp_client();
}

serial_server_driver* create_modbus_serial_server() {
	return new serial_server();
}

tcp_server_driver* create_modbus_tcp_server() {
	return new tcp_server();
}
