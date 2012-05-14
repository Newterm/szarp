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
 * Krzysztof Ga³±zka <kg@newterm.pl>
 * 
 */
/*
 Daemon description block.

 @description_start

 @class 4

 @devices This is a borutadmn subdriver for Lumel protocol.
 @devices.pl Sterownik do demona borutadmn, obs³uguj±cy protokó³ Lumel.

 @description_end

*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <event.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "liblog.h"
#include "xmlutils.h"
#include "conversion.h"
#include "ipchandler.h"
#include "tokens.h"
#include "borutadmn.h"

namespace {

class lumel_register {
	unsigned char m_address;
	std::string m_val;
	time_t m_mod_time;
public:
	lumel_register(unsigned char addr) : m_address(addr) {} ;
	unsigned char get_address() { return m_address; };
	void set_val(std::string & val);
	int get_val(double & value);

};

typedef std::map<unsigned char, lumel_register*> LRMAP;

//maps regisers values to parcook short values
class read_val_op {
    protected:
	lumel_register* m_reg;
    public:
	read_val_op(lumel_register *reg, int prec) : m_reg(reg) {} ;
	virtual unsigned short val() = 0;
};

class short_read_val_op : public read_val_op {
public:
	short_read_val_op(lumel_register *reg, int prec) : read_val_op(reg, prec) {} ;
	unsigned short val();
};

class long_read_val_op : public read_val_op {
    protected:
	bool m_lsw;
public:
	long_read_val_op(lumel_register *reg, int prec, bool lsw) : read_val_op(reg, prec), m_lsw(lsw) {};
	unsigned short val();
};

void lumel_register::set_val(std::string & val) {
    m_val = val;
}

int lumel_register::get_val(double & value) {
    char * endptr;

    errno = 0; // according to man to check for errors in conversion
    const char * str = m_val.c_str();
    double val = strtod(str, &endptr); 
 
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {
	dolog(2, "Error in ascii to doble conversion");
	return 1;
    }

   if (endptr == str) {
	dolog(2, "No value in string");
	return 1;
   }
   value = val;

   return 0;
}

unsigned short short_read_val_op::val() {
    double val;

    if (m_reg->get_val(val))
	return SZARP_NO_DATA;

    return (unsigned short)(val);
}

unsigned short long_read_val_op::val() {
    double val;

    if (m_reg->get_val(val))
	return SZARP_NO_DATA;

    int v = (val);
    return m_lsw ? (unsigned short)(v & 0xFFFF) : (unsigned short)(v >> 16);
}

}


class lumel_serial_client : public serial_client_driver {

protected:
    unsigned char m_unit_id;
    short* m_read;
    size_t m_read_count;
    short* m_send;
    size_t m_send_count;
    struct bufferevent* m_bufev;
    struct event m_read_timer;
    bool m_read_timer_started;
    int m_timeout;

    enum { IDLE, REQUEST, RESPONSE } m_state;

    std::vector<unsigned char> m_request_buffer;

    std::vector<unsigned char> m_buffer;

    LRMAP m_registers;
    LRMAP::iterator m_registers_iterator;
    
    std::vector<read_val_op *> m_read_operators;

    unsigned char checksum(std::vector<unsigned char> & buffer) ;
    void finished_cycle();
    void next_request();
    void send_request();
    void make_read_request();
    int parse_frame();
    void to_parcook();
	
    void start_read_timer();
    void stop_read_timer();

public:
    virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration& spc);
    virtual void connection_error(struct bufferevent *bufev);
    virtual void starting_new_cycle();
    virtual void scheduled(struct bufferevent* bufev, int fd);
    virtual void data_ready(struct bufferevent* bufev, int fd);
    
    void read_timer_event();
    static void read_timer_callback(int fd, short event, void* lumel_serial_client);
};


void lumel_serial_client::stop_read_timer() {
	dolog(10, "lumel_serial_client::stop_read_timer");
	if (m_read_timer_started) {
		event_del(&m_read_timer);
		m_read_timer_started = false;
	}
}

void lumel_serial_client::start_read_timer() {
	dolog(10, "lumel_serial_client::start_read_timer");
	stop_read_timer();

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = m_timeout;
	evtimer_add(&m_read_timer, &tv); 
	m_read_timer_started = true;
	dolog(10, "lumel_serial_client::stop_read_timer: Read timer started");
}

void lumel_serial_client::read_timer_callback(int fd, short event, void* client) {
	dolog(10, "lumel_serial_client::read_timer_callback");
	((lumel_serial_client*) client)->read_timer_event();
}

void lumel_serial_client::read_timer_event() {
    dolog(2, "lumel_serial_client::read_timer_event, state: %d, reg: %d", m_state, m_registers_iterator->first);
    next_request();
}

int lumel_serial_client::configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration &spc) {

    m_read = read;
    m_read_count = unit->GetParamsCount();

    m_timeout = 1000000;
	m_read_timer_started = false;
	evtimer_set(&m_read_timer, read_timer_callback, this);
	event_base_set(m_event_base, &m_read_timer);
	m_state = IDLE;

	unsigned char id;
	std::string _id;
	get_xml_extra_prop(node, "id", _id, true);
	if (_id.empty()) {
		switch (unit->GetId()) {
			case L'0'...L'9':
				id = unit->GetId() - L'0';
				break;
			case L'a'...L'z':
				id = (unit->GetId() - L'a') + 10;
				break;
			case L'A'...L'Z':
				id = (unit->GetId() - L'A') + 10;
				break;
			default:
				id = unit->GetId();
				break;
		}
	} else {
		id = strtol(_id.c_str(), NULL, 0);
	}

	if (id > 31) {
	    dolog(0, "Unit id out of allowed range in unit element at line: %ld", xmlGetLineNo(node));
	    return 1;
	}
	m_unit_id = id;

	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(node->doc);
	xp_ctx->node = node;
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert(ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "extra", BAD_CAST IPKEXTRA_NAMESPACE_STRING);
    
	TParam* param = unit->GetFirstParam();
	for (size_t i = 0; i < m_read_count ; i++) {
		char *expr;
	        asprintf(&expr, ".//ipk:param[position()=%d]",  i + 1);
		xmlNodePtr pnode = uxmlXPathGetNode(BAD_CAST expr, xp_ctx);
		assert(pnode);
		free(expr);

		std::string _addr;
		if (get_xml_extra_prop(pnode, "address", _addr, false)) {
		    dolog(0, "Invalid address attribute in param element at line: %ld", xmlGetLineNo(pnode));
		    return 1;
		}

		char *e;
		long l = strtol(_addr.c_str(), &e, 0);
		if (*e != 0 || l < 0 || l > 0x32) {
			dolog(0, "Invalid address attribute value: %ld (line %ld), between 0 and 0x32", l, xmlGetLineNo(pnode));
			return 1;
		} 
		unsigned char addr = (unsigned char)l;
		dolog(10, "lumel_serial_client::configure _addr: %ld, addr: %hhu", l, addr);
		
		std::string val_op;
		if (get_xml_extra_prop(node, "val_op", val_op, true)) {
		    dolog(0, "Invalid val_op attribute in param element at line: %ld", xmlGetLineNo(pnode));
		    return 1;
		}

		int prec = exp10(param->GetPrec());
		lumel_register* reg = NULL;

		if (val_op.empty()) {
		    if (m_registers.find(addr) != m_registers.end()) {
			dolog(0, "Already configured register with address (%hd) in param element at line: %ld", addr, xmlGetLineNo(pnode));
			return 1;
		    }
		    reg = new lumel_register(addr); 
		    m_registers[addr] = reg;
		    m_read_operators.push_back(new short_read_val_op(reg, prec));
		}
		else {
		    if (m_registers.find(addr) == m_registers.end()) {
			reg = new lumel_register(addr);
			m_registers[addr] = reg;
		    }
		    else {
			reg = m_registers[addr];
		    }

		    if (val_op == "LSW") {
			m_read_operators.push_back(new long_read_val_op(reg, prec, true));
		    }
		    else if (val_op == "MSW") {
			m_read_operators.push_back(new long_read_val_op(reg, prec, false));
		    }
		    else {
			dolog(0, "Unsupported val_op attribute value - %s, line %ld", val_op.c_str(), xmlGetLineNo(pnode));
			return 1;
		    }
		}
	}

	return 0;
}

void lumel_serial_client::connection_error(struct bufferevent *bufev) {
	dolog(10, "lumel_serial_client::connection_error");
	m_state = IDLE;
	m_bufev = NULL;
	m_buffer.clear();
	stop_read_timer();
	m_manager->driver_finished_job(this);
}

void lumel_serial_client::starting_new_cycle() {
	dolog(10, "lumel_serial_client::starting_new_cycle");
}

void lumel_serial_client::scheduled(struct bufferevent* bufev, int fd) {
	m_bufev = bufev;
	dolog(10, "lumel_serial_client::scheduled");
	switch (m_state) {
	    case IDLE:
		m_registers_iterator = m_registers.begin();
		send_request();
		break;
	    case REQUEST:
	    case RESPONSE:
		dolog(2, "New cycle before end of querying");
		break;
	    default:
		dolog(2, "Unknown state, something went teribly wrong");
		assert(false);
		break;
	}
}

void lumel_serial_client::finished_cycle() {
    dolog(10, "lumel_serial_client::finished_cycle");
    to_parcook();
}

void lumel_serial_client::send_request() {
    dolog(10, "lumel_serial_client::send_request");
    m_buffer.clear();
    make_read_request();
    bufferevent_write(m_bufev, &m_request_buffer[0], m_request_buffer.size());
    m_state = REQUEST;
    start_read_timer();
}

void lumel_serial_client::next_request() {
    dolog(10, "lumel_serial_client::next_request");
    switch (m_state) {
	case REQUEST:
	    // TODO error?
	    break;
	case RESPONSE:
	    // TODO error?
	    break;
	default:
	    break;
    }
    
    m_registers_iterator++;
    if (m_registers_iterator == m_registers.end()) {
	// TODO end of cycle
	dolog(8, "lumel_serial_client::next_request, no more registers to query, driver finished job");
	m_state = IDLE;
	m_manager->driver_finished_job(this);
	return;
    } 

    send_request();
}


unsigned char lumel_serial_client::checksum(std::vector<unsigned char> & buffer) {
    unsigned int tmp = 0;
    unsigned char sum = 0;
    for (size_t i = 1; i < buffer.size(); i++) {
	tmp += buffer[i];
    }
    sum = tmp & 0xFF;
    sum = ~sum;
    return sum + 1;
}

void lumel_serial_client::make_read_request() {
    dolog(10, "lumel_serial_client::make_read_request");
    m_request_buffer.clear();
    lumel_register* reg = m_registers_iterator->second;
    m_request_buffer.push_back(':');
    m_request_buffer.push_back(ascii::value2char(m_unit_id >> 4));
    m_request_buffer.push_back(ascii::value2char(m_unit_id & 0xF));
    m_request_buffer.push_back('6');
    m_request_buffer.push_back('5');
    m_request_buffer.push_back(ascii::value2char(reg->get_address() >> 4));
    m_request_buffer.push_back(ascii::value2char(reg->get_address() & 0xF));

    unsigned char sum = checksum(m_request_buffer);
    m_request_buffer.push_back(ascii::value2char(sum >> 4));
    m_request_buffer.push_back(ascii::value2char(sum & 0xF));
    m_request_buffer.push_back('\r');
    m_request_buffer.push_back('\n');
}

void lumel_serial_client::data_ready(struct bufferevent* bufev, int fd) {
    char c;

    switch (m_state) {
	case IDLE:
	    // TODO something wrong hapend?
	    dolog(2, "Got unrequested message, ignoring");
	    while (bufferevent_read(bufev, &c, sizeof(c)) != 0);
	    break;
	case REQUEST:
	    while (bufferevent_read(bufev, &c, sizeof(c)) != 0) {
		if (c == ':')
		    break;
	    }
	    if (c != ':') {
		   dolog(8, "Start of frame not found, waiting");
		   break;
	    }
	    stop_read_timer();
	    m_buffer.push_back(c);
	    m_state = RESPONSE;
	case RESPONSE:
	    while (bufferevent_read(bufev, &c, sizeof(c)) != 0) {
		if ('\n' == c && '\r' == m_buffer.back()) {
		    // end of frame
		    m_buffer.push_back(c);
		    break;
		}
		m_buffer.push_back(c);
	    }
	    parse_frame();
	    while (bufferevent_read(bufev, &c, sizeof(c)) != 0) ; // some junk on line, ignoring
	    next_request();
	    break;
	default:
	    break;
    }
}

int lumel_serial_client::parse_frame() {
    if (m_buffer.front() != ':') {
	// TODO error
	return 1;
    }

    dolog(8, "lumel_serial_client::parse_frame, l: %d", m_buffer.size());

    for (size_t i = 0; i < 7; i++) {
	if (m_buffer[i] != m_request_buffer[i]) {
	    // TODO error
	    dolog(2, "Wrong response at character %d", i);
	    return 1;
	}
    }

    m_buffer.pop_back(); // '\n'
    m_buffer.pop_back(); // '\r'
    unsigned char s1 = m_buffer.back();
    m_buffer.pop_back();
    unsigned char s2 = m_buffer.back();
    m_buffer.pop_back();
    unsigned char sum1;
    if(ascii::from_ascii(s2, s1, sum1)) {
	dolog(2, "Cannot convert checksum from ascii");
	return 1;
    }
    unsigned char sum2 = checksum(m_buffer);

    if (sum1 != sum2) {
	dolog(2, "Wrong checksum, ignoring frame");
	return 1;
    }

    std::stringstream val;

    for (size_t i = 8; i < m_buffer.size(); i++) {
	val.put(m_buffer[i]);
    }

    std::string sv = val.str();

    m_registers_iterator->second->set_val(sv);
    
    return 0;
}


void lumel_serial_client::to_parcook() {
    dolog(10, "lumel_serial_client::to_parcook, m_read_count: %d", m_read_count);
    for (size_t i = 0; i < m_read_count; i++) {
	m_read[i] = m_read_operators[i]->val();
	dolog(9, "Parcook param no %zu set to %hu", i, m_read[i]);
    }
}

serial_client_driver* create_lumel_serial_client() {
	return new lumel_serial_client();
}

