/* 
 * SZARP: SCADA software 
 *
 * Copyright (C) 
 * 2011 - Jakub Kotur <qba@newterm.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */
#ifndef BASE_DAEMON_H
#define BASE_DAEMON_H
/** 
 * @file basedmn.h
 * @brief Base class for line daemons.
 * 
 * Contains basic functions that may be useful.
 *
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-16
 */

#include <ctime>

#include <szarp_config.h>

#include <dmncfg.h>
#include <ipchandler.h>

const int DEFAULT_EXPIRE = 660;	/* 11 minutes */
const int DAEMON_INTERVAL = 10;

#include "zmqhandler.h"
#include <boost/optional.hpp>
#include "utils.h"

template <typename V>
using bopt = boost::optional<V>;

struct IPCFacade {
	zmq::context_t m_zmq_ctx{1};
	std::unique_ptr<IPCHandler> sz3_ipc;
	std::unique_ptr<zmqhandler> sz4_ipc;

	IPCFacade(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg);

	void InitSz3(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg);
	void InitSz4(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg);

	void publish();
	void receive();

	template <typename VT>
	void setRead(size_t ind, VT value) {
		szlog::log() << szlog::debug << "Setting param no " << ind << " to " << value << szlog::endl;
		if (sz3_ipc) {
			sz3_ipc->m_read[ind] = value;
		}

		if (sz4_ipc) {
			// auto now = szarp::time_now<sz4::nanosecond_time_t>();
			auto now = time(NULL);
			sz4_ipc->set_value(ind, now, value);
		}
	}

	void setNoData(size_t ind) {
		szlog::log() << szlog::debug << "Setting param no " << ind << " to nodata" << szlog::endl;
		if (sz3_ipc) {
			sz3_ipc->m_read[ind] = SZARP_NO_DATA;
		}

		if (sz4_ipc) {
			auto now = time(NULL);
			sz4_ipc->set_no_data(ind, now);
		}
	}

	template <typename VT>
	VT getSend(size_t ind) {
		if (sz4_ipc) {
			auto value = sz4_ipc->get_send<VT>(ind, 1).value;
			szlog::log() << szlog::debug << "Got send at " << ind << " with " << value << szlog::endl;
			return value;
			// TODO: We have to add checking precision and data timeouts either here or in daemons
		}

		if (sz3_ipc) {
			auto value = sz3_ipc->m_send[ind];
			szlog::log() << szlog::debug << "Got send at " << ind << " with " << value << szlog::endl;
			return sz3_ipc->m_send[ind];
		}
	}
};

struct BaseDaemon {
	ArgsManager args_mgr;
	std::unique_ptr<DaemonConfigInfo> dmn_cfg;
	std::unique_ptr<IPCFacade> ipc;

	Scheduler m_scheduler;
	bopt<std::function<void(BaseDaemon&)>> daemon_cycle_callback = boost::none;

	BaseDaemon(ArgsManager&&, std::unique_ptr<DaemonConfigInfo>, std::unique_ptr<IPCFacade>);

	void initSignals();
	void setCycleHandler(std::function<void(BaseDaemon&)> cb);

	void new_cycle();
	void poll_forever();

	void publish();
	void receive();

	const DaemonConfigInfo& getDaemonCfg() const;

	template <typename VT>
	void setRead(size_t ind, VT value) {
		ipc->setRead(ind, value);
	}

	void setNoData(size_t ind) {
		ipc->setNoData(ind, value);
	}

	template <typename VT>
	VT getSend(size_t ind) {
		return ipc->getSend<VT>(ind);
	}
};

struct BaseDaemonFactory {
	static ArgsManager InitArgsManager(int argc, const char *argv[], const std::string& daemon_name);

	static std::unique_ptr<DaemonConfigInfo> InitDaemonConfig(ArgsManager& args_mgr);

	static std::unique_ptr<IPCFacade> InitIPC(ArgsManager& args_mgr, DaemonConfigInfo& daemon_config);

	static BaseDaemon Init(int argc, const char *argv[], const std::string& daemon_name);

	/* this can be overloaded to fit daemon's needs */
	template <typename DaemonType>
	static void Go(int argc, const char *argv[], const std::string& daemon_name) {
		auto base_dmn = BaseDaemonFactory::Init(argc, argv, daemon_name);
		DaemonType daemon(base_dmn);
		base_dmn.poll_forever();
	}
};

#endif /*BASE_DAEMON_H*/
