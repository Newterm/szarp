#include "utils.h"

struct event_base* EventBaseHolder::evbase = nullptr;

namespace szarp {

template class time_spec<util::_ms>;
template class time_spec<util::_us>;

template <typename ts>
time_spec<ts> operator+(const time_spec<ts>& ms1, const time_spec<ts>& ms2) {
	time_spec<ts> m = ms1;
	m += ms2;
	return m;
}

template <typename ts>
time_spec<ts> operator-(const time_spec<ts>& ms1, const time_spec<ts>& ms2) {
	time_spec<ts> m = ms1;
	m -= ms2;
	return m;
}

template time_spec<util::_ms> operator-(const time_spec<util::_ms>&, const time_spec<util::_ms>&);
template time_spec<util::_us> operator-(const time_spec<util::_us>&, const time_spec<util::_us>&);

sz4::second_time_t time_now() {
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return sz4::second_time_t(time.tv_sec);
}

template <>
sz4::nanosecond_time_t time_now() {
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return sz4::nanosecond_time_t(time.tv_sec, time.tv_nsec);
}

} // ns szarp

Scheduler::Scheduler() {
	evtimer_set(&m_timer, timer_callback, this);
	event_base_set(EventBaseHolder::evbase, &m_timer);
}

void Scheduler::set_callback(Callback* _callback) {
	callback = _callback;
}

void Scheduler::schedule() {
	evtimer_add(&m_timer, &tv);
}

void Scheduler::reschedule() {
	cancel();
	schedule();
}

void Scheduler::cancel() {
	event_del(&m_timer);
}

void Scheduler::timer_callback(int fd, short event, void *obj) {
	try {
		auto *sched = (Scheduler*) obj;
		(*sched->callback)();
	} catch(const std::exception& e) {
		sz_log(2, "Exception caught in timer callback: %s", e.what());
	}
}

CycleScheduler::CycleScheduler(cycle_callback_handler* handler): scheduler() {
	scheduler.set_callback(this);
	cycle_handler = handler;
}

void CycleScheduler::configure(UnitInfo* unit) {
	auto cycle_timeout = unit->getAttribute<int>("ms_cycle_period", 10000);
	scheduler.set_timeout(szarp::ms(cycle_timeout));
	scheduler.schedule();
}

void CycleScheduler::operator()() {
	std::lock_guard<std::mutex> cycle_guard(cycle_mutex);
	scheduler.schedule();
	cycle_handler->start_cycle();
}

