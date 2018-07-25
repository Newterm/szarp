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
#include "cfgdealer_handler.h"

#define REGISTER_NOT_FOUND -1

#define RESTART_INTERVAL_MS 15000

bool single;

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
	class KamsParamInfo {
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

	KamsParamInfo *m_params;
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
			m_params = new KamsParamInfo[params];
			ASSERT(m_params != NULL);
		} else
			m_params = NULL;
	}

	~KamstrupInfo() {
		delete m_params;
		free(unique_registers);
	}

	void parseXML(DaemonConfigInfo * cfg);

	void parseDevice(DeviceInfo*);

	void parseParam(IPCParamInfo* param, size_t param_no);

	RegisterType DetermineRegisterType(unsigned short RegisterAddress);

	int FindRegisterIndex(unsigned short RegisterAddress, ParamMode PM);

	unsigned long DetermineRegisterMultiplier(unsigned short
						  RegisterAddress);
};

void KamstrupInfo::parseDevice(DeviceInfo* device)
{
	delay_between_requests_ms = device->getAttribute<unsigned>("extra:delay_between_requests")*1000;
}

void KamstrupInfo::parseParam(IPCParamInfo* param, size_t param_no)
{
	m_params[param_no].reg = param->getAttribute<int>("extra:register");
	m_params[param_no].mul = param->getAttribute<int>("extra:multiplier");

	auto type_attr = param->getAttribute<std::string>("extra:type", "single");

	if (type_attr == "lsb")
		m_params[param_no].type = T_LSB;
	else if (type_attr == "msb")
		m_params[param_no].type = T_MSB;
	else if (type_attr == "single")
		m_params[param_no].type = T_SINGLE;
	else {
		throw SzException("Invalid type attribute "+type_attr);
	}
}

void KamstrupInfo::parseXML(DaemonConfigInfo * cfg)
{
	sz_log(10, "KamstrupInfo:parseXML");

	try {
		parseDevice(cfg->GetDeviceInfo());
	} catch (const std::exception& e) {
		throw SzException("Error configuring device. Reason was "+std::string(e.what()));
	}

	size_t param_no = 0;

	const auto& units = cfg->GetUnits();
	if (units.size() != 1) {
		throw SzException("Bad number of units. Should be one, is "+std::to_string(units.size()));
	}

	for (auto param: units.front()->GetParams()) {
		try {
			parseParam(param, param_no);
			++param_no;
		} catch (const std::exception& e) {
			throw SzException("Error configuring param "+SC::S2A(param->GetName())+". Reason was "+std::string(e.what()));
		}
	}

	/**
	 * Calculating unique addresses of registers;
	 */
	unique_registers =
	    (unsigned short *)malloc(m_params_count* sizeof(unsigned short));
	unsigned short *registers =
	    (unsigned short *)malloc(m_params_count* sizeof(unsigned short));
	unique_registers_count = 0;
	memset(registers, 0, m_params_count* sizeof(unsigned short));
	auto k = 0;
	for (auto i = 0; i < m_params_count; i++) {
		auto latch = false;
		for (auto j = 0; j < i; j++) {
			if (m_params[i].reg == registers[j]) {
				latch = true;
				break;
			}
		}

		if (!latch) {
			unique_registers_count++;
			registers[i] = m_params[i].reg;
			unique_registers[k] = m_params[i].reg;
			k++;
		}
	}
	free(registers);
}

RegisterType KamstrupInfo::DetermineRegisterType(unsigned short RegisterAddress)
{
	unsigned short i;
	sz_log(10, "KamstrupInfo:DetermineRegisterType");

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
	sz_log(10, "KamstrupInfo::FindRegisterIndex");
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
	sz_log(10, "KamstrupInfo::DetermineRegisterMultiplier");

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
	DaemonConfigInfo *cfg;
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
	sz_log(2, "%s: SetConfigurationFinished", m_id.c_str());
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
	ArgsManager args_mgr("k601dmn");
	args_mgr.parse(argc, argv, DefaultArgs(), DaemonArgs());
	args_mgr.initLibpar();

	if (args_mgr.has("use-cfgdealer")) {
		szlog::init(args_mgr, "k601dmn");
		cfg = new ConfigDealerHandler(args_mgr);
	} else {
		auto d_cfg = new DaemonConfig("k601dmn");
		if (d_cfg->Load(args_mgr))
			throw SzException("Could not load configuraion");
		cfg = d_cfg;
	}

	kamsinfo = new KamstrupInfo(cfg->GetParamsCount());

	try {
		kamsinfo->parseXML(cfg);
	} catch (const std::exception& e) {
		delete kamsinfo;
		if (cfg) delete cfg;
		throw;
	}

	auto device = cfg->GetDeviceInfo();
	if (device->hasAttribute("extra:tcp-ip")) {
		auto ip = device->getAttribute("extra:tcp-ip");
		auto data_port = device->getAttribute<int>("extra:tcp-data-port", SerialAdapter::DEFAULT_DATA_PORT);
		auto cmd_port = device->getAttribute<int>("extra:tcp-cmd-port", SerialAdapter::DEFAULT_CMD_PORT);

		m_id = ip + ":" + std::to_string(data_port);

		/* Setting tcp connecton */
		try {
			SerialAdapter *client = new SerialAdapter(m_event_base);
			client->InitTcp(ip, data_port, cmd_port);
			m_connection = client;
		} catch (SerialPortException &e) {
			sz_log(1, "%s: %s", m_id.c_str(), e.what());
			SetRestart();
			ScheduleNext(RESTART_INTERVAL_MS);
		}

	} else if (device->hasAttribute("extra:atc-ip")) {
		auto ip = device->getAttribute("extra:atc-ip");
		auto data_port = device->getAttribute<int>("extra:tcp-data-port", AtcConnection::DEFAULT_DATA_PORT);
		auto cmd_port = device->getAttribute<int>("extra:tcp-cmd-port", AtcConnection::DEFAULT_CONTROL_PORT);

		m_id = ip + ":" + std::to_string(data_port);

		/* Setting atc configuration */
		try {
			AtcConnection *client = new AtcConnection(m_event_base);
			client->InitTcp(ip, data_port, cmd_port);
			m_connection = client;
		} catch (SerialPortException &e) {
			sz_log(1, "%s: %s", m_id.c_str(), e.what());
			SetRestart();
			ScheduleNext(RESTART_INTERVAL_MS);
		}
	} else if (device->hasAttribute("path")) {
		m_id = device->getAttribute("path");
	
		/* Setting serial */
		try {
			SerialPort *port = new SerialPort(m_event_base);
			port->Init(m_id);
			m_connection = port;
		} catch (SerialPortException &e) {
			sz_log(0, "%s: %s", m_id.c_str(), e.what());
			SetRestart();
			ScheduleNext(RESTART_INTERVAL_MS);
		}
	} else {
		throw SzException("ERROR!: neither IP nor device path has been specified");
	}

	sz_log(2, "Connection address %s", m_id.c_str());

	single = cfg->GetSingle();

	if (cfg->GetSingle()) {
		printf("\
line number: %d\n\
device: %s\n\
id: %s\n\
params in: %d\n\
Delay between requests [s]: %d\n\
Unique registers (read params): %d\n\
", cfg->GetLineNumber(), device->getAttribute("path").c_str(), m_id.c_str(), kamsinfo->m_params_count, kamsinfo->delay_between_requests_ms / 1000, kamsinfo->unique_registers_count);

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
		throw std::runtime_error("ERROR!: IPC init failed");
	}

	sz_log(2, "starting main loop");

	char *plugin_path;
	auto ret = asprintf(&plugin_path, "%s/szarp-prop-plugins.so", dirname(argv[0]));
	if (ret == -1) throw SzException("Error getting dl path");

	plugin = dlopen(plugin_path, RTLD_LAZY);
	if (plugin == NULL) {
		sz_log(0,
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
		sz_log(0, "Error loading symbols frop prop-plugins.so library: %s",
				dlerror());
		dlclose(plugin);
		exit(1);
	}
}

void K601Daemon::StartDo() {
	evtimer_set(&m_ev_timer, TimerCallback, this);
	event_base_set(m_event_base, &m_ev_timer);
	try {
		m_connection->AddListener(this);
		sz_log(10, "%s: Opening connection...", m_id.c_str());
		m_connection->Open();
	} catch (SerialPortException &e) {
		sz_log(1, "%s: %s", m_id.c_str(), e.what());
		SetRestart();
		ScheduleNext(RESTART_INTERVAL_MS);
	}
	event_base_dispatch(m_event_base);
}

void K601Daemon::OpenFinished(const BaseConnection *conn)
{
	std::string info = m_id + ": connection established, setting configuration..";
	sz_log(2, "%s: %s", m_id.c_str(), info.c_str());
	m_connection->SetConfiguration(m_serial_conf);
}

void K601Daemon::Do()
{
	unsigned int wait_ms = 0;
	std::vector<unsigned char> query;
	switch (m_state) {
		case SEND:
			sz_log(10, "SEND");
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
				sz_log(1, "%s: %s", m_id.c_str(), e.what());
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
			sz_log(10, "READ");
			if (!m_data_was_read) {
				if (m_read_buffer.size() > 0) {
					m_state = PROCESS;
				} else {
					sz_log(10, "setting current register to nodata - buffer is empty");
					SetCurrRegisterNoData();
					m_state = FINALIZE;
				}
			} else {
				if (m_read_buffer.size() > MyKMPReceiveClass->GetBufferSize()) {
					sz_log(10, "setting current register to nodata - too many chars in buffer");
					SetCurrRegisterNoData();
					m_state = FINALIZE;
				} else {
					m_data_was_read = false;
					wait_ms = kamsinfo->DELAY_BETWEEN_CHARS;
				}
			}
			break;
		case PROCESS:
			sz_log(10, "PROCESS");
			if (MyKMPReceiveClass->ReceiveResponse(m_read_buffer) ==
					MyKMPReceiveClass->GetREAD_OK()) {
				ProcessResponse();
				m_params_read_in_cycle++;
			} else {
				sz_log(10, "setting current register to nodata - parser reported error");
				SetCurrRegisterNoData();
			}
			m_state = FINALIZE;
			break;
		case FINALIZE:
			sz_log(10, "FINALIZE");
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
				sz_log(10, "STARTING NEW CYCLE");
				m_curr_register = 0;
				m_params_read_in_cycle = 0;
				// we don't want to set NODATA at this point, as the daemon may not finish a full cycle in 10s
				if (!cfg->GetSingle()) {
					sz_log(10, "WAITING....");
					wait_ms = kamsinfo->delay_between_requests_ms;
				}
			}
			m_state = SEND;
			break;
		case RESTART:
			sz_log(10, "RESTART");
			try {
				m_connection->Close();
				m_connection->Open();
			} catch (SerialPortException &e) {
				sz_log(1, "%s: Restart failed: %s", m_id.c_str(), e.what());
				SetNoData();
				ipc->GoParcook();
				wait_ms = RESTART_INTERVAL_MS;
				sz_log(10, "Schedule next (restart) in %dms", wait_ms);
				ScheduleNext(wait_ms);
			}
			// in the case of restart we should either
			// get OpenFinished event or ReadError
			// which should ScheduleNext on themselves
			return;
	}
	sz_log(10, "Schedule next in %dms", wait_ms);
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
		sz_log(0, "Error while initializing daemon: %s, exiting.", e.what());
		exit(1);
	}

	try {
		daemon.StartDo();
	} catch (const std::exception& e) {
		sz_log(0, "FATAL!: daemon killed by exception: %s", e.what());
		exit(1);
	}
}
