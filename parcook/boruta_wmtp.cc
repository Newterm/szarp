
#include <string>
#include <map>
#include <stdio.h>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "borutadmn.h"

#include <iostream>
#include "conversion.h"		// for SC::S2a

#define WMTP_CTRLSUMCHECK true 

// definitions of auto parameters names
#define WMTP_EFFICIENCY "wydajnosc"
#define WMTP_COUNTER_MSW "licznik_msw"
#define WMTP_COUNTER_LSW "licznik_lsw"
#define WMTP_WEIGHING_POINT_SHIFTED "punkt_wazenia_przesuniety"
#define WMTP_CONVEYOR_RUNS "tasma_pracuje"

// definitions of non-auto parameters names
#define WMTP_CONVEYOR_SPEED "szybkosc_tasmociagu"
#define WMTP_STRAIN_GAUGE "odczyt_tensometru"
#define WMTP_ADDITIONAL_FUN "dodatkowa_funkcja"

/** Calculates two-char control sum of given string.
 * Ctrl sum is calculated by adding all characters in string
 * (without carry).
 * Ctrl sum is written as two chars 0..F, with the left char 
 * being most important half-byte.
 */
std::string wmtp_calculate_ctrlsum(std::string str) {
	int sum = 0;
	for (unsigned int i = 0; i < str.size(); i++)
	{
		sum += static_cast<int>(str[i]);
	}
	char ss[2];
	// pad with zeros
	sprintf(ss, "%.02X", sum % 256);
	return std::string(ss);
}

/** Parses WMTP responses */
class wmtp_response_parser {
public:
	wmtp_response_parser(driver_logger* log) :  m_address("00"), m_main_counter(0),
		m_weighing_point_shifted(false), m_conveyor_running(false), m_efficiency(0), 
		m_communication_mode('x'), m_fun_id("xx"), m_fun_val(0), m_endline(false), m_log(log)
		{
		m_values[WMTP_EFFICIENCY] = 0;
		m_values[WMTP_COUNTER_MSW] = 0;
		m_values[WMTP_COUNTER_LSW] = 0;
		m_values[WMTP_WEIGHING_POINT_SHIFTED] = 0;
		m_values[WMTP_CONVEYOR_RUNS] = 0;
	}

	/** Parses given response.
	 * @param response response obtained from scale
	 * @param ctrlsum_check check control sum of response
	 * @return true if parsing was done without error
	 */
	bool parse(std::string response, bool ctrlsum_check=false) {
		std::string sre_ctrlsum = "([0-9a-fA-F]{2})";
		std::string sre_lineend = "\r\n";
		boost::regex re("\\[([0-9]{2}),(L|R)([0-9]{10}),(C|T)([0-9]{5}),(P|A|Z)(.{2}),([0-9]{6})\\]"
			+ sre_ctrlsum + sre_lineend);
		// group names for better access to matches[x]
		enum RE_GROUPS { ALL, ADDR, MAIN_COUNTER_INFO, MAIN_COUNTER_VAL, CONVEYOR_RUNNING, EFFICIENCY,
			COMMUNICATION_MODE, FUN_ID, FUN_VAL, CTRL_SUM};
		
		m_response = response;
		m_log->log(10, "Response being parsed: %s", response.c_str());
		boost::cmatch matches;
		if (boost::regex_match(response.c_str(), matches, re)) {
			m_log->log(10, "Parse OK");
			// check ctrl sum
			if (ctrlsum_check) {
				boost::regex rectrl("\\[([^]]*)\\]" + sre_ctrlsum + sre_lineend);
				boost::cmatch ctrl_matches;
				
				boost::regex_match(response.c_str(), ctrl_matches, rectrl);
				std::string command_root(ctrl_matches[1].first, ctrl_matches[1].second);
				std::string ctrl_sum(ctrl_matches[2].first, ctrl_matches[2].second);
	
				std::string calculated_ctrl_sum = wmtp_calculate_ctrlsum(command_root);
				if (ctrl_sum != calculated_ctrl_sum) {
					m_log->log(1, "Ctrl sum mismatch! calculated: %s received: %s",
						calculated_ctrl_sum.c_str(), ctrl_sum.c_str());
					//TODO error? ignore?
				} else {
					m_log->log(10, "Ctrl sum OK.");
				}
			}

			// extract information
			// 1: standard info (in every response)
			m_address.assign(matches[ADDR].first, matches[ADDR].second);
			
			std::string main_counter(matches[MAIN_COUNTER_VAL].first, matches[MAIN_COUNTER_VAL].second);
			m_main_counter = boost::lexical_cast<unsigned int>(main_counter);
			m_values[WMTP_COUNTER_MSW] = (unsigned short)(m_main_counter >> 16);
			m_values[WMTP_COUNTER_LSW] = (unsigned short)(m_main_counter & 0xffff);

			char main_counter_info = *(matches[MAIN_COUNTER_INFO].first);
			if (main_counter_info == 'L') {
				m_weighing_point_shifted = false;
				m_values[WMTP_WEIGHING_POINT_SHIFTED] = 0;
			} else {
				// TODO configuration variable P7 holds info about weighing point
				m_weighing_point_shifted = true;
				m_values[WMTP_WEIGHING_POINT_SHIFTED] = 1;
			}

			char conveyor_running = *(matches[CONVEYOR_RUNNING].first);
			if (conveyor_running == 'C') {
				m_conveyor_running = true;
				m_values[WMTP_CONVEYOR_RUNS] = 1;
			} else {
				m_conveyor_running = false;
				m_values[WMTP_CONVEYOR_RUNS] = 0;
			}
			
			std::string efficiency(matches[EFFICIENCY].first, matches[EFFICIENCY].second);
			m_values[WMTP_EFFICIENCY] = boost::lexical_cast<unsigned short>(efficiency);

			m_communication_mode = *(matches[COMMUNICATION_MODE].first);
		
			// 2: info depending on requested function
			m_fun_id.assign(matches[FUN_ID].first, matches[FUN_ID].second);
			std::string fun_value(matches[FUN_VAL].first, matches[FUN_VAL].second);
			m_fun_val = boost::lexical_cast<unsigned int>(fun_value);

		} else {
			m_log->log(1, "Parse Failed");
			return false;
		}
		return true;
	}

	/** Prints all info gathered from last response */
	void print_info() {
		std::cout << "address: " << m_address << std::endl;
		std::cout << "main_counter: " << m_main_counter << std::endl;
		std::cout << "main_counter msw: " << m_values[WMTP_COUNTER_MSW] << std::endl;
		std::cout << "main_counter lsw: " << m_values[WMTP_COUNTER_LSW] << std::endl;
		std::cout << "weighing_point_shifted: " << m_values[WMTP_WEIGHING_POINT_SHIFTED] << std::endl;
		std::cout << "conveyor_running: " << m_values[WMTP_CONVEYOR_RUNS] << std::endl;
		std::cout << "efficiency: " << m_values[WMTP_EFFICIENCY] << std::endl;
		std::cout << "communication_mode: " << m_communication_mode << std::endl;
		std::cout << "fun_id: " << m_fun_id << std::endl;
		std::cout << "fun_val: " << m_fun_val << std::endl;
	}

	/** Return auto param of given name */
	unsigned short get_auto_param(std::string name) {
		return m_values[name];
	}

	/** Returns least significant 2B of returned function value */
	unsigned short get_fun_value_lsw() {
		return m_fun_val & 0xffff;
	}

protected:
	std::map<std::string, unsigned short> m_values;
					/**< values for all auto-parameters, accessible by name */

	// values read from last valid message
	std::string m_address;		/**< device address */
	int m_main_counter;		/**< main scale counter [kg] */
	bool m_weighing_point_shifted;	/**< true if weighing point (affecting main counter) has been shifted */
	bool m_conveyor_running;	/**< true if conveyor is running */
	unsigned int m_efficiency;	/**< scale efficiency [.x t/h] */
	char m_communication_mode;	/**< P - "display" mode
					 * Z - "control" mode
					 * A - "locked" mode */
	std::string m_fun_id;		/**< id of function realised */
	unsigned int m_fun_val;		/**< value of realised function */

protected:
	bool m_endline;			/**< true if \n occured in feed() */
	std::string m_response;		/**< whole response being parsed */

	driver_logger *m_log;
	
};


/** produces WMTP requests. (WMTP "Display" mode). */
class wmtp_request_producer {
public:

	/** initializes device address and prepares a map of requests */
	int initialize(std::string device_address) {
		if (device_address.size() != 2)
			return 1;
		m_device_address = device_address;

		// initialize possible requests map
		m_requests[WMTP_CONVEYOR_SPEED] = compose_request(m_device_address, "A9", "000000", "0");
		m_requests[WMTP_STRAIN_GAUGE] = compose_request(m_device_address, "AA", "000000", "0");

		return 0;
	}

	/** returns request for provided parameter name.
	 * Each response contains additionally additional info: @see wmtp_response_parser
	 */
	std::string produce(std::string param_name) {
		// TODO check if requested param exists
		return m_requests[param_name];	
	}

protected:
	
	/** creates a "display" mode command from given parameters 
	 * @param device_address 2-char address of wmtp device
	 * @param f_num 2-char function code
	 * @param constant 6-char constant
	 * @param order_num 1-char order number */
	std::string compose_request(std::string device_address, std::string f_num, std::string constant, std::string order_num) {
		std::string lp = "[";
		std::string rp = "]";
		std::string sep = ",";
		std::string P = "P";
		std::string R = "R";
		std::string ending = "\r\n";

		std::string command_root = device_address + sep + P + f_num + sep + constant + sep + R + order_num;
		std::string ctrl_sum = wmtp_calculate_ctrlsum(command_root);
		std::string command = lp + command_root + rp + ctrl_sum + ending;
		return command;
	}

	std::string m_device_address;			/**< address of WMTP device */
	std::map<std::string, std::string> m_requests;	/**< stores available requests */
};

class wmtp_driver : public tcp_client_driver {
public:
	wmtp_driver() : m_log(this), m_read(NULL), m_auto_params_retrieved(false), m_response_parser(&m_log)
	{
		m_default_param = WMTP_CONVEYOR_SPEED;
		m_auto_params[m_default_param] = false;

		m_auto_params[WMTP_STRAIN_GAUGE] = false;
		m_auto_params[WMTP_ADDITIONAL_FUN] = false;

		m_auto_params[WMTP_EFFICIENCY] = true;
		m_auto_params[WMTP_COUNTER_MSW] = true;
		m_auto_params[WMTP_COUNTER_LSW] = true;
		m_auto_params[WMTP_WEIGHING_POINT_SHIFTED] = true;
		m_auto_params[WMTP_CONVEYOR_RUNS] = true;
	}

	const char* driver_name() { return "wmtp_driver"; }

	virtual int configure(TUnit* unit, short* read, short *send);
	
	virtual void connection_error(struct bufferevent *bufev);

	virtual void scheduled(struct bufferevent* bufev, int fd);
	
	virtual void data_ready(struct bufferevent* bufev, int fd);

	/** callback for timer */
	static void timeout_cb(int fd, short event, void *wmpt_driver);

protected:
	/** starts timeout timer */
	void start_timer();
	/** stops timeout timer */
	void stop_timer();
	/** called at the end of every cycle (or error), sets all unititialized
	 * params to no_data and notifies manager */
	void finalize();
	/** sends next request to device.
	 * Manages begin and end of a cycle. */
	void request_next(struct bufferevent* bufev);
	/** reads as many chars from bufev as available
	 * @return true, if whole line (\n) was read */
	bool read_line(struct bufferevent* bufev);

	/** writes value to shared memory */
	void write_param_value(std::string param_name, short value) {
		if (m_requested_params.find(param_name) != m_requested_params.end()) {
			unsigned int offset = m_requested_params[param_name];
			*((unsigned short*)&m_read[offset]) = value;
		}
	}
	/** debug */
	short read_param_value(std::string param_name) {
		if (m_requested_params.find(param_name) != m_requested_params.end()) {
			unsigned int offset = m_requested_params[param_name];
			return *((unsigned short*)&m_read[offset]);
		}
		return -1;
	}
	
protected:
	typedef std::map<std::string, unsigned int> offset_map;

	driver_logger m_log;

	std::string m_address;		/**< address of WMTP device */
	short *m_read;			/**< buffer for writing received params values */

	std::string m_default_param;	/**< if no other non-auto parameter has been requested,
					 * all requests ask for this parameter value */
	std::map<std::string, bool> m_auto_params;
					/**< map of parameters names that can be handled by driver
					 * string: name, bool: if true, parameter value is
					 * present in response to every request */
	offset_map m_requested_params;
					/**< params declared for this unit, with offsets in shared memory */
	std::vector<std::string> m_necessary_requests;
					/**< all parameters that should be requested to get values of requested params */
	std::vector<std::string>::iterator m_last_request_it;
					/**< iterator to last request made (waiting for response) */
	bool m_auto_params_retrieved;	/**< true if auto-params values were correctly retrieved during cycle */
	wmtp_request_producer m_request_producer;
					/**< produces requests for given WMTP device */
	wmtp_response_parser m_response_parser;
					/**< parses responses from WMTP device */
	std::string m_input_buffer;
					/**< buffer storing incoming chars */
	struct event m_timer;		/**< for timeout handling */

};

void wmtp_driver::start_timer() {
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	evtimer_add(&m_timer, &tv); 
}

void wmtp_driver::stop_timer() {
	event_del(&m_timer);
}

void wmtp_driver::connection_error(struct bufferevent *bufev) {
	stop_timer();
	if (m_last_request_it != m_necessary_requests.end()) {
		finalize();
	}
}

void wmtp_driver::scheduled(struct bufferevent* bufev, int fd) {
	request_next(bufev);
}

void wmtp_driver::finalize() {
	// set nodata for all auto-params, if they weren't retrieved
	if (m_auto_params_retrieved == false) {
		for (offset_map::iterator it = m_requested_params.begin();
			it != m_requested_params.end(); ++it) {
			
			std::string param_name = it->first;
			if (m_auto_params[param_name]) {
				write_param_value(param_name, SZARP_NO_DATA);
			}
		}
	}
	// set nodata for all non-auto params, that weren't processed
	for ( ; m_last_request_it != m_necessary_requests.end();
		++m_last_request_it) {
	
		std::string param_name = *m_last_request_it;
		write_param_value(param_name, SZARP_NO_DATA);
	}
	// notify communication layer manager
	m_manager->driver_finished_job(this);

	m_log.log(10, "WMTP driver received in cycle: ");
	for (offset_map::iterator it = m_requested_params.begin();
		it != m_requested_params.end(); ++it) {
		
		std::string param_name = it->first;
		short value = read_param_value(param_name);
		if (value == SZARP_NO_DATA)
			m_log.log(10, "%s : NO_DATA", param_name.c_str());
		else
			m_log.log(10, "%s : %d", param_name.c_str(), value);
	}
}

void wmtp_driver::timeout_cb(int fd, short event, void *_wmtp_driver) {
	wmtp_driver* z = (wmtp_driver*) _wmtp_driver;
	if (z->m_last_request_it != z->m_necessary_requests.end()) {
		z->finalize();
	}
	z->m_log.log(1, "WMTP timeout");
}

void wmtp_driver::request_next(struct bufferevent* bufev) {
	if (m_last_request_it == m_necessary_requests.end()) {
		// begin cycle
		m_last_request_it = m_necessary_requests.begin();
		m_auto_params_retrieved = false;
	} else {
		++m_last_request_it;
	}
	
	if (m_last_request_it == m_necessary_requests.end()) {
		// end cycle
		stop_timer();
		finalize();
	} else {
		// next param request
		m_input_buffer.clear();
		std::string request = m_request_producer.produce(*m_last_request_it);
		m_log.log(10, "WMTP sending request for : %s", (*m_last_request_it).c_str());
		bufferevent_write(bufev, (void*)request.c_str(), request.size());
		start_timer();
	}
}

void wmtp_driver::data_ready(struct bufferevent* bufev, int fd) {
	if (read_line(bufev) == false)
		return;
	if (m_response_parser.parse(m_input_buffer, WMTP_CTRLSUMCHECK)) {
		// copy all auto-params
		if (m_auto_params_retrieved == false) {
			for (offset_map::iterator it = m_requested_params.begin();
				it != m_requested_params.end(); ++it) {
			
				std::string param_name = it->first;
				if (m_auto_params[param_name]) {
					write_param_value(param_name,
						m_response_parser.get_auto_param(param_name));
				}
			}
			m_auto_params_retrieved = true;
		}
		// copy the received non-auto param
		// TODO MSW support if necessary
		std::string param_name = *m_last_request_it;
		write_param_value(param_name, m_response_parser.get_fun_value_lsw());
	} else {
		// set nodata for non-auto param
		// TODO MSW support if necessary
		std::string param_name = *m_last_request_it;
		write_param_value(param_name, SZARP_NO_DATA);
	}
	request_next(bufev);
}

bool wmtp_driver::read_line(struct bufferevent* bufev) {
	char c;
	while (bufferevent_read(bufev, &c, 1)) {
		m_input_buffer.push_back(c);
		if (c == '\n')
			return true;
	}
	return false;
}

int wmtp_driver::configure(TUnit* unit, short* read, short *send) {
	// read address
	m_address = unit->getAttribute<std::string>("extra:address", "");
	if (m_address.empty())
		return 1;
	if (m_address.size() != 2)
		return 1;
	m_request_producer.initialize(m_address);

	m_log.log(0, "Boruta WMTP configure() - address is: %s", m_address.c_str());

	// read target memory space
	m_read = read;
	
	// read requested params
	unsigned int offset_in_shared_memory = 0;
	for (auto param = unit->GetFirstParam(); param != nullptr; param = param->GetNext()) {
		std::string param_name = param->getAttribute<std::string>("extra:name", "");
		if (param_name.empty()) {
			m_log.log(0, "No extra:name in param %ls", param->GetName().c_str());
			return 1;
		}

		// check if param name is valid
		if (m_auto_params.find(param_name) == m_auto_params.end()) {
			m_log.log(0, "ERROR: param extra:name invalid: %s", param_name.c_str());
			return 1;
		}
		m_requested_params[param_name] = offset_in_shared_memory;

		// if param is non-auto, add to requests
		if (m_auto_params[param_name] == false) {
			m_necessary_requests.push_back(param_name);
		}
		
		offset_in_shared_memory++;
	}
	
	// if only auto params where requested, add default param to requests
	if ((m_requested_params.size() != 0) && (m_necessary_requests.size() == 0)) {
		m_necessary_requests.push_back(m_default_param);
	}
	
	m_log.log(2, "Requested params are: ");
	for (offset_map::iterator it = m_requested_params.begin();
		it != m_requested_params.end(); ++it) {
		
		std::string param_name = it->first;
		m_log.log(2, "%s at offset: %d", it->first.c_str(), it->second);
	}
	m_log.log(2, "Necessary requests are: ");
	for (std::vector<std::string>::iterator it = m_necessary_requests.begin(); it != m_necessary_requests.end(); ++it) {
		m_log.log(2, "%s", (*it).c_str());
	}
	
	// initialize requests iterator
	m_last_request_it = m_necessary_requests.end();

	// initialize timeout timer
	evtimer_set(&m_timer, timeout_cb, this);
	event_base_set(m_event_base, &m_timer);
	return 0;
}


tcp_client_driver* create_wmtp_tcp_client() {
	return new wmtp_driver();
}




