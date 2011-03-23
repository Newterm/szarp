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
 * 
 * Darek Marcinkiewicz <reksio@newterm.pl>
 * 
 */

#ifndef __BORUTADMN_H_
#define __BORUTADMN_H_

/* Boruta is a meta-daemon, a generic daamon that does not implement any protocol.
 * Instead it relies on its modules - drivers to implement particular protocols.
 * Boruta is able to run any number of drivers within one process. Boruta's
 * drivers can be either clients or servers that can communicate over one of two
 * mediums: serial line or tcp protocol. 
 * Boruta itself performs following tasks:
 *  1. Manages connections 
 *  2. Schedules client drivers.
 *
 * Connections management boils down to estalibshing tcp connections (in case of
 * tcp client drivers), accepting new connections (for tcp server drivers), 
 * opening/configuring serial port (for serial connections).
 *
 * Scheduling is performed for client drivers that use the same connection (the
 * same serial line or tcp address-port pair). Client drivers are scheduled in
 * turn and each deliver is supposed to notify boruta when it's done with 
 * enquiring its peer, so that boruta can pass connection to next driver - this is 
 * kind of cooperative multitasking with respect to connections utilization.
 *
 * All drivers are supposed to perform no blocking calls, I/O
 * and timeout handling have to be perfomed through libevent API.
 *
 */

/**self-descriptive struct holding all aspects of serial port conifguration in one place*/
struct serial_port_configuration {
	std::string path;
	enum PARITY {
		NONE,
		ODD,
		EVEN
	} parity;
	int stop_bits;
	int speed;
};

/**common class for types of drivers, actually the only thing they share is a fact
that they hold a pointer to event_base*/
class boruta_driver {
protected:
	struct event_base* m_event_base;
public:
	void set_event_base(struct event_base* ev_base);
};

/**base class for server drivers, server drivers have a single numeric identifier,
and respond to connection_error 'event'*/
class server_driver : public boruta_driver {
	size_t m_id;
public:
	virtual void connection_error(struct bufferevent *bufev) = 0;
	size_t& id();
};

class client_manager;

/** base class for client driver */
class client_driver : public boruta_driver {
	/** client driver identificator consiting of two numbers - id of connection
	 * this driver is assigned to and id of driver itself */
	std::pair<size_t, size_t> m_id;
protected:
	client_manager* m_manager;
public:
	std::pair<size_t, size_t> & id();
	void set_manager(client_manager* manager);
	/** through this method manager informs a driver that there was
	 * problem with connection, if a connection could not be established,
	 * bufev will be set to NULL, otherwise it will point to a buffer
	 * of a connection that caused problem*/
	virtual void connection_error(struct bufferevent *bufev) = 0;
	/** this method is called when it is this drivers turn to use
	 * a connection, after compilting his job driver should release
	 * connection by calling @see driver_finished_job on its manager
	 * @param bufev connection's buffer event
	 * @param fd connection's descriptior */
	virtual void scheduled(struct bufferevent* bufev, int fd) = 0;
	/** called by manager for currently scheduled clients when there
	 * are incoming data for their connetions*/
	virtual void data_ready(struct bufferevent* bufev, int fd) = 0;
	/** called for driver when cycle is finshed, just after this
	 * method is called data will be passed to parcook, so driver
	 * should put any data it has received into it's portion of read buffer*/
	virtual void finished_cycle();
	/** the new cycle has just been started - data from sender has
	 * been updated*/
	virtual void starting_new_cycle();
};

/**client driver operating over tcp connection*/
class tcp_client_driver : public client_driver {
public:
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send) = 0;
};

/**client driver operating over serial line*/
class serial_client_driver : public client_driver {
public:
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration& spc) = 0;
};

/**client driver operating over tcp connection that pretends */
class tcp_proxy_2_serial_client : public tcp_client_driver {
	serial_client_driver* m_serial_client;
public:
	tcp_proxy_2_serial_client(serial_client_driver* _client_driver);

	virtual void connection_error(struct bufferevent *bufev);

	virtual void scheduled(struct bufferevent* bufev, int fd);

	virtual void data_ready(struct bufferevent* bufev, int fd);

	virtual void finished_cycle();

	virtual void starting_new_cycle();

	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send);

	~tcp_proxy_2_serial_client();
};

class serial_server_manager;
/**server driver operating over serial line*/
class serial_server_driver : public server_driver {
protected:
	serial_server_manager* m_manager;
public:
	void set_manager(serial_server_manager* manager);
	virtual void connection_error(struct bufferevent *bufev) = 0;
	virtual void data_ready(struct bufferevent* bufev) = 0;
	virtual void starting_new_cycle() = 0;
	virtual void finished_cycle() = 0;
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration&) = 0;
};

class tcp_server_manager;
/**server driver operating over tcp connection*/
class tcp_server_driver : public server_driver {
protected:
	tcp_server_manager* m_manager;
public:
	void set_manager(tcp_server_manager* manager);
	/** signfies connection error
	 * @param bufev a connection which caused error*/
	virtual void connection_error(struct bufferevent *bufev) = 0;
	/** notifies driver that data arrived through connection
	 * @parma bufev connection that received data*/
	virtual void data_ready(struct bufferevent* bufev) = 0;
	/** notifies driver that new connection was accepted
	 * @param bufev bufferevent associated with connection
	 * @param socket connection socket
	 * @param addr address of a peer establishing connection
	 * @return 0 if connection should be accepted, non zero status if daemon should terminate 
	 * (refuse) connection*/
	virtual int connection_accepted(struct bufferevent* bufev, int socket, struct sockaddr_in* addr) = 0;
	/** notifies driver that new cycle was started*/
	virtual void starting_new_cycle() = 0;
	/** notifies driver that we are finishing cycle*/
	virtual void finished_cycle() = 0;

	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send) = 0;
};

/**container for protocols factories*/
class protocols {
	typedef std::map<std::string, tcp_client_driver* (*)()> tcp_client_factories_table;
	/**factory of tcp client drivers*/
	tcp_client_factories_table m_tcp_client_factories;

	typedef std::map<std::string, serial_client_driver* (*)()> serial_client_factories_table;
	/**factory of serial client drivers*/
	serial_client_factories_table m_serial_client_factories;

	/**factory of tcp server drivers*/
	typedef std::map<std::string, tcp_server_driver* (*)()> tcp_server_factories_table;
	tcp_server_factories_table m_tcp_server_factories;

	/**factory of serial server drivers*/
	typedef std::map<std::string, serial_server_driver* (*)()> serial_server_factories_table;
	serial_server_factories_table m_serial_server_factories;

	/**retrives protocol name from unit node*/
	std::string get_proto_name(xmlNodePtr node);
public:
	protocols();
	/**following four methods create appropriate driver as specified in the node*/
	tcp_client_driver* create_tcp_client_driver(xmlNodePtr node);
	serial_client_driver* create_serial_client_driver(xmlNodePtr node);
	tcp_server_driver* create_tcp_server_driver(xmlNodePtr node);
	serial_server_driver* create_serial_server_driver(xmlNodePtr node);
};

class serial_connection;

/**boruta interface for clasess dealing with @see serial_connection*/
class serial_connection_manager { 
public:
	/** connection read ready callback*/
	virtual void connection_read_cb(serial_connection* connection) = 0;
	/** connection error callback*/
	virtual void connection_error_cb(serial_connection* connection) = 0;
};

struct serial_connection {
	serial_connection(size_t conn_no, serial_connection_manager* _manager);
	size_t conn_no;
	serial_connection_manager *manager;
	int fd;
	struct bufferevent *bufev;
public:
	int open_connection(const std::string& path, struct event_base* ev_base);
	void close_connection();
	static void connection_read_cb(struct bufferevent *ev, void* _connection);
	static void connection_error_cb(struct bufferevent *ev, short event, void* _connection);
};

class boruta_daemon;

enum CONNECTION_STATE { CONNECTED, NOT_CONNECTED, CONNECTING };

/** a client manager class performing client drivers scheduling, it's
 * an abstract class requiring from its subclasses to implement 
 * methods resposinble for connections handling*/
class client_manager { 
protected:
	/**maps client drivers to connections*/
	std::vector<std::vector<client_driver*> > m_connection_client_map;
	/**holds ids of current clients for each connection*/
	std::vector<size_t> m_current_client;
	/**to be implemented by subclass - returns connecton state*/
	virtual CONNECTION_STATE do_get_connection_state(size_t conn_no) = 0;
	/**to be implemented by subclass - return buffer of connection*/
	virtual struct bufferevent* do_get_connection_buf(size_t conn_no) = 0;
	/**to be implemented by subclass - shall terminate connection*/
	virtual void do_terminate_connection(size_t conn_no) = 0;
	/**to be implemented by subclass - shall schedule client on a connection*/
	virtual void do_schedule(size_t conn_no, size_t client_no) = 0;
	/**to be implemented by subclass - shall establish connection and return zero value
	 * if there was no problem with connection establishment*/
	virtual int do_establish_connection(size_t conn_no) = 0;

	/** method handling arrival data on a connection, it simply routes this information
	 * to current driver for this connection*/
	void connection_read_cb(size_t connection, struct bufferevent *bufev, int fd);
	/** method handling connection error*/
	void connection_error_cb(size_t connection);
	/** method handling connection establishment event*/
	void connection_established_cb(size_t connection);
public:
	/**propagates this event to client drivers*/
	void finished_cycle();
	/**propagates this event to client drivers also schedules drivers for those
	 * connection which are currently 'idle'*/
	void starting_new_cycle();
	/** should be called by a driver when it's done with querying his
	 * peer, will cause scheduling of next driver utlizing the same connection*/
	void driver_finished_job(client_driver *driver);
	/** when driver encounters an error in communication with its peer - like
	 * timeout in the middle of communication or something that could possibly
	 * interfere with a next scheduled driver, it should call this method. It
	 * will cause a connection to be reponed before next driver is scheduled
	 * (and has the same effect as calling @driver_finished_job*/
	void terminate_connection(client_driver *driver);
};

/**implementation of class deadling with tcp client drivers*/
class tcp_client_manager : public client_manager {
	boruta_daemon *m_boruta;
	struct tcp_connection {
		tcp_connection(tcp_client_manager *manager, size_t conn_no, std::string address);
		void close();
		CONNECTION_STATE state;	
		int fd;
		struct bufferevent *bufev;
		size_t conn_no;
		tcp_client_manager *manager;
		std::string address;
	};
	/**adresses for connections*/
	std::vector<sockaddr_in> m_addresses;
	/**tcp connections array*/
	std::vector<tcp_connection> m_tcp_connections;
	/**retrieves tcp connection setting from xln node*/
	int configure_tcp_address(xmlNodePtr node, struct sockaddr_in &addr);
	/**closes given connection*/
	void close_connection(tcp_connection &c);
	/**closes given connection, returns 0 in case of succes, 1 otherwise*/
	int open_connection(tcp_connection &c, struct sockaddr_in& addr);
protected:
	virtual CONNECTION_STATE do_get_connection_state(size_t conn_no);
	virtual struct bufferevent* do_get_connection_buf(size_t conn_no);
	virtual void do_terminate_connection(size_t conn_no);
	virtual void do_schedule(size_t conn_no, size_t client_no);
	virtual int do_establish_connection(size_t conn_no);
public:
	tcp_client_manager(boruta_daemon *boruta) : m_boruta(boruta) {}
	int configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols &_protocols);
	int initialize();
	static void connection_read_cb(struct bufferevent *ev, void* _tcp_connection);
	static void connection_write_cb(struct bufferevent *ev, void* _tcp_connection);
	static void connection_error_cb(struct bufferevent *ev, short event, void* _tcp_connection);
};

/**implementation of class deadling with serial client drivers*/
class serial_client_manager : public serial_connection_manager, public client_manager {
	boruta_daemon *m_boruta;
	/**confiugrations of serial port for each driver*/
	std::vector<std::vector<serial_port_configuration> > m_configurations;
	/**maps serial port paths to a connection number*/
	std::map<std::string, size_t> m_ports_client_no_map;
	/**list of @see serial_connection managed by this class*/
	std::vector<serial_connection> m_serial_connections;
protected:
	virtual CONNECTION_STATE do_get_connection_state(size_t conn_no);
	virtual struct bufferevent* do_get_connection_buf(size_t conn_no);
	virtual void do_terminate_connection(size_t conn_no);
	virtual void do_schedule(size_t conn_no, size_t client_no);
	virtual int do_establish_connection(size_t conn_no);
public:
	serial_client_manager(boruta_daemon *boruta) : m_boruta(boruta) {}
	int configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols &_protocols);
	int initialize();
	void connection_read_cb(serial_connection *c);
	void connection_error_cb(serial_connection *c);
};

class serial_server_manager : public serial_connection_manager {
	boruta_daemon *m_boruta;
	/**drivers for particular paths*/
	std::vector<serial_server_driver*> m_drivers;
	/**connections for each path*/
	std::vector<serial_connection> m_connections;
	/**connections settings associated with each path*/
	std::vector<serial_port_configuration> m_configurations;
public:
	serial_server_manager(boruta_daemon *boruta) : m_boruta(boruta) {}
	/**configures unit*/
	int configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols &_protocols);
	/**does nothing at this moment*/
	int initialize();

	void finished_cycle();

	void starting_new_cycle();
	/**reopen connections at driver requests*/
	void restart_connection_of_driver(serial_server_driver* driver);
	void connection_read_cb(serial_connection *c);
	void connection_error_cb(serial_connection *c);
};

class tcp_server_manager {
	boruta_daemon *m_boruta;
	/**description of port at which boruta listens for connections*/
	struct listen_port {
		listen_port(tcp_server_manager *manager, int port, int serv_no);
		tcp_server_manager *manager;
		/**port number*/
		int port;
		/**socket descriptor*/
		int fd;
		/**id of server associated with this port*/
		int serv_no;
		/**event used by libevent for notifiactions of new connections coming
		 * to this port*/
		struct event _event;
	};
	struct connection {
		/**id of server associated with this connection*/
		size_t serv_no;
		/**socket descriptor*/
		int fd;
		/**connection buffer for this connection*/
		struct bufferevent *bufev;
	};
	/**array of drivers*/
	std::vector<tcp_server_driver*> m_drivers;
	/**ports we listen to*/
	std::vector<listen_port> m_listen_ports;
	/**maps connection buffers to connections associated with them*/
	std::map<struct bufferevent*, connection> m_connections;
	int start_listening_on_port(int port);
	void close_connection(struct bufferevent* bufev);
public:
	tcp_server_manager(boruta_daemon *boruta) : m_boruta(boruta) {}
	int configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols &_protocols);
	int initialize();
	void finished_cycle();
	void starting_new_cycle();
	static void connection_read_cb(struct bufferevent *ev, void* _tcp_connection);
	static void connection_error_cb(struct bufferevent *ev, short event, void* _tcp_connection);
	static void connection_accepted_cb(int fd, short event, void* listen_port);
};

class boruta_daemon {
	tcp_client_manager m_tcp_client_mgr;
	tcp_server_manager m_tcp_server_mgr;
	serial_client_manager m_serial_client_mgr;
	serial_server_manager m_serial_server_mgr;

	DaemonConfig* m_cfg;
	IPCHandler* m_ipc;

	struct event_base* m_event_base;
	struct event m_timer;

	int configure_ipc();
	int configure_events();
	int configure_units();
public:
	boruta_daemon();
	struct event_base* get_event_base();	
	int configure(DaemonConfig *cfg);
	void go();
	static void cycle_timer_callback(int fd, short event, void* daemon);
};

serial_client_driver* create_zet_serial_client();
tcp_client_driver* create_zet_tcp_client();

serial_client_driver* create_modbus_serial_client();
tcp_client_driver* create_modbus_tcp_client();

serial_server_driver* create_modbus_serial_server();
tcp_server_driver* create_modbus_tcp_server();

serial_client_driver* create_fp210_serial_client();

void dolog(int level, const char * fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

int get_serial_port_config(xmlNodePtr node, serial_port_configuration &spc);

int set_serial_port_settings(int fd, serial_port_configuration &sc);


template<class T> int get_xml_extra_prop(xmlNodePtr node, const char* pname, T& value, bool optional = false) {
	xmlChar* prop = xmlGetNsProp(node, BAD_CAST pname, BAD_CAST IPKEXTRA_NAMESPACE_STRING);
	if (prop == NULL) {
		if (!optional) {
			dolog(0, "No attribute %s given in line %ld", pname, xmlGetLineNo(node));
			return 1;
		} else {
			return 0;
		}
	}
	std::stringstream ss((char*)prop);
	bool ok = ss >> value;
	if (!ok) {
		if (ss.eof())
			ok = true;
		else
			dolog(0, "Invalid value %s for attribute %s in line, %ld", (char*)prop, pname, xmlGetLineNo(node));
	}
	xmlFree(prop);	
	return ok ? 0 : 1;
}


#endif
