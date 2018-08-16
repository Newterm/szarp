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
	bopt<std::string> sub_conn_address = args_mgr.get<std::string>("parhub", "sub_conn_addr");
	bopt<std::string> pub_conn_address = args_mgr.get<std::string>("parhub", "pub_conn_addr");

	if (!sub_conn_address || !pub_conn_address) {
		throw std::runtime_error("Could not get parhub's address from configuration, exiting!");
	}

	try {
		sz4_ipc = std::make_unique<zmqhandler>(&dmn_cfg, m_zmq_ctx, pub_conn_address->c_str(), sub_conn_address->c_str());
	} catch (zmq::error_t& e) {
		throw std::runtime_error(std::string("ZMQ initialization failed: ") + std::string(e.what()));
	}
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

template <typename VT>
void IPCFacade::setRead(size_t ind, VT value) {
	if (ind > read_count || ind < 0) {
		szlog::log() << szlog::warning << "Got invalid read index " << ind << szlog::endl;
		return;
	}

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

template void IPCFacade::setRead<int16_t>(size_t, int16_t);
template void IPCFacade::setRead<int32_t>(size_t, int32_t);
template void IPCFacade::setRead<int64_t>(size_t, int64_t);
template void IPCFacade::setRead<float>(size_t, float);
template void IPCFacade::setRead<double>(size_t, double);

void IPCFacade::setNoData(size_t ind) {
	if (ind > read_count || ind < 0) {
		szlog::log() << szlog::warning << "Got invalid read index " << ind << szlog::endl;
		return;
	}

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
VT IPCFacade::getSend(size_t ind) {
	if (ind > send_count || ind < 0) {
		szlog::log() << szlog::warning << "Got invalid send index " << ind << szlog::endl;
		return sz4::no_data<VT>();
	}

	if (sz4_ipc) {
		auto prec_adj = send_prec_adjs[ind];
		auto value = sz4_ipc->get_send<VT>(ind, prec_adj).value;
		szlog::log() << szlog::debug << "Got send at " << ind << " with " << value << szlog::endl;
		return value;
	}

	if (sz3_ipc) {
		auto value = sz3_ipc->m_send[ind];
		szlog::log() << szlog::debug << "Got send at " << ind << " with " << value << szlog::endl;
		return sz3_ipc->m_send[ind];
	}

	return sz4::no_data<VT>();
}

template int16_t IPCFacade::getSend<int16_t>(size_t);
template int32_t IPCFacade::getSend<int32_t>(size_t);
template int64_t IPCFacade::getSend<int64_t>(size_t);
template float IPCFacade::getSend<float>(size_t);
template double IPCFacade::getSend<double>(size_t);

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

ArgsManager BaseDaemonFactory::InitArgsManager(int argc, const char *argv[], const std::string& daemon_name) {
	ArgsManager args_mgr(daemon_name);
	args_mgr.parse(argc, argv, DefaultArgs(), DaemonArgs());
	args_mgr.initLibpar();
	return args_mgr;
}

std::unique_ptr<DaemonConfigInfo> BaseDaemonFactory::InitDaemonConfig(ArgsManager& args_mgr, const std::string& daemon_name) {
	if (args_mgr.has("use-cfgdealer")) {
		szlog::init(args_mgr, daemon_name);
		return std::make_unique<ConfigDealerHandler>(args_mgr);
	} else {
		auto d_cfg = std::make_unique<DaemonConfig>(daemon_name.c_str());
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
		auto dmn_cfg = BaseDaemonFactory::InitDaemonConfig(args_mgr, daemon_name);
		auto ipc_handler = BaseDaemonFactory::InitIPC(args_mgr, *dmn_cfg);
		EventBaseHolder::Initialize();
		return BaseDaemon(std::move(args_mgr), std::move(dmn_cfg), std::move(ipc_handler));
	} catch(const std::exception& e) {
		sz_log(0, "Encountered error during daemon configuration: %s. Daemon will now exit.", e.what());
		exit(1);
	}
}

