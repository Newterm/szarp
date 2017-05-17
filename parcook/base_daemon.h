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
 * @brief Base struct for line daemons.
 * 
 * Contains basic functions that may be useful.
 *
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-16
 */

#include <szarp_config.h>

#include <dmncfg.h>
#include <ipchandler.h>

#include "sz4/time.h"
#include "zmqhandler.h"

#include "libpar.h"

const int DEFAULT_EXPIRE = 660;	/* 11 minutes */
const int DAEMON_INTERVAL = 10;

#include "sz4/defs.h"
#include "sz4/util.h"
#include "sz4/time.h"
#include <chrono>
#include <mutex>
#include <event.h>
#include <evdns.h>
#include <stdexcept>

#include "interface_wrapper.h"

namespace sz4 {
template <class time_type> inline time_type getTimeNow() { return time_type(NULL); }

template<> sz4::second_time_t inline getTimeNow<sz4::second_time_t>() {
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	return sz4::second_time_t(seconds);
}

template<> sz4::nanosecond_time_t inline getTimeNow<sz4::nanosecond_time_t>() {
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
	return sz4::nanosecond_time_t(seconds, nanoseconds);
}
}

struct IPCProvider {
	virtual void Clear() { Clear(0); }
	virtual void Clear( short val ) = 0;

	virtual void Publish() = 0;
	virtual void Receive() = 0;

	virtual size_t GetParamsCount() const = 0;
	virtual size_t GetSendsCount() const = 0;

	virtual void SetVal( size_t i, int16_t val ) = 0;
	virtual void SetVal( size_t i, int32_t val ) { SetVal(i, static_cast<short>(val)); }
	virtual void SetVal( size_t i, float val ) { SetVal(i, static_cast<short>(val)); }
	virtual void SetVal( size_t i, double val ) { SetVal(i, static_cast<short>(val)); }

	virtual int16_t GetSendShort( size_t i ) = 0;
	virtual int32_t GetSendInt( size_t i ) { return GetSendShort(i); }
	virtual float GetSendFloat( size_t i ) { return GetSendShort(i); }
	virtual double GetSendDouble( size_t i ) { return GetSendShort(i); }
};


struct IPCHandlerWrapper: IPCProvider {
	using Args = DaemonConfig*;
	IPCHandlerWrapper(DaemonConfig *cfg): _ipc(new IPCHandler(cfg)) { Init(); };
	IPCHandlerWrapper(IPCHandler*&& ipc): _ipc(std::move(ipc)) {};
	IPCHandlerWrapper(IPCHandler*& ipc): _ipc(new IPCHandler(*ipc)) {};

	void Publish() override { _ipc->GoParcook(); }
	void Receive() override { _ipc->GoSender(); }

	virtual void Clear( short val ) override {
		for (size_t i = 0; i < GetParamsCount(); ++i) {
			_ipc->m_read[i] = val;
		}
	}

	void SetVal(size_t p_no, short val) override { _ipc->m_read[p_no] = std::move(val); }
	short GetVal(size_t p_no) const { return _ipc->m_read[p_no]; }
	short GetSendShort(size_t p_no) override { return _ipc->m_send[p_no]; }
	size_t GetSendsCount() const override { return _ipc->m_sends_count; }
	size_t GetParamsCount() const override { return _ipc->m_params_count; }

	void Init() { if(_ipc->Init()) throw std::runtime_error("Could not initialize IPCHandler"); } // Maybe just validate zmq

private:
	std::unique_ptr<IPCHandler> _ipc;
};

template <typename time_type = sz4::nanosecond_time_t>
struct ZMQWrapper: IPCProvider {
	using Args = DaemonConfig*;
	ZMQWrapper(DaemonConfig *cfg): _zmq(nullptr) {
		std::string sub_addr{libpar_getpar("parhub", "pub_conn_addr", 1)};
		std::string pub_addr{};
		if (!cfg->GetSingle()) pub_addr = libpar_getpar("parhub", "sub_conn_addr", 1);
		zmq::context_t ctx{1};
		_zmq.reset(new zmqhandler(cfg->GetIPK(), cfg->GetDevice(), ctx, sub_addr, pub_addr));
	}

	ZMQWrapper(zmqhandler*&& zmq): _zmq(std::move(zmq)) {}

	void Publish() override { _zmq->publish(); }

	void SetVal( size_t i, int16_t val ) { _zmq->set_value(index, sz4::getTimeNow<time_type>(), val); }
	void SetVal( size_t i, int32_t val ) { _zmq->set_value(index, sz4::getTimeNow<time_type>(), val); }
	void SetVal( size_t i, float val ) { _zmq->set_value(index, sz4::getTimeNow<time_type>(), val); }
	void SetVal( size_t i, double val ) { _zmq->set_value(index, sz4::getTimeNow<time_type>(), val); }

	// I don't think sends work ATM, if they are needed use sz4base or defdmn
	void GetSend( size_t i, int16_t& val ) { val = _GetSend<int16_t>(i); }
	void GetSend( size_t i, int32_t& val ) { val = _GetSend<int32_t>(i); }
	void GetSend( size_t i, float& val ) { val = _GetSend<float>(i); }
	void GetSend( size_t i, double& val ) { val = _GetSend<double>(i); }

	size_t GetParamsCount() const override { return _zmq->GetParamsCount(); }
	size_t GetSendsCount() const override { return _zmq->GetSendsCount(); }

private:
	std::unique_ptr<zmqhandler> _zmq;

	template <typename VT>
	VT _GetSend(size_t p_no) { // TODO: this might be not the way we want to implement this
	                          // Maybe boost::variant
		szarp::ParamValue& tp = _zmq->get_value(p_no);
		if (!tp.IsInitialized()) return std::numeric_limits<VT>::quiet_NaN();
		else if (tp.has_int_value()) return VT{tp.int_value()};
		else if (tp.has_float_value()) return VT{tp.float_value()};
		else if (tp.has_double_value()) return VT{tp.double_value()};
		else return std::numeric_limits<VT>::quiet_NaN();
	}
};

/*
template <typename IT, typename... CTs>
struct MultiWrapper;
*/

struct EventProvider {
	virtual void RegisterTimerCallback(std::function<void()> f, int32_t seconds = -1, int32_t useconds = -1) = 0;
	virtual void ClearTimerEvents() = 0;
	virtual void RegisterEventWithDelay(std::function<void()> f, int32_t seconds = -1, int32_t useconds = -1) = 0;
	virtual void PollForever() = 0;
};

namespace BaseDaemonHelper {

struct CallbackWrapper {
	friend class LibeventWrapper;

	CallbackWrapper(struct timeval tv, struct event_base* base, std::function<void()> callback, std::mutex& mutex): _timer(evtimer_new(base, CallbackWrapper::timer_callback, this)), _tv(tv), _callback(callback), _mutex(mutex) {
		evtimer_add(_timer, &_tv);
	}

	static void timer_callback(int fd, short event, void* arg) {
		if (arg == nullptr) return;
		auto ptr = (CallbackWrapper*) arg;
		if (ptr == nullptr) return;
		std::lock_guard<std::mutex> lock(ptr->_mutex);

 		ptr->_callback();

		if (ptr->_timer == nullptr) return;
		evtimer_add(ptr->_timer, &ptr->_tv);
	}

	struct event *_timer;

private:
	struct timeval _tv;
	std::function<void()> _callback;

	std::mutex& _mutex;
};

} // BaseDaemonHelper

struct LibeventWrapper: EventProvider {
	using Args = DaemonConfig*;
	using CallbackWrapper = BaseDaemonHelper::CallbackWrapper;

	LibeventWrapper(DaemonConfig* dmn): EventProvider(), default_tv(dmn->GetDevice()->getDeviceTimeval()) {
		m_event_base = event_base_new();
		if (!m_event_base) {
			throw std::runtime_error("Could not initialize event base");
		}
	}

	std::vector<CallbackWrapper*> callbacks;

	void RegisterTimerCallback(std::function<void()> f, int32_t seconds = -1, int32_t useconds = -1) override {
		struct timeval tv_cb;
		tv_cb.tv_sec = seconds == -1? default_tv.tv_sec : seconds;
		tv_cb.tv_usec = useconds == -1? default_tv.tv_usec : useconds;
		
		std::lock_guard<std::mutex> lock(mutex);
		CallbackWrapper* wrapper = new CallbackWrapper(tv_cb, m_event_base, f, mutex);
		callbacks.push_back(wrapper);
	}

	virtual void ClearTimerEvents() override {
		std::lock_guard<std::mutex> lock(mutex);
		for (auto callback: callbacks) {
			if (callback->_timer) delete callback->_timer;
		}
	}

	void RegisterEventWithDelay(std::function<void()> f, int32_t seconds = -1, int32_t useconds = -1) override {
		struct timeval tv_cb;
		tv_cb.tv_sec = seconds == -1? default_tv.tv_sec : seconds;
		tv_cb.tv_usec = useconds == -1? default_tv.tv_usec : useconds;

		std::lock_guard<std::mutex> lock(mutex);
		CallbackWrapper* wrapper = new CallbackWrapper(tv_cb, m_event_base, f, mutex);
		delete wrapper->_timer;
	}

	virtual void PollForever() override {
		event_base_loop(m_event_base, 0);
	}

private:
	struct event_base *m_event_base;
	struct timeval default_tv;

	std::mutex mutex;
};

struct ConfigProvider {
	virtual std::string GetName() const = 0;
	virtual bool GetSingle() const = 0;

	virtual TDevice* GetDevice() = 0;
	virtual xmlDocPtr GetXMLDoc() = 0;
	virtual xmlNodePtr GetXMLDevice() = 0;
};


#include <signal.h>
namespace BaseDaemonHelper {

RETSIGTYPE terminate_handler(int signum);

} // BaseDaemonHelper

#include <sys/prctl.h>
struct SignalProvider {
	using Args = DaemonConfig*;
	SignalProvider(DaemonConfig* cfg) {}

	virtual void InitSignals() {
		struct sigaction sa;
		sigset_t block_mask;

		sigfillset(&block_mask);
		sigdelset(&block_mask, SIGKILL);
		sigdelset(&block_mask, SIGSTOP);
		sa.sa_flags = SA_RESTART;
		sa.sa_handler = BaseDaemonHelper::terminate_handler;
		if (sigaction(SIGTERM, &sa, NULL) || sigaction(SIGINT, &sa, NULL) || sigaction(SIGHUP, &sa, NULL))
			throw std::runtime_error("");
		
		prctl(PR_SET_PDEATHSIG, SIGHUP);
	}
};


struct LogProvider {
};

struct DaemonConfigWrapper: ConfigProvider {
	using Args = DaemonConfig*;
	DaemonConfigWrapper( const char* name, int argc, const char *argv[], TSzarpConfig* sz_cfg ) {
		m_cfg = new DaemonConfig(name);
		if(m_cfg->Load(&argc, const_cast<char **>(argv), 1, sz_cfg)) throw std::runtime_error("");
		m_cfg->GetIPK()->SetConfigId(0);
	}

	DaemonConfigWrapper( DaemonConfig* cfg ): m_cfg(cfg) {
		if (!m_cfg) throw std::runtime_error("");
	}
	
	std::string GetName() const { return m_cfg->GetName(); }
	bool GetSingle() const { return m_cfg->GetSingle(); }

	TDevice* GetDevice() { return m_cfg->GetDevice(); }
	xmlDocPtr GetXMLDoc() { return m_cfg->GetXMLDoc(); }
	xmlNodePtr GetXMLDevice() { return m_cfg->GetXMLDevice(); }

	DaemonConfig* GetDaemonConfig() { return m_cfg; }

private:
	DaemonConfig* m_cfg;

};

#endif /*BASE_DAEMON_H*/
