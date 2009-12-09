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
 * Modbus TCP/IP driver
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 * 
 * $Id$
 * 
 * Configuration example, all attributes from 'modbus' namespace not explicity
 *  described as optional are required.
 * ...
 * <device 
 *      xmlns:modbus="http://www.praterm.com.pl/SZARP/ipk-extra"
 *      daemon="/opt/szarp/bin/mbtcpdmn" 
 *      path="/dev/ttyA11"
 *      modbus:tcp-mode="server"
 		allowe modes are 'server' and 'client'
 *      modbud:tcp-port="502"
		TCP port we are listenning on/connecting to (server/client)
		port TCP na którym mamy nas³uchiwaæ/do którego mamy siê ³±czyæ(zale¿nie od trybu)
 *      modbus:tcp-allowed="192.9.200.201 192.9.200.202"
		(optional) list of allowed clients IP addresses for server mode, if empty all
		addresses are allowed
 *      modbus:tcp-address="192.9.200.201"
 		server IP address (required in client mode)
 *      modbus:tcp-keepalive="yes"
 		should we set TCP Keep-Alive options? "yes" or "no"
 *	modbus:tcp-timeout="30"
		(optional) connection timeout in seconds, after timeout expires, connection
		is closed; default empty value means no timeout
 *	modbus:nodata-timeout="15"
		(optional) timeout (in seconds) to set data to 'NO_DATA' if data is not available,
		default is 20 seconds
 *	modbus:nodata-value="-1"
 		(optional) float value to send instead of 'NO_DATA', default is 0
 *	modbus:FloatOrder="msblsb"
 		(optional) registers order for 4 bytes (2 registers) float order - "msblsb"
		(default) or "lsbmsb"; values names are a little misleading, it shoud be 
		msw/lsw (most/less significant word) not msb/lsb (most/less significant byte),
		but it's left like this for compatibility with Modbus RTU driver configuration

 *      >
 *      <unit id="1">
 *              <param
			Read value using ReadHoldingRegisters (0x03) Modbus function              
 *                      name="..."
 *                      ...
 *                      modbus:address="0x03"
 	                      	modbus register number, starting from 0
 *                      modbus:val_type="integer">
 	                      	register value type, 'integer' (2 bytes, 1 register) or float (4 bytes,
	                       	2 registers)
 *                      ...
 *              </param>
 *              <param
 *                      name="..."
 *                      ...
 *                      modbus:address="0x04"
 *                      modbus:val_type="float"
 *			modbus:val_op="LSW">
				(optional) operator for converting data from float to 2 bytes integer;
				default is 'NONE' (simple conversion to short int), other values
				are 'LSW' and 'MSW' - converting to 4 bytes long and getting less/more
				significant word; in this case there should be 2 parameters with the
				same register address and different val_op attributes - LSW and MSW.
				s³owa
 *                      ...
 *              </param>
 *              ...
 *              <send 
              		Sending value using WriteMultipleRegisters (0x10) Modbus function
 *                      param="..." 
 *                      type="min"
 *                      modbus:address="0x1f"
 *                      modbus:val_type="float">
 *                      ...
 *              </send>
 *              ...
 *      </unit>
 * </device>
 *
 * Logi l±duj± domy¶lnie w /opt/szarp/log/mbtcpdmn.
 *
 * Serwer pozwala na jedno po³±czenie i jedn± transakcjê na raz. Nie sprawdza
 * kiedy ostatnio przysz³y dane. £±czenie z parcookiem i senderem nastêpuje
 * raz na 10 sekund.
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

#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include <libxml/parser.h>

#ifdef LIBXML_XPATH_ENABLED

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "modbus.h"
#include "conversion.h"
#include "ipchandler.h"
#include "liblog.h"
#include "xmlutils.h"
#include "tokens.h"

#define MODBUS_DEFAULT_PORT 502

#define MODBUS_ERROR_CODE 0x80

#define FLOAT_ORDER_MSWLSW 0
#define FLOAT_ORDER_LSWMSW 1

/* uncomment it to turn on socket close on timeout */
//#define SERVER_SOCKET_RESET

/**
 * Modbus communication config.
 */
class ModbusTCP {
public :
	enum ModbusType {
		SERVER, 
		CLIENT,
	};

	/** operators for converting float values from mobus registers */
	typedef enum { 
		NONE, /**< default - just copy and convert according to
			precision */
		LSW, /**< convert to 32 bytes integer according to precision
			and then get less significant 16 bytes */
		MSW /**< convert to 32 bytes integer according to precision
			and then get less significant 16 bytes */
	} ValueOperator;
	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	ModbusTCP(int params, int sends);
	~ModbusTCP();

	/** 
	 * Parses XML 'device' element. Device element must have attributes
	 * 'tcp-mode' ("server"), tcp-port, and optionally 'tcp-allowed' 
	 * and 'tcp-keepalive'.
	 * Every 'param' and 'send' children of first 'unit'
	 * element must have attributes 'address' (modebus address, decimal or
	 * hex) and 'val_type' ("integer" or "float").
	 * @param cfg pointer to daemon config object
	 * @return - 0 on success, 1 on error 
	 */
	int ParseConfig(DaemonConfig * cfg);

	/**
	 * Starts listening on port.
	 * @return 0 on success, 1 on error
	 */
	int StartServer();

	int SendMessage(int timeout);

	int ReadMessage(int timeout);

	/**
	 * Exchange data with IPC handler. For float values NAN is used
	 * to represent non-existing values.
	 * @param ipc IPC handler
	 */
	void UpdateParamRegisters(IPCHandler *ipc);
	void UpdateSendRegisters(IPCHandler *ipc);

	/**
	  Print registers values to stdout.
	 */
	void PrintRegisters(IPCHandler *ipc);

	/** 
	 * Closes opened sockets. Returns without error if server state is 
	 * 'not running'.
	 * @return 0 on success, 1 on error
	 */
	int Stop();

	/**
	 * Closes communication socket. */
	void Reset();
	/**
	 * Calls Reset if last event on socket was more then m_tcp_timeout
	 * seconds ago */
	void CheckReset();
	
protected :
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckMode(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckPort(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckAllowedIP(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckServerIP(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckKeepAlive(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckServerTimeout(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckNodataTimeout(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckNodataValue(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLCheckFloatOrder(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing 
	 * @return 1 on error, 0 otherwise */
	int XMLLoadParams(xmlXPathContextPtr xp_ctx, int dev_num);
	/** Checks if IP is allowed.
	 * @return 1 if IP is allowed, 0 if not. */
	int IPAllowed(struct sockaddr_in *addr);

	/** Tries to read size bytes of data from client (starting from unit
	 * identifier).
	 * @param buffer return buffer containing read data
	 * @return 0 on success, 1 on error
	 */
	int Read(int size, unsigned char **buffer);
	/** Creates response as an exception frame (PDU).
	 * @param function code
	 * @param code exception code
	 */
	void CreateException(char function, char code);
	/** Subroutine of CreateResponse. Parses READ HOLDING REGISTERS
	 * frame and creates response. */
	void ParseFunReadHoldingReg(unsigned char * frame, int size);
	/** Subroutine of CreateResponse. Parses WRITE MULTIPLE REGISTER
	 * frame and creates response. */
	void ParseFunWriteMultReg(unsigned char * frame, int size);

	int SendReadMultReg(int from, int to, int timeout);

	int SendWriteMultReg(int from, int to, int timeout);

	int CheckServerResponseError(unsigned char *frame, int size);

	/** Creates registers map using m_params and m_sends attributes.
	 * Checks for validity of addresses.
	 * @return 0 if addressed are valid, 1 if not
	 */
	int CreateRegisters();
	/** Subroutine of CreateRegisters. Checks addresses validity.
	 * @return 0 if addresses are correct, 1 otherwise */
	int CheckRegisters();
	/** Fills registers with 0. */
	void CleanRegisters();
	/** Checks if given address range is correct.
	 * @param start starting address, addressed from 0
	 * @param quantity quantity of registers
	 * @return 0 if address range is correct, 1 otherwise
	 */
	int CheckAddress(int start, int quantity);
	/**
	 * Save write time for registers.
	 * @param start address of first register
	 * @param quantity number of registers
	 */
	void SetWriteTime(int start, int quantity);
	/**
	 * Returns data from register converted to SZARP data.
	 * @param address register address
	 * @param type type of data (int or float)
	 * @param prec data precision (needed for conversion to SZARP data)
	 * @param t current time
	 * @return converted value, may be SZARP_NO_DATA
	 */
	short int ReadRegister(int address, ModbusValType type, int prec,
			time_t t, ValueOperator op = NONE);
	/**
	 * Convert and write SZARP data to modbus register.
	 * @param address register address
	 * @param type type of data (int or float)
	 * @param prec data precision (needed for conversion from SZARP data)
	 * @param value SZARP value (SZARP_NO_DATA is converted to NAN)
	 */
	void WriteRegister(int address, ModbusValType type, int prec, 
			short int value);
	/**
	 * Fill m_read structure with NO_DATA value
	 */
	void SetNoData(IPCHandler * ipc);
	
	/** info about single parameter */
	class ParamInfo {
	public:
		unsigned short  addr;	/**< parameter modbus address */
		ModbusValType   type;	/**< value type */
		int             prec;	/**< parameter precision - number by
					  which you have to divide integer
					  value to get real value; for example
					  if integer value is 131 and prec is
					  100, real value is 131/100 = 1.31 */
		ValueOperator	op;	/**< Operator for converting float
					values. */
	};

	int m_keepalive;	/**< 1 if TCP KeepAlive option should
					  be set, 0 otherwise */

	ModbusType m_type;	/**< denotes operation mode of deamon - server or client*/
	public:
	/** Accepts connection from client (if there is no connection).
	 * Modifies m_socket (unless timeout occured).
	 * @param timeout timeout, in seconds
	 * @return 0 on success or timeout, 1 on error */
	int Accept(int timeout);
	/** Parses data from client (PDU) and creates response (PDU only). 
	 * Previous response is deleted if exists.
	 )* @param unit_id Modbus Unit identifier, copied to response
	 * @param frame pointer to frame data
	 * @param size length of frame data in bytes
	 */
	void CreateResponse();

	int SendReadRegisters(int timeout);

	int SendWriteRegisters(int timeout);

	int Connect(int timeout);

	ModbusType GetDaemonType() { return m_type; } 
				/**< return the role daemon is playing now*/
	ParamInfo *m_params;	/**< array of params to read */
	int m_params_count;	/**< size of params array */
	ParamInfo *m_sends;	/**< array of params to write (send) */
	int m_sends_count;	/**< size of sends array */
	protected:
	unsigned short int m_port;
				/**< port number to listen on */
	float m_nodata_value;	/**< value sended instead of NO_DATA */
	int m_nodata_timeout;	/**< timeout for 'NO_DATA' */
	int m_tcp_timeout;	/**< timeout for closing conection (0 - don't close) */
	int m_float_order;	/**< 0 is MSW LSW, 1 is LSW MSW */
	time_t m_last_tcp_event;
				/**< time of last event on socket */
	
	struct in_addr * m_allowed_ip;
				/**< array of allowed client IP addresses */
	struct in_addr * m_server_ip;
				/**< addres of modbus server client IP addresses */
	int m_allowed_count;	/**< size of m_allowed_ip array */
	
	int m_running;		/**< Server state (1 - running, 0 - stopped) */
	int m_listen_socket;		/**< Listen socket descriptor. */
	int m_socket;	/**< Socket for accepted connection. */
	int m_trans_id;		/**< Current transaction id. */
	int m_unit_id;		/**< Current transaction unit id. */
	char *m_send_message;	/**< Frame to send (if different then NULL) */
	int m_send_message_size; /**< Size of frame to send. */

	unsigned char *m_received_message;	/**< Received frame (if different then NULL) */
	int m_received_message_size;	/**< Size of received frame. */

	int16_t * m_registers;	/**< Array of modbus internal registers. 
				Modbus registers are addressed from 1, but
				this array is addressed from 0. */
	int m_registers_size;	/**< Size of registers array. */
	time_t * m_reg_write;	/**< Table with last write time for registers. 
				*/
};

/**
 * @param params number of params to read
 * @param sends number of params to send (write)
 */
ModbusTCP::ModbusTCP(int params, int sends) 
{
	assert(params >= 0);
	assert(sends >= 0);

	m_params_count = params;
	if (params > 0) {
		m_params = new ParamInfo[params];
		assert(m_params != NULL);
	} else {
		m_params = NULL;
	}
	m_sends_count = sends;
	if (sends > 0) {
		m_sends = new ParamInfo[sends];
		assert(m_sends != NULL);
	} else {
		m_sends = NULL;
	}
	m_keepalive = 0;
	m_port = 0;
	m_allowed_ip = NULL;
	m_allowed_count = 0;
	m_server_ip = NULL;
	m_running = 0;
	m_listen_socket = -1;
	m_socket = -1;
	m_send_message = NULL;
	m_send_message_size = 0;
	m_registers = NULL;
	m_registers_size = 0;
	m_reg_write = NULL;
	m_nodata_timeout = 20;
	m_tcp_timeout = 0;
	m_last_tcp_event = time(NULL);
	m_nodata_value = 0.0;
	m_float_order = FLOAT_ORDER_MSWLSW;
}

ModbusTCP::~ModbusTCP() 
{
	Stop();
	delete m_params;
	delete m_sends;
	if (m_allowed_ip) {
		free(m_allowed_ip);
	}
	if (m_server_ip) {
		free(m_server_ip);
	}
	if (m_send_message) {
		free(m_send_message);
	}
	if (m_registers)
		free(m_registers);
	if (m_reg_write)
		free(m_reg_write);
}

void ModbusTCP::SetNoData(IPCHandler * ipc)
{
	int i;
	assert (ipc != NULL);

	for (i = 0; i < ipc->m_params_count; i++)
		ipc->m_read[i] = SZARP_NO_DATA;
}

int ModbusTCP::XMLCheckMode(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;
	
	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@modbus:tcp-mode",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c == NULL)
		return 1;
	if (!xmlStrcmp(c, BAD_CAST "server"))
		m_type = SERVER;
	else if (!xmlStrcmp(c, BAD_CAST "client"))
		m_type = CLIENT;
	else {
		sz_log(0, "tpc-mode '%s' found, the only supported types are 'server' and 'client'",
				c);
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	return 0;
}

int ModbusTCP::XMLCheckPort(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;
	long l;

	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@modbus:tcp-port",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c == NULL)
		return 1;
	l = strtol((char *)c, &e, 0);
	if ((*c == 0) || (*e != 0)) {
		sz_log(0, "incorrect value '%s' for tcp-port, number expected",
				SC::U2A(c).c_str());
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	if ((l < 1) || (l > 0xFFFF)) {
		sz_log(0, "value '%ld' for tcp-port outside range [1..%d]",
				l, 0xFFFF);
		return 1;
	}
	m_port = (unsigned short int)l;
	if (m_port != MODBUS_DEFAULT_PORT) {
		sz_log(1, "warning: using non-standard tcp port %d", m_port);
	}
	return 0;
}

int ModbusTCP::XMLCheckAllowedIP(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;

	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@modbus:tcp-allowed",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c != NULL) {
		int tokc = 0;
		char **toks;
		tokenize(SC::U2A(c).c_str(), &toks, &tokc);
		xmlFree(c);
		if (tokc > 0) {
			m_allowed_count = tokc;
			m_allowed_ip = (struct in_addr *) calloc(
					m_allowed_count,
					sizeof(struct in_addr));
			assert (m_allowed_ip != NULL);
		}
		for (int i = 0; i < tokc; i++) {
			int ret = htonl(inet_aton(toks[i], &m_allowed_ip[i]));
			if (ret == 0) {
				sz_log(0, "incorrect IP address '%s'", toks[i]);
				return 1;
			} else {
				sz_log(5, "IP address '%s' allowed", toks[i]);
			}
		}
		// free memory
		tokenize(NULL, &toks, &tokc);
	}
	if (m_allowed_count <= 0) {
		sz_log(1, "warning: all IP addresses allowed");
	}
	return 0;
}

int ModbusTCP::XMLCheckServerIP(xmlXPathContextPtr xp_ctx, int dev_num) {
	assert(m_type == CLIENT);
	char *e;
	xmlChar *c;

	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@modbus:tcp-address",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c != NULL) {
		m_server_ip = (struct in_addr*) calloc(1, sizeof(struct in_addr));
		int ret = inet_aton((char*)c, m_server_ip);
		if (ret == 0) {
			sz_log(0, "incorrect IP address '%s'", c);
			xmlFree(c);
			return 1;
		}
		// free memory
		xmlFree(c);
	} else {
		sz_log(0, "server adddress not given in client mode");
		return 1;
	}
	return 0;
}

int ModbusTCP::XMLCheckKeepAlive(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;
	
	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@modbus:tcp-keepalive",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c == NULL)
		return 1;
	if (!xmlStrcmp(c, BAD_CAST "yes")) {
		m_keepalive = 1;
		sz_log(5, "setting TCP Keep-Alive options to \"yes\"");
	} else if (!xmlStrcmp(c, BAD_CAST "no")) {
		m_keepalive = 0;
		sz_log(5, "setting TCP Keep-Alive options to \"no\"");
	} else {
		sz_log(0, "tcp-keepalive=\"%s\" found, \"yes\" or \"no\" exptected",
				SC::U2A(c).c_str());
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	return 0;
}
	
int ModbusTCP::XMLCheckServerTimeout(xmlXPathContextPtr xp_ctx, 
		int dev_num)
{
	char *e;
	xmlChar *c;
	long l;
	
	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@modbus:tcp-timeout",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c == NULL)
		return 0;
	
	l = strtol((char *)c, &e, 0);
	if ((*c == 0) || (*e != 0)) {
		sz_log(0, "incorrect value '%s' for modbus:tcp-timeout for device %d -  integer expected",
					SC::U2A(c).c_str(), dev_num); 
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	if ((l < 1) || (l > 600)) {
		sz_log(0, "value '%ld' for modbus:tcp-timeout for device %d outside expected range [1..600]", 
				l, dev_num);
		return 1;
	}
	m_tcp_timeout = (int) l;
	sz_log(10, "Setting modbus:tcp-timeout to %d", m_tcp_timeout);
	return 0;
}

int ModbusTCP::XMLCheckNodataTimeout(xmlXPathContextPtr xp_ctx, 
		int dev_num)
{
	char *e;
	xmlChar *c;
	long l;
	
	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@modbus:nodata-timeout",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c == NULL)
		return 0;
	
	l = strtol((char *)c, &e, 0);
	if ((*c == 0) || (*e != 0)) {
		sz_log(0, "incorrect value '%s' for modbus:nodata-timeout for device %d -  integer expected",
					(char *)c, dev_num); 
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	if ((l < 1) || (l > 600)) {
		sz_log(0, "value '%ld' for modbus:nodata-timeout for device %d outside expected range [1..600]", 
				l, dev_num);
		return 1;
	}
	m_nodata_timeout = (int) l;
	sz_log(10, "Setting modbus:nodata-timeout to %d", m_nodata_timeout);
	return 0;
}

int ModbusTCP::XMLCheckNodataValue(xmlXPathContextPtr xp_ctx, 
		int dev_num)
{
	char *e;
	xmlChar *c;
	float f;
	
	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@modbus:nodata-value",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c == NULL)
		return 0;
	
	f = strtof((char *)c, &e);
	if ((*c == 0) || (*e != 0)) {
		sz_log(0, "incorrect value '%s' for modbus:nodata-value for device %d -  float expected",
					SC::U2A(c).c_str(), dev_num); 
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	m_nodata_value = f;
	sz_log(10, "Setting modbus:nodata-value to %g", m_nodata_value);
	return 0;
}

int ModbusTCP::XMLCheckFloatOrder(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;
	
	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@modbus:FloatOrder",
			dev_num);
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c == NULL) {
		m_float_order = FLOAT_ORDER_MSWLSW;
		sz_log(5, "setting FloatOrder to MSWLSW as default value");
		return 0;
	}
	if (!xmlStrcmp(c, BAD_CAST "msblsb")) {
		m_float_order = FLOAT_ORDER_MSWLSW;
		sz_log(5, "setting FloatOrder to MSWLSW");
	} else if (!xmlStrcmp(c, BAD_CAST "lsbmsb")) {
		m_float_order = FLOAT_ORDER_LSWMSW;
		sz_log(5, "setting FloatOrder to LSWMSW");
	} else {
		sz_log(0, "FloatOrder=\"%s\" found, \"msblsb\" or \"lsbmsb\" exptected",
				SC::U2A(c).c_str());
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	return 0;
}

int ModbusTCP::XMLLoadParams(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;
	long l;
	int i, j;
	ParamInfo *arr;
	
	for (j = 0; j < m_params_count + m_sends_count; j++) {
		i = j;
		if (i >= m_params_count) {
			arr = m_sends;
			i -= m_params_count;
		} else {
			arr = m_params;
		}
		asprintf(&e, "/ipk:params/ipk:device[position()=%d]/ipk:unit[position()=1]/ipk:%s[position()=%d]/@modbus:address",
			dev_num, 
			i != j ? "send" : "param" ,
			i + 1);
		assert (e != NULL);
		c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
		free(e);
		if (c == NULL)
			return 1;
		l = strtol((char *)c, &e, 0);
		if ((*c == 0) || (*e != 0)) {
			sz_log(0, "incorrect value '%s' for modbus:address for device %d, %sparameter %d -  number expected",
					SC::U2A(c).c_str(), dev_num, 
					i != j ? "send " : "", 
					i + 1);
			xmlFree(c);
			return 1;
		}
		xmlFree(c);
		if ((l < 0) || (l > MB_MAX_ADDRESS)) {
			sz_log(0, "modbus:address value outside range [0..%x] (device %d, %sparameter %d)",
					MB_MAX_ADDRESS, 
					dev_num, 
					i != j ? "send " : "", 
					i + 1);
			return 1;
		}
		arr[i].addr = (unsigned short)l;

		asprintf(&e, "/ipk:params/ipk:device[position()=%d]/ipk:unit[position()=1]/ipk:%s[position()=%d]/@modbus:val_type",
			dev_num, 
			i != j ? "send" : "param" ,
			i + 1);
		assert (e != NULL);
		c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
		free(e);
		if (c == NULL)
			return 1;
		if (!xmlStrcmp(c, BAD_CAST "integer")) {
			arr[i].type = MB_TYPE_INT;
		} else if (!xmlStrcmp(c, BAD_CAST "float")) {
			arr[i].type = MB_TYPE_FLOAT;
		} else {
			sz_log(0, "incorrect value '%s' for modbus:val_type for device %d, %sparameter %d -  \"integer\" or \"float\" expected",
					SC::U2A(c).c_str(), dev_num, 
					i != j ? "send " : "", 
					i + 1);
			xmlFree(c);
			return 1;
		}
		xmlFree(c);
		arr[i].op = NONE;
		/* only for params */
		if (i != j) {
			continue;
		}
		asprintf(&e, "/ipk:params/ipk:device[position()=%d]/ipk:unit[position()=1]/ipk:param[position()=%d]/@modbus:val_op",
			dev_num, 
			i + 1);
		assert (e != NULL);
		c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
		free(e);
		if (c == NULL) {
			continue;
		}
		if (!xmlStrcmp(c, BAD_CAST "LSW")) {
			arr[i].op = LSW;
		} else if (!xmlStrcmp(c, BAD_CAST "MSW")) {
			arr[i].op = MSW;
		} else if (xmlStrcmp(c, BAD_CAST "NONE")) {
			sz_log(0, "incorrect value '%s' for modbus:val_op for device %d, parameter %d -  \"LSW\" or \"MSW\" or \"NONE\" expected",
					SC::U2A(c).c_str(), dev_num, 
					i + 1);
			xmlFree(c);
			return 1;
		}
		xmlFree(c);
	}
	return 0;
}

int ModbusTCP::ParseConfig(DaemonConfig * cfg)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xp_ctx;
	int dev_num;
	int ret;
	
	/* get config data */
	assert (cfg != NULL);
	dev_num = cfg->GetLineNumber();
	assert (dev_num > 0);
	doc = cfg->GetXMLDoc();
	assert (doc != NULL);

	/* prepare xpath */
	xp_ctx = xmlXPathNewContext(doc);
	assert (xp_ctx != NULL);

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert (ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "modbus",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);

	if (XMLCheckPort(xp_ctx, dev_num))
		return 1;

	if (XMLCheckMode(xp_ctx, dev_num))
		return 1;

	if (m_type == SERVER) {
		if (XMLCheckAllowedIP(xp_ctx, dev_num))
			return 1;
	} else {
		if (XMLCheckServerIP(xp_ctx, dev_num))
			return 1;
		m_unit_id = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetId() - L'0';
		if (m_unit_id > 9) {
			m_unit_id = 10 + (cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetId() - L'A');
		}
	}

	if (XMLCheckKeepAlive(xp_ctx, dev_num))
		return 1;

	if (XMLCheckNodataTimeout(xp_ctx, dev_num))
		return 1;

	if (XMLCheckServerTimeout(xp_ctx, dev_num))
		return 1;

	if (XMLCheckNodataValue(xp_ctx, dev_num))
		return 1;

	if (XMLCheckFloatOrder(xp_ctx, dev_num))
		return 1;

	if (XMLLoadParams(xp_ctx, dev_num))
		return 1;

	xmlXPathFreeContext(xp_ctx);

	TParam *p = cfg->GetDevice()->GetFirstRadio()->
		GetFirstUnit()->GetFirstParam();
	assert (p != NULL);
	for (int i = 0; i < m_params_count; i++, p = p->GetNext()) {
		assert(p != NULL);
		m_params[i].prec = (int)lrint(exp10(p->GetPrec()));
	}
	if (m_sends_count > 0) { 
		TSendParam *s = cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetFirstSendParam();
		assert (s != NULL);
		for (int i = 0; i < m_sends_count; i++, s = s->GetNext()) {
			assert (s != NULL);
			p = cfg->GetIPK()->getParamByName(s->GetParamName());
			if (p == NULL) {
				sz_log(0, "parameter with name '%ls' not found (send parameter %d for device %d)",
				s->GetParamName().c_str(), i + 1, dev_num);
				return 1;
			}
			m_sends[i].prec = (int)lrint(exp10(p->GetPrec()));
			m_sends[i].op = NONE;
		}
	}

	if (CreateRegisters())
		return 1;

	return 0;
}

int ModbusTCP::CreateRegisters()
{
	int max;
	int i;
	
	assert (m_registers == NULL);
	assert (m_reg_write == NULL);
	/* search for max address */
	max = 0;
	for (i = 0; i < m_params_count; i++) {
		if (m_params[i].addr > max) {
			max = m_params[i].addr;
			if (m_params[i].type == MB_TYPE_FLOAT)
				max++;
		}
	}
	for (i = 0; i < m_sends_count; i++) {
		if (m_sends[i].addr > max) {
			max = m_sends[i].addr;
			if (m_sends[i].type == MB_TYPE_FLOAT)
				max++;
		}
	}
	if (max < 0) {
		sz_log(0, "CreateRegisters(): No correct modbus address found");
		return 1;
	}
	if (max >= MB_MAX_ADDRESS) {
		sz_log(0, "CreateRegisters(): Modbus address 0x%x to big",
				max);
		return 1;
	}
	m_registers_size = max + 1;
	m_registers = (int16_t *) calloc(m_registers_size, sizeof(int16_t));
	assert (m_registers != NULL);
	m_reg_write = (time_t *) calloc(m_registers_size, sizeof(time_t));
	assert (m_reg_write != NULL);
	return CheckRegisters();
}

int ModbusTCP::CheckRegisters()
{
	int i, j;
	ParamInfo *arr;
	
	assert (m_registers_size > 0);
	assert (m_registers != NULL);

	arr = m_params;
	for (i = 0, j = 0; j < m_params_count + m_sends_count; j++, i++) {
		if (j == m_params_count) {
			i = 0;
			arr = m_sends;
		}
		switch (arr[i].type) {
			case MB_TYPE_FLOAT:
				if (arr[i].op != MSW) {
					if (m_registers[arr[i].addr + 1]) {
						sz_log(0, "CheckRegisters(): double use of register address 0x%x",
								arr[i].addr + 1);
						return 1;
					}
					m_registers[arr[i].addr + 1] = 1;
				}
				if (arr[i].op == LSW) {
					break;
				}
				/* no break ! */;
			case MB_TYPE_INT :
				if (m_registers[arr[i].addr]) {
					sz_log(0, "CheckRegisters(): double use of register address 0x%x",
							arr[i].addr);
					return 1;
				}
				m_registers[arr[i].addr] = 1;
				break;
			default :
				break;
		}
	}
	CleanRegisters();
	return 0;
}

void ModbusTCP::CleanRegisters()
{
	assert (m_registers != NULL);
	assert (m_registers_size > 0);
	memset(m_registers, 0, m_registers_size * sizeof(int16_t));
}

int ModbusTCP::IPAllowed(struct sockaddr_in *addr)
{
	int i;
	assert (addr != NULL);
	assert (addr->sin_family = AF_INET);
	if (m_allowed_count <= 0)
		return 1;
	for (i = 0; i < m_allowed_count; i++) {
		if (m_allowed_ip[i].s_addr == addr->sin_addr.s_addr)
			return 1;
	}
	return 0;
}

int ModbusTCP::StartServer()
{
	int ret;
	struct sockaddr_in addr;

	m_listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	m_last_tcp_event = time(NULL);
	if (m_listen_socket < 0) {
		sz_log(0, "socket() failed, errno %d (%s)",
				errno, strerror(errno));
		return 1;
	}
	ret = setsockopt(m_listen_socket, SOL_SOCKET, SO_KEEPALIVE, 
			&m_keepalive, sizeof(m_keepalive));
	if (ret < 0) {
		sz_log(0, "setsockopt() failed, errno %d (%s)",
				errno, strerror(errno));
		close(m_listen_socket);
		m_listen_socket = -1;
		return 1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	for (int i = 0; i < 30; i++) {
		ret = bind(m_listen_socket, (struct sockaddr *)&addr, sizeof(addr));
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
	if (ret < 0) {
		sz_log(0, "bind() failed, errno %d (%s)",
				errno, strerror(errno));
		close(m_listen_socket);
		m_listen_socket = -1;
		return 1;
	}
	ret = listen(m_listen_socket, 1);
	if (ret < 0) {
		sz_log(0, "listen() failed, errno %d (%s)",
				errno, strerror(errno));
		close(m_listen_socket);
		m_listen_socket = -1;
		return 1;
	}
	m_running = 1;
	return 0;
}

int ModbusTCP::Accept(int timeout)
{
	struct sockaddr_in addr;
	socklen_t size;
	struct timeval tv;
	fd_set set;
	int ret;
	time_t t1, t2;

	assert (m_listen_socket >= 0);
	if (timeout <= 0)
		return 1;
	time(&t1);
	
	while (m_socket < 0) {
		time(&t2);
		tv.tv_sec = timeout - (t2 - t1);
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(m_listen_socket, &set);

		sz_log(10, "Enterning accept()");
		ret = select(m_listen_socket + 1, &set, NULL, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			}
			sz_log(1, "select(m_listen_socket) failed, errno %d (%s)",
					errno, strerror(errno));
			m_last_tcp_event = time(NULL);
			return 1;
		}
		if (ret == 0) {
			/* timeout */
			return 1;
		}
		m_last_tcp_event = time(NULL);
		size = sizeof(addr);
		m_socket = accept(m_listen_socket, (struct sockaddr *)&addr, &size);
		if (m_socket < 0) {
			if (errno != EINTR) {
				sz_log(1, "accept() failed, errno %d (%s)",
						errno, strerror(errno));
				return 1;
			} else {
				continue;
			}
		}
		if (!IPAllowed(&addr)) {
			sz_log(3, "Connection from %s not allowed",
					inet_ntoa(addr.sin_addr));
			close(m_socket);
			m_socket = -1;
			return 1;
		}
		sz_log(5, "Connection accepted");
	}
	return 0;
}

int ModbusTCP::Read(int size, unsigned char **buffer)
{
	int toread, c;
	
	assert (size > 0);
	assert (m_socket >= 0);
	
	*buffer = (unsigned char *) calloc (size, sizeof(unsigned char));
	assert (*buffer != NULL);
	toread = size;
	while (toread > 0) {
		errno = 0;
		c = read(m_socket, *buffer + (size - toread), toread);
		if (c <= 0) {
			if (errno == EINTR) {
				continue;
			} else {
				free(*buffer);
				sz_log(10, "Read(): %d bytes", size - toread);
				return 1;
			}
		}
		toread -= c;
	}
	return 0;
}

void ModbusTCP::CreateResponse()
{
	assert (m_received_message != NULL);
	assert (m_received_message_size >= 2);

	if (m_received_message_size == 2) {
		CreateException(m_received_message[0], MB_ILLEGAL_FUNCTION);
		return;
	}
	switch (m_received_message[1]) {
		case MB_F_RHR:
			ParseFunReadHoldingReg(&m_received_message[1], m_received_message_size - 1);
			break;
		case MB_F_WMR:
			ParseFunWriteMultReg(&m_received_message[1], m_received_message_size - 1);
			break;
		default:
			CreateException(m_received_message[0], MB_ILLEGAL_FUNCTION);
			break;
	}
	free(m_received_message);
	m_received_message = NULL;
	m_received_message_size = 0;
}

void ModbusTCP::CreateException(char function, char code)
{
	m_send_message_size = 2;
	m_send_message = (char *) calloc(m_send_message_size, sizeof(char));
	assert (m_send_message != NULL);
	m_send_message[0] = function | MODBUS_ERROR_CODE;
	m_send_message[1] = code;
	sz_log(4, "created exception response, function %d, code %d",
			function, code);
}

void ModbusTCP::ParseFunReadHoldingReg(unsigned char *frame, int size)
{
	int16_t start_address;
	int16_t quantity;

	if (size != 5) {
		return CreateException(MB_F_RHR, MB_ILLEGAL_DATA_VALUE);
	}
	start_address = ntohs(* (int16_t *)&frame[1]);
	quantity = ntohs(* (int16_t *)&frame[3]);
	sz_log(10, "MB_F_RHR: start %d, quantity %d", start_address, quantity);
	
	if (CheckAddress(start_address, quantity)) {
		return CreateException(MB_F_RHR, MB_ILLEGAL_DATA_ADDRESS);
	}
	
	m_send_message_size = 2 + 2 * quantity;
	m_send_message = (char *) calloc(m_send_message_size, sizeof(char));
	assert (m_send_message != NULL);
	
	m_send_message[0] = MB_F_RHR;
	m_send_message[1] = 2 * quantity;
	memcpy((void *)&m_send_message[2], 
			(void *)&m_registers[start_address], 
			quantity * 2);
}

int ModbusTCP::CheckServerResponseError(unsigned char *frame, int size) {
	if (size < 3) {
		sz_log(2, "Response from server too short %d", size);
		return 1;
	}

	if (frame[1] & MODBUS_ERROR_CODE) {
		switch (frame[1] & 0xf) {
			case MB_F_RHR:
				sz_log(1, "Error is response to read holding registers, error is:");
				break;
			case MB_F_RIR:
				sz_log(1, "Error is response to read input registers, error is:");
				break;
			case MB_F_WSC:
				sz_log(1, "Error is response to write single coil, error is:");
				break;
			case MB_F_WSR:
				sz_log(1, "Error is response to write single register, error is:");
				break;
			case MB_F_WMR:
				sz_log(1, "Error is response to write multiple registers, error is:");
				break;
		}

		switch (frame[2]) {
			case MB_ILLEGAL_FUNCTION:
				sz_log(1, "MB_ILLEGAL_FUNCTION");
				break;
			case MB_ILLEGAL_DATA_ADDRESS:
				sz_log(1, "MB_ILLEGAL_DATA_ADDRESS");
				break;
			case MB_ILLEGAL_DATA_VALUE:
				sz_log(1, "MB_ILLEGAL_DATA_VALUE");
				break;
			case MB_SLAVE_DEVICE_FAILURE:
				sz_log(1, "MB_SLAVE_DEVICE_FAILURE");
				break;
			case MB_ACKNOWLEDGE:
				sz_log(1, "MB_ACKNOWLEDGE");
				break;
			case MB_SLAVE_DEVICE_BUSY:
				sz_log(1, "MB_SLAVE_DEVICE_BUSY");
				break;
			case MB_MEMORY_PARITY_ERROR:
				sz_log(1, "MB_MEMORY_PARITY_ERROR");
				break;
			case MB_GATEWAY_PATH_UNAVAILABLE:
				sz_log(1, "MB_GATEWAY_PATH_UNAVAILABLE");
				break;
			case MB_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND:
				sz_log(1, "MB_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND");
				break;
		}

		return 1;

	}

	return 0;

}

int ModbusTCP::SendReadMultReg(int from, int to, int timeout) {
	assert(to > from);
	int quantity = to - from;
	m_send_message_size = 5;
	m_send_message = (char*) calloc(m_send_message_size, sizeof(char));
	m_send_message[0] = MB_F_RHR;
	*(int16_t*)&m_send_message[1] = htons(from);
	*(int16_t*)&m_send_message[3] = htons(quantity);

	time_t start_time;
	time(&start_time);
	sz_log(10, "SendReadMultReg(): starting");
	if (SendMessage(timeout)) {
		sz_log(10, "SendReadMultReg(): SendMessage() failed");
		return 1;
	}

	if (ReadMessage(timeout - (time(NULL) - start_time))) {
		sz_log(10, "SendReadMultReg(): ReadMessage() failed");
		return 1;
	}

	int ret = 1;
	if (CheckServerResponseError(m_received_message, m_received_message_size)) {
		sz_log(10, "SendReadMultReg(): CheckServerResponseError() failed");
		goto end;
	}

	if (m_received_message_size != 3 + quantity * 2) {
		sz_log(10, "SendReadMultReg(): m_received_message_size (%d) != 3 + quantity * 2 (%d)", 
				m_received_message_size, 3 + quantity * 2);
		goto end;
	}

	if (m_received_message[1] != MB_F_RHR) {
		sz_log(10, "SendReadMultReg(): m_received_message[1] != MB_F_RHR");
		goto end;
	}

	if (m_received_message[2] != quantity * 2) {
		sz_log(10, "SendReadMultReg(): m_received_message[2] (%d) != quantity * 2 (%d)", 
				m_received_message[2], quantity * 2);
		goto end;
	}

	memcpy((char*)&m_registers[from], 
			m_received_message + 3,
			quantity * 2);
	ret = 0;
	SetWriteTime(from, quantity);
end:
	free(m_received_message);
	m_received_message = NULL;

	return ret;
}

int ModbusTCP::SendReadRegisters(int timeout) {
	if (m_params_count == 0)
		return 0;

	time_t start_time, t;
	time(&start_time);

	int start, next;
 	start = m_params[0].addr;
	next = start + (m_params[0].type == MB_TYPE_FLOAT ? 2 : 1);

	for (int i = 1; i < m_params_count; i++) {
		if (m_params[i].addr != next) {
			time(&t);
			SendReadMultReg(start, next, timeout - (t - start_time));
			m_trans_id++;
			start = m_params[i].addr;
			next = start;
		}
		next += m_params[i].type == MB_TYPE_FLOAT ? 2 : 1;
	}
	time(&t);
	SendReadMultReg(start, next, timeout - (t - start_time));

	m_trans_id++;
	return 0;
}

int ModbusTCP::SendWriteMultReg(int from, int to, int timeout) {
	assert(to > from);
	int quantity = to - from;

	m_send_message_size = 6 + 2 * quantity;
	m_send_message = (char *) calloc(m_send_message_size, sizeof(char));
	m_send_message[0] = MB_F_WMR;
	*(int16_t*)&m_send_message[1] = htons(from);
	*(int16_t*)&m_send_message[3] = htons(quantity);
	m_send_message[5] = 2 * quantity;

	memcpy((void *)&m_send_message[6], 
			(void *)&m_registers[from], 
			quantity * 2);

	time_t start_time;
	time(&start_time);
	if (SendMessage(timeout))
		return 1;

	if (ReadMessage(timeout - (time(NULL) - start_time)))
		return 1;

	int ret = 1;
	if (CheckServerResponseError(m_received_message, m_received_message_size))
		goto end;

	if (m_received_message_size != 5)
		goto end;

	if (m_received_message[0] == MB_F_WMR)
		ret = 0;
		
end:
	free(m_received_message);
	m_received_message = NULL;
	m_received_message_size = 0;
	
	return ret;
}

int ModbusTCP::SendWriteRegisters(int timeout) {
	time_t start_time, t;

	m_trans_id = 1;
	if (m_sends_count == 0)
		return 0;
	int start, next;
 	start = m_sends[0].addr;
	next = start + (m_sends[0].type == MB_TYPE_FLOAT ? 2 : 1);

	time(&start_time);
	for (int i = 1; i < m_sends_count; i++) {
		if (m_sends[i].addr != next) {
			time(&t);
			SendWriteMultReg(start, next, timeout - (t - start_time));
			m_trans_id++;
			start = m_sends[i].addr;
			next = start;
		}
		next += m_sends[i].type == MB_TYPE_FLOAT ? 2 : 1;
	}
	time(&t);
	SendWriteMultReg(start, next, timeout - (t - start_time));
	m_trans_id++;
	return 0;
}

void ModbusTCP::ParseFunWriteMultReg(unsigned char *frame, int size)
{
	int16_t start_address;
	int16_t quantity;
	unsigned char byte_count;
	
	if (size < 6) {
		return CreateException(MB_F_WMR, MB_ILLEGAL_DATA_VALUE);
	}
	start_address = ntohs(*(int16_t *)&frame[1]);
	quantity = ntohs(* (int16_t *)&frame[3]);
	sz_log(10, "MB_F_WMR: start %d, quantity %d", start_address, quantity);
	
	if (CheckAddress(start_address, quantity)) {
		return CreateException(MB_F_RHR, MB_ILLEGAL_DATA_ADDRESS);
	}
	
	byte_count = frame[5];
	if (byte_count != (quantity * 2)) {
		return CreateException(MB_F_WMR, MB_ILLEGAL_DATA_VALUE);
	}
	if (size != (byte_count + 6)) {
		return CreateException(MB_F_WMR, MB_ILLEGAL_DATA_VALUE);
	}
	memcpy(&m_registers[start_address], &frame[6], byte_count);
	SetWriteTime(start_address, quantity);

	m_send_message_size = 5;
	m_send_message = (char *) calloc (m_send_message_size, sizeof(char));
	assert (m_send_message != NULL);
	memcpy(m_send_message, frame, 5);
}

int ModbusTCP::CheckAddress(int start, int quantity)
{
	if (start < 0)
		return 1;
	if (quantity < 1)
		return 1;
	if (start + quantity > m_registers_size)
		return 1;
	return 0;
}

void ModbusTCP::SetWriteTime(int start, int quantity)
{
	time_t t;
	int i;
	
	time(&t);
	for (i = 0; i < quantity; i++) {
		m_reg_write[start + i] = t;
	}
}

int ModbusTCP::Connect(int timeout) {
	m_running = 1;

	bool connected = false;

	time_t start_time;

	if (m_socket >= 0)
		close(m_socket);

	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket < 0) {
		sz_log(0, "Socket error: %d(%s)", errno, strerror(errno));
		return 1;
	}
	
	int flags = fcntl(m_socket, F_GETFL);
	if (flags < 0) {
		sz_log(0, "fcntl error: %d(%s)", errno, strerror(errno));
		goto error;
	}

	time(&start_time);

	flags |= O_NONBLOCK;
	if (fcntl(m_socket, F_SETFL, flags) < 0) {
		sz_log(0, "fcntl error: %d(%s)", errno, strerror(errno));
		goto error;
	}

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr = *m_server_ip;
	sin.sin_port = htons(m_port);
	memcpy(&sin.sin_addr, m_server_ip, sizeof(struct in_addr));

	while (!connected) {
		int ret = connect(m_socket, (struct sockaddr*) &sin, sizeof(sin));
		if (ret == 0)
			connected = true;
		else {
			assert(ret < 0);
			if (errno == EINTR)
				continue;
			if (errno == EINPROGRESS) while (!connected) {
				struct timeval tv;
				fd_set set;

				time_t t = time(NULL);
				tv.tv_sec = timeout - (t - start_time);
				if (tv.tv_sec <= 0)
					goto error;

				tv.tv_usec = 0;
				FD_ZERO(&set);
				FD_SET(m_socket, &set);
				ret = select(m_socket + 1, NULL, &set, NULL, &tv);
				if (ret == -1) {
					if (errno == EINTR)
						continue;
					sz_log(0, "Error while connecting to server: %d(%s)", errno, strerror(errno));
					goto error;
				} else if (ret == 0) {
					sz_log(0, "Timeout while waiting for connection to server");
					goto error;
				} else 
					connected = true;
			} else {
				sz_log(0, "Error while connecting to server: %d(%s)", errno, strerror(errno));
				goto error;
			}
		}
	}
	return 0;

error:
	close(m_socket);
	m_socket = -1;
	return 1;
}

#if 0
int ModbusTCP::CloseClientConection() {
	close(m_socket);
	m_socket = -1;
}
#endif

int ModbusTCP::ReadMessage(int timeout) 
{
	int16_t buf[3];
	int ret;
	struct timeval tv;
	fd_set set;
	time_t t1, t2;
	
	assert (m_running != 0);
	time(&t1);
	
	if (m_socket < 0)
		return 1;
	if (timeout <= 0)
		return 1;
	sz_log(10, "Waiting for data");
	while (1) {
		time(&t2);
		tv.tv_sec = timeout - (t2 - t1);
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(m_socket, &set);
		
		ret = select(m_socket + 1, &set, NULL, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			}
			sz_log(1, "select(m_socket) failed, errno %d (%s)",
					errno, strerror(errno));
			close(m_socket);
			m_socket = -1;
			m_last_tcp_event = time(NULL);
			return 1;
		}
		if (ret == 0) {
			/* timeout */
			return 1;
		}
	
		m_last_tcp_event = time(NULL);
		ret = read(m_socket, buf, 6);
		sz_log(10, "return from read(): %d", ret);
		if (ret != 6) 
			goto error;

		buf[0] = ntohs(buf[0]);
		buf[1] = ntohs(buf[1]);
		buf[2] = ntohs(buf[2]);
		m_trans_id = buf[0];
		sz_log(10, "Transaction id: %d", m_trans_id);
		if (buf[1] != 0) {
			sz_log(0, "Bad protocol identifier in frame from client (0x%x)", buf[1]);
			goto error;
		}
		if (buf[2] <= 0) {
			sz_log(0, "Frame length to small (%d)", buf[2]);
			goto error;
		} else {
			sz_log(10, "Frame length: %d", buf[2]);
		}
		unsigned char *buffer;
		if (Read(buf[2], &buffer) == 0) {
			m_unit_id = buffer[0];
			m_received_message = buffer;
			m_received_message_size = buf[2];
			sz_log(10, "All data received OK");
			return 0;
		} else {
			sz_log(0, "Error receiving data, errno %d (%s)",
					errno, strerror(errno));
			goto error;
		}
		
	}
error:
	close(m_socket);
	m_socket = -1;

	return 1;
}

int ModbusTCP::SendMessage(int timeout)
{
	char ADUHeader[7];
	int ret;
	struct timeval tv;
	fd_set set;
	time_t t1, t2;
	int c, towrite;
	struct iovec iobuf[2];
	
	assert (m_running != 0);
	if (m_socket < 0) {
		ret = 0;
		goto out;
	}
	if (timeout < 0) {
		ret = 1;
		goto out;
	}
	if (m_send_message == NULL) {
		ret = 1;
		goto out;
	}
	sz_log(10, "Sending response");
	time(&t1);

	*(int16_t *)(&ADUHeader[0]) = htons(m_trans_id); /* transaction id */
	*(int16_t *)(&ADUHeader[2]) = htons(0); /* Modbus protocol identifier */
	*(int16_t *)(&ADUHeader[4]) = htons(m_send_message_size + 1);
						/* ReplyToMaster size */
	ADUHeader[6] = m_unit_id;
	
	while (1) {
		time(&t2);
		tv.tv_sec = timeout - (t2 - t1);
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(m_socket, &set);
		
		ret = select(m_socket + 1, NULL, &set, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			}
			sz_log(1, "select(m_socket) failed, errno %d (%s)",
					errno, strerror(errno));
			close(m_socket);
			m_socket = -1;
			ret = 1;
			m_last_tcp_event = time(NULL);
			goto out;
		}
		if (ret == 0) {
			/* timeout, we would better close socket */
			close(m_socket);
			m_socket = -1;
			ret = 1;
			goto out;
		}
		break;
	}
	
	iobuf[0].iov_base = ADUHeader;
	iobuf[0].iov_len = 7;
	iobuf[1].iov_base = m_send_message;
	iobuf[1].iov_len = m_send_message_size;

	towrite = 7 + m_send_message_size;
	while (towrite > 0) {
		c = writev(m_socket, iobuf, 2);
		m_last_tcp_event = time(NULL);
		if (c <= 0) {
			if (errno == EINTR) {
				continue;
			} else {
				sz_log(10, "Write(): %d bytes to write", towrite);
				ret = 1;
				goto out;
			}
		}
		towrite -= c;
	}
	sz_log(10, "All data successfully written");
	
	ret = 0;
out:
	m_send_message_size = 0;
	free(m_send_message);
	m_send_message = NULL;
	return ret;
}

void ModbusTCP::UpdateParamRegisters(IPCHandler *ipc) {
	assert (ipc != NULL);
	assert (m_registers != NULL);
	assert (m_registers_size > 0);
	assert (m_params_count == ipc->m_params_count);

	time_t t;
	int i;
	time(&t);

	for (i = 0; i < m_params_count; i++) {
		ipc->m_read[i] = ReadRegister(
				m_params[i].addr,
				m_params[i].type, 
				m_params[i].prec,
				t,
				m_params[i].op);
	}

}

void ModbusTCP::UpdateSendRegisters(IPCHandler *ipc)
{
	assert (ipc != NULL);
	assert (m_registers != NULL);
	assert (m_registers_size > 0);
	assert (m_sends_count == ipc->m_sends_count);

	time_t t;
	int i;
	time(&t);
	for (i = 0; i < m_sends_count; i++) {
		WriteRegister(m_sends[i].addr,
				m_sends[i].type,
				m_sends[i].prec,
				ipc->m_send[i]);
	}
}

void ModbusTCP::PrintRegisters(IPCHandler *ipc)
{
	for (int i = 0; i < m_registers_size; i++) {
		printf("REGISTER INT [0x%x - %d]: %d\n", i, i, m_registers[i]);
	}
	for (int i = 0; i < m_registers_size; i += 2) {
		printf("REGISTER FLOAT [0x%x - %d]: %g\n", i, i, *((float *)(&m_registers[i])));
	}
	for (int i = 0; i < m_params_count; i++) {
	        printf("IPC PARAM %d (register 0x%x): %d\n", i, m_params[i].addr, ipc->m_read[i]);
	}
	for (int i = 0; i < m_sends_count; i++) {
	        printf("IPC SEND %d (register 0x%x): %d\n", i, m_sends[i].addr, ipc->m_send[i]);
	}

}


short int ModbusTCP::ReadRegister(int address, ModbusValType type,
		int prec, time_t t, ValueOperator op)
{
	float f;

	if (t - m_reg_write[address] > m_nodata_timeout) {
		return SZARP_NO_DATA;
	}
	if (type == MB_TYPE_INT) {
		return ntohs(m_registers[address]);
	}
	if (type != MB_TYPE_FLOAT) {
		return SZARP_NO_DATA;
	}

	int16_t d[2];
	
	if (m_float_order == FLOAT_ORDER_MSWLSW) {
		d[0] = ntohs(m_registers[address]);
		d[1] = ntohs(m_registers[address + 1]);
	} else {
		d[0] = ntohs(m_registers[address + 1]);
		d[1] = ntohs(m_registers[address]);
	}
	
	memcpy(&f, d, sizeof(float));

	if (op != ModbusTCP::NONE) {
		int32_t v = (int32_t)(f * (float)prec);
		if (op == ModbusTCP::LSW) {
			sz_log(10, "val_op=\"LSW\" for address %d: v=%d, LSW=%d\n", address, v, (short int)(v & 0xFFFF));
			return (short int)(v & 0xFFFF);
		} else if (op == ModbusTCP::MSW) {
			sz_log(10, "val_op=\"LSW\" for address %d: v=%d, MSW=%d\n", address, v, (short int)((v >> 16) & 0xFFFF));
			return (short int)((v >> 16) & 0xFFFF);
		}
	}
	return (short int)(f * (float)prec);
}

void ModbusTCP::WriteRegister(int address, ModbusValType type,
		int prec, short int value)
{
	sz_log(10, "WriteRegister address %d value %d type %d: ", address, value, type);
	if (type == MB_TYPE_INT) {
		printf(" as int\n");
		if (value == SZARP_NO_DATA) {
			m_registers[address] = htons((short int)m_nodata_value);
		} else {
			m_registers[address] = htons(value);
		}
		return;
	}
	assert (type == MB_TYPE_FLOAT);
	float f;
	if (value == SZARP_NO_DATA) {
		f = m_nodata_value;
	} else {
		f = value;
		f /= prec;
	}
	sz_log(10, " as float (%g)\n", f);

	memcpy(&m_registers[address], &f, sizeof(float));
	if (m_float_order == FLOAT_ORDER_MSWLSW) {
		m_registers[address] = htons(m_registers[address]);
		m_registers[address + 1] = htons(m_registers[address + 1]);
	} else {
		m_registers[address] = htons(m_registers[address + 1]);
		m_registers[address + 1] = htons(m_registers[address]);
	}
}

int ModbusTCP::Stop()
{
	if (!m_running)
		return 0;
	if (m_socket >= 0)
		close(m_socket);
	if (m_listen_socket >= 0)
		close(m_listen_socket);
	m_listen_socket = -1;
	m_socket = -1;
	m_running = 0;
	return 0;
}

void ModbusTCP::Reset()
{
	if (m_socket >= 0) {
		sz_log(10, "Reseting idle connection");
		close(m_socket);
	}
	m_socket = -1;
}

void ModbusTCP::CheckReset()
{
	if (m_tcp_timeout <= 0) {
		return;
	}
	time_t t = time(NULL);
	if ((t - m_last_tcp_event) > m_tcp_timeout) {
		Reset();
	}
}

/* Global server object, for signal handling */
static ModbusTCP *g_mtcp = NULL;

void exit_handler()
{
	sz_log(2, "exit_handler called, cleaning up");
	if (g_mtcp)
		g_mtcp->Stop();
	sz_log(2, "cleanup completed");
}

RETSIGTYPE terminate_handler(int signum)
{
	sz_log(2, "signal %d cought", signum);
	exit(1);
}

void init_signals()
{
	int ret;
	struct sigaction sa;
	sigset_t block_mask;

	sigfillset(&block_mask);
	sigdelset(&block_mask, SIGKILL);
	sigdelset(&block_mask, SIGSTOP);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = terminate_handler;
	sigemptyset(&sa.sa_mask);
	ret = sigaction(SIGTERM, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	assert (ret == 0);
}

int server_loop(ModbusTCP *mtcp, IPCHandler *ipc, DaemonConfig *cfg)
{
	time_t	last_read, t;

	if (mtcp->StartServer()) {
		return 1;
	}

	time(&last_read);
	while (1) {
		int c = 0;
		/* send data to sender */
		ipc->GoSender();
		mtcp->UpdateSendRegisters(ipc);
		time(&t);
		while (t - last_read < 10) {
			if (mtcp->Accept(10 - (t - last_read)) == 0) {
				time(&t);
				if (mtcp->ReadMessage(10 - (t - last_read))) {
					time(&t);
					continue;
				}
				mtcp->CreateResponse();
				time(&t);
				if (mtcp->SendMessage(10 - (t - last_read)))
					break;	
				c++;
				sz_log(10, "Counter increased to %d", c);
				time(&t);
			} else
				time(&t);
		}
		/* check for timeout */
		if (c <= 1) {
			/* no communication - close socket if timeout exceeded */
			mtcp->CheckReset();
		}
		time(&last_read);
		mtcp->UpdateParamRegisters(ipc);
		/* store data to parcook segment */
		ipc->GoParcook();
		if (cfg->GetSingle() || cfg->GetDiagno()) {
			mtcp->PrintRegisters(ipc);
		}
	}
	return 0;

}

int client_loop(ModbusTCP *mtcp, IPCHandler *ipc, DaemonConfig *cfg)
{
	time_t t, last_time;

	time(&last_time);
	while (1) {
		time(&t);
		ipc->GoSender();

		mtcp->UpdateSendRegisters(ipc);

		if (mtcp->Connect(10 - (t - last_time)) == 0) {
			time(&t);
			if (mtcp->SendWriteRegisters(10 - (t - last_time)) == 0) {
				time(&t);
				mtcp->SendReadRegisters(10 - (t - last_time));
			}
		}
		mtcp->CheckReset();

		mtcp->UpdateParamRegisters(ipc);
		ipc->GoParcook();
		/* send data to sender */
		if (cfg->GetSingle() || cfg->GetDiagno()) {
			mtcp->PrintRegisters(ipc);
		}

		time(&t);
		int s = 10 - (t - last_time);
		while (s > 0)
			s = sleep(s);
		
		time(&last_time);
	}

	return 0;

}

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	IPCHandler     *ipc;
	ModbusTCP *mtcp;

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);

	cfg = new DaemonConfig("mbtcpdmn");
	assert (cfg != NULL);
	
	if (cfg->Load(&argc, argv))
		return 1;
	
	mtcp = new ModbusTCP(cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetParamsCount(),
			cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetSendParamsCount());
	assert (mtcp != NULL);
	
	if (mtcp->ParseConfig(cfg)) {
		return 1;
	}

	if (cfg->GetSingle() || cfg->GetDiagno()) {
		printf("line number: %d\ndevice: %ls\nparams in: %d\nparams out %d\n", 
				cfg->GetLineNumber(), 
				cfg->GetDevice()->GetPath().c_str(), 
				cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount(),
				cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetSendParamsCount()				);
		for (int i = 0; i < mtcp->m_params_count; i++)
			printf("  IN:  addr %04x type %s prec %d\n",
			       mtcp->m_params[i].addr,
			       mtcp->m_params[i].type == MB_TYPE_INT ?
			       "integer" : "float  ",
			       mtcp->m_params[i].prec);
		for (int i = 0; i < mtcp->m_sends_count; i++)
			printf("  OUT: addr %04x type %s prec %d\n",
			       mtcp->m_sends[i].addr,
			       mtcp->m_sends[i].type == MB_TYPE_INT ?
			       "integer" : "float  ",
			       mtcp->m_sends[i].prec);
	}

	ipc = new IPCHandler(cfg);
	if (!cfg->GetSingle()) {
		if (ipc->Init())
			return 1;
	}

	if (atexit(exit_handler)) {
		sz_log(0, "atexit failed, errno %d (%s)",
				errno, strerror(errno));
		return 1;
	}

	init_signals();

	switch (mtcp->GetDaemonType()) {
		case ModbusTCP::SERVER:	
			server_loop(mtcp, ipc, cfg);
			break;
		case ModbusTCP::CLIENT:
			client_loop(mtcp, ipc, cfg);
			break;
	}

	sz_log(2, "Starting Modbus TCP Daemon");

}

#else /* LIBXML_XPATH_ENABLED */

int main(void)
{
	printf("libxml2 XPath support not enabled!\n");
	return 1;
}

#endif /* LIBXML_XPATH_ENABLED */

