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
/** 
 * @file basedmn.cc
 * @brief Implementatio of BaseDaemon class
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-16
 */

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <liblog.h>
#include <sys/prctl.h>

#include "cfgdealer_handler.h"

#include "base_daemon.h"
#include "custom_assert.h"

void IPCFacade::InitSz3(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg) {
	sz3_ipc = std::make_unique<IPCHandler>(&dmn_cfg);
}

void IPCFacade::InitSz4(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg) {
	bopt<char*> sub_conn_address = args_mgr.get<char*>("parhub", "sub_conn_addr");
	bopt<char*> pub_conn_address = args_mgr.get<char*>("parhub", "pub_conn_addr");

	if (!sub_conn_address || !pub_conn_address) {
		throw std::runtime_error("Could not get parhub's address from configuration, exiting!");
	}

	try {
		sz4_ipc = std::make_unique<zmqhandler>(&dmn_cfg, m_zmq_ctx, *pub_conn_address, *sub_conn_address);
	} catch (zmq::error_t& e) {
		free(*sub_conn_address);
		free(*pub_conn_address);
		throw std::runtime_error(std::string("ZMQ initialization failed: ") + std::string(e.what()));
	}

	free(*sub_conn_address);
	free(*pub_conn_address);
}

IPCFacade::IPCFacade(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg) {
	if (args_mgr.has("single"))
		return;

	send_count = dmn_cfg.GetSendsCount();
	read_count = dmn_cfg.GetParamsCount();

	auto ipc_type = args_mgr.get<std::string>("ipc_type").get_value_or("sz3");

	if (ipc_type == "sz3") {
		InitSz3(args_mgr, dmn_cfg);
	} else if (ipc_type == "sz4") {
		InitSz4(args_mgr, dmn_cfg);
	} else if (ipc_type == "sz3+sz4") {
		InitSz3(args_mgr, dmn_cfg);
		InitSz4(args_mgr, dmn_cfg);
	} else {
		throw std::runtime_error("Invalid ipc type specified");
	}

	parse_precs(dmn_cfg);
}

void IPCFacade::parse_precs(const DaemonConfigInfo& dmn) {
	send_prec_adjs.reserve(send_count);

	size_t ind = 0;
	for (auto unit: dmn.GetUnits()) {
		for (auto send: unit->GetSendParams()) {
			send_prec_adjs[ind++] = exp10(send->GetPrec());
		}
	}
}

void IPCFacade::publish() {
	if (sz3_ipc) {
		sz3_ipc->GoParcook();
	}

	if (sz4_ipc) {
		sz4_ipc->publish();
	}
}

void IPCFacade::receive() {
	if (sz3_ipc) {
		sz3_ipc->GoSender();
	}

	if (sz4_ipc) {
		sz4_ipc->receive();
	}
}

void BaseDaemon::new_cycle() {
	if (daemon_cycle_callback)
		(*daemon_cycle_callback)(*this);
	m_scheduler.schedule();
}

void BaseDaemon::initSignals() {
	// exit with parent
	prctl(PR_SET_PDEATHSIG, SIGHUP);
}

BaseDaemon::BaseDaemon(ArgsManager&& _args_mgr, std::unique_ptr<DaemonConfigInfo> _dmn_cfg, std::unique_ptr<IPCFacade> _ipc): args_mgr(std::move(_args_mgr)), dmn_cfg(std::move(_dmn_cfg)), ipc(std::move(_ipc)) {
	initSignals();

	m_scheduler.set_callback(new FnPtrScheduler([this](){ new_cycle(); }));
	int duration = dmn_cfg->GetDeviceInfo()->getAttribute<int>("extra:cycle_duration", 10000);
	m_scheduler.set_timeout(szarp::ms(duration));
}

const DaemonConfigInfo& BaseDaemon::getDaemonCfg() const { return *dmn_cfg; }

void BaseDaemon::setCycleHandler(std::function<void(BaseDaemon&)> cb) { daemon_cycle_callback = cb; }

void BaseDaemon::poll_forever() {
	new_cycle();
	event_base_dispatch(EventBaseHolder::evbase);

	/* if needed
	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGHUP);

	sigprocmask (SIG_BLOCK, &mask, &oldmask);
	sigsuspend (&oldmask);
	wait();
	*/
}

void BaseDaemon::publish() {
	ipc->publish();
}

void BaseDaemon::receive() {
	ipc->receive();
}

ArgsManager BaseDaemonFactory::InitArgsManager(int argc, const char *argv[], const std::string& daemon_name) {
	ArgsManager args_mgr(daemon_name);
	args_mgr.parse(argc, argv, DefaultArgs(), DaemonArgs());
	args_mgr.initLibpar();
	return args_mgr;
}

std::unique_ptr<DaemonConfigInfo> BaseDaemonFactory::InitDaemonConfig(ArgsManager& args_mgr) {
	if (args_mgr.has("use-cfgdealer")) {
		szlog::init(args_mgr, "borutadmn");
		return std::make_unique<ConfigDealerHandler>(args_mgr);
	} else {
		auto d_cfg = std::make_unique<DaemonConfig>("borutadmn_z");
		if (d_cfg->Load(args_mgr)) {
			throw std::runtime_error("Unable to parse configuration, check previous logs.");
		}

		return d_cfg;
	}
}

std::unique_ptr<IPCFacade> BaseDaemonFactory::InitIPC(ArgsManager& args_mgr, DaemonConfigInfo& daemon_config) {
	return std::make_unique<IPCFacade>(args_mgr, daemon_config);
}

BaseDaemon BaseDaemonFactory::Init(int argc, const char *argv[], const std::string& daemon_name) {
	try {
		auto args_mgr = BaseDaemonFactory::InitArgsManager(argc, argv, daemon_name);
		auto dmn_cfg = BaseDaemonFactory::InitDaemonConfig(args_mgr);
		auto ipc_handler = BaseDaemonFactory::InitIPC(args_mgr, *dmn_cfg);
		EventBaseHolder::Initialize();
		return BaseDaemon(std::move(args_mgr), std::move(dmn_cfg), std::move(ipc_handler));
	} catch(const std::exception& e) {
		sz_log(0, "Encountered error during daemon configuration: %s. Daemon will now exit.", e.what());
		exit(1);
	}
}

