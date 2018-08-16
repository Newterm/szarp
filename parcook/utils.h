#ifndef __DAEMON_UTILS__H_
#define __DAEMON_UTILS__H_

#include <event.h>
#include <utility>
#include <mutex>

#include "sz4/util.h"

template <typename T>
struct non_owning_ptr {
	T* t;

	non_owning_ptr(): t(nullptr) {}
	non_owning_ptr(T* _t): t(_t) {}
	non_owning_ptr(T&& _t): t(new T(std::move(_t))) {}

	~non_owning_ptr() {}

	T* operator->() const { return t; }
	T& operator*() const { return *t; }

	operator T*() const { return t; }
};

namespace szarp {

namespace util {

struct _us {
	static struct timeval get_timeval(int64_t m_t) {
		struct timeval tv;
		tv.tv_sec = m_t / 1000000;
		tv.tv_usec = (m_t % 1000000);
		return tv;
	}
};

struct _ms {
	static struct timeval get_timeval(int64_t m_t) {
		struct timeval tv;
		tv.tv_sec = m_t / 1000;
		tv.tv_usec = (m_t % 1000) * 1000;
		return tv;
	}
};

struct _s {
	static struct timeval get_timeval(int64_t m_t) {
		struct timeval tv;
		tv.tv_sec = m_t;
		tv.tv_usec = 0;
		return tv;
	}
};

} // ns util

template <typename ts>
struct time_spec {
	int64_t m_t;
	time_spec(int64_t t): m_t(t) {}

	explicit operator struct timeval() {
		return ts::get_timeval(m_t);
	}

	operator int64_t() { return m_t; }

	time_spec<ts>& operator+=(const time_spec<ts>& other) {
		m_t -= other.m_t;
		return *this;
	}

	time_spec<ts>& operator-=(const time_spec<ts>& other) {
		m_t -= other.m_t;
		return *this;
	}
};

using sec = time_spec<util::_s>;
using ms = time_spec<util::_ms>;
using us = time_spec<util::_us>;

template <typename ts> time_spec<ts> operator+(const time_spec<ts>& time_spec1, const time_spec<ts>& time_spec2);
template <typename ts> time_spec<ts> operator-(const time_spec<ts>& time_spec1, const time_spec<ts>& time_spec2);

template <typename time_type>
time_type time_now() = delete;

template <>
sz4::second_time_t time_now();

template <>
sz4::nanosecond_time_t time_now();

} // ns szarp

struct EventBaseHolder {
	static struct event_base* evbase;
	static void Initialize() {
		if (EventBaseHolder::evbase)
			return;

		EventBaseHolder::evbase = (struct event_base*) event_init();
		if (!EventBaseHolder::evbase) {
			throw std::runtime_error("Could not initialize event base");
		}
	}
};

class Callback {
public:
	virtual void operator()() = 0;
};

class Scheduler {
	struct event m_timer;
	struct timeval tv;

	non_owning_ptr<Callback> callback;

public:
	Scheduler();

	void set_callback(Callback* _callback);

	template <typename ts>
	void set_timeout(ts time) {
		tv = (struct timeval) time;
	}

	template <typename ts>
	void schedule(ts time) {
		set_timeout(time);
		schedule();
	}

	void schedule();
	void reschedule();

	void cancel();

	static void timer_callback(int fd, short event, void *obj);
};

class FnPtrScheduler: public Callback {
	std::function<void()> f;

public:
	FnPtrScheduler(std::function<void()> _f): f(_f) {}
	void operator()() override { f(); }
};

class cycle_callback_handler {
public:
	virtual void start_cycle() = 0;
};

// scheduler that configures cycle freq from UnitInfo
class CycleScheduler: public Callback {
	std::mutex cycle_mutex;

	Scheduler scheduler;
	non_owning_ptr<cycle_callback_handler> cycle_handler;

public:
	CycleScheduler(cycle_callback_handler* handler);
	void configure(UnitInfo* unit);
	void operator()() override;
};

#endif
