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
 SZARP daemon description block.

 @description_start

 @devices This daemon is designed to read data from PRO 2000 Scada system, requires
	a proxy application running on a machine with PRO 2000.

 @class 4

 @devices.pl Sterownik ten mo¿e s³u¿yæ do odczytu danych z systemu PRO 2000. Wymaga
 	programu proxy uruchomionego na maszynie z dzia³aj±cym PRO 2000.

 @protocol Daemon uses simple text based protocol, with following definition:
	Communication is performed over TCP/IP network. After establishing connection
	client sends a semiconol delimited list of params numbers, a \n char terminates message.
	In response server sends a semicolon delimited list of values with \n char terminating
	message. Upon sednding response server terminates connection. Example:
	Request:
	7001;7004\n
	Response:
	24.0000;39.13\n
 @protocol.pl Sterownik wykorzystuje prosty protokó³ tekstowy do wymiany danych z aplikacj±
 	proxy dzia³aj±c± na maszynie z uruchomionym PRO 200.

 @comment Launch obl_ch1.exe as proxy on PRO 2000 Windows machine.
 @comment.pl Wymaga uruchomienia na serwerze z Windows programu obl_ch1.exe.

 @config_example

 <device daemon="/opt/szarp/bin/prodmn" path="/dev/null" pro2000:address="192.168.1.1" pro2000:port="9911" pro2000:read_freq="10"
     xmlns:pro2000="http://www.praterm.com.pl/SZARP/ipk-extra">
   <unit id="1" type="2" subtype="1" bufsize="1">
      <param name="Kocio³ 1:DDE:Temperatura wody przed kot³em" short_name="Twe" unit="°C" prec="1"
          pro2000:param="7001" base_ind="auto">
       ....

 Where:
 pro2000:address is an IP address of machine running proxy
 pro2000:port is a port number at which proxy listens for connections
 pro2000:param number of param in pro system
 @description_end

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <typeinfo>
#include <event.h>

#include <stdarg.h>

#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "liblog.h"
#include "dmncfg.h"
#include "ipchandler.h"
#include "xmlutils.h"
#include "conversion.h"
#include "custom_assert.h"

bool g_single;

void dolog(int level, const char * fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

int set_nonblock(int fd)
{

	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		dolog(0, "set_nonblock: Unable to get socket flags");
		return -1;
	}

	flags |= O_NONBLOCK;
	flags = fcntl(fd, F_SETFL, flags);
	if (flags == -1) {
		dolog(0, "set_nonblock: Unable to set socket flags");
		return -1;
	}

	return 0;
}

void dolog(int level, const char *fmt, ...)
{
	va_list fmt_args;

	if (g_single) {
		char *l;
		va_start(fmt_args, fmt);
		if (vasprintf(&l, fmt, fmt_args) == -1) {
			sz_log(0, "Error occured when logging in single mode");
			return;
		}
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

class PRO2000Daemon {
	DaemonConfig *m_cfg;
	IPCHandler *m_ipc;
	int m_read_freq;
	enum {
		IDLE,
		WAITING_FOR_CONNECT,
		WAITING_FOR_RESPONSE
	} m_state;
	int m_fd;
	struct sockaddr_in m_addr;
	struct event_base *m_event_base;
	struct event m_timer;
	struct bufferevent *m_bufev;
	std::vector<int> m_precs;
	std::vector<int> m_param_addrs;

	std::string m_buffer;
	size_t m_semicolon_count;

	void Connect();
	void Disconnect();
	void SendQuery();
public:
	void HandleConnect();
	void HandleReadReady();
	void HandleError();
	void NextCycle();
	void Run();

	static void ErrorCallback(struct bufferevent *ev, short int error, void *daemon);
	static void ConnectCallback(struct bufferevent *ev, void *daemon);
	static void ReadReadyCallback(struct bufferevent *ev, void *daemon);
	static void TimerCallback(int fd, short event, void *daemon);

	int Configure(DaemonConfig* cfg);
};

int PRO2000Daemon::Configure(DaemonConfig * cfg)
{
	int ret, i;
	char *c;
	m_cfg = cfg;

	try {
		auto ipc_ = std::unique_ptr<IPCHandler>(new IPCHandler(m_cfg));
		m_ipc = ipc_.release();
	} catch(...) {
		return 1;
	}

	dolog(10, "IPC initialized successfully");

	m_fd = -1;
	m_bufev = NULL;
	m_state = IDLE;
	m_event_base = (struct event_base *)event_init();
	evtimer_set(&m_timer, TimerCallback, this);
	event_base_set(m_event_base, &m_timer);
	xmlDocPtr doc = cfg->GetXMLDoc();
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(doc);
	ASSERT(xp_ctx != NULL);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
				 SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "pro2000",
				 BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	ASSERT(ret == 0);
	xp_ctx->node = cfg->GetXMLDevice();
	m_addr.sin_family = AF_INET;
	c = (char *)xmlGetNsProp(cfg->GetXMLDevice(), BAD_CAST("address"),
				 BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (c == NULL) {
		dolog(0,
		      "Missing address attribute in device element at line: %ld",
		      xmlGetLineNo(cfg->GetXMLDevice()));
		return 1;
	}
	if (inet_aton((char *)c, &m_addr.sin_addr) == 0) {
		dolog(0, "incorrect server's IP address '%s'", (char *)c);
		return 1;
	}
	xmlFree(c);
	c = (char *)xmlGetNsProp(cfg->GetXMLDevice(), BAD_CAST("port"),
				 BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (c == NULL) {
		dolog(0,
		      "Missing port attribute in device element at line: %ld",
		      xmlGetLineNo(cfg->GetXMLDevice()));
		return 1;
	}
	m_addr.sin_port = htons(atoi(c));
	xmlFree(c);
	i = 0;
	for (TParam* p = cfg->GetDevice()->GetFirstUnit()->GetFirstParam();
			p;
			++i, p = p->GetNext()) {
		char *expr;
		if (asprintf(&expr, ".//ipk:param[position()=%d]", i + 1) == -1) {
			dolog(0, "Could not get param element");
			return 1;
		}
		xmlNodePtr node = uxmlXPathGetNode(BAD_CAST expr, xp_ctx, false);
		ASSERT(node);
		free(expr);
		c = (char *)xmlGetNsProp(node, BAD_CAST("address"),
					 BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
		if (c == NULL) {
			dolog(0,
			      "Missing address attribute in param element at line: %ld",
			      xmlGetLineNo(node));
			return 1;
		}
		m_param_addrs.push_back(atoi(c));
		xmlFree(c);
		m_precs.push_back(pow10(p->GetPrec()));
	}
	c = (char *)xmlGetNsProp(cfg->GetXMLDevice(), BAD_CAST("read_freq"),
				 BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (c == NULL) {
		dolog(0,
		      "Missing read_freq attribute in device element at line: %ld",
		      xmlGetLineNo(cfg->GetXMLDevice()));
		return 1;
	}
	m_read_freq = atoi(c);
	if (m_read_freq < 10) {
		dolog(0,
		      "Invalid/too small read_freq attribute value, setting 60 seconds");
		m_read_freq = 60;
	}
	xmlFree(c);
	return 0;
}

void PRO2000Daemon::Connect()
{
	Disconnect();
	m_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (m_fd < 0) {
		dolog(0, "socket() failed, errno %d (%s)", errno,
		      strerror(errno));
		exit(1);
	}
	if (set_nonblock(m_fd)) {
		dolog(1, "Failed to set non blocking mode on socket");
		exit(1);
	}

do_connect:
	int ret =::connect(m_fd, (struct sockaddr *)&m_addr, sizeof(m_addr));
	if (ret != 0) {
		if (errno == EINTR) {
			goto do_connect;
		} else if (errno == EWOULDBLOCK || errno == EINPROGRESS) {
			m_state = WAITING_FOR_CONNECT;
		} else {
			dolog(0, "Failed to connect: %s", strerror(errno));
			close(m_fd);
			m_fd = -1;
			return;
		}
	}
	m_bufev =
	    bufferevent_new(m_fd, PRO2000Daemon::ReadReadyCallback,
			    PRO2000Daemon::ConnectCallback,
			    PRO2000Daemon::ErrorCallback, this);
	bufferevent_base_set(m_event_base, m_bufev);
	bufferevent_enable(m_bufev, EV_READ | EV_WRITE | EV_PERSIST);
	if (ret == 0)
		SendQuery();
}

void PRO2000Daemon::Disconnect()
{
	if (m_fd >= 0) {
		bufferevent_free(m_bufev);
		m_bufev = NULL;
		close(m_fd);
		m_fd = -1;
	}
}

void PRO2000Daemon::HandleConnect()
{
	switch (m_state) {
	case WAITING_FOR_CONNECT:
		SendQuery();
		break;
	default:
		break;
	}
}

void PRO2000Daemon::SendQuery()
{
	size_t i;
	ASSERT(m_param_addrs.size());
	struct evbuffer *evb = evbuffer_new();
	for (i = 0; i < m_param_addrs.size() - 1; i++)
		evbuffer_add_printf(evb, "%d;", m_param_addrs[i]);
	evbuffer_add_printf(evb, "%d\n", m_param_addrs[i]);
	bufferevent_write_buffer(m_bufev, evb);
	evbuffer_free(evb);
	m_state = WAITING_FOR_RESPONSE;
	m_buffer.clear();
	m_semicolon_count = 0;
}

void PRO2000Daemon::HandleReadReady()
{
	char c;
	while (bufferevent_read(m_bufev, &c, 1)) {
		m_buffer.push_back(c);
		if (c == ';')
			m_semicolon_count += 1;
	}
	if (m_semicolon_count != m_param_addrs.size() - 1)
		return;
	dolog(10, "Received respone: %s", m_buffer.c_str());
	std::istringstream iss(m_buffer);
	double v;
	size_t i = 0;
	while (i < m_param_addrs.size() && (iss >> v)) {
		dolog(10, "Param %d(%zu), value %f", m_param_addrs[i], i, v);
		m_ipc->m_read[i] = v * m_precs[i];
		iss.ignore(1);
		i += 1;
	}
	if (i != m_param_addrs.size())
		dolog(1,
		      "Number of params in response does not match number of params in request");
	Disconnect();
	m_state = IDLE;
}

void PRO2000Daemon::HandleError()
{
	dolog(0, "Error in communicating with proxy");
	Disconnect();
	m_state = IDLE;
}

void PRO2000Daemon::NextCycle()
{
	struct timeval tv;
	tv.tv_sec = m_read_freq;
	tv.tv_usec = 0;
	evtimer_add(&m_timer, &tv);
	m_ipc->GoParcook();
	for (size_t i = 0; i < m_param_addrs.size(); i++)
		m_ipc->m_read[i] = SZARP_NO_DATA;
	switch (m_state) {
	case WAITING_FOR_CONNECT:
	case WAITING_FOR_RESPONSE:
		dolog(1, "Next cycle while in state %d, terminating connection", m_state);
		Disconnect();
	case IDLE:
		Connect();
	}

}

void PRO2000Daemon::Run()
{
	for (size_t i = 0; i < m_param_addrs.size(); i++)
		m_ipc->m_read[i] = SZARP_NO_DATA;
	NextCycle();
	event_base_dispatch(m_event_base);
}

void PRO2000Daemon::ErrorCallback(struct bufferevent *ev, short int error, void *daemon)
{
	((PRO2000Daemon *) daemon)->HandleError();
}

void PRO2000Daemon::ConnectCallback(struct bufferevent *ev, void *daemon)
{
	((PRO2000Daemon *) daemon)->HandleConnect();
}

void PRO2000Daemon::ReadReadyCallback(struct bufferevent *ev, void *daemon)
{
	((PRO2000Daemon *) daemon)->HandleReadReady();
}

void PRO2000Daemon::TimerCallback(int fd, short event, void *daemon)
{
	((PRO2000Daemon *) daemon)->NextCycle();
}

int main(int argc, char *argv[])
{
	DaemonConfig *cfg = new DaemonConfig("prodmn");
	if (cfg->Load(&argc, argv))
		return 1;
	g_single = cfg->GetSingle() || cfg->GetDiagno();
	PRO2000Daemon pro;
	if (pro.Configure(cfg))
		return 1;
	pro.Run();
}
