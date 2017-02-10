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
 * Demon do komunikacji ze sterownikami ZET/SK2000/SK4000 (protokó³ Pratermu)
 * z wykorzystaniem ethernetowego serwera portów szeregowych, np. dla trybu
 * TCP Server urz±dzenia Moxa NPort W2250/2150.
 *
 * $Id$
 */
/*
 @description_start
 @class 4
 @devices Z-Elektronik, SK-2000 or SK-4000 PLCs, using ethernet serial port
 server, for example Moxa NPort W2250/2150 in TCP Server mode.
 @devices.pl Sterowniki Z-Elektronik, SK-2000 i SK-4000 z wykorzystaniem
 sieciowego serwera portów szeregowych, np. Moxa  NPort W2250/2150 w trybie TCP Server.
 @protocol Z-Elektronik through TCP/IP link.
 @config.pl Konfiguracja w params.xml, atrybuty z przestrzeni nazw 'tcp' s± wymagane,
 chyba ¿e napisano inaczej:
 @config_example.pl
 <device
      xmlns:tcp="http://www.praterm.com.pl/SZARP/ipk-extra"
      daemon="/opt/szarp/bin/tcpdmn"
      tcp:tcp-ip="192.168.6.10"
      		serial ports' server address
      tcp:tcp-port="4001"
		serial ports TCP port
      tcp:tcp-keepalive="yes"
 		czy po³±czenie TCP powinno mieæ opcjê Keep-Alive; dopuszczalne
		warto¶ci to "yes" i "no"
	tcp:nodata-timeout="15"
		czas w sekundach, po jakim ustawiamy 'NO_DATA' je¿eli dane nie
		przysz³y, opcjonalny - domy¶lnie 20 sekund
	tcp:nodata-value="-1"
		warto¶æ (typu float) jak± wysy³amy zamiast 'NO_DATA' dla
		parametrów typu send, domy¶lnie wysy³amy 0
      >
      <unit id="1">
              <param
                      name="..."
                      ...
              </param>
               ...
              <send
                      param="..."
                      type="min"
                      ...
              </send>
              ...
      </unit>
 </device>
 @description_end
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

#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include <libxml/parser.h>

#ifdef LIBXML_XPATH_ENABLED

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "ipchandler.h"
#include "liblog.h"
#include "xmlutils.h"
#include "tokens.h"
#include "conversion.h"
#include "custom_assert.h"

#define TCP_DEFAULT_PORT 4001

#define DAEMON_INTERVAL 10

#define DEBUG_FREQ 10

/**
 * Modbus communication config.
 */
class TCPServer {
public :
	/**
	 * @param params number of params to read
	 * @param sends number of params to send (write)
	 */
	TCPServer(int params, int sends, char id);
	~TCPServer();

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
	 * Starts daemon
	 * @return 0 on success, 1 on error
	 */
	int Start();

	/** Sends query string (and 'send' parameters data if available)
	 * @param ipc IPCHandler - object for contacting with parcook/sender
	 * @param timeout timeout, in seconds
	 * @return 0 on success or timeout, 1 on error
	 */
	int Send(int timeout);

	/** Try to connect to to server.
	 * @return 0 on success, -1 on error
	 */
	int Connect();

	/**
	 * @return -1 on error, 0 on timeout, 1 if data is available
	 */
	int Wait(int timeout);

	/**
	 * Waits for response from PLC.
	 * @param ipc IPCHandler object
	 * @return 0 on success or timeout, 1 on error
	 */
	int GetData(IPCHandler *ipc);

	/**
	 * Closes opened sockets. Returns without error if server state is
	 * 'not running'.
	 * @return 0 on success, 1 on error
	 */
	int Stop();

	/**
	 * Closes communication socket. */
	void Reset();

protected :
	/** helper function for XML parsing
	 * @return 1 on error, 0 otherwise */
	int XMLCheckIP(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing
	 * @return 1 on error, 0 otherwise */
	int XMLCheckPort(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing
	 * @return 1 on error, 0 otherwise */
	int XMLCheckKeepAlive(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing
	 * @return 1 on error, 0 otherwise */
	int XMLCheckNodataTimeout(xmlXPathContextPtr xp_ctx, int dev_num);
	/** helper function for XML parsing
	 * @return 1 on error, 0 otherwise */
	int XMLCheckNodataValue(xmlXPathContextPtr xp_ctx, int dev_num);
	/** Checks if IP is allowed.
	 * @return 1 if IP is allowed, 0 if not. */

	char m_id;		/**< unit id */
	int m_keepalive;	/**< 1 if TCP KeepAlive option should
					  be set, 0 otherwise */
	int m_single;		/**< are we in 'single' mode */
	public:
	int m_params_count;	/**< size of params array */
	int m_sends_count;	/**< size of sends array */
	protected:
	unsigned short int m_port;
				/**< port number to listen on */
	float m_nodata_value;	/**< value sended instead of NO_DATA */
	int m_nodata_timeout;	/**< timeout for 'NO_DATA' */

	struct in_addr m_ip;	/**< IP address to connect to */

	int m_socket;		/**< Listen socket descriptor. */

	long int m_debug_send;	/**< Number of questions sent */
	long int m_debug_ok;	/**< Number of correct answers */

};

/**
 * @param params number of params to read
 * @param sends number of params to send (write)
 */
TCPServer::TCPServer(int params, int sends, char id)
{
	ASSERT(params >= 0);
	ASSERT(sends >= 0);

	m_id = id;
	m_params_count = params;
	m_sends_count = sends;
	m_keepalive = 0;
	m_port = 4001;
	m_socket = -1;
	m_nodata_timeout = 20;
	m_nodata_value = 0.0;
	m_debug_send = 0;
	m_debug_ok = 0;
}

TCPServer::~TCPServer()
{
	Stop();
}


int TCPServer::XMLCheckPort(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;
	long l;

	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@tcp:tcp-port",
			dev_num);
	ASSERT(e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx, false);
	free(e);
	if (c == NULL){
		sz_log(2, "using default tcp port %d", m_port);
		return 0;
	}
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
	sz_log(2, "using tcp port %d", m_port);
	return 0;
}

int TCPServer::XMLCheckIP(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;

	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@tcp:tcp-ip",
			dev_num);
	ASSERT(e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c != NULL) {
		int ret = inet_aton((char *)c, &m_ip);
		if (ret == 0) {
			sz_log(0, "incorrect IP address '%s'", c);
			return 1;
		} else {
			sz_log(2, "IP address to connecto to: '%s'", c);
			return 0;
		}
	}
	return 1;
}

int TCPServer::XMLCheckKeepAlive(xmlXPathContextPtr xp_ctx, int dev_num)
{
	char *e;
	xmlChar *c;

	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@tcp:tcp-keepalive",
			dev_num);
	ASSERT(e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx, false);
	free(e);
	if (c == NULL) {
		sz_log(5, "setting TCP Keep-Alive options to default \"%s\"", (m_keepalive) ? "yes":"no");
		return 0;
	}
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

int TCPServer::XMLCheckNodataTimeout(xmlXPathContextPtr xp_ctx,
		int dev_num)
{
	char *e;
	xmlChar *c;
	long l;

	asprintf(&e, "/ipk:params/ipk:device[position()=%d]/@tcp:nodata-timeout",
			dev_num);
	ASSERT(e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx, false);
	free(e);
	if (c == NULL)
	{
		sz_log(10, "Setting tcp:nodata_timeout to default %d", m_nodata_timeout);
		return 0;
	}
	l = strtol((char *)c, &e, 0);
	if ((*c == 0) || (*e != 0)) {
		sz_log(0, "incorrect value '%s' for tcp:nodata-timeout for device %d -  integer expected",
					SC::U2A(c).c_str(), dev_num);
		xmlFree(c);
		return 1;
	}
	xmlFree(c);
	if ((l < 1) || (l > 600)) {
		sz_log(0, "value '%ld' for tcp:nodata-timeout for device %d outside expected range [1..600]",
				l, dev_num);
		return 1;
	}
	m_nodata_timeout = (int) l;
	sz_log(10, "Setting tcp:nodata-timeout to %d", m_nodata_timeout);
	return 0;
}


int TCPServer::ParseConfig(DaemonConfig * cfg)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xp_ctx;
	int dev_num;
	int ret;

	/* get config data */
	ASSERT(cfg != NULL);
	dev_num = cfg->GetLineNumber();
	ASSERT(dev_num > 0);
	doc = cfg->GetXMLDoc();
	ASSERT(doc != NULL);

	/* prepare xpath */
	xp_ctx = xmlXPathNewContext(doc);
	ASSERT(xp_ctx != NULL);

	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "tcp",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	ASSERT(ret == 0);

	if (XMLCheckIP(xp_ctx, dev_num))
		return 1;

	if (XMLCheckPort(xp_ctx, dev_num))
		return 1;

	if (XMLCheckKeepAlive(xp_ctx, dev_num))
		return 1;

	if (XMLCheckNodataTimeout(xp_ctx, dev_num))
		return 1;

	xmlXPathFreeContext(xp_ctx);

	m_single = cfg->GetSingle();

	return 0;
}


int TCPServer::Connect()
{
	struct sockaddr_in addr;

	ASSERT(m_socket < 0);
	m_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (m_socket < 0) {
		sz_log(0, "socket() failed, errno %d (%s)",
				errno, strerror(errno));
		return -1;
	}
	int ret = setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE,
			&m_keepalive, sizeof(m_keepalive));
	if (ret < 0) {
		sz_log(0, "setsockopt() failed, errno %d (%s)",
				errno, strerror(errno));
		close(m_socket);
		m_socket = -1;
		return -1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);
	addr.sin_addr.s_addr = htonl(m_ip.s_addr);
	addr.sin_addr.s_addr = m_ip.s_addr;
	ret = connect(m_socket, (struct sockaddr *) &addr, sizeof(addr));
	if (ret < 0) {
		sz_log(0, "connect() failed, errno %d (%s)",
				errno, strerror(errno));
		close(m_socket);
		m_socket = -1;
		return -1;
	}
	return 0;
}

int TCPServer::Send(int timeout)
{
	m_debug_send++;
	if ((m_debug_send % DEBUG_FREQ) == 1) {
		sz_log(3, "Connection log: sent %ld, received %ld",
				m_debug_send - 1, m_debug_ok);
	}
	if (m_socket < 0) {
		if (Connect()) {
			return -1;
		}
	}
	char *outbuf;
	asprintf(&outbuf, "\x11\x02P%c\x03", m_id);
	if (m_single)
		printf("DEBUG: sending data (%s)\n", outbuf);
	int ret = write(m_socket, outbuf, strlen(outbuf));
	if (ret < 0) {
		sz_log(1, "write() failed, errno is %d (%s)",
				errno, strerror(errno));
		close(m_socket);
		m_socket = -1;
		return -1;
	}
	if (m_single)
		printf("DEBUG: wrote %d bytes\n", ret);
	free(outbuf);
	return 0;
}

int TCPServer::Start()
{
	return 0;
}


int TCPServer::Stop()
{
	if (m_socket >= 0)
		close(m_socket);
	m_socket = -1;
	sz_log(2, "Server stopped, sent %ld frames, received %ld",
			m_debug_send, m_debug_ok);
	return 0;
}


int TCPServer::Wait(int timeout)
{
	int ret;
	struct timeval tv;
	fd_set set;
	time_t t1, t2;

	ASSERT(m_socket >= 0);
	time(&t1);

	sz_log(10, "Waiting for data");
	if (m_single)
		printf("DEBUG: Waiting for data\n");
	while (1) {
		if (m_socket < 0) {
			if (Connect() < 0) {
				return -1;
			}
		}
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
			sz_log(1, "select(m__socket) failed, errno %d (%s)",
					errno, strerror(errno));
			close(m_socket);
			m_socket = -1;
			return -1;
		}
		if (ret == 0) {
			/* timeout */
			if (m_single)
				printf("DEBUG: timeout!\n");
			return 0;
		}
		if (ret > 0) {
			return 1;
		}
	}

	return 0;
}

int TCPServer::GetData(IPCHandler *ipc)
{
	int count;
#define BUFSIZE 1024
	char inbuf[BUFSIZE];
	int i;
	int checksum = 0;

	if (m_socket < 0)
		return -1;

	if (m_single)
		printf("Reading data\n");

	int c;
	int wret;
	count = 0;
	while ((wret = Wait(0)) == 1) {
again:
		c = read(m_socket, inbuf + count, BUFSIZE - 1 - count);
		if (m_single) {
			printf("Received packet of %d bytes\n", c);
		}
		if (c > 0) {
			count += c;
		} else if (c < 0 && errno == EINTR) {
			goto again;
		} else {
			if (m_single)
				printf("Reading data error: %s\n", strerror(errno));
			sz_log(0, "Reading data error: %s\n", strerror(errno));
			close(m_socket);
			m_socket = -1;
			return 1;
		}
		/* repeat if data is still available */
	}
	if (wret < 0)
		return 1;
	if (count <= 0) {
		if (m_single) {
			printf("NO DATA RECEIVED\n");
		}
		return 1;
	}
	inbuf[count] = 0;
	for (int j = 0; j < count; j++) {
		checksum += (uint) inbuf[j];
	}

	if (m_single) {
		printf("GOT DATA - %d bytes\n", count);
	}

	char **toks;
	int tokc = 0;
	tokenize_d(inbuf, &toks, &tokc, "\r");
	if (m_single)
		printf("GOT DATA - %d lines (%d parameters)\n", tokc, tokc - 5);

	if (tokc - 5 != m_params_count ) {
		if (m_single)
			printf("Incorrect number of params (%d expected)\n", m_params_count);
		sz_log(0, "Incorrect number of params (got %d, %d expected)\n", tokc - 5, m_params_count);
		goto error;
	}

	if (toks[2][0] != m_id) {
		if (m_single) {
			printf("Bad FunId code (expected '%c' [%d] got '%c' [%d]\n",
					m_id, m_id, toks[2][0], toks[2][0]);
		}
		return 1;
	}
	if (m_single) {
		printf("Raport ID: %s\n", toks[0]);
		printf("Raport SubID: %s\n", toks[1]);
		printf("Raport FunID: %s\n", toks[2]);
		printf("Date: %s\n", toks[3]);
	}

	for (i = 0; i < m_params_count; i++) {
		ipc->m_read[i] = atoi(toks[i+4]);
		if (m_single)
			printf("val[%d] = %d\n", i, ipc->m_read[i]);
	}
	/* Checksum without checksum ;-) and last empty lines */
	for (char *c = toks[tokc-1]; *c; c++)
		checksum -= (uint) *c;
	for (char *c = &inbuf[count-1]; *c == '\r'; c--)
		checksum -= (uint) *c;
	if (m_single) {
		printf("Checksum calculated: %d, received %d\n",
				checksum, atoi(toks[tokc-1]));
	}
	if (checksum != atoi(toks[tokc-1]))
		goto error;

	tokenize_d(NULL, &toks, &tokc, NULL);

	m_debug_ok++;
	return 0;
error:
	tokenize_d(NULL, &toks, &tokc, NULL);
	return 1;
}


/* Global server object, for signal handling */
static TCPServer *g_server = NULL;

void exit_handler()
{
	sz_log(2, "exit_handler called, cleaning up");
	if (g_server)
		g_server->Stop();
	sz_log(2, "cleanup completed");
}

RETSIGTYPE terminate_handler(int signum)
{
	sz_log(2, "signal %d caught", signum);
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
	sa.sa_mask = block_mask;
	ret = sigaction(SIGTERM, &sa, NULL);
	ASSERT(ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	ASSERT(ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	ASSERT(ret == 0);
	signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char *argv[])
{
	DaemonConfig   *cfg;
	IPCHandler     *ipc;
	TCPServer *server;

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);

	cfg = new DaemonConfig("tcpdmn");
	ASSERT(cfg != NULL);

	if (cfg->Load(&argc, argv))
		return 1;

	server = new TCPServer(cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetParamsCount(),
			cfg->GetDevice()->GetFirstRadio()->
			GetFirstUnit()->GetSendParamsCount(),
			cfg->GetDevice()->GetFirstRadio()->GetFirstUnit()->GetId());
	ASSERT(server != NULL);

	g_server = server;

	if (server->ParseConfig(cfg)) {
		return 1;
	}

	if (cfg->GetSingle()) {
		printf("line number: %d\ndevice: %ls\nparams in: %d\nparams out %d\n",
				cfg->GetLineNumber(),
				cfg->GetDevice()->GetPath().c_str(),
				cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetParamsCount(),
				cfg->GetDevice()->GetFirstRadio()->
				GetFirstUnit()->GetSendParamsCount()				);
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
	cfg->CloseXML();

	if (server->Start()) {
		return 1;
	}
	sz_log(2, "Starting TCP Daemon");

	while (true) {
		time_t t, t1;
		time(&t);
		/* ask for data */
		if (server->Send(DAEMON_INTERVAL)) {
			int rest = sleep(2 * DAEMON_INTERVAL);
			/* sleep can be aborted by signal */
			while (rest > 0) {
				rest = sleep(rest);
			}
			continue;
		}
		/* wait for response */
		if (server->Wait(DAEMON_INTERVAL) > 0) {
			/* A little sleep to make sure that all data is transfered. */
			sleep(1);
			if (server->GetData(ipc)) {
				for (int i = 0; i < server->m_params_count; i++)
					ipc->m_read[i] = SZARP_NO_DATA;
			}
		} else {
			int rest = sleep(2 * DAEMON_INTERVAL);
			/* sleep can be aborted by signal */
			while (rest > 0) {
				rest = sleep(rest);
			}
			continue;
		}
		/* sleep up to 10 seconds */
		time(&t1);
		if (t1 - t < DAEMON_INTERVAL) {
			sz_log(10, "Sleeping for %ld seconds",
					DAEMON_INTERVAL - (t1 - t));
			int rest = sleep(DAEMON_INTERVAL - (t1 - t));
			while (rest > 0) {
				rest = sleep(rest);
			}
		}
		sz_log(10, "Updating data");
		/* read data from parcook segment */
		ipc->GoParcook();
		/* send data to sender */
		ipc->GoSender();
	}
	return 0;
}

#else /* LIBXML_XPATH_ENABLED */

int main(void)
{
	printf("libxml2 XPath support not enabled!\n");
	return 1;
}

#endif /* LIBXML_XPATH_ENABLED */

