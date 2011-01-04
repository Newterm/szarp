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

class fp210_driver : public serial_client_driver {
	std::string m_id;
	enum STATE { IDLE, READING_FLOW, READING_SUMS } m_state;
	short *m_read;
	size_t m_read_count;
	std::vector<char> m_input_buffer;
	int m_flow_prec;
	int m_sum_prec;
	struct event m_timer;
	void set_no_data();
	void start_timer();
	void stop_timer();
	bool readline(struct bufferevent* bufev);
	void send_flow_query(struct bufferevent* bufev);
	void send_sum_query(struct bufferevent* bufev);
	void set_flow_value(char *val);
	void set_sum_value(char *val);
public:
	fp210_driver();
	virtual void starting_new_cycle();
	virtual void connection_error(struct bufferevent *bufev);
	virtual void data_ready(struct bufferevent* bufev, int fd);
	virtual void scheduled(struct bufferevent* bufev, int fd);
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration& spc);
	static void timeout_cb(int fd, short event, void *fp210_driver);
};

fp210_driver::fp210_driver() {
	m_state = IDLE;
}

void fp210_driver::set_no_data() {
	for (size_t i = 0; i < m_read_count; i++)
		m_read[i] = SZARP_NO_DATA;
}

void fp210_driver::start_timer() {
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	evtimer_add(&m_timer, &tv); 
}

void fp210_driver::stop_timer() {
	event_del(&m_timer);
}

void fp210_driver::send_flow_query(struct bufferevent* bufev) {
	std::stringstream ss;
	ss << "\x1b" << m_id << "D\r";
	bufferevent_write(bufev, (void*)ss.str().c_str(), ss.str().size());
	start_timer();
	m_input_buffer.resize(0);
	m_state = READING_FLOW;
}

void fp210_driver::send_sum_query(struct bufferevent* bufev) {
	std::stringstream ss;
	ss << "\x1b" << m_id << "L\r";
	bufferevent_write(bufev, (void*)ss.str().c_str(), ss.str().size());
	start_timer();
	m_input_buffer.resize(0);
	m_state = READING_SUMS;
}

void fp210_driver::starting_new_cycle() {
}

void fp210_driver::connection_error(struct bufferevent *bufev) {
	m_state = IDLE;
	set_no_data();
	stop_timer();
}

void fp210_driver::set_flow_value(char *val) {
	m_read[0] = m_flow_prec * atof(val);
}

void fp210_driver::set_sum_value(char *val) {
	std::string vs(val);
	std::string vs1 = vs.substr(8, 11);
	std::string vs2 = vs.substr(19, 11);
	dolog(4, "Sumator 1: %s", vs1.c_str());
	dolog(4, "Sumator 2: %s", vs2.c_str());
	int v1 = atof(vs1.c_str()) * m_sum_prec;
	int v2 = atof(vs2.c_str()) * m_sum_prec;
	unsigned* pv1 = (unsigned*)&v1;
	unsigned* pv2 = (unsigned*)&v2;
	*((unsigned short*)&m_read[1]) = *pv1 >> 16;
	*((unsigned short*)&m_read[2]) = *pv1 & 0xffff; 
	*((unsigned short*)&m_read[3]) = *pv2 >> 16;
	*((unsigned short*)&m_read[4]) = *pv2 & 0xffff; 
}

bool fp210_driver::readline(struct bufferevent* bufev) {
	char c;
	while (bufferevent_read(bufev, &c, 1)) {
		size_t bs = m_input_buffer.size();
		if (c == '\r' && bs > 1 && m_input_buffer.at(bs - 1) == '\r') {
			m_input_buffer.at(bs - 1) = '\0';
			return true;
		}
		m_input_buffer.push_back(c);
	}
	return false;
}

void fp210_driver::data_ready(struct bufferevent* bufev, int fd) {
	int tokc = 0;
	char **toks;
	if (!readline(bufev))
		return;
	dolog(4, "fp210, got response: %s", &m_input_buffer[0]);
	tokenize_d(&m_input_buffer[0], &toks, &tokc, " ");
	switch (m_state) {
		case READING_FLOW:
			if (tokc != 4) {
				dolog(0, "Invalid number of fileds in flow response");
				m_manager->terminate_connection(this);
				break;
			}
			set_flow_value(toks[3]);
			stop_timer();
			send_sum_query(bufev);
			break;
		case READING_SUMS:
			if (tokc != 3) {
				dolog(0, "Invalid number of fileds in sum response");
				m_manager->terminate_connection(this);
				break;
			}
			set_sum_value(toks[2]);
			stop_timer();
			m_manager->driver_finished_job(this);
			m_state = IDLE;
			break;
		case IDLE:
			break;
	}
	tokenize_d(NULL, &toks, &tokc, NULL);
}


void fp210_driver::scheduled(struct bufferevent* bufev, int fd) {
	send_flow_query(bufev);
}

int fp210_driver::configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration &spc) {
	if (get_xml_extra_prop(node, "id", m_id))
		return 1;
	m_read_count = unit->GetParamsCount();
	if (m_read_count != 5) {
		dolog(0, "FP210 reqires exactly 5 parameters in szarp configuration");
		return 1;
	}
	m_read = read;
	TParam *fp = unit->GetFirstParam();
	m_flow_prec = pow10(fp->GetPrec());
	m_sum_prec = pow10(fp->GetNext()->GetPrec());

	evtimer_set(&m_timer, timeout_cb, this);
	event_base_set(m_event_base, &m_timer);
	return 0;
}

void fp210_driver::timeout_cb(int fd, short event, void *_fp210_driver) {
	fp210_driver* z = (fp210_driver*) _fp210_driver;
	z->set_no_data();
	if (z->m_state != IDLE)
		z->m_manager->driver_finished_job(z);
}

serial_client_driver* create_fp210_serial_client() {
	return new fp210_driver();
}
