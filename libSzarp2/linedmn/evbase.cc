#include "evbase.h"

// necessary includes for evutil_socket_error_to_string
#include <cerrno>
#include <cstring>

#include <cmath>

EventBase::EventBase(PEvBase base)
{
	if (base) {
		m_event_base = base;
		m_priorities_num = -1;	// in libevent 2.0 we have no way of knowing it
	} else {
		// construct a new event base
		const int default_priorities_num = 3;	// priors are 0,1,..,(p_num - 1)
		// the default priority = floor(priorities_num / 2)
		struct event_base* eb = event_base_new();
		event_base_priority_init(eb, default_priorities_num);
		m_event_base = std::shared_ptr<struct event_base>(eb, event_base_free);
		m_priorities_num = default_priorities_num;
	}
}

PEvent EventBase::CreateEvent(evutil_socket_t fd, short what,
	event_callback_fn cb, void *arg) const
{
	struct event *ev = event_new(m_event_base.get(), fd, what, cb, arg);
	if (!ev) {
		throw EventBaseException("CreateEvent() failed");
	}
	PEvent event = std::shared_ptr<struct event>(ev, event_free);
	return event;
}

PEvent EventBase::CreateTimer(event_callback_fn cb, void *arg) const
{
	struct event *ev = evtimer_new(m_event_base.get(), cb, arg);
	if (!ev) {
		throw EventBaseException("CreateEvent() failed");
	}
	PEvent timer = std::shared_ptr<struct event>(ev, event_free);
	return timer;
}

PEvent EventBase::CreateUserEvent(event_callback_fn cb, void *arg) const
{
	return CreateEvent(-1, 0, cb, arg);
}

PBufferevent EventBase::CreateBufferevent(evutil_socket_t fd, int options) const
{
	struct bufferevent *ev = 
		bufferevent_socket_new(m_event_base.get(), fd, options);
	if (!ev) {
		throw EventBaseException("CreateBufferevent() failed");
	}
	// bufferevents are created with write on and read off

	PBufferevent pb = std::shared_ptr<struct bufferevent>(ev, bufferevent_free);
	return pb;
}

void EventBase::BuffereventConnect(PBufferevent bev,
	const struct sockaddr_in *sin) const
{
	int res = bufferevent_socket_connect(bev.get(),
		(struct sockaddr*)sin, sizeof(*sin));
	if (res != 0) {
		const int error = EVUTIL_SOCKET_ERROR();	// on Unix same as errno
		throw EventBaseException(error, "bufferevent_socket_connect() failed");
	}
}

PEvconnListener EventBase::CreateListenerBind(evconnlistener_cb cb,
	void *ptr, unsigned flags, int backlog,
	const struct sockaddr_in *sin) const
{
	struct evconnlistener *listener = evconnlistener_new_bind(
		m_event_base.get(), cb, ptr, flags, backlog,
		(struct sockaddr*) sin, sizeof(*sin));
	
	if (!listener) {
		const int port = ntohs(sin->sin_port);
		const int error = EVUTIL_SOCKET_ERROR();
		throw EventBaseException(error,
				"evconnlistener_new_bind() failed on port "
				+ std::to_string(port));
	}
	return std::shared_ptr<struct evconnlistener>(listener, evconnlistener_free);
}

void EventBase::Dispatch()
{
	int res = event_base_dispatch(m_event_base.get());
	if (res == -1) {
		throw EventBaseException("event_base_dispatch() error");
	} else if (res == 1) {
		throw EventBaseException("event_base_dispatch() exit no events");
	}
}

void EventBase::EvtimerDel(PEvent ev) const
{
	int res = evtimer_del(ev.get());
	if (res != 0) {
		throw EventBaseException("evtimer_del() failed");
	}
}

void EventBase::EvtimerAddMs(PEvent ev, ms_time_t ms) const
{
	struct timeval tv = ms2timeval(ms);
	int res = evtimer_add(ev.get(), &tv);
	if (res != 0) {
		throw EventBaseException("evtimer_add_ms() failed");
	}
}

void EventBase::SetTimeoutsMs(PBufferevent bev,
	ms_time_t read_ms, ms_time_t write_ms) const
{
	struct timeval tv_read = ms2timeval(read_ms);
	struct timeval tv_write = ms2timeval(write_ms);
	bufferevent_set_timeouts(bev.get(), &tv_read, &tv_write);
}

void EventBase::UserEventAddMs(PEvent ev, ms_time_t ms) const
{
	struct timeval tv = ms2timeval(ms);
	int res = event_add(ev.get(), &tv);
	if (res != 0) {
		throw EventBaseException("event_add() failed");
	}
}

void EventBase::UserEventAdd(PEvent ev) const
{
	int res = event_add(ev.get(), &default_userevent_timeout);
	if (res != 0) {
		throw EventBaseException("event_add() failed");
	}
}

void EventBase::SetEventPriority(PEvent ev, const EventPriority priority) const
{
	int pending = event_pending(ev.get(),
		EV_READ | EV_WRITE | EV_SIGNAL | EV_TIMEOUT, NULL);
	if (pending) {
		throw EventBaseException("SetEventPriority: cannot set priority "
				"on a pending event");
	}
	const int priority_int = PriorityToInt(priority);
	int res = event_priority_set(ev.get(), priority_int);
	if (res != 0) {
		throw EventBaseException("SetEventPriority: setting priority failed");
	}
}

void EventBase::TriggerEvent(PEvent ev) const
{
	event_active(ev.get(), 0, 1);
}

int EventBase::PriorityToInt(const EventPriority priority) const
{
	// event_base_get_npriorities is not available for libevent2.0 (wheezy)
	if (m_priorities_num < 0) {
		throw EventBaseException("PriorityToInt: setting priorities "
				"not supported");
	}
	if (priority == EventPriority::HIGH) {
		return 0;
	} else if (priority == EventPriority::LOW) {
		return m_priorities_num - 1;
	} else {	// MEDIUM
		return std::floor(m_priorities_num / 2);
	}
}

#ifdef EVTHREAD_USE_PTHREADS

void EventBase::EventThreadUsePthreads()
{
	evthread_use_pthreads();
}

void EventBase::EventThreadMakeBaseNotifiable() const
{
	if (evthread_make_base_notifiable(m_event_base.get()) < 0) {
		throw EventBaseException("EventThreadMakeNotifiable: making base notifiable failed");
	}
}

unsigned long long EventBase::ReserveEventId()
{
	return m_event_pool_manager.ReserveId();
}

void EventBase::ManageEvent(const unsigned long long id, PEvent ev)
{
	m_event_pool_manager.InsertEvent(id, ev);
}

PEvent EventBase::ReleaseEvent(const unsigned long long id)
{
	return m_event_pool_manager.DeleteEvent(id);
}

EventPoolManager::EventPoolManager()
	:	m_max_id(0)
{}

EventPoolManager::~EventPoolManager()
{}

unsigned long long EventPoolManager::InsertEvent(PEvent ev)
{
	std::lock_guard<std::mutex> lock(m_event_pool_lock);
	unsigned long long id = m_freed_ids.empty() ? ++m_max_id : pop_free_id();
	m_event_pool[id] = ev;
	return id;
}

void EventPoolManager::InsertEvent(const unsigned long long id, PEvent ev)
{
	std::lock_guard<std::mutex> lock(m_event_pool_lock);
	auto pool_iterator = m_event_pool.find(id);
	if (pool_iterator == m_event_pool.end()) {
		m_event_pool[id] = ev;
	} else {
		throw EventBaseException("EventPoolManage::InsertEvent "
				"event id already used");
	}
}

PEvent EventPoolManager::DeleteEvent(const unsigned long long id)
{
	std::lock_guard<std::mutex> lock(m_event_pool_lock);
	auto pool_iterator = m_event_pool.find(id);
	if (pool_iterator != m_event_pool.end()) {
		PEvent ev = pool_iterator->second;
		m_event_pool.erase(pool_iterator);
		m_freed_ids.push(id);
		return ev;
	}
	return PEvent();
}

unsigned long long EventPoolManager::ReserveId()
{
	std::lock_guard<std::mutex> lock(m_event_pool_lock);
	unsigned long long id = m_freed_ids.empty() ? ++m_max_id : pop_free_id();
	return id;
}

unsigned long long EventPoolManager::pop_free_id()
{
	unsigned long long id = m_freed_ids.top();
	m_freed_ids.pop();
	return id;
}

#endif /* EVTHREAD_USE_PTHREADS */


