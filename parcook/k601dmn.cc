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
      m601:delay_between_chars="10000"
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
#include "xmlutils.h"

#define REGISTER_NOT_FOUND -1

bool single;

void dolog(int level, const char * fmt, ...)
	__attribute__ ((format (printf, 2, 3)));


void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	if (single) {
		char *l;
		va_start(fmt_args, fmt);
		vasprintf(&l, fmt, fmt_args);
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
	asprintf(&e, "./@%s", prop);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	return c;
}

xmlChar* get_device_node_extra_prop(xmlXPathContextPtr xp_ctx, const char* prop) {
	xmlChar *c;
	char *e;
	asprintf(&e, "./@extra:%s", prop);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
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
	unsigned long delay_between_chars;
	unsigned short delay_between_requests;

	ParamInfo *m_params;
	int m_params_count;
	unsigned short *unique_registers;
	unsigned short unique_registers_count;

	/**
	 * @param params number of params to read
	 */
	KamstrupInfo(int params) {
		assert(params >= 0);

		m_params_count = params;
		if (params > 0) {
			m_params = new ParamInfo[params];
			assert(m_params != NULL);
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

	void SetNoData(IPCHandler * ipc);
};

int KamstrupInfo::parseDevice(xmlNodePtr node)
{
	assert(node != NULL);
	char *str;
	char *tmp;
	dolog(10, "KamstrupInfo::parseDevice");

	str = (char *)xmlGetNsProp(node,
				   BAD_CAST("delay_between_chars"),
				   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		dolog(0,
		       "attribute k601:delay_between_chars not found in device element, line %ld",
		       xmlGetLineNo(node));
		return 1;
	}

	delay_between_chars = strtol(str, &tmp, 0);
	if (tmp[0] != 0) {
		dolog(0,
		       "error parsing k601:delay_between_chars attribute ('%s'), line %ld",
		       str, xmlGetLineNo(node));
		return 1;
	}
	free(str);
	str = (char *)xmlGetNsProp(node,
				   BAD_CAST("delay_between_requests"),
				   BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (str == NULL) {
		dolog(0,
		       "attribute k601:delay_between_requests not found in device element, line %ld",
		       xmlGetLineNo(node));
		return 1;
	}

	delay_between_requests = strtol(str, &tmp, 0);
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
			assert(params_found < m_params_count);
			m_params[params_found].reg = reg;
			m_params[params_found].mul = mul;
			m_params[params_found].type = type;
			TParam *p =
			    cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->
			    GetFirstParam()->GetNthParam(params_found);
			assert(p != NULL);
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

	assert(params_found == m_params_count);

	return 0;
}

void KamstrupInfo::SetNoData(IPCHandler * ipc)
{
	int i;
	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;
}

int KamstrupInfo::parseXML(xmlNodePtr node, DaemonConfig * cfg)
{
	assert(node != NULL);
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
class K601Daemon: public SerialPortListener {
public:
	class K601Exception : public MsgException { } ;
	typedef enum {RESTART, SEND, READ, PROCESS, FINALIZE} CommunicationState;

	K601Daemon() :
			m_state(SEND),
			m_id(""),
			m_path(""),
			m_ip(""),
			m_curr_register(0),
			m_serial_port(NULL),
			plugin(NULL)
	{
		serial_conf.c_iflag = 0;
		serial_conf.c_oflag = 0;
		serial_conf.c_cflag = B1200 | CS8 | CLOCAL | CREAD | CSTOPB;
		serial_conf.c_lflag = 0;
	}

	~K601Daemon()
	{
		if (m_serial_port != NULL) {
			delete m_serial_port;
		}
		if (plugin != NULL) {
			dlclose(plugin);
			plugin = NULL;
		}
	}

	void Init(int argc, char *argv[]);

	/** Start event-based state machine */
	void StartDo();

protected:
	/** Schedule next state machine step */
	void ScheduleNext(unsigned int wait_ms);

	/** Callback for next step of timed state machine. */
	static void TimerCallback(int fd, short event, void* thisptr);

	/** One step of state machine */
	void Do();

	/** SerialPortListener interface */
	virtual void ReadError(short event);
	virtual void ReadData(const std::vector<unsigned char>& data);

	void ProcessResponse();
	void SetCurrRegisterNoData();

	/** Sets NODATA and schedules reconnect */
	void SetRestart()
	{
		m_state = RESTART;
		for (int i = 0; i < ipc->m_params_count; i++) {
			ipc->m_read[i] = SZARP_NO_DATA;
		}
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
	std::string m_path;		/**< Serial port file descriptor path */

	std::string m_ip;		/**< SerialAdapter ip */
	int m_data_port;		/**< SerialAdapter data port number */
	int m_cmd_port;			/**< SerialAdapter command port number */

	int m_curr_register;	/**< number of currently processed register */
	BaseSerialPort *m_serial_port;
	std::vector<unsigned char> m_read_buffer;	/**< buffer for data received from Kamstrup meter */
	bool m_data_was_read;	/**< was any data read since last check? */
	struct termios serial_conf;

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

void K601Daemon::ReadError(short event)
{
	dolog(0, "%s: %s", m_id.c_str(), "ReadError, closing connection..");
	m_serial_port->Close();
	SetRestart();
}

void K601Daemon::ReadData(const std::vector<unsigned char>& data)
{
	m_read_buffer.insert(m_read_buffer.end(), data.begin(), data.end());
	m_data_was_read = true;
}

void K601Daemon::Init(int argc, char *argv[])
{
	cfg = new DaemonConfig("k601dmn");

	if (cfg->Load(&argc, argv)) {
		K601Exception ex;
		ex.SetMsg("ERROR!: Load config failed");
		throw ex;
	}
	kamsinfo =
	    new KamstrupInfo(cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->
			     GetParamsCount());
	if (kamsinfo->parseXML(cfg->GetXMLDevice(), cfg)) {
		K601Exception ex;
		ex.SetMsg("ERROR!: Parse XML failed");
		throw ex;
	}

	xmlDocPtr doc;

	/* get config data */
	doc = cfg->GetXMLDoc();
	assert (doc != NULL);

	/* prepare xpath */
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(doc);
	assert (xp_ctx != NULL);
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert (ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "extra",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);

	xp_ctx->node = cfg->GetXMLDevice();
	xmlChar *c = get_device_node_extra_prop(xp_ctx, "tcp-ip");
	if (c == NULL) {
		xmlChar *path = get_device_node_prop(xp_ctx, "path");
		if (path == NULL) {
			K601Exception ex;
			ex.SetMsg("ERROR!: neither IP nor device path has been specified");
			throw ex;
		}
		m_path.assign((const char*)path);
		xmlFree(path);

		m_id = m_path;
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
				K601Exception ex;
				ex.SetMsg("ERROR!: Invalid data port value: %s", tcp_data_port);
				throw ex;
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
				K601Exception ex;
				ex.SetMsg("ERROR!: Invalid cmd port value: %s", tcp_cmd_port);
				throw ex;
			}
		}
		xmlFree(tcp_cmd_port);

		std::stringstream istr;
		std::string data_port_str;
		istr << m_data_port;
		istr >> data_port_str;
		m_id = m_ip + ":" + data_port_str;
	}

	single = cfg->GetSingle() || cfg->GetDiagno();

	if (cfg->GetSingle()) {

		printf("\
line number: %d\n\
device: %ls\n\
params in: %d\n\
Delay between chars [us]: %ld\n\
Delay between requests [s]: %d\n\
Unique registers (read params): %d\n\
", cfg->GetLineNumber(), cfg->GetDevice()->GetPath().c_str(), kamsinfo->m_params_count, kamsinfo->delay_between_chars, kamsinfo->delay_between_requests, kamsinfo->unique_registers_count);
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

	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init()) {
			K601Exception ex;
			ex.SetMsg("ERROR!: IPC init failed");
			throw ex;
		}
	}

	dolog(2, "starting main loop");

	char *plugin_path;
	asprintf(&plugin_path, "%s/szarp-prop-plugins.so", dirname(argv[0]));
	plugin = dlopen(plugin_path, RTLD_LAZY);
	if (plugin == NULL) {
		dolog(0,
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
		dolog(0, "Error loading symbols frop prop-plugins.so library: %s",
				dlerror());
		dlclose(plugin);
		exit(1);
	}
}

void K601Daemon::StartDo() {
	try {
		if (m_ip.compare("") != 0) {
			SerialAdapter *client = new SerialAdapter();
			m_serial_port = client;
			client->InitTcp(m_ip, m_data_port, m_cmd_port);
		} else {
			SerialPort *port = new SerialPort();
			m_serial_port = port;
			port->Init(m_path);
		}
		m_serial_port->AddListener(this);

		m_serial_port->Open();
		m_serial_port->SetConfiguration(&serial_conf);

		std::string info = m_id + ": connection established.";
		dolog(2, "%s: %s", m_id.c_str(), info.c_str());
	} catch (SerialPortException &e) {
		dolog(0, "%s: %s", m_id.c_str(), e.what());
		SetRestart();
	}
	evtimer_set(&m_ev_timer, TimerCallback, this);
	Do();
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
				m_serial_port->WriteData(&query[0], query.size());
				m_state = READ;
			} catch (SerialPortException &e) {
				dolog(0, "%s: %s", m_id.c_str(), e.what());
				SetRestart();
			}
			destroySend(MyKMPSendClass);
			MyKMPReceiveClass = createReceive();
			wait_ms = kamsinfo->delay_between_chars;
			break;
		case READ:
			dolog(10, "READ");
			if (!m_data_was_read) {
				if (m_read_buffer.size() > 0) {
					m_state = PROCESS;
				} else {
					SetCurrRegisterNoData();
					m_state = FINALIZE;
				}
			} else {
				if (m_read_buffer.size() > MyKMPReceiveClass->GetBufferSize()) {
					SetCurrRegisterNoData();
					m_state = FINALIZE;
				} else {
					m_data_was_read = false;
					wait_ms = kamsinfo->delay_between_chars;
				}
			}
			break;
		case PROCESS:
			dolog(10, "PROCESS");
			if (MyKMPReceiveClass->ReceiveResponse(m_read_buffer) ==
					MyKMPReceiveClass->GetREAD_OK()) {
				ProcessResponse();
			} else {
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
				m_curr_register = 0;
				if (!cfg->GetSingle()) {
					wait_ms = kamsinfo->delay_between_requests;
				}
			}
			m_state = SEND;
			break;
		case RESTART:
			dolog(10, "RESTART");
			try {
				m_serial_port->Close();
				m_serial_port->Open();
				m_serial_port->SetConfiguration(&serial_conf);
				dolog(2, "%s: %s", m_id.c_str(), "Restart successful!");
				m_state = SEND;
				m_curr_register = 0;
			} catch (SerialPortException &e) {
				dolog(0, "%s: %s %s", m_id.c_str(), "Restart failed:", e.what());
				for (int i = 0; i < ipc->m_params_count; i++) {
					ipc->m_read[i] = SZARP_NO_DATA;
				}
				ipc->GoParcook();
			}
			wait_ms = 15 * 1000;
			break;
	}
	dolog(10, "Schedule next in %dms", wait_ms);
	ScheduleNext(wait_ms);
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
	struct timeval tv;
	tv.tv_sec = wait_ms / 1000;
	tv.tv_usec = (wait_ms % 1000) * 1000;
	evtimer_add(&m_ev_timer, &tv);
}

void K601Daemon::TimerCallback(int fd, short event, void* thisptr)
{
        reinterpret_cast<K601Daemon*>(thisptr)->Do();
}

int main(int argc, char *argv[])
{
	event_init();
	dolog(10, "create..");
	K601Daemon daemon;
	dolog(10, "..finished creating");

	try {
		dolog(10, "Initializing..");
		daemon.Init(argc, argv);
		dolog(10, "..finished initializing.");
	} catch (K601Daemon::K601Exception &e) {
		dolog(0, "%s", e.what());
		exit(1);
	}
	daemon.StartDo();

	try {
		event_dispatch();
	} catch (MsgException &e) {
		dolog(0, "FATAL!: daemon killed by exception: %s", e.what());
		exit(1);
	} catch (...) {
		dolog(0, "FATAL!: daemon killed by unknown exception");
		exit(1);
	}
}
