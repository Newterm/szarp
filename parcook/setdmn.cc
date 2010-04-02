/*
 SZARP daemon description block, see README file.

@description_start

@class 4

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
	extra:tcp-allowed="192.168.1.2"	<!-- list of allowed IP addresses -->
	>
	<unit id="1" type="1" subtype="1" buffer_size="1">
		<param name="Some:Setable:Parameter" ... 
			extra:min="0"		<!-- minimum allowed value -->
			extra:max="27.8"	<!-- maximum allowed value -->
		> ... </param>
	</unit>
</device>

@description_end

*/

#include <iostream>
#include <sstream>
#include "conversion.h"
#include "szbase/szbbase.h"
#include "libpar.h"

#include "ipchandler.h"
#include "liblog.h"
#include "xmlutils.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <string>



#include <map>
#include <event.h>
#include <netinet/in.h>

int g_debug = 0;	/**< global debug flag */

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
 * It acts as a libevent-based telnet-like server, that
 * This is a simple server that accepts connections from clients and sends 'OK'
 * on every received piece of data. You need to overwrite at least Read() method to 
 * do something usefull. 
 */
class SetDaemon {
	public:
		struct ParamInfo {
			ParamInfo(): min(0), max(0), current(0) { };
			double min;
			double max;
			double current;
		};
		class TcpServException : public std::exception { };
		SetDaemon(DaemonConfig& cfg, IPCHandler& ipc);
		~SetDaemon();
		/** Initializes server listening on given port. Starts events
		 * loop. Libevent must be initialized prior to calling this method.
		 * @param port port number to listen on
		 */
		virtual bool Init(int port);
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
		 */
		void AddParam(const std::wstring& name, double min, double max, double current);
	protected:
		int m_listen_fd;	/**< Server listening socket file descriptor. */
		int m_listen_port;	/**< Port to listen on. */
		struct event m_listen_event;
					/**< Libevent event for listen descriptor. */
		std::map<struct bufferevent*, int> m_connections;
					/**< Pool of existing clients connections, map keys
					 * are pointers to libevent buffer objects, map values
					 * are file descriptors of sockets. */
		std::map<struct bufferevent*, std::wstring> m_buffers;
					/**< Pool of clients buffers. */
		std::map<std::wstring, ParamInfo> m_parameters;
		DaemonConfig& m_cfg;
		IPCHandler& m_ipc;

		/** Searches for min and max attributes of parameter.
		 * @param xp_ctx XPath context to use
		 * @param name name of parameter
		 * @param min reference to min value
		 * @param max reference to max value
		 * @return 1 on error, 0 on success
		 */
		bool SearchMinMax(xmlXPathContextPtr xp_ctx, const std::wstring& name, double&min, double& max);
		/** Reads last value of parameter from szbase.
		 * @param name name of parameter
		 */
		double GetLastValue(const std::wstring& name);
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
	m_listen_fd(-1), m_cfg(cfg), m_ipc(ipc)
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

double SetDaemon::GetLastValue(const std::wstring& name)
{
	bool ok;
	return 0;
}

void SetDaemon::AddParam(const std::wstring& name, double min, double max, double current)
{
	ParamInfo p;
	p.min = min;
	p.max = max;
	p.current = current;
	m_parameters[name] = p;
}

bool SetDaemon::Init(int port)
{
	IPKContainer::Init(SC::A2S(PREFIX), SC::A2S(PREFIX), L"", new NullMutex());
	Szbase::Init(SC::A2S(PREFIX), NULL);
	Szbase* szb = Szbase::GetObject();

	szb_buffer_t* m_szb = szb_create_buffer(szb, SC::A2S(libpar_getpar(NULL, "datadir", 1)), 
			m_ipc.m_params_count, m_cfg.GetIPK());

	/* prepare xpath */
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(m_cfg.GetXMLDoc());
	assert (xp_ctx != NULL);
	int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	assert (ret == 0);
	ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "modbus",
			BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	assert (ret == 0);

	xp_ctx->node = m_cfg.GetXMLDevice();
	TParam *p = m_cfg.GetDevice()->GetFirstRadio()->GetFirstUnit()->GetFirstParam();
	while (p != NULL) {
		std::wstring name = p->GetName();
		double min, max;
		if (SearchMinMax(xp_ctx, name, min, max)) return 1;
		AddParam(name, min, max, GetLastValue(name));
	}

	/* standard TCP server stuff - socket, setsockopt, bind, listen, select */
	m_listen_port = port;
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

struct bufferevent* SetDaemon::NewConnection(int fd)
{
	struct bufferevent* bufev =  bufferevent_new(fd,
			ReadCallback, NULL, ErrorCallback,
			this);
	m_connections[bufev] = fd;
	bufferevent_enable(bufev, EV_READ | EV_WRITE | EV_PERSIST);
	return bufev;
}

void SetDaemon::Read(struct bufferevent* bufev)
{
	std::wstring& buf = m_buffers[bufev];
	unsigned char c[2];
	c[1] = 0;
	while (bufferevent_read(bufev, c, 1) == 1) {
		if (c[0] == '\n') {
			std::cout << "READ DONE!\n";
			if (not ParseRequest(bufev)) {
				dolog(1, "request parsing failed: %s", SC::S2A(buf).c_str());
			}
		} else {
			if (buf.size() > 1024) {
				CloseConnection(bufev);
				return;
			}
			buf += SC::U2S(c);
		}
	}

}
		
bool SetDaemon::ParseRequest(struct bufferevent* bufev)
{
	std::wstring& buf = m_buffers[bufev];
	if (buf.size() <= 0) return false;
	if (buf[0] != '"') return false;
	size_t pos = buf.find('"', 1);
	if (pos == std::string::npos) return false;
	try {
		double val = string2num<double>(buf.substr(pos + 2));
		m_parameters.at(buf.substr(1, pos - 2)).current = val;
		//TODO - set value
	} catch (std::exception) {
		return false;
	}
	return true;
}

bool SetDaemon::AddressAllowed(struct sockaddr_in* addr)
{
	return true;
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

	g_debug = cfg.GetDiagno() || cfg.GetSingle();

	SetDaemon daemon(cfg, ipc);
	if (daemon.Init(8010)) return 1;
	libpar_done();
	cfg.CloseXML(1);
	event_dispatch();
}
