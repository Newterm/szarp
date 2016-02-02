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
 * $Id$
 */
/*
 @description_start
 @class 4
 @devices Kamstrup Multical IV, Kamstrup Multical 66CDE heatmeters.
 @devices.pl Ciep³omierz Kamstrup Multical IV, Kamstrup Multical 66CDE.

 @description_end
*/

/* "Publiczne udostêpnianie protoko³u do liczników Multical III i Multical 66C nie jest zabronione. Ograniczenia s± zwi±zane tylko z protoko³em KMP (do liczników Multical 601 i nowszych)." (Kamstrup, 04.12.2012) */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <event.h>
#include <libgen.h>
#include <boost/tokenizer.hpp>

#include "liblog.h"
#include "xmlutils.h"
#include "conversion.h"
#include "ipchandler.h"
#include "serialport.h"
#include "serialadapter.h"
#include "atcconn.h"
#include "daemonutils.h"

#ifndef SZARP_NO_DATA
#define SZARP_NO_DATA -32768
#endif

#define NUMBER_OF_TOKENS 10
#define NUMBER_OF_VALS 16

#define RESPONSE_SIZE 82

#define RESTART_INTERVAL_MS 15000

bool single;

void dolog(int level, const char * fmt, ...)
	__attribute__ ((format (printf, 2, 3)));


void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	if (single) {
		char *l;
		va_start(fmt_args, fmt);
		int res = vasprintf(&l, fmt, fmt_args);
		(void)res;
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

xmlChar* get_device_node_prop(xmlXPathContextPtr xp_ctx, const char* prop) {
	xmlChar *c;
	char *e;
	int res = asprintf(&e, "./@%s", prop);
	(void)res;
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	return c;
}

xmlChar* get_device_node_extra_prop(xmlXPathContextPtr xp_ctx, const char* prop) {
	xmlChar *c;
	char *e;
	int res = asprintf(&e, "./@extra:%s", prop);
	(void)res;
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	return c;
}

/** Daemon reading data from heatmeters.
 * Can use either a SerialPort (if 'path' is declared in params.xml)
 * or a SerialAdapter connection
 * (if 'extra:tcp-ip' and optionally 'extra:tcp-data-port' and 'extra:tcp-cmd-port' are declared)
 */
class kams_daemon: public ConnectionListener {
public:
	typedef enum {MODE_B300, MODE_B1200_EVEN, MODE_B1200} SerialMode;
	typedef enum {SET_COMM_WRITE, WRITE, REPEAT_WRITE, SET_COMM_READ, READ, RESTART} CommunicationState;

	class KamsDmnException : public SzException {
		SZ_INHERIT_CONSTR(KamsDmnException, SzException)
	};
	class NoDataException : public KamsDmnException {
		SZ_INHERIT_CONSTR(NoDataException, KamsDmnException)
	};

	kams_daemon() : m_daemon_conf(NULL), m_ipc(NULL),
			m_state(SET_COMM_WRITE),
			m_read_mode(MODE_B1200_EVEN),
			m_chars_written(0),
			m_wait_for_data_ms(6000),
			m_path(""),
			m_ip(""),
			m_use_atc(false),
			m_connection(NULL)
	{
		m_query_command.push_back('/');
		m_query_command.push_back('#');
		m_query_command.push_back('1');
		m_event_base = event_base_new();
	}

	~kams_daemon()
	{
		if (m_daemon_conf != NULL) {
			delete m_daemon_conf;
		}
		if (m_ipc != NULL) {
			delete m_ipc;
		}
		if (m_connection != NULL) {
			delete m_connection;
		}
	}

	/** Read commandline and params.xml configuration */
	void ReadConfig(int argc, char **argv);

	/** Start event-based state machine */
	void StartDo();

protected:
	/** Schedule next state machine step */
	void ScheduleNext(unsigned int wait_ms=0);

	/** Callback for next step of timed state machine. */
	static void TimerCallback(int fd, short event, void* thisptr);

	/** One step of state machine */
	void Do();

	/** Sets NODATA and changes state to RESTART */
	void SetRestart()
	{
		dolog(10, "%s: SetRestart", m_id.c_str());
		m_state = RESTART;
		for (int i = 0; i < m_ipc->m_params_count; i++) {
			m_ipc->m_read[i] = SZARP_NO_DATA;
		}
		m_ipc->GoParcook();
	}

	/** Write single char of query command to device */
	void WriteChar();

	/** ConnectionListener interface */
	virtual void ReadError(const BaseConnection *conn, short event);
	virtual void ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data);
	virtual void OpenFinished(const BaseConnection *conn);
	virtual void SetConfigurationFinished(const BaseConnection *conn);

	/** Increase wait for data time */
	void IncreaseWaitForDataTime();

	/** Process received data */
	void ProcessResponse();

	/** Helper method for ProcessResponse */
	std::vector<std::string> SplitMessage();

	/** Sets serial communication mode - changes SerialAdapter configuration */
	void SelectSerialMode(SerialMode mode);

	/** Switches read mode to be used in next read */
	void SwitchSerialReadMode()
	{
		if (m_read_mode == MODE_B1200_EVEN) {
			m_read_mode = MODE_B1200;
		} else {
			m_read_mode = MODE_B1200_EVEN;
		}
	}

protected:
	DaemonConfig *m_daemon_conf;
	IPCHandler *m_ipc;

	CommunicationState m_state;	/**< state of communication state machine */
	SerialMode m_read_mode;		/**< serial comm mode currently used */
	struct event m_ev_timer;
					/**< event timer for calling QueryTimerCallback */

	std::vector<unsigned char> m_query_command;
					/**< commmand used for communication with device */
	unsigned int m_chars_written;	/**< counter of number of chars written to device */
	unsigned int m_query_interval_ms;
					/**< interval between single queries */
	unsigned int m_wait_for_data_ms;/**< time for which daemon waits for data from Kamstrup meter */

	std::string m_read_buffer;	/**< buffer for data received from Kamstrup meter */
	std::vector<unsigned int> m_params;
					/**< numeric values of received params */

	std::string m_id;		/**< ID of given kamsdmn */

	std::string m_path;		/**< Serial port file descriptor path */

	std::string m_ip;		/**< SerialAdapter/AtcConnection ip */
	int m_data_port;		/**< SerialAdapter data port number */
	int m_cmd_port;			/**< SerialAdapter command port number */
	bool m_use_atc;

	BaseConnection *m_connection;
	struct event_base *m_event_base;
};

void kams_daemon::StartDo() {
	evtimer_set(&m_ev_timer, TimerCallback, this);
	event_base_set(m_event_base, &m_ev_timer);
	dolog(10, "%s: Creating connection...", m_id.c_str());
	try {
		if (m_ip.compare("") != 0) {
			if (m_use_atc) {
				AtcConnection *client = new AtcConnection(m_event_base);
				client->InitTcp(m_ip);
				m_connection = client;
			} else {
				SerialAdapter *client = new SerialAdapter(m_event_base);
				client->InitTcp(m_ip, m_data_port, m_cmd_port);
				m_connection = client;
			}
		} else {
			SerialPort *port = new SerialPort(m_event_base);
			m_connection = port;
			port->Init(m_path);
		}
		m_connection->AddListener(this);
		dolog(10, "%s: Opening connection...", m_id.c_str());
		m_connection->Open();
	} catch (SerialPortException &e) {
		dolog(0, "%s: Open port resulted in: %s", m_id.c_str(), e.what());
		SetRestart();
		ScheduleNext(RESTART_INTERVAL_MS);
	}
	event_base_dispatch(m_event_base);
}

void kams_daemon::OpenFinished(const BaseConnection* conn)
{
	std::string info = m_id + ": connection established.";
	dolog(2, "%s: %s", m_id.c_str(), info.c_str());
	m_state = SET_COMM_WRITE;
	ScheduleNext();
}

void kams_daemon::SetConfigurationFinished(const BaseConnection *conn)
{
	// do nothing
	// 1) WRITE: according to previous implementation we should wait 2s nevertheless
	//    (commit: d6b08af2f8d184088ac237c20a9125df25fadb53)
	// 2) READ: ScheduleNext will be triggered by ReadData
}

void kams_daemon::Do()
{
	unsigned int wait_ms = 0;
	switch (m_state) {
		case SET_COMM_WRITE:
			dolog(10, "%s: SET_COMM_WRITE", m_id.c_str());
			SelectSerialMode(MODE_B300);
			m_read_buffer.clear();
			m_chars_written = 0;
			m_state = WRITE;
			wait_ms = 2000;
			break;
		case WRITE:
			dolog(10, "%s: WRITE", m_id.c_str());
			try {
				WriteChar();
			} catch (SerialPortException &e) {
				dolog(0, "%s: %s", m_id.c_str(), e.what());
				SetRestart();
			}
			if (m_chars_written == 3) {
				m_chars_written = 0;
				m_state = REPEAT_WRITE;
			}
			wait_ms = 50;
			break;
		case REPEAT_WRITE:
			dolog(10, "%s: REPEAT_WRITE", m_id.c_str());
			try {
				WriteChar();
			} catch (SerialPortException &e) {
				dolog(0, "%s: %s", m_id.c_str(), e.what());
				SetRestart();
			}
			if (m_chars_written < 3) {
				wait_ms = 50;
			} else {
				m_state = SET_COMM_READ;
				wait_ms = 10;
			}
			break;
		case SET_COMM_READ:
			dolog(10, "%s: SET_COMM_READ", m_id.c_str());
			SelectSerialMode(m_read_mode);
			m_state = READ;
			wait_ms = m_wait_for_data_ms;
			dolog(10, "%s: waiting for data, wait time = %dms", m_id.c_str(),
				m_wait_for_data_ms);
			break;
		case READ:
			dolog(10, "%s: READ", m_id.c_str());
			try {
				ProcessResponse();
				wait_ms = m_query_interval_ms;
				m_state = SET_COMM_WRITE;
				for (int i = 0; i < m_ipc->m_params_count; i++) {
					m_ipc->m_read[i] = m_params.at(i);
				}
				m_ipc->GoParcook();
			} catch (NoDataException &e) {
				dolog(0, "%s: %s", m_id.c_str(), e.what());
				wait_ms = 0;
				IncreaseWaitForDataTime();
				SetRestart();
			} catch (KamsDmnException &e) {
				dolog(0, "%s: %s", m_id.c_str(), e.what());
				wait_ms = 1000;
				m_state = SET_COMM_WRITE;
				for (int i = 0; i < m_ipc->m_params_count; i++) {
					m_ipc->m_read[i] = SZARP_NO_DATA;
				}
				m_ipc->GoParcook();
			} catch (SerialPortException &e) {
				dolog(0, "%s: %s", m_id.c_str(), e.what());
				wait_ms = 0;
				IncreaseWaitForDataTime();
				SetRestart();
			}
			break;
		case RESTART:
			dolog(10, "%s: RESTART", m_id.c_str());
			try {
				m_connection->Close();
				m_connection->Open();
			} catch (SerialPortException &e) {
				dolog(0, "%s: %s %s", m_id.c_str(), "Restart failed:", e.what());
				for (int i = 0; i < m_ipc->m_params_count; i++) {
					m_ipc->m_read[i] = SZARP_NO_DATA;
				}
				m_ipc->GoParcook();
				wait_ms = RESTART_INTERVAL_MS;
				ScheduleNext(wait_ms);
			}
			// in the case of restart we should either
			// get OpenFinished event or ReadError
			// which should ScheduleNext on themselves
			return;
	}
	ScheduleNext(wait_ms);
}

void kams_daemon::SelectSerialMode(SerialMode mode)
{
	SerialPortConfiguration conf;
	conf.char_size = SerialPortConfiguration::CS_7;
	conf.stop_bits = 2;

	switch (mode) {
		case MODE_B300:
			conf.speed = 300;
			break;
		case MODE_B1200_EVEN:
			conf.speed = 1200;
			conf.parity = SerialPortConfiguration::EVEN;
			break;
		case MODE_B1200:
			conf.speed = 1200;
			break;
	}

	m_connection->SetConfiguration(conf);
}

void kams_daemon::WriteChar()
{
	unsigned char c = m_query_command.at(m_chars_written);
	m_connection->WriteData(&c, 1);
	m_chars_written++;
}

void kams_daemon::ReadError(const BaseConnection *conn, short event)
{
	dolog(0, "%s: %s", m_id.c_str(), "ReadError, closing connection..");
	m_connection->Close();
	SetRestart();
	ScheduleNext(RESTART_INTERVAL_MS);
}

void kams_daemon::ReadData(const BaseConnection *conn,
		const std::vector<unsigned char>& data)
{
	if (m_daemon_conf->GetSingle()) {
		std::cout << "+";
	}
	m_read_buffer.insert(m_read_buffer.end(), data.begin(), data.end());
	unsigned int expected_response_size = RESPONSE_SIZE - 1;	// LF isn't read by bufferevent
	if (m_read_buffer.size() == expected_response_size) {
		ScheduleNext();
	}
}

void kams_daemon::IncreaseWaitForDataTime()
{
	unsigned int max_ms = 30 * 1000;
	unsigned int step_ms = 6 * 1000;
	if (m_wait_for_data_ms < max_ms)
		m_wait_for_data_ms += step_ms;
}

void kams_daemon::ProcessResponse()
{
	m_params.clear();
	m_params.resize(NUMBER_OF_VALS, SZARP_NO_DATA);

	unsigned int message_size = RESPONSE_SIZE - 3;			// ' ',CR,LF
	unsigned int expected_response_size = RESPONSE_SIZE - 1;	// LF isn't read by bufferevent

	if (m_read_buffer.size() == expected_response_size) {
		m_read_buffer.resize(message_size);
		if (m_daemon_conf->GetSingle()) {
			std::cout << "Frame received: \"" << m_read_buffer << '"' << std::endl;
		}
	} else if (m_read_buffer.size() == 0) {
		throw NoDataException("ERROR!: No data was read from socket.");
	} else {
		SwitchSerialReadMode();					// try with another parity next time
		throw KamsDmnException("ERROR!: Frame size "
				+ std::to_string(m_read_buffer.size())
				+ " is incorrect ("+ std::to_string(expected_response_size)
				+ ")");
	}

	std::vector<std::string> tokens = SplitMessage();
	for (unsigned int i = 0; i < tokens.size(); i++) {
		std::istringstream istr(tokens[i]);
		unsigned int kamdata;
		bool conversion_failed = (istr >> kamdata).fail();
		if (conversion_failed) {
			SwitchSerialReadMode();
			throw KamsDmnException("ERROR!: Couldn't parse token nr "
					+ std::to_string(i));
		}
		switch (i) {
		case 0:	/* energy in 0.1 GJ - 4 bytes */
			m_params[0] = (int)(kamdata & 0xffff);
			m_params[1] = (int)(kamdata >> 16);
			break;
		case 1:	/* water in m3 - 4 bytes */
			m_params[2] = (int)(kamdata & 0xffff);
			m_params[3] = (int)(kamdata >> 16);
			break;
		case 2:	/* time in hours - 4 bytes */
			m_params[4] = (int)(kamdata & 0xffff);
			m_params[5] = (int)(kamdata >> 16);
			break;
		case 3:	/* feed water temp. in C degrees - 2 bytes */
			m_params[6] = (int)kamdata;
			break;
		case 4:	/* return water temp. in C degrees - 2 bytes */
			m_params[7] = (int)kamdata;
			break;
		case 5:	/* water temp. difference in C degrees - 2 bytes */
			m_params[8] = (int)kamdata;
			break;
		case 6:	/* power in 0.1 kW - 4 bytes */
			m_params[9] = (int)(kamdata & 0xffff);
			m_params[10] = (int)(kamdata >> 16);
			break;
		case 7:	/* flow in liters per hour - 4 bytes */
			m_params[11] = (int)(kamdata & 0xffff);
			m_params[12] = (int)(kamdata >> 16);
			break;
		case 8:	/* peak power in 0.1 kW - 4 bytes */
			m_params[13] = (int)(kamdata & 0xffff);
			m_params[14] = (int)(kamdata >> 16);
			break;
		default:	/* case 9: info - 2 bytes */
			m_params[15] = (int)kamdata;
			break;
		}
	}
	if (tokens.size() != NUMBER_OF_TOKENS) {
		throw KamsDmnException("ERROR!: Received " + std::to_string(tokens.size()) +
				" values instead of " + std::to_string(NUMBER_OF_TOKENS));
	}
}

std::vector<std::string> kams_daemon::SplitMessage()
{
	// use boost::tokenizer to split message at each space
	typedef boost::tokenizer<boost::char_separator<char> > tok_t;
	boost::char_separator<char> sep(" ", "", boost::keep_empty_tokens);
	tok_t tok(m_read_buffer, sep);
	std::vector<std::string> tokens;
	for(tok_t::iterator it = tok.begin(); it != tok.end(); ++it)
		tokens.push_back(*it);
	return tokens;
}

void kams_daemon::ScheduleNext(unsigned int wait_ms)
{
	const struct timeval tv = ms2timeval(wait_ms);
	evtimer_add(&m_ev_timer, &tv);
	dolog(10, "%s: ScheduleNext in %dms", m_id.c_str(), wait_ms);
}

void kams_daemon::TimerCallback(int fd, short event, void* thisptr)
{
	reinterpret_cast<kams_daemon*>(thisptr)->Do();
}

void kams_daemon::ReadConfig(int argc, char **argv) {
	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);

	m_daemon_conf = new DaemonConfig("kamsdmn");
	assert(m_daemon_conf != NULL);
	if (m_daemon_conf->Load(&argc, argv)) {
		throw KamsDmnException("Cannot load configuration");
	}
	if (m_daemon_conf->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetParamsCount()
			!= NUMBER_OF_VALS) {
		throw KamsDmnException("Incorrect number of parameters: " +
				std::to_string(m_daemon_conf->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount()) +
				+ ", must be " + std::to_string(NUMBER_OF_VALS));
	}

	m_ipc = new IPCHandler(m_daemon_conf);
	if (!m_daemon_conf->GetSingle()) {
		if (m_ipc->Init()) {
			throw KamsDmnException("Cannot initialize IPCHandler");
		}
	}
	if (m_daemon_conf->GetSingle() || m_daemon_conf->GetDiagno())
		m_query_interval_ms = 3000;
	else
		m_query_interval_ms = 280 * 1000;	/* for saving heatmeter battery */


	xmlDocPtr doc;

	/* get config data */
	doc = m_daemon_conf->GetXMLDoc();
	assert (doc != NULL);

	/* prepare xpath */
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(doc);
	assert (xp_ctx != NULL);
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert (ret == 0);
	(void)ret;
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "extra",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);

	xp_ctx->node = m_daemon_conf->GetXMLDevice();

	xmlChar *c = get_device_node_extra_prop(xp_ctx, "tcp-ip");
	if (c == NULL) {
		xmlChar *atc_ip = get_device_node_extra_prop(xp_ctx, "atc-ip");
		if (atc_ip == NULL) {
			xmlChar *path = get_device_node_prop(xp_ctx, "path");
			if (path == NULL) {
				throw KamsDmnException("ERROR!: neither IP nor device path "
						"has been specified");
			}
			m_path.assign((const char*)path);
			xmlFree(path);
			m_id = m_path;
		} else {
			m_ip.assign((const char*)atc_ip);
			xmlFree(atc_ip);
			m_id = m_ip;
			m_use_atc = true;
		}
	} else {
		m_ip.assign((const char*)c);
		xmlFree(c);

		xmlChar* tcp_data_port = get_device_node_extra_prop(xp_ctx, "tcp-data-port");
		if (tcp_data_port == NULL) {
			m_data_port = SerialAdapter::DEFAULT_DATA_PORT;
			dolog(2, "Unspecified tcp data port, assuming default port: %hu", m_data_port);
		} else {
			std::istringstream istr((char*) tcp_data_port);
			bool conversion_failed = (istr >> m_data_port).fail();
			if (conversion_failed) {
				throw KamsDmnException("ERROR!: Invalid data port value: "
						+ std::string((char*) tcp_data_port));
			}
		}
		xmlFree(tcp_data_port);

		xmlChar* tcp_cmd_port = get_device_node_extra_prop(xp_ctx, "tcp-cmd-port");
		if (tcp_cmd_port == NULL) {
			m_cmd_port = SerialAdapter::DEFAULT_CMD_PORT;
			dolog(2, "Unspecified cmd port, assuming default port: %hu", m_cmd_port);
		} else {
			std::istringstream istr((char*) tcp_cmd_port);
			bool conversion_failed = (istr >> m_cmd_port).fail();
			if (conversion_failed) {
				throw KamsDmnException("ERROR!: Invalid cmd port value: "
						+ std::string((char*) tcp_cmd_port));
			}
		}
		xmlFree(tcp_cmd_port);

		std::stringstream istr;
		std::string data_port_str;
		istr << m_data_port;
		istr >> data_port_str;
		m_id = m_ip + ":" + data_port_str;
	}
	single = m_daemon_conf->GetSingle() || m_daemon_conf->GetDiagno();
}

int main(int argc, char *argv[])
{
	kams_daemon daemon;

	try {
		dolog(10, "Reading config");
		daemon.ReadConfig(argc, argv);
	} catch (kams_daemon::KamsDmnException &e) {
		dolog(0, "Exception occured:");
		dolog(0, "%s", e.what());
		exit(1);
	}
	dolog(10, "Starting daemon");

	try {
		daemon.StartDo();
	} catch (SzException &e) {
		dolog(0, "FATAL!: daemon killed by exception: %s", e.what());
		exit(1);
	} catch (...) {
		dolog(0, "FATAL!: daemon killed by unknown exception");
		exit(1);
	}
	dolog(0, "FATAL!: abnormal termination of event loop");
}
