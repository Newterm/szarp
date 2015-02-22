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
 * Pawel Palucha <pawel@newterm.pl>
 * $Id$
 */

/*
 SZARP daemon description block, see README file.

@description_start

@class 4

@comment Initial parameter values are read from SZARP database (last assessible
values are used).

@protocol Daemon uses telnet-like protocol to allow setting values of handled
parameters by network client, especially 'setter' program. On first connection,
client gets list of handled parameters with their current values and allowed
min/max values, one parameter per line in format:

"parameter name" [current] [min] [max]

Client can set value of parameter with command:

"parameter name" [new_value]

If value of parameter is changed, all connected clients gets refreshed info
about parameter (in format like after first connect). Server can also set
error info, one of strings:
 
:E_MIN if new value is less then allowed minimum

:E_MAX if new value is grater then allowed maximum

:E_NOTALLOWED if client is not allowed to connect (based on client IP)

:E_OTHER for other unspecified server error

Error info string can be followed by optional message, that should be 
presented to user.

@devices TCP client, for example SZARP 'setter' program.

@config_example

<device daemon="/opt/szarp/bin/setdmn" xmlns:extra="http://www.praterm.com.pl/SZARP/ipk-extra"
	path="/dev/null" 
	extra:tcp-port="8010" 		<!-- TCP port to listen on -->
	extra:tcp-allowed="192.168.1.2"	<!-- list of allowed IP addresses, delimited with spaces -->
	>
	<unit id="1" type="1" subtype="1" bufsize="1">
		<param name="Some:Setable:Parameter" ... 
			extra:min="0"		<!-- minimum allowed value -->
			extra:max="27.8"	<!-- maximum allowed value -->
		> ... </param>
	</unit>
</device>

@description_end

*/

#include <sstream>
#include <map>

#include <arpa/inet.h>
#include <event.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "conversion.h"
#include "ipchandler.h"
#include "liblog.h"
#include "libpar.h"
#include "szbase/szbbase.h"
#include "tokens.h"
#include "xmlutils.h"

int g_debug = 0;		/**< global debug flag */
const int DEFAULT_PORT = 8010;	/**< default tcp port number */

/** Log message to stdout (in debug mode) or szarp log.
 * @param level liblog log level
 * @param fmt message to log
 */
void dolog(int level, const char * fmt, ...) {
	va_list fmt_args;

	if (g_debug) {
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

/** String to number conversion function.
 * @param str string representation of number
 * @return converted number
 * @throw std::invalid_argument if conversion fails
 */
template <typename T> T string2num(std::wstring str) throw (std::invalid_argument)
{
	std::wistringstream iss(str);
	T ret;
	if (not (iss >> ret)) {
		throw std::invalid_argument("string2num coversion error");
	}
	return ret;
}

/** 
 * Main daemon class.
 * Libevent-based telnet-like server that allows clients to set value of configured
 * parameters.
 */
class SetDaemon {
	public:
		class ValueToSmall: public std::exception { };
		class ValueToLarge: public std::exception { };
		class TcpServException : public std::exception { };
		/** Setable parameter info. */
		struct ParamInfo {
			ParamInfo(): min(0), max(0), current(0), param(NULL) { };
			double min;		/**< min. allowed value */
			double max;		/**< max. allowed value */
			double current;
			TParam* param;
			size_t index;
		};
		SetDaemon(DaemonConfig& cfg, IPCHandler& ipc);
		~SetDaemon();
		/** Initializes server listening on given port. Starts events
		 * loop. Libevent must be initialized prior to calling this method.
		 * @param port port number to listen on
		 */
		virtual bool Init();
		/** Called when new connection is going to be accepted.
		 * Calls AcceptConnection().
		 * @param fd listen socket file descriptor
		 * @param event type of event on listen socket, ignored
		 * @param ds 'this' pointer
		 */
		static void AcceptCallback(int fd, short event, void* ds);
		/** Called on read event on client connection. Calls Read().
		 * @param bufev libevent buffered event
		 * @param ds 'this' pointer 
		 */
		static void ReadCallback(struct bufferevent *bufev, void* ds);
		/** Called on error (or when client closes connections). Calls 
		 * CloseConnection().
		 * @param bufev libevent buffered event
		 * @param event type of event, ignored
		 * @param ds 'this' pointer
		 */
		static void ErrorCallback(struct bufferevent *bufev, short event, void* ds);
		/** Registers new parameter to set.
		 * @param name of parameter
		 * @param min minimum allowe value
		 * @param max maximum allowed value
		 * @param current current value of parameter
		 * @param pointer pointer to TParam object
		 * @param index index of param in ipc m_read array
		 */
		void AddParam(const std::wstring& name, double min, double max, double current, 
				TParam* pointer, size_t index);
	protected:
		int m_listen_fd;	/**< Server listening socket file descriptor. */
		int m_listen_port;	/**< Port to listen on. */
		std::vector<struct in_addr> m_allowed_ips;
					/**< List of allowed client IP addresses. */
		struct event m_listen_event;
					/**< Libevent event for listen descriptor. */
		std::map<struct bufferevent*, int> m_connections;
					/**< Pool of existing clients connections, map keys
					 * are pointers to libevent buffer objects, map values
					 * are file descriptors of sockets. */
		std::map<struct bufferevent*, std::vector<unsigned char> > m_buffers;
					/**< Pool of clients buffers. */
		std::map<std::wstring, ParamInfo> m_parameters;
		DaemonConfig& m_cfg;	/**< Configuration handler */
		IPCHandler& m_ipc;	/**< IPC object. */
		szb_buffer_t* m_szb;	/**< Pointer to szbase buffer */

		/** Searches for min and max attributes of parameter.
		 * @param xp_ctx XPath context to use
		 * @param name name of parameter
		 * @param min reference to min value
		 * @param max reference to max value
		 * @return 1 on error, 0 on success
		 */
		bool SearchMinMax(xmlXPathContextPtr xp_ctx, const std::wstring& name, double&min, double& max);
		/** Read previous values of params from database.
		 */
		virtual void ReadVals();
		/** Set new value of parameter.
		 * @param name parameter name
		 * @param val new value of parameter
		 * @throws exception when value is outside of allowed range
		 */
		void SetValue(const std::wstring& name, double val);
		/** Write value of parameter to client.
		 * @param buf client's bufferevent structure
		 * @param name name of parameter to write
		 */
		void WriteValue(struct bufferevent* buf, const std::wstring& name);
		/** Accepts connections, add to pool of existing connections.
		 * Called by AcceptCallback().
		 */
		virtual void AcceptConnection();
		/** Adds new connection to pool, sets libevent callbacks. Helper method 
		 * for AcceptConnection().
		 * @param fd file descriptor of new connection
		 * @return libevent buffer pointer for newly created connection
		 */
		virtual struct bufferevent* NewConnection(int fd);
		/** Actual reading function, overwrite it in child class. Called
		 * by ReadCallback().
		 * @param bufev libevent event buffer pointer, used to access connection
		 * data
		 */
		virtual void Read(struct bufferevent* bufev);
		/** Parse tcp-allowed attribute.
		 * @param xp_ctx initialized XPath context pointing to device node
		 * @return false on parsing error, true otherwise
		 */
		bool ParseIP(xmlXPathContextPtr xp_ctx);
		/** Method for performing access check, basing on client addres. You can
		 * overwrite it in child class - this implementation always return true, 
		 * thus allowing connections from any address.
		 * Method is called be AcceptConnection to see if connection should
		 * be accepted.
		 * @param addr address of connecting client
		 * @return true if client is allowed to connect, false otherwise
		 */
		virtual bool AddressAllowed(struct sockaddr_in* addr);
		/** Close connection, remove libevent buffer.
		 * @param bufev event buffer connected with connection we should close
		 */
		virtual void CloseConnection(struct bufferevent* bufev);
		/** Parses client request.
		 * @param bufev clients identification
		 */
		bool ParseRequest(struct bufferevent* bufev);
};

int set_fd_nonblock(int fd) {

        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
                return -1;
        }

        flags |= O_NONBLOCK;
        flags = fcntl(fd, F_SETFL, flags);
        if (flags == -1) {
                return -1;
        }
        return 0;
}

SetDaemon::SetDaemon(DaemonConfig& cfg, IPCHandler& ipc):
	m_listen_fd(-1), m_cfg(cfg), m_ipc(ipc), m_szb(NULL)
{ }

SetDaemon::~SetDaemon()
{
	for (std::map<struct bufferevent*, int>::iterator i = m_connections.begin(); 
			i != m_connections.end(); ++i) {
		CloseConnection((*i).first);
	}
	if (m_listen_fd != -1) {
        	event_del(&m_listen_event);
		close(m_listen_fd);
	}
	if (m_szb) {
		szb_free_buffer(m_szb);
		m_szb = NULL;
	}
}

bool SetDaemon::SearchMinMax(xmlXPathContextPtr xp_ctx, const std::wstring& name, double&min, double& max)
{
	char *expr;
	asprintf(&expr, ".//ipk:param[@name='%s']", SC::S2U(name).c_str());
	xmlNodePtr node = uxmlXPathGetNode(BAD_CAST expr, xp_ctx);
	assert(node);
	free(expr);
	xmlChar* c = xmlGetNsProp(node, BAD_CAST("min"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (c == NULL) {
		dolog(0, "missing min attribute in param element at line: %ld", xmlGetLineNo(node));
		return 1;
	}
	min = string2num<double>(SC::U2S(c));
	free(c);

	c = xmlGetNsProp(node, BAD_CAST("max"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (c == NULL) {
		dolog(0, "missing max attribute in param element at line: %ld", xmlGetLineNo(node));
		return 1;
	}
	max = string2num<double>(SC::U2S(c));
	free(c);
	return 0;
}

void SetDaemon::AddParam(const std::wstring& name, double min, double max, double current, TParam* pointer,
		size_t index)
{
	ParamInfo p;
	p.min = min;
	p.max = max;
	p.current = current;
	p.param = pointer;
	p.index = index;
	m_parameters[name] = p;
}

void SetDaemon::ReadVals()
{
	IPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"");
	Szbase::Init(SC::L2S(PREFIX), NULL);
	Szbase* szb = Szbase::GetObject();

	szb_buffer_t* m_szb = szb_create_buffer(szb, SC::L2S(libpar_getpar("", "datadir", 1)),
			m_ipc.m_params_count, m_cfg.GetIPK());
	assert(m_szb != NULL);
		
	std::map<std::wstring, ParamInfo>::iterator i;
	for (i = m_parameters.begin(); i != m_parameters.end(); ++i) {
		dolog(10, "Searching for data for parameter (%s)", SC::S2A(i->first).c_str());
		time_t t = szb_search_last(m_szb, i->second.param);
		if (t == -1) {
			dolog(10, "No data found for parameter");
			continue;
		}
		t = szb_search(m_szb, i->second.param, t, -1, -1);
		if (t == -1) {
			dolog(10, "No data found for parameter");
			continue;
		}
		SZBASE_TYPE data = szb_get_data(m_szb, i->second.param, t);
		dolog(10, "Got value %g", data);
		i->second.current = data;
	}
	for (i = m_parameters.begin(); i != m_parameters.end(); ++i) {
		SetValue(i->first, i->second.current);
	}
	m_ipc.GoParcook();
}

bool SetDaemon::ParseIP(xmlXPathContextPtr xp_ctx)
{
	xmlChar *c;
	char *e;
	asprintf(&e, "./@extra:tcp-allowed");
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c != NULL) {
		int tokc = 0;
		char **toks;
		tokenize(SC::U2A(c).c_str(), &toks, &tokc);
		xmlFree(c);
		for (int i = 0; i < tokc; i++) {
			struct in_addr allowed_ip;
			int ret = inet_aton(toks[i], &allowed_ip);
			if (ret == 0) {
				dolog(0, "incorrect IP address '%s'", toks[i]);
				return false;
			} else {
				dolog(5, "IP address '%s' allowed", toks[i]);
				m_allowed_ips.push_back(allowed_ip);
			}
		}
		tokenize(NULL, &toks, &tokc);
	}
	if (m_allowed_ips.size() == 0) {
		dolog(1, "warning: all IP allowed");
	}
	return true;
}

bool SetDaemon::Init()
{
	/* prepare xpath */
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(m_cfg.GetXMLDoc());
	assert (xp_ctx != NULL);
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert (ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "extra",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);

	xp_ctx->node = m_cfg.GetXMLDevice();
	if (not ParseIP(xp_ctx)) return false;

	xmlChar *c;
	char *e;
	asprintf(&e, "./@extra:tcp-port");
	assert (e != NULL);
	c = uxmlXPathGetProp(BAD_CAST e, xp_ctx);
	free(e);
	if (c == NULL) {
		dolog(1, "missing tcp-port attribute, exiting, using default value %d", DEFAULT_PORT);
		m_listen_port = DEFAULT_PORT;
	} else {
		m_listen_port = string2num<uint16_t>(SC::U2S(c));
		dolog(2, "using port number %d", m_listen_port);
		free(c);
	}

	TParam *p = m_cfg.GetDevice()->GetFirstRadio()->GetFirstUnit()->GetFirstParam();
	size_t num = 0;
	while (p != NULL) {
		std::wstring name = p->GetName();
		double min, max;
		if (SearchMinMax(xp_ctx, name, min, max)) return 1;
		AddParam(name, min, max, min, p, num++);
		p = p->GetNext();
	}
	
	ReadVals();

	/* standard TCP server stuff - socket, setsockopt, bind, listen, select */
	m_listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (m_listen_fd == -1) {
		TcpServException ex;
		dolog(0, "cannot create listen socket, errno %d (%s)",
				errno, strerror(errno));
		throw ex;
	}

	try {
		int op = 1;
		int ret = setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR,
				&op, sizeof(op));
		if (ret == -1) {
			TcpServException ex;
			dolog(0, "cannot set socket options on listen socket, errno %d (%s)",
					errno, strerror(errno));
			close(m_listen_fd);
			m_listen_fd = -1;
			throw ex;
		}
	
		struct sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(m_listen_port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		ret = bind(m_listen_fd, reinterpret_cast<struct sockaddr *>(&addr), 
				sizeof(addr));
		if (ret == -1) {
			TcpServException ex;
			dolog(0, "cannot bind to INADDR_ANY, port %d, errno %d (%s)",
					m_listen_port, errno, strerror(errno));
			throw ex;
		}
		ret = listen(m_listen_fd, 1);
		if (ret == -1) {
			TcpServException ex;
			dolog(0, "listen() failed, errno %d (%s)",
					errno, strerror(errno));
			throw ex;
		}
		if (set_fd_nonblock(m_listen_fd)) {
			TcpServException ex;
			dolog(0, "set_fd_nonblock() failed, errno %d (%s)",
					errno, strerror(errno));
			throw ex;
		}
	} catch (TcpServException ex) {
		close(m_listen_fd);
		m_listen_fd = -1;
		throw ex;
	}
	/* set 'accept' callback */
	event_set(&m_listen_event, m_listen_fd, EV_READ | EV_PERSIST, 
			AcceptCallback, this);
        event_add(&m_listen_event, NULL);
	return 0;
}

void SetDaemon::AcceptCallback(int fd, short event, void* ds)
{
	reinterpret_cast<SetDaemon*>(ds)->AcceptConnection();
}


void SetDaemon::ReadCallback(struct bufferevent *bufev, void* ds)
{
	reinterpret_cast<SetDaemon*>(ds)->Read(bufev);
}

void SetDaemon::ErrorCallback(struct bufferevent *bufev, short event, void* ds)
{
	reinterpret_cast<SetDaemon*>(ds)->CloseConnection(bufev);
}

void SetDaemon::AcceptConnection()
{
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(addr);
	int fd;

	while ((fd = accept(m_listen_fd, reinterpret_cast<sockaddr*>(&addr), 
					&addr_size) ) == -1) {
		switch (errno) {
			case EINTR: 
				continue;
			case EAGAIN:
			//case EWOULDBLOCK:
			case ECONNABORTED:
				return;
			default:
				TcpServException ex;
				dolog(1, "accept() failed, errno %d (%s)",
						errno, strerror(errno));
				throw ex;
		}
	}
	if (not AddressAllowed(&addr)) {
		dolog(1, "refused connection from %s", inet_ntoa(addr.sin_addr));
		close(fd);
		return;
	}
	NewConnection(fd);
}

void SetDaemon::WriteValue(struct bufferevent* buf, const std::wstring& name)
{
	ParamInfo& p = m_parameters.at(name);
	std::ostringstream s;
	s << std::string("\"") << SC::S2U(name).c_str() << "\" " << p.current 
			<< " " << p.min << " " << p.max << "\n";
	bufferevent_write(buf, const_cast<char*>(s.str().c_str()), strlen(s.str().c_str()));
}

struct bufferevent* SetDaemon::NewConnection(int fd)
{
	struct bufferevent* bufev =  bufferevent_new(fd,
			ReadCallback, NULL, ErrorCallback,
			this);
	m_connections[bufev] = fd;
	bufferevent_enable(bufev, EV_READ | EV_WRITE | EV_PERSIST);

	std::map<std::wstring, ParamInfo>::iterator i;
	for (i = m_parameters.begin(); i != m_parameters.end(); ++i) {
		WriteValue(bufev, i->first);
	}
	return bufev;
}

void write_string(struct bufferevent* bufev, const char* str)
{
	bufferevent_write(bufev, const_cast<char*>(str), strlen(str) + 1);
}

void SetDaemon::Read(struct bufferevent* bufev)
{
	std::vector<unsigned char>& buf = m_buffers[bufev];
	unsigned char c;
	while (bufferevent_read(bufev, &c, 1) == 1) {
		if (c == '\n') {
			try {
				if (not ParseRequest(bufev)) {
					write_string(bufev, ":E_OTHER\n");
					dolog(1, "request parsing failed");
				}
			} catch (ValueToSmall) {
				write_string(bufev, ":E_MIN\n");
			} catch (ValueToLarge) {
				write_string(bufev, ":E_MAX\n");
			} catch (std::exception) {
				write_string(bufev, ":E_OTHER\n");
			}
			buf.clear();
		} else {
			if (buf.size() > 1024) {
				dolog(3, "request to long, closing connection");
				CloseConnection(bufev);
				return;
			}
			buf.push_back(c);
		}
	}
}
		
bool SetDaemon::ParseRequest(struct bufferevent* bufev)
{
	std::vector<unsigned char>& buf = m_buffers[bufev];

	std::string sstr(reinterpret_cast<const char*>(&(buf[0])), buf.size());
	std::wstring str = SC::U2S(BAD_CAST sstr.c_str());

	if (str.size() <= 0) return false;
	if (str[0] != '"') return false;
	size_t pos = str.find('"', 1);
	if (pos == std::string::npos) return false;
	double val = string2num<double>(str.substr(pos + 2));
	std::wstring name = str.substr(1, pos - 1);
	dolog(10, "setting value of '%s' to %g", SC::S2A(name).c_str(), val);
	SetValue(name, val);
	m_ipc.GoParcook();
	return true;
}

void SetDaemon::SetValue(const std::wstring& name, double val)
{
	ParamInfo& p = m_parameters.at(name);
	if (val < p.min) throw ValueToSmall();
	if (val > p.max) throw ValueToLarge();
	m_ipc.m_read[p.index] = p.param->ToIPCValue(val);
	dolog(10, "Setting value to %d", m_ipc.m_read[p.index]);
	p.current = double(m_ipc.m_read[p.index]) / pow10(p.param->GetPrec());
	
	std::map<struct bufferevent*, int>::iterator i;
	for (i = m_connections.begin(); i != m_connections.end(); ++i) {
		WriteValue(i->first, name);
	}

}

bool SetDaemon::AddressAllowed(struct sockaddr_in* addr)
{
	if (m_allowed_ips.size() == 0) return true;
	
	for (size_t i = 0; i < m_allowed_ips.size(); i++) {
		if (m_allowed_ips[i].s_addr == addr->sin_addr.s_addr) return true;
	}
	return false;
}

void SetDaemon::CloseConnection(struct bufferevent* bufev)
{
	close(m_connections[bufev]);
	m_buffers.erase(bufev);
	m_connections.erase(bufev);
	bufferevent_free(bufev);
}

int main(int argc, char** argv)
{
	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);

	event_init();

	DaemonConfig cfg("setdmn");
	
	if (cfg.Load(&argc, argv, false))
		return 1;

	IPCHandler ipc(&cfg);
	if (not cfg.GetSingle()) {
		if (ipc.Init())
			return 1;
	}

	g_debug = cfg.GetDiagno() or cfg.GetSingle();

	SetDaemon daemon(cfg, ipc);
	if (daemon.Init()) return 1;
	libpar_done();
	cfg.CloseXML(1);
	dolog(2, "starting...");
	event_dispatch();
}
