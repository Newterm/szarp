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

class IPCFacade {
public:
	IPCFacade(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg);

	void InitSz3(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg);
	void InitSz4(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg);

	void publish();
	void receive();

	template <typename VT>
	void setRead(size_t ind, VT value);

	void setNoData(size_t ind);

	template <typename VT>
	VT getSend(size_t ind);

private:
	zmq::context_t m_zmq_ctx{1};
	std::unique_ptr<IPCHandler> sz3_ipc;
	std::unique_ptr<zmqhandler> sz4_ipc;

	size_t read_count;
	size_t send_count;

	std::vector<size_t> send_prec_adjs;
	void parse_precs(const DaemonConfigInfo& dmn);

};

class BaseDaemon {
public:
	BaseDaemon(ArgsManager&&, std::unique_ptr<DaemonConfigInfo>, std::unique_ptr<IPCFacade>);

	void setCycleHandler(std::function<void(BaseDaemon&)> cb);
	void poll_forever();

	const DaemonConfigInfo& getDaemonCfg() const { return *dmn_cfg; }
	const ArgsManager& getArgsMgr() const { return args_mgr; }
	IPCFacade& getIpc() const {return *ipc; }

private:
	ArgsManager args_mgr;
	std::unique_ptr<DaemonConfigInfo> dmn_cfg;
	std::unique_ptr<IPCFacade> ipc;

	Scheduler m_scheduler;
	bopt<std::function<void(BaseDaemon&)>> daemon_cycle_callback = boost::none;

	void initSignals();

	void new_cycle();

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
