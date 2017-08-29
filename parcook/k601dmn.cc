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
 * (C) Pawe³ Kolega 2007
 * (C) Pawe³ Pa³ucha 2009
 * $Id$
 */
/*
 @description_start
 @class 4
 @devices Kamstrup Multical 601 heatmeter.
 @devices.pl Ciep³omierz Kamstrup Multical 601.
 @protocol Kamstrup Heat Meter Protocol.

 @comment Kamstrup Company does not allow to publish communication protocol, so this 
 daemon needs closed-source plugin, loaded from szarp-prop-plugins.so.
 @comment.pl Producent ciep³omierzy Kamstrup nie zezwoli³ na upublicznienie protoko³u
 komunikacyjnego, wobec czego sterownik ten wymaga zamkniêtej wtyczki, dostêpnej w bibliotece
 szarp-prop-plugins.so.
 
 @config Example of params.xml configuration, attributes from 'm601' namespace are mandatory.
 @config.pl Przyk³adowa konfiguracja w pliku params.xml, atrybuty z przetrzenie nazw 'm601' 
 s± obowi±zkowe.
 @config_example
 <device 
      xmlns:m601="http://www.praterm.com.pl/SZARP/ipk-extra"
      daemon="/opt/szarp/bin/k601dmn" 
      path="/dev/ttyA11"
      m601:delay_between_requests="10">
      <unit id="1">
              <param
                      name="..."
                      ...
                      k601:register="0x80"
                      k601:type="lsb"
                      k601:multiplier="100">
                      ...
              </param>
              <param
                      name="..."
                      ...
                      k601:address="0x80"
                      k601:type="msb"
			k601:multiplier="100">
                      ...
              </param>
              ...
              <param
                      name="..."
                      ...
                      k601:address="0x81"
                      k601:type="single"
			k601:multiplier="1">
                      ...
              </param>
      </unit>
 </device>
 @description_end
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <termio.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <libgen.h>

#include "szarp.h"
#include "conversion.h"

#include "ipchandler.h"
#include "liblog.h"

#include "k601-prop-plugin.h"
#include "serialport.h"
#include "serialadapter.h"
#include "atcconn.h"
#include "daemonutils.h"
#include "xmlutils.h"
#include "custom_assert.h"

#define REGISTER_NOT_FOUND -1

#define RESTART_INTERVAL_MS 15000

bool single;

void dolog(int level, const char * fmt, ...)
	__attribute__ ((format (printf, 2, 3)));


void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	if (single) {
		char *l;
		va_start(fmt_args, fmt);
		int ret = vasprintf(&l, fmt, fmt_args);
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

xmlChar* get_device_node_prop(xmlXPathContextPtr xp_ctx, const char* prop) {
	xmlChar *c;
	char *e;
	int ret = asprintf(&e, "./@%s", prop);
	ASSERT(e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx, false);
	free(e);
	(void)ret;
	return c;
}

xmlChar* get_device_node_extra_prop(xmlXPathContextPtr xp_ctx, const char* prop) {
	xmlChar *c;
	char *e;
	int ret = asprintf(&e, "./@extra:%s", prop);
	ASSERT(e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx, false);
	free(e);
	(void)ret;
	return c;
}

/* This ID-s will be shown in single (test mode) */
unsigned short TO_SHOW_CODES[] =
	{ 60, 94, 63, 61, 62, 95, 96, 97, 110, 64, 65, 68, 69, 84, 85, 72, 73, 99,
		86, 87, 88, 122, 89, 91, 92, 74, 75, 80 };

/** possible types of modbus communication modes */
typedef enum {
	T_LSB,
	T_MSB,
	T_SINGLE,
	T_ERROR,
} ParamMode;

typedef enum {
	RT_LSBMSB,
	RT_SINGLE,
	RT_ERROR,
} RegisterType;

/**
 * Modbus communication config.
 */
class KamstrupInfo {
public:
	/** info about single parameter */
	class ParamInfo {
	public:
		unsigned short reg;  /**< KMP REGISTER */
		unsigned long mul;   /**< ADDITIONAL MULTIPLIER */
		ParamMode type;
	};
	
	// for ip connections must account for ping delay, chosen arbitrarily.
	// in the first implementation, was set to 10ms, but it proved too short.
	static const unsigned int DELAY_BETWEEN_CHARS = 1000;

	static const unsigned int READ_TIMEOUT = 10000; // timeout before first read is made
	unsigned int delay_between_requests_ms;	// time to sleep between full request cycles

	ParamInfo *m_params;
	int m_params_count;
	unsigned short *unique_registers;
	unsigned short unique_registers_count;

	/**
	 * @param params number of params to read
	 */
	KamstrupInfo(int params) {
		ASSERT(params >= 0);

		m_params_count = params;
		if (params > 0) {
			m_params = new ParamInfo[params];
			ASSERT(m_params != NULL);
		} else
			m_params = NULL;
	}

	~KamstrupInfo() {
		delete m_params;
		free(unique_registers);
	}

	int parseXML(xmlNodePtr node, DaemonConfig * cfg);

	int parseDevice(xmlNodePtr node);

	int parseParams(xmlNodePtr unit, DaemonConfig * cfg);

	RegisterType DetermineRegisterType(unsigned short RegisterAddress);

	int FindRegisterIndex(unsigned short RegisterAddress, ParamMode PM);

	unsigned long DetermineRegisterMultiplier(unsigned short
						  RegisterAddress);
};

int KamstrupInfo::parseDevice(xmlNodePtr node)
{
	ASSERT(node != NULL);
	char *str;
	char *tmp;
	dolog(10, "KamstrupInfo::parseDevice");

	str = (char *)xmlGetNsProp(node,
				   BAD_CAST("delay_between_requests"),
				   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		dolog(0,
		       "attribute k601:delay_between_requests not found in device element, line %ld",
		       xmlGetLineNo(node));
		return 1;
	}

	delay_between_requests_ms = strtol(str, &tmp, 0) * 1000;
	if (tmp[0] != 0) {
		dolog(0,
		       "error parsing k601:delay_between_requests attribute ('%s'), line %ld",
		       str, xmlGetLineNo(node));
		return 1;
	}
	free(str);
	return 0;
}

int KamstrupInfo::parseParams(xmlNodePtr unit, DaemonConfig * cfg)
{
	int params_found = 0;
	char *str;
	char *tmp;
	unsigned short reg;
	unsigned long mul;
	ParamMode type;
	unsigned short i, j, k;
	unsigned short *registers;
	char latch;
	dolog(10, "KamstrupInfo::parseParams");

	for (xmlNodePtr node = unit->children; node; node = node->next) {
		if (node->ns == NULL)
			continue;
		if (strcmp
		    ((char *)node->ns->href,
		     (char *)SC::S2U(IPK_NAMESPACE_STRING).c_str()) != 0)
			continue;
		if (strcmp((char *)node->name, "param"))
			continue;

		str = (char *)xmlGetNsProp(node, BAD_CAST("register"),
					   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (str == NULL) {
			dolog(0,
			       "attribute k601:register not found (line %ld)",
			       xmlGetLineNo(node));
			return 1;
		}
		reg = strtol(str, &tmp, 0);
		if (tmp[0] != 0) {
			dolog(0,
			       "error parsing k601:register attribute value ('%s') - line %ld",
			       str, xmlGetLineNo(node));
			return 1;
		}
		free(str);

		str = (char *)xmlGetNsProp(node, BAD_CAST("multiplier"),
					   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (str == NULL) {
			dolog(0,
			       "attribute k601:multiplier not found (line %ld)",
			       xmlGetLineNo(node));
			free(str);
			return 1;
		}
		mul = strtol(str, &tmp, 0);
		if (tmp[0] != 0) {
			dolog(0,
			       "error parsing k601:multiplier attribute value ('%s') - line %ld",
			       str, xmlGetLineNo(node));
			return 1;
		}
		free(str);

		str = (char *)xmlGetNsProp(node, BAD_CAST("type"),
					   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (str == NULL) {
			dolog(0,
			       "attribute k601:type not found (line %ld)",
			       xmlGetLineNo(node));
			type = T_SINGLE;
		}

		if (!strcasecmp(str, "lsb"))
			type = T_LSB;
		else if (!strcasecmp(str, "msb"))
			type = T_MSB;
		else if (!strcasecmp(str, "single"))
			type = T_SINGLE;
		else {
			type = T_ERROR;
			dolog(0,
			       "attribute k601:type is illegal (%s) (line %ld)",
			       str, xmlGetLineNo(node));
			return 1;
		}

		free(str);

		if (!strcmp((char *)node->name, "param")) {
			/*
			 * for 'param' element 
			 */
			ASSERT(params_found < m_params_count);
			m_params[params_found].reg = reg;
			m_params[params_found].mul = mul;
			m_params[params_found].type = type;
			TParam *p =
			    cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->
			    GetFirstParam()->GetNthParam(params_found);
			ASSERT(p != NULL);
			(void)p;
			params_found++;
		}

	}

	/**
	 * Calculating unique addresses of registers;
	 */
	unique_registers =
	    (unsigned short *)malloc(params_found * sizeof(unsigned short));
	registers =
	    (unsigned short *)malloc(params_found * sizeof(unsigned short));
	unique_registers_count = 0;
	memset(registers, 0, params_found * sizeof(unsigned short));
	latch = 0;
	k = 0;
	for (i = 0; i < params_found; i++) {
		latch = 0;
		for (j = 0; j < i; j++) {
			if (m_params[i].reg == registers[j])
				latch = 1;
		}

		if (!latch) {
			unique_registers_count++;
			registers[i] = m_params[i].reg;
			unique_registers[k] = m_params[i].reg;
			k++;
		}
	}
	free(registers);

	ASSERT(params_found == m_params_count);

	return 0;
}

int KamstrupInfo::parseXML(xmlNodePtr node, DaemonConfig * cfg)
{
	ASSERT(node != NULL);
	dolog(10, "KamstrupInfo:parseXML");

	if (parseDevice(node))
		return 1;

	for (node = node->children; node; node = node->next) {
		if ((node->ns &&
		     !strcmp((char *)node->ns->href,
			     (char *)SC::S2U(IPK_NAMESPACE_STRING).c_str())) ||
		    !strcmp((char *)node->name, "unit"))
			break;
	}
	if (node == NULL) {
		dolog(0, "can't find element 'unit'");
		return 1;
	}

	return parseParams(node, cfg);
}

RegisterType KamstrupInfo::DetermineRegisterType(unsigned short RegisterAddress)
{
	unsigned short i;
	dolog(10, "KamstrupInfo:DetermineRegisterType");

	for (i = 0; i < m_params_count; i++) {
		if (RegisterAddress == m_params[i].reg) {
			switch (m_params[i].type) {
			case T_LSB:
				return (RT_LSBMSB);
				break;
			case T_MSB:
				return (RT_LSBMSB);
				break;
			case T_SINGLE:
				return (RT_SINGLE);
				break;
			default:
				return (RT_ERROR);
			}

		}

	}
	return (RT_ERROR);
}

int KamstrupInfo::FindRegisterIndex(unsigned short RegisterAddress,
				    ParamMode PM)
{
	int i;
	dolog(10, "KamstrupInfo::FindRegisterIndex");
	for (i = 0; i < m_params_count; i++) {
		if (RegisterAddress == m_params[i].reg
		    && PM == m_params[i].type)
			return i;
	}
	return REGISTER_NOT_FOUND;
}

unsigned long KamstrupInfo::
DetermineRegisterMultiplier(unsigned short RegisterAddress)
{
	int i;
	char lsb_msb_mode = 0;
	unsigned long multiplier = 1;
	dolog(10, "KamstrupInfo::DetermineRegisterMultiplier");

	for (i = 0; i < m_params_count; i++) {
		if (RegisterAddress == m_params[i].reg
		    && m_params[i].type == T_LSB) {
			if (!lsb_msb_mode) {
				multiplier = m_params[i].mul;
			} else {
				multiplier += m_params[i].mul;
				multiplier /= 2;
			}
		}
		if (RegisterAddress == m_params[i].reg
		    && m_params[i].type == T_MSB) {
			if (!lsb_msb_mode) {
				multiplier = m_params[i].mul;
			} else {
				multiplier += m_params[i].mul;
				multiplier /= 2;
			}
		}

		if (RegisterAddress == m_params[i].reg
		    && m_params[i].type == T_SINGLE) {
			multiplier = m_params[i].mul;
		}
	}
	return multiplier;
}


/** Daemon reading data from heatmeters.
 * Can use either a SerialPort (if 'path' is declared in params.xml)
 * or a SerialAdapter connection
 * (if 'extra:tcp-ip' and optionally 'extra:tcp-data-port' and 'extra:tcp-cmd-port' are declared)
 */
class K601Daemon: public ConnectionListener {
public:
	class K601Exception : public SzException {
		SZ_INHERIT_CONSTR(K601Exception, SzException)
	};
	typedef enum {RESTART, SEND, READ, PROCESS, FINALIZE} CommunicationState;

	K601Daemon() :
			m_state(SEND),
			m_id(""),
			m_curr_register(0),
			m_params_read_in_cycle(0),
			m_connection(nullptr),
			plugin(nullptr)
	{
		m_serial_conf.speed = 1200;
		m_serial_conf.char_size = SerialPortConfiguration::CS_8;
		m_serial_conf.stop_bits = 2;
		m_event_base = event_base_new();
	}

	~K601Daemon()
	{
		if (m_connection != nullptr) {
			m_connection->Close();
			delete m_connection;
		}
		if (plugin != nullptr) {
			dlclose(plugin);
			plugin = nullptr;
		}
	}

	void Init(int argc, char *argv[]);
	int GetTcpDataPort(const xmlXPathContextPtr&, int);
	int GetTcpCmdPort(const xmlXPathContextPtr&, int);
	int get_int_attr_def(const xmlXPathContextPtr&, const char *, int);

	/** Start event-based state machine */
	void StartDo();

protected:
	/** Schedule next state machine step */
	void ScheduleNext(unsigned int wait_ms=0);

	/** Callback for next step of timed state machine. */
	static void TimerCallback(int fd, short event, void* thisptr);

	/** One step of state machine */
	void Do();

	/** ConnectionListener interface */
	virtual void ReadError(const BaseConnection *conn, short event);
	virtual void ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data);
	virtual void SetConfigurationFinished(const BaseConnection *conn);
	virtual void OpenFinished(const BaseConnection *conn);

	void ProcessResponse();
	void SetCurrRegisterNoData();
	void SetNoData();

	/** Sets NODATA and schedules reconnect */
	void SetRestart()
	{
		m_state = RESTART;
		SetNoData();
		ipc->GoParcook();
	}

protected:
	CommunicationState m_state;	/**< state of communication state machine */
	struct event m_ev_timer;
					/**< event timer for calling QueryTimerCallback */
	unsigned int m_query_interval_ms;
					/**< interval between single queries */
	unsigned int m_wait_for_data_ms;
					/**< time for which daemon waits for data from Kamstrup meter */

	std::string m_id;		/**< ID of given kamsdmn */

	int m_curr_register;	/**< number of currently processed register */
	int m_params_read_in_cycle;	/**< count of params which were successfully read in a cycle (not NODATA) */
	BaseConnection *m_connection;
	std::vector<unsigned char> m_read_buffer;	/**< buffer for data received from Kamstrup meter */
	bool m_data_was_read;	/**< was any data read since last check? */
	SerialPortConfiguration m_serial_conf;
	struct event_base *m_event_base;

	/** from previous implementation */
	void *plugin;
	DaemonConfig *cfg;
	KamstrupInfo *kamsinfo;
	IPCHandler *ipc;
	KMPSendInterface *MyKMPSendClass;
	KMPReceiveInterface *MyKMPReceiveClass;
	char e_c;
	unsigned short actual_register;
	RegisterType register_type;
	double tmp_float_value;
	int tmp_int_value;

	KMPSendCreate_t *createSend;
	KMPReceiveCreate_t *createReceive;
	KMPSendDestroy_t *destroySend;
	KMPReceiveDestroy_t *destroyReceive;
};

void K601Daemon::ReadError(const BaseConnection *conn, short event)
{
	sz_log(1, "%s: ReadError, closing connection..", m_id.c_str());
	m_connection->Close();
	SetRestart();
	ScheduleNext(RESTART_INTERVAL_MS);
}

void K601Daemon::SetConfigurationFinished(const BaseConnection *conn) {
	std::string info = m_id + ": SetConfigurationFinished.";
	dolog(2, "%s: %s", m_id.c_str(), info.c_str());
	m_state = SEND;
	m_curr_register = 0;
	ScheduleNext();
}

void K601Daemon::ReadData(const BaseConnection *conn,
		const std::vector<unsigned char>& data)
{
	m_read_buffer.insert(m_read_buffer.end(), data.begin(), data.end());
	m_data_was_read = true;
	ScheduleNext();
}

void K601Daemon::Init(int argc, char *argv[])
{
	cfg = new DaemonConfig("k601dmn");

	if (cfg->Load(&argc, argv)) {
		throw K601Exception("ERROR!: Load config failed");
	}
	kamsinfo =
	    new KamstrupInfo(cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->
			     GetParamsCount());
	if (kamsinfo->parseXML(cfg->GetXMLDevice(), cfg)) {
		throw K601Exception("ERROR!: Parse XML failed");
	}

	xmlDocPtr doc;

	/* get config data */
	doc = cfg->GetXMLDoc();
	ASSERT(doc != NULL);

	/* prepare xpath */
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(doc);
	ASSERT(xp_ctx != NULL);
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "extra",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	ASSERT(ret == 0);
	(void)ret;

	xp_ctx->node = cfg->GetXMLDevice();
	xmlChar *c = get_device_node_extra_prop(xp_ctx, "tcp-ip");
	if (c == nullptr) {
		xmlChar *atc_ip = get_device_node_extra_prop(xp_ctx, "atc-ip");
		if(atc_ip == nullptr) {
			xmlChar *path = get_device_node_prop(xp_ctx, "path");
			if (path == nullptr) {
				throw K601Exception("ERROR!: neither IP nor device "
						"path has been specified");
			}
			std::string _path;
			_path.assign((const char*)path);
			xmlFree(path);
			m_id = _path;

			/* Setting serial configuration */
			try {
				SerialPort *port = new SerialPort(m_event_base);
				port->Init(_path);
				m_connection = port;
			} catch (SerialPortException &e) {
				dolog(0, "%s: %s", m_id.c_str(), e.what());
				SetRestart();
				ScheduleNext(RESTART_INTERVAL_MS);
			}
		}
		else {
			std::string _ip;
			int _data_port;
			int _cmd_port;
			_ip.assign((const char*)atc_ip);
			xmlFree(atc_ip);
			_data_port = GetTcpDataPort(xp_ctx, AtcConnection::DEFAULT_DATA_PORT);
			_cmd_port = GetTcpCmdPort(xp_ctx, AtcConnection::DEFAULT_CONTROL_PORT);
			m_id = _ip + ":" + std::to_string(_data_port);

			/* Setting atc configuration */
			try {
				AtcConnection *client = new AtcConnection(m_event_base);
				client->InitTcp(_ip, _data_port, _cmd_port);
				m_connection = client;
			} catch (SerialPortException &e) {
				dolog(0, "%s: %s", m_id.c_str(), e.what());
				SetRestart();
				ScheduleNext(RESTART_INTERVAL_MS);
			}
		}
	}
	else {
		std::string _ip;
		int _data_port;
		int _cmd_port;
		_ip.assign((const char*)c);
		xmlFree(c);

		_data_port = GetTcpDataPort(xp_ctx, SerialAdapter::DEFAULT_DATA_PORT);
		_cmd_port = GetTcpCmdPort(xp_ctx, SerialAdapter::DEFAULT_CMD_PORT);
		m_id = _ip + ":" + std::to_string(_data_port);

		/* Setting tcp connecton */
		try {
			SerialAdapter *client = new SerialAdapter(m_event_base);
			client->InitTcp(_ip, _data_port, _cmd_port);
			m_connection = client;
		} catch (SerialPortException &e) {
			dolog(0, "%s: %s", m_id.c_str(), e.what());
			SetRestart();
			ScheduleNext(RESTART_INTERVAL_MS);
		}
	}

	single = cfg->GetSingle() || cfg->GetDiagno();

	if (cfg->GetSingle()) {

		printf("\
line number: %d\n\
device: %ls\n\
id: %s\n\
params in: %d\n\
Delay between requests [s]: %d\n\
Unique registers (read params): %d\n\
", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), m_id.c_str(), kamsinfo->m_params_count, kamsinfo->delay_between_requests_ms / 1000, kamsinfo->unique_registers_count);
		for (int i = 0; i < kamsinfo->m_params_count; i++)
			printf("  IN:  reg %04d multiplier %ld type %s\n",
			       kamsinfo->m_params[i].reg,
			       kamsinfo->m_params[i].mul,
			       kamsinfo->m_params[i].type == T_LSB ? "lsb" :
			       kamsinfo->m_params[i].type == T_MSB ? "msb" :
			       kamsinfo->m_params[i].type ==
			       T_SINGLE ? "single" : "error");

		for (int i = 0; i < kamsinfo->unique_registers_count; i++)
			printf("unique register[%d] = %d\n", i,
			       kamsinfo->unique_registers[i]);

	}

	try {
		ipc = new IPCHandler(cfg);
	} catch (const std::exception& e) {
		throw K601Exception("ERROR!: IPC init failed");
	}

	dolog(2, "starting main loop");

	char *plugin_path;
	ret = asprintf(&plugin_path, "%s/szarp-prop-plugins.so", dirname(argv[0]));
	(void)ret;
	plugin = dlopen(plugin_path, RTLD_LAZY);
	if (plugin == NULL) {
		dolog(1,
		       "Cannot load %s library: %s",
		       plugin_path, dlerror());
		exit(1);
	}
	dlerror();
	free(plugin_path);
	createSend = (KMPSendCreate_t *) dlsym(plugin, "CreateKMPSend");
	createReceive = (KMPReceiveCreate_t *) dlsym(plugin, "CreateKMPReceive");
	destroySend = (KMPSendDestroy_t *) dlsym(plugin, "DestroyKMPSend");
	destroyReceive = (KMPReceiveDestroy_t *) dlsym(plugin, "DestroyKMPReceive");
	if ((createSend == NULL) || (createReceive == NULL) || (destroySend == NULL)
			|| (destroyReceive == NULL)) {
		dolog(1, "Error loading symbols frop prop-plugins.so library: %s",
				dlerror());
		dlclose(plugin);
		exit(1);
	}
}

int K601Daemon::GetTcpDataPort(const xmlXPathContextPtr& xp_ctx, int default_value) {
	return get_int_attr_def(xp_ctx, "tcp-data-port", default_value);
}

int K601Daemon::GetTcpCmdPort(const xmlXPathContextPtr& xp_ctx, int default_value) {
	return get_int_attr_def(xp_ctx, "tcp-cmd-port", default_value);
}

int K601Daemon::get_int_attr_def(const xmlXPathContextPtr& xp_ctx, const char *name, int default_value) {
	xmlChar* str_value = get_device_node_extra_prop(xp_ctx, name);
	if (str_value == nullptr) {
		dolog(2, "Unspecified '%s', assuming default: %hu", name, default_value);
		return default_value;
	}

	int value = 0;
	try {
		value = std::stoi((char *)str_value);
	} catch(const std::logic_error& e) {
		xmlFree(str_value);
		throw K601Exception("Error!: Invalid " + std::string(name) + " value: " + std::string((char *)str_value));
	}
	xmlFree(str_value);
	return value;
}

void K601Daemon::StartDo() {
	evtimer_set(&m_ev_timer, TimerCallback, this);
	event_base_set(m_event_base, &m_ev_timer);
	try {
		m_connection->AddListener(this);
		dolog(10, "%s: Opening connection...", m_id.c_str());
		m_connection->Open();
	} catch (SerialPortException &e) {
		dolog(1, "%s: %s", m_id.c_str(), e.what());
		SetRestart();
		ScheduleNext(RESTART_INTERVAL_MS);
	}
	event_base_dispatch(m_event_base);
}

void K601Daemon::OpenFinished(const BaseConnection *conn)
{
	std::string info = m_id + ": connection established, setting configuration..";
	dolog(2, "%s: %s", m_id.c_str(), info.c_str());
	m_connection->SetConfiguration(m_serial_conf);
}

void K601Daemon::Do()
{
	unsigned int wait_ms = 0;
	std::vector<unsigned char> query;
	switch (m_state) {
		case SEND:
			dolog(10, "SEND");
			actual_register = kamsinfo->unique_registers[m_curr_register];
			register_type = kamsinfo->DetermineRegisterType(actual_register);
			MyKMPSendClass = createSend();
			MyKMPSendClass->CreateQuery(0x10, 0x3F, actual_register);
			query = MyKMPSendClass->GetQuery();
			m_read_buffer.clear();
			m_data_was_read = false;
			try {
				m_connection->WriteData(&query[0], query.size());
				m_state = READ;
			} catch (SerialPortException &e) {
				dolog(1, "%s: %s", m_id.c_str(), e.what());
				destroySend(MyKMPSendClass);
				SetRestart();
				wait_ms = RESTART_INTERVAL_MS;
				break;
			}
			destroySend(MyKMPSendClass);
			MyKMPReceiveClass = createReceive();
			wait_ms = kamsinfo->READ_TIMEOUT;
			break;
		case READ:
			dolog(10, "READ");
			if (!m_data_was_read) {
				if (m_read_buffer.size() > 0) {
					m_state = PROCESS;
				} else {
					dolog(10, "setting current register to nodata - buffer is empty");
					SetCurrRegisterNoData();
					m_state = FINALIZE;
				}
			} else {
				if (m_read_buffer.size() > MyKMPReceiveClass->GetBufferSize()) {
					dolog(10, "setting current register to nodata - too many chars in buffer");
					SetCurrRegisterNoData();
					m_state = FINALIZE;
				} else {
					m_data_was_read = false;
					wait_ms = kamsinfo->DELAY_BETWEEN_CHARS;
				}
			}
			break;
		case PROCESS:
			dolog(10, "PROCESS");
			if (MyKMPReceiveClass->ReceiveResponse(m_read_buffer) ==
					MyKMPReceiveClass->GetREAD_OK()) {
				ProcessResponse();
				m_params_read_in_cycle++;
			} else {
				dolog(10, "setting current register to nodata - parser reported error");
				SetCurrRegisterNoData();
			}
			m_state = FINALIZE;
			break;
		case FINALIZE:
			dolog(10, "FINALIZE");
			ipc->GoParcook();
			destroyReceive(MyKMPReceiveClass);
			if (cfg->GetSingle())
				printf("\n");
			m_curr_register++;
			if (m_curr_register > kamsinfo->unique_registers_count) {
				if (m_params_read_in_cycle == 0) {
					SetRestart();
					wait_ms = RESTART_INTERVAL_MS;
					break;
				}
				dolog(10, "STARTING NEW CYCLE");
				m_curr_register = 0;
				m_params_read_in_cycle = 0;
				// we don't want to set NODATA at this point, as the daemon may not finish a full cycle in 10s
				if (!cfg->GetSingle()) {
					dolog(10, "WAITING....");
					wait_ms = kamsinfo->delay_between_requests_ms;
				}
			}
			m_state = SEND;
			break;
		case RESTART:
			dolog(10, "RESTART");
			try {
				m_connection->Close();
				m_connection->Open();
			} catch (SerialPortException &e) {
				dolog(1, "%s: %s %s", m_id.c_str(), "Restart failed:", e.what());
				SetNoData();
				ipc->GoParcook();
				wait_ms = RESTART_INTERVAL_MS;
				dolog(10, "Schedule next (restart) in %dms", wait_ms);
				ScheduleNext(wait_ms);
			}
			// in the case of restart we should either
			// get OpenFinished event or ReadError
			// which should ScheduleNext on themselves
			return;
	}
	dolog(10, "Schedule next in %dms", wait_ms);
	ScheduleNext(wait_ms);
}

void K601Daemon::SetNoData()
{
	for (int i = 0; i < ipc->m_params_count; i++) {
		ipc->m_read[i] = SZARP_NO_DATA;
	}
}

void K601Daemon::SetCurrRegisterNoData()
{
	switch (register_type) {
	case RT_LSBMSB:
		ipc->m_read[kamsinfo->
				FindRegisterIndex
				(actual_register, T_LSB)] =
			SZARP_NO_DATA;
		ipc->m_read[kamsinfo->
				FindRegisterIndex
				(actual_register, T_MSB)] =
			SZARP_NO_DATA;
		break;
	case RT_SINGLE:
		ipc->m_read[kamsinfo->
				FindRegisterIndex
				(actual_register,
				 T_SINGLE)] = SZARP_NO_DATA;
		break;
	case RT_ERROR:
		break;
	}
}

void K601Daemon::ProcessResponse()
{
	MyKMPReceiveClass->UnStuffResponse();

	if (MyKMPReceiveClass->CheckResponse() ==
		MyKMPReceiveClass->GetRESPONSE_OK()) {

		tmp_float_value =
			MyKMPReceiveClass->
			ReadFloatRegister(&e_c);
		tmp_int_value =
			(int)(tmp_float_value *
			  (double)kamsinfo->
			  DetermineRegisterMultiplier
			  (actual_register));

		if (e_c ==
			MyKMPReceiveClass->GetCONVERT_OK()) {

			switch (register_type) {
			case RT_LSBMSB:
				ipc->m_read[kamsinfo->
						FindRegisterIndex
						(actual_register,
						 T_LSB)] =
					tmp_int_value &
					0xffff;
				ipc->m_read[kamsinfo->
						FindRegisterIndex
						(actual_register,
						 T_MSB)] =
					tmp_int_value >> 16;

				break;
			case RT_SINGLE:
				ipc->m_read[kamsinfo->
						FindRegisterIndex
						(actual_register,
						 T_SINGLE)]
					= tmp_int_value;
				break;
			case RT_ERROR:
				break;
			}
			if (cfg->GetSingle())
				printf("Value[%d]: %d\n",
					   m_curr_register,tmp_int_value);
		} else {
			switch (register_type) {
			case RT_LSBMSB:
				ipc->m_read[kamsinfo->
						FindRegisterIndex
						(actual_register,
						 T_LSB)] =
					SZARP_NO_DATA;
				ipc->m_read[kamsinfo->
						FindRegisterIndex
						(actual_register,
						 T_MSB)] =
					SZARP_NO_DATA;
				break;
			case RT_SINGLE:
				ipc->m_read[kamsinfo->
						FindRegisterIndex
						(actual_register,
						 T_SINGLE)]
					= SZARP_NO_DATA;
				break;

			case RT_ERROR:
				break;
			}
			if (cfg->GetSingle())
				printf
					("Value[%d]: SZARP_NO_DATA (BAD CONVERT)\n", m_curr_register);
		}
	}
}

void K601Daemon::ScheduleNext(unsigned int wait_ms)
{
	evtimer_del(&m_ev_timer);
	const struct timeval tv = ms2timeval(wait_ms);
	evtimer_add(&m_ev_timer, &tv);
}

void K601Daemon::TimerCallback(int fd, short event, void* thisptr)
{
	reinterpret_cast<K601Daemon*>(thisptr)->Do();
}

int main(int argc, char *argv[])
{
	K601Daemon daemon;

	try {
		daemon.Init(argc, argv);
	} catch (const std::exception& e) {
		dolog(0, "Error while initializing daemon: %s, exiting.", e.what());
		exit(1);
	}

	try {
		daemon.StartDo();
	} catch (SzException &e) {
		dolog(0, "FATAL!: daemon killed by exception: %s", e.what());
		exit(1);
	} catch (...) {
		dolog(0, "FATAL!: daemon killed by unknown exception");
		exit(1);
	}
}
