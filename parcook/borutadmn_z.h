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

#include <netinet/in.h>
#include <evdns.h>
#include "zmqhandler.h"
#include "dmncfg.h"
#include "custom_assert.h"
#include "baseconn.h"

#include "sz4/defs.h"
#include "sz4/util.h"
#include "utils.h"

template <typename reg_value_type>
class register_iface_t {
public:
	virtual bool is_valid() const = 0;

	virtual reg_value_type get_val() const = 0;
	virtual void set_val(reg_value_type val) = 0;

	virtual sz4::nanosecond_time_t get_mod_time() const = 0;
	virtual void set_mod_time(const sz4::nanosecond_time_t&) = 0;
};

class parcook_val_op {
public:
	virtual void publish_val(zmqhandler& handler, size_t index) = 0;
};

class sender_val_op {
public:
	virtual void update_val(zmqhandler& handler, size_t index) = 0;
};

class driver_iface {
public:
	virtual ~driver_iface() {}
	virtual int configure(UnitInfo* unit, size_t read, size_t send, const SerialPortConfiguration&) = 0;
};

class bc_driver: public ConnectionListener, public driver_iface {
protected:
	non_owning_ptr<BaseConnection> m_connection = nullptr;

public:
	bc_driver(BaseConnection* conn): m_connection(conn) {
		conn->AddListener(this);
	}
};

struct boruta_logger {
	std::string header;
	boruta_logger(const std::string& _header): header(_header) {}

	void log(int level, const char * fmt, ...) __attribute__ ((format (printf, 3, 4)));
	void vlog(int level, const char * fmt, va_list fmt_args);
};

using slog = std::shared_ptr<boruta_logger>;

class boruta_daemon {
	std::set<std::unique_ptr<driver_iface>> conns;

	zmq::context_t m_zmq_ctx;

	non_owning_ptr<zmqhandler> m_zmq;
	struct timeval m_cycle;

	Scheduler m_timer;

	int configure_ipc(const ArgsManager&, DaemonConfigInfo&);
	int configure_units(const ArgsManager&, DaemonConfigInfo&);
	void configure_timer(const ArgsManager&, DaemonConfigInfo&);
public:
	boruta_daemon();

	non_owning_ptr<zmqhandler> get_zmq();
	int configure(int *argc, char *argv[]);
	void go();
	void cycle_timer_callback();
};

driver_iface* create_bc_modbus_tcp_client(BaseConnection* conn, boruta_daemon*, slog);
driver_iface* create_bc_modbus_driver(BaseConnection* conn, boruta_daemon* boruta, slog log);
driver_iface* create_fc_serial_client(BaseConnection* conn, boruta_daemon*, slog);

void dolog(int level, const char * fmt, ...)
	__attribute__ ((format (printf, 2, 3)));

namespace ascii {
	int from_ascii(unsigned char c1, unsigned char c2, unsigned char &c) ;
	void to_ascii(unsigned char c, unsigned char& c1, unsigned char &c2) ;
}

#endif
