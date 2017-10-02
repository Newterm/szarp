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

 @devices This is a borutadmn subdriver for Metronic FP210 flowmeter.
 @devices.pl Sterownik do demona borutadmn, obs³uguj±cy przep³ywomierz FP210 firmy Metronic.

 @protocol FP210 ASCII protocol over serial line or ethernet/serial converter.
 @protocol.pl Protokó³ tekstowy FP210 na linii szeregowej RS232/485 lub konwerterze ethernet/RS232.

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
			ignored
		type="1"
			ignored, should be 1
	       	subtype="1" 
			ignored, should be 1
		bufsize="1"
			average buffer size, at least 1
		extra:proto="fp210" 
			protocol name, denoting boruta driver to use, should be "fp210" for this driver
		extra:id="0x03"
			unit identifier, must correspond to id set in flowmeter
		extra:mode="client" 
			unit working mode, for this driver must be "client"
		extra:medium="serial"
			data transmission medium, may be "serial" (for RS232) or "tcp" (for TCP)
		extra:tcp-address="17/2.18.2.2"
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
	...
      </unit>
 </device>

 @config_example.pl
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
			ignorowany
		type="1"
			ignorowany, powinno byæ "1"
	       	subtype="1" 
			ignorowany, powinno byæ "1"
		bufsize="1"
			wielko¶æ bufora u¶redniania, 1 lub wiêcej
		extra:id="0x03"
			identyfikator przelicznika, musi odpowiadaæ indetyfikatorowi ustawionemu w urz±dzeniu
		extra:proto="fp210" 
			nazwa protoko³u, u¿ywana przez Borutê do ustalenia u¿ywanego sterownika, dla
			tego sterownika musi byæ "fp210"
		extra:mode="client" 
			tryb pracy jednostki, dla tego sterownika powinien byæ "client"
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
		<param name="...:..:przep³yw" unit="m3/h" prec="2" .../>
		<param name="...:..:licznik msw" unit="m3" prec="2" .../>
		<param name="...:..:licznik lsw" unit="m3" prec="2" .../>
		<param name="...:..:sumator 2 msw" unit="m3" prec="2" .../>
		<param name="...:..:sumator 2 lsw" unit="m3" prec="2" .../>
			z urz±dzenia czytanych jest 5 parametrów, pierwszy to aktualny przep³yw,
			pozosta³e to 2 sumatory, ka¿dy reprezentowany przez 2 parametry 'kombinowane'
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

class fp210_driver : public serial_client_driver {
	std::string m_id;
	enum STATE { IDLE, READING_FLOW, READING_SUMS } m_state;
	short *m_read;
	size_t m_read_count;
	std::vector<char> m_input_buffer;
	int m_flow_prec;
	int m_sum_prec;
	struct event m_timer;
	driver_logger m_log;
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
	const char* driver_name() { return "fp210_driver"; }
	virtual void starting_new_cycle();
	virtual void connection_error(struct bufferevent *bufev);
	virtual void data_ready(struct bufferevent* bufev, int fd);
	virtual void scheduled(struct bufferevent* bufev, int fd);
	virtual int configure(TUnit* unit, short* read, short *send, serial_port_configuration& spc);
	static void timeout_cb(int fd, short event, void *fp210_driver);
};

fp210_driver::fp210_driver() : m_log(this) {
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
	m_log.log(4, "Sumator 1: %s", vs1.c_str());
	m_log.log(4, "Sumator 2: %s", vs2.c_str());
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
	m_log.log(4, "fp210, got response: %s", &m_input_buffer[0]);
	tokenize_d(&m_input_buffer[0], &toks, &tokc, " ");
	switch (m_state) {
		case READING_FLOW:
			if (tokc != 4) {
				m_log.log(0, "Invalid number of fields in flow response");
				m_manager->terminate_connection(this);
				break;
			}
			set_flow_value(toks[3]);
			stop_timer();
			send_sum_query(bufev);
			break;
		case READING_SUMS:
			if (tokc != 3) {
				m_log.log(0, "Invalid number of fields in sum response");
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

int fp210_driver::configure(TUnit* unit, short* read, short *send, serial_port_configuration &spc) {
	if (!unit->hasAttribute("extra:id"))
		return 1;
	m_id = unit->getAttribute<int>("extra:id");

	m_read_count = unit->GetParamsCount();
	if (m_read_count != 5) {
		m_log.log(0, "FP210 requires exactly 5 parameters in szarp configuration");
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
