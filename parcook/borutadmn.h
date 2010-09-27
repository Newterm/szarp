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

void dolog(int level, const char * fmt, ...)
  __attribute__ ((format (printf, 2, 3)));

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

int get_serial_port_config(xmlNodePtr node, serial_port_configuration &spc);

int set_serial_port_settings(int fd, serial_port_configuration &sc);

class boruta_driver {
protected:
	struct event_base* m_event_base;
public:
	void set_event_base(struct event_base* ev_base);
};

class server_driver : public boruta_driver {
	size_t m_id;
public:
	virtual void connection_error(struct bufferevent *bufev) = 0;
	size_t& id();
};

class tcp_client_manager;
class serial_client_manager;
class tcp_server_manager;
class serial_server_manager;
class client_manager;

class client_driver : public boruta_driver {
	std::pair<size_t, size_t> m_id;
protected:
	client_manager* m_manager;
public:
	std::pair<size_t, size_t> & id();
	void set_manager(client_manager* manager);
	virtual void connection_error(struct bufferevent *bufev) = 0;
	virtual void scheduled(struct bufferevent* bufev, int fd) = 0;
	virtual void data_ready(struct bufferevent* bufev, int fd) = 0;
	virtual void starting_new_cycle();
};

class tcp_client_driver : public client_driver {
public:
	virtual void connection_error(struct bufferevent *bufev) = 0;
	virtual void scheduled(struct bufferevent* bufev, int fd) = 0;
	virtual void data_ready(struct bufferevent* bufev, int fd) = 0;
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send) = 0;
};

class serial_client_driver : public client_driver {
public:
	virtual void connection_error(struct bufferevent *bufev) = 0;
	virtual void scheduled(struct bufferevent* bufev, int fd) = 0;
	virtual void data_ready(struct bufferevent* bufev, int fd) = 0;
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration& spc) = 0;
};

class serial_server_driver : public server_driver {
protected:
	client_manager* m_manager;
public:
	void set_manager(serial_server_manager* manager);
	virtual void connection_error(struct bufferevent *bufev) = 0;
	virtual void data_ready(struct bufferevent* bufev) = 0;
	virtual void starting_new_cycle() = 0;
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send, serial_port_configuration&) = 0;
};

class tcp_server_driver : public server_driver {
protected:
	tcp_server_manager* m_manager;
public:
	void set_manager(tcp_server_manager* manager);
	virtual void connection_error(struct bufferevent *bufev) = 0;
	virtual void data_ready(struct bufferevent* bufev) = 0;
	virtual int connection_accepted(struct bufferevent* bufev, int socket, struct sockaddr_in* addr) = 0;
	virtual void starting_new_cycle() = 0;
	virtual int configure(TUnit* unit, xmlNodePtr node, short* read, short *send) = 0;
};

class protocols {
	typedef std::map<std::string, tcp_client_driver* (*)()> tcp_client_factories_table;
	tcp_client_factories_table m_tcp_client_factories;

	typedef std::map<std::string, serial_client_driver* (*)()> serial_client_factories_table;
	serial_client_factories_table m_serial_client_factories;

	typedef std::map<std::string, tcp_server_driver* (*)()> tcp_server_factories_table;
	tcp_server_factories_table m_tcp_server_factories;

	typedef std::map<std::string, serial_server_driver* (*)()> serial_server_factories_table;
	serial_server_factories_table m_serial_server_factories;

	std::string get_proto_name(xmlNodePtr node);
public:
	protocols();
	tcp_client_driver* create_tcp_client_driver(xmlNodePtr node);
	serial_client_driver* create_serial_client_driver(xmlNodePtr node);
	tcp_server_driver* create_tcp_server_driver(xmlNodePtr node);
	serial_server_driver* create_serial_server_driver(xmlNodePtr node);
};

class serial_connection;

class serial_connection_manager { 
public:
	virtual void connection_read_cb(serial_connection* connection) = 0;
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

class client_manager { 
protected:
	std::vector<std::vector<client_driver*> > m_connection_client_map;
	std::vector<size_t> m_current_client;
	std::vector<struct bufferevent*> m_connection_buffers;
	virtual CONNECTION_STATE do_get_connection_state(size_t conn_no) = 0;
	virtual struct bufferevent* do_get_connection_buf(size_t conn_no) = 0;
	virtual void do_terminate_connection(size_t conn_no) = 0;
	virtual void do_schedule(size_t conn_no, size_t client_no) = 0;
	virtual int do_establish_connection(size_t conn_no) = 0;

	void connection_read_cb(size_t connection, struct bufferevent *bufev, int fd);
	void connection_error_cb(size_t connection);
	void connection_established_cb(size_t connection);
public:
	void starting_new_cycle();
	void driver_finished_job(client_driver *driver);
	void terminate_connection(client_driver *driver);
};

class tcp_client_manager : public client_manager {
	boruta_daemon *m_boruta;
	std::vector<sockaddr_in> m_addresses;
	struct tcp_connection {
		tcp_connection(tcp_client_manager *manager, size_t conn_no);
		void close();
		CONNECTION_STATE state;	
		int fd;
		struct bufferevent *bufev;
		size_t conn_no;
		tcp_client_manager *manager;
	};
	std::vector<tcp_connection> m_tcp_connections;
	int configure_tcp_address(xmlNodePtr node, struct sockaddr_in &addr);
	void close_connection(tcp_connection &c);
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

class serial_client_manager : public serial_connection_manager, public client_manager {
	boruta_daemon *m_boruta;
	std::vector<std::vector<serial_port_configuration> > m_configurations;
	std::map<std::string, size_t> m_ports_client_no_map;
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
	std::vector<serial_server_driver*> m_drivers;
	std::vector<serial_connection> m_connections;
	std::vector<serial_port_configuration> m_configurations;
public:
	serial_server_manager(boruta_daemon *boruta) : m_boruta(boruta) {}
	int configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols &_protocols);
	int initialize();
	void starting_new_cycle();
	void restart_connection_of_driver(serial_server_driver* driver);
	void connection_read_cb(serial_connection *c);
	void connection_error_cb(serial_connection *c);
};

class tcp_server_manager {
	boruta_daemon *m_boruta;
	struct listen_port {
		listen_port(tcp_server_manager *manager, int port, int serv_no);
		tcp_server_manager *manager;
		int port;
		int fd;
		int serv_no;
		struct event _event;
	};
	struct connection {
		size_t serv_no;
		int fd;
		struct bufferevent *bufev;
	};
	std::vector<tcp_server_driver*> m_drivers;
	std::vector<listen_port> m_listen_ports;
	std::map<struct bufferevent*, connection> m_connections;
	int start_listening_on_port(int port);
	void close_connection(struct bufferevent* bufev);
public:
	tcp_server_manager(boruta_daemon *boruta) : m_boruta(boruta) {}
	int configure(TUnit *unit, xmlNodePtr node, short* read, short* send, protocols &_protocols);
	int initialize();
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

#endif
