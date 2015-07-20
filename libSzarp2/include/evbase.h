#ifndef __EVENTBASE_H__
#define __EVENTBASE_H__

#include "exception.h"
#include "mstime.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <memory>

/** Exception specific to EventBase class. */
class EventBaseException : public SzException {
	SZ_INHERIT_CONSTR(EventBaseException, SzException)
};

typedef std::shared_ptr<struct bufferevent> PBufferevent;
typedef std::shared_ptr<struct event> PEvent;
typedef std::shared_ptr<struct evconnlistener> PEvconnListener;
typedef std::shared_ptr<struct event_base> PEvBase;	// don't confuse with PEventBase

class EventBase;
typedef std::shared_ptr<EventBase> PEventBase;	// don't confuse with PEvBase

/** possible event priorities */
enum class EventPriority { HIGH, MEDIUM, LOW };

/** Interface providing methods for safely creating new events.
 * Checks underlying libevent error codes and throws exceptions.
 */
class EventBase {
public:
	/** Initialize using constructed event base.
	 * The event base pointer should have destructor bound to it,
	 * i.e.: std::shared_ptr<struct event_base>(eb, event_base_free),
	 * the event base should have preferably at least 3 priorities.
	 * If pointer 'base' is empty, a new base will be constructed
	 * (with 3 priorities mapping to EventPriority types).
	 * No default empty value is given, as an implicit default base
	 * proved to cause errors in libevents authors opinion.
	 *
	 * TODO: when all code using event_base is modified to use
	 * EventBase, EventBase should be the only place in which
	 * event_base is created and freed.
	 */
	EventBase(PEvBase base);
	EventBase() = delete;

	/** Create new event */
	PEvent CreateEvent(evutil_socket_t fd, short what,
		event_callback_fn cb, void *arg) const;

	/** Create new evtimer */
	PEvent CreateTimer(event_callback_fn cb, void *arg) const;

	/** Create a new bufferevent for both TCP sockets and file descriptors.
	 * @param fd use -1 to create a TCP socket later with BuffereventConnect
	 */
	PBufferevent CreateBufferevent(evutil_socket_t fd,
		int options = 0) const;

	/** Connect an existing TCP bufferevent to address sin */
	void BuffereventConnect(PBufferevent bev,
		const struct sockaddr_in *sin) const;

	/** Create a new TCP listener using provided sockaddr_in */
	PEvconnListener CreateListenerBind(evconnlistener_cb cb,
		void *ptr, unsigned flags, int backlog,
	    const struct sockaddr_in *sin) const;

	/** Dispatch the contained event_base, should be called only once */
	void Dispatch();

	/** Wrapper for evtimer_del, checks result */
	void EvtimerDel(PEvent ev) const;

	/** Wrapper for evtimer_add, checks result */
	void EvtimerAddMs(PEvent ev, ms_time_t ms) const;

	/** Wrapper for bufferevent_set_timeouts */
	void SetTimeoutsMs(PBufferevent bev,
		ms_time_t read_ms, ms_time_t write_ms) const;

	/** Wrapper for set_event_priority, checks
	 * if event wasn't added to a base */
	void SetEventPriority(PEvent ev,
		const EventPriority priority) const;

private:
	int PriorityToInt(const EventPriority priority) const;

private:

	PEvBase m_event_base; /**< event base for all created events */
	int m_priorities_num;	/**< number of priorities defined in event_base */
};

#endif
