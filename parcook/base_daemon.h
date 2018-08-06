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

/** 
 * @brief Base class for line daemons
 *
 * This class provides core functionality for implementing
 * parcook line demons
 */
class BaseDaemon {
public:
	/** 
	 * @brief Constructs class with name
	 * 
	 * @param name this name will be used by logger
	 */
	BaseDaemon( const char* name );
	virtual ~BaseDaemon();

	/** 
	 * @brief Initialize class with command line arguments
	 * 
	 * @param argc arguments count
	 * @param argv arguments
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int Init(int argc, const char *argv[] , int dev_index = -1 );

	/** 
	 * @brief Wait for next cycle. 
	 */
	virtual void Wait( time_t interval = DAEMON_INTERVAL );

	/** 
	 * @brief Reads data from device
	 */
	virtual int Read() = 0;

	/** 
	 * @brief Transfer data to parcook
	 */
	virtual void Transfer();

	/** 
	 * @brief clears all parameters with 0
	 */
	void Clear();
	/** 
	 * @brief sets all parameters to specified value.
	 * 
	 * @param val value to set
	 */
	void Clear( short val );

	/** 
	 * @return daemon name
	 */
	const char* Name() { return name; }

protected:
	/** 
	 * @brief Initialize daemon
	 * 
	 * @param name daemon name -- used by logger
	 * @param argc arguments count
	 * @param argv arguments values
	 * @param sz_cfg szarp configuration, if null read from file
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int Init( const char*name , int argc, const char *argv[], int dev_index = -1 );
	/** 
	 * @brief Creates and initialize DaemonConfig member
	 * 
	 * @param name daemon name -- used by logger
	 * @param argc arguments count
	 * @param argv arguments values
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int InitConfig( const char*name , int argc, const char *argv[], int dev_index = -1 );
	/** 
	 * @brief Creates and initialize IPCHandler member
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int InitIPC();
	/** 
	 * @brief Defines signals handlers and masks
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int InitSignals();

	/** 
	 * @brief Parse daemon configuration 
	 *
	 * This function is called at initialization with 
	 * previously constructed DaemonConfig
	 * 
	 * @param cfg daemon configuration 
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int ParseConfig(DaemonConfig * cfg) = 0;

	/** 
	 * @return length of parameters
	 */
	unsigned int Count();
	/** 
	 * @brief values setter
	 * 
	 * @param i index of parameter
	 * @param val value of parameter
	 */
	void Set( unsigned int i , short val );
	/** 
	 * @brief values getter
	 *
	 * @param i index of parameter
	 * 
	 * @return value of parameter
	 */
	short At( unsigned int i );
	/** 
	 * @brief values from sender
	 *
	 * @param i index of send
	 * 
	 * @return value of send
	 */
	short Send( unsigned int i );

	DaemonConfig *m_cfg;
	IPCHandler   *ipc;
	time_t m_last;

	const char* name;
};

#include "zmqhandler.h"
#include <boost/optional.hpp>
#include "utils.h"
#include "cfgdealer_handler.h"

template <typename V>
using bopt = boost::optional<V>;

struct IPCFacade {
	zmq::context_t m_zmq_ctx{1};

	void InitSz3(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg) {
		sz3_ipc = std::make_unique<IPCHandler>(&dmn_cfg);
	}

	void InitSz4(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg) {

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

	IPCFacade(ArgsManager& args_mgr, DaemonConfigInfo& dmn_cfg) {
		if (args_mgr.has("single"))
			return;

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
	}

	std::unique_ptr<IPCHandler> sz3_ipc;
	std::unique_ptr<zmqhandler> sz4_ipc;

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

	void publish() {
		if (sz3_ipc) {
			sz3_ipc->GoParcook();
		}

		if (sz4_ipc) {
			sz4_ipc->publish();
		}

	}
};

#include <sys/prctl.h>

struct BaseDaemon2 {
	ArgsManager args_mgr;
	std::unique_ptr<DaemonConfigInfo> dmn_cfg;
	std::unique_ptr<IPCFacade> ipc;

	Scheduler m_scheduler;
	bopt<std::function<void(BaseDaemon2&)>> daemon_cycle_callback = boost::none;

	void new_cycle() {
		if (daemon_cycle_callback)
			(*daemon_cycle_callback)(*this);
		m_scheduler.schedule();
		ipc->publish();
	}

	void initSignals() {
		// exit with parent
		prctl(PR_SET_PDEATHSIG, SIGHUP);
	}

	BaseDaemon2(ArgsManager&& _args_mgr, std::unique_ptr<DaemonConfigInfo> _dmn_cfg, std::unique_ptr<IPCFacade> _ipc): args_mgr(std::move(_args_mgr)), dmn_cfg(std::move(_dmn_cfg)), ipc(std::move(_ipc)) {
		initSignals();

		m_scheduler.set_callback(new FnPtrScheduler([this](){ new_cycle(); }));
		int duration = dmn_cfg->GetDeviceInfo()->getAttribute<int>("extra:cycle_duration", 10000);
		m_scheduler.set_timeout(szarp::ms(duration));
	}

	const DaemonConfigInfo& getDaemonCfg() const { return *dmn_cfg; }
	void setCycleHandler(std::function<void(BaseDaemon2&)> cb) { daemon_cycle_callback = cb; }

	void poll_forever() {
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

	template <typename VT>
	void setRead(size_t ind, VT value) {
		ipc->setRead(ind, value);
	}
};

struct BaseDaemonFactory {
	static ArgsManager InitArgsManager(int argc, const char *argv[], const std::string& daemon_name) {
		ArgsManager args_mgr(daemon_name);
		args_mgr.parse(argc, argv, DefaultArgs(), DaemonArgs());
		args_mgr.initLibpar();
		return args_mgr;
	}

	static std::unique_ptr<DaemonConfigInfo> InitDaemonConfig(ArgsManager& args_mgr) {
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

	static std::unique_ptr<IPCFacade> InitIPC(ArgsManager& args_mgr, DaemonConfigInfo& daemon_config) {
		return std::make_unique<IPCFacade>(args_mgr, daemon_config);
	}

	static BaseDaemon2 Init(int argc, const char *argv[], const std::string& daemon_name) {
		try {
			auto args_mgr = BaseDaemonFactory::InitArgsManager(argc, argv, daemon_name);
			auto dmn_cfg = BaseDaemonFactory::InitDaemonConfig(args_mgr);
			auto ipc_handler = BaseDaemonFactory::InitIPC(args_mgr, *dmn_cfg);
			EventBaseHolder::Initialize();
			return BaseDaemon2(std::move(args_mgr), std::move(dmn_cfg), std::move(ipc_handler));
		} catch(const std::exception& e) {
			sz_log(0, "Encountered error during daemon configuration: %s. Daemon will now exit.", e.what());
			exit(1);
		}
	}

	/* this can be overloaded to fit daemon's needs */
	template <typename DaemonType>
	static void Go(int argc, const char *argv[], const std::string& daemon_name) {
		auto base_dmn = BaseDaemonFactory::Init(argc, argv, daemon_name);
		DaemonType daemon(base_dmn);
		base_dmn.poll_forever();
	}
};

#endif /*BASE_DAEMON_H*/
