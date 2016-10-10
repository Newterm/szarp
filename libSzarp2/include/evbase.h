#ifndef __EVENTBASE_H__
#define __EVENTBASE_H__

#include "exception.h"
#include "mstime.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <memory>

#ifdef EVTHREAD_USE_PTHREADS
#include <event2/thread.h>
#include <thread>
#include <future>
#include <functional>
#include <utility>
#include <stack>
#include <unordered_map>
#include <mutex>
#endif /* EVTHREAD_USE_PTHREADS */

/** Exception specific to EventBase class. */
class EventBaseException : public SzException {
	SZ_INHERIT_CONSTR(EventBaseException, SzException)
};

typedef std::shared_ptr<struct bufferevent> PBufferevent;
typedef std::shared_ptr<struct event> PEvent;
typedef std::shared_ptr<struct evconnlistener> PEvconnListener;
typedef std::shared_ptr<struct event_base> PEvBase;	// don't confuse with PEventBase

#ifdef EVTHREAD_USE_PTHREADS
/**
 * This is a simple manager that keeps PEvent instances
 * alive and identifiable by unique id so that they can
 * be safely added and removed from multiple threads.
 *
 * The whole event id and its reservation concept is
 * needed because we need to pass event identification
 * as callback arguments when creating event so that
 * it will be later possible to delete it
 * (we cannot pass event beacause it does not yet exist).
 */
class EventPoolManager {
public:
	EventPoolManager();
	~EventPoolManager();
	/** Thread safe insert event and get unique id */
	unsigned long long InsertEvent(PEvent ev);
	/** Thread safe insert event with id */
	void InsertEvent(const unsigned long long id, PEvent ev);
	/** Thread safe reserve id */
	unsigned long long ReserveId();
	/** Thread safe delete event by id */
	PEvent DeleteEvent(const unsigned long long id);
private:
	/** Unsafe get first freed id */
	unsigned long long pop_free_id();

	unsigned long long m_max_id; /**< current max id used*/
	std::stack<unsigned long long> m_freed_ids; /**< ids freed so far*/
	std::unordered_map<unsigned long long,PEvent> m_event_pool; /**< pool of events by id*/
	std::mutex m_event_pool_lock; /**< single lock for event pool manager*/
};
#endif /* EVTHREAD_USE_PTHREADS */

class EventBase;
typedef std::shared_ptr<EventBase> PEventBase;	// don't confuse with PEvBase

/** possible event priorities */
enum class EventPriority { HIGH, MEDIUM, LOW };

/** Interface providing methods for safely creating new events.
 * Checks underlying libevent error codes and throws exceptions.
 */
class EventBase {
public:
	/**
	 * This is a ugly hack because libevent 2.0 seems
	 * to have a bug when adding user events with NULL
	 * timeout. This is a very long timeout (1 week)
	 * that will be provided if user does not provide
	 * his own.
	 */
	const struct timeval
		default_userevent_timeout = { 604800, 0 };

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

	/** Create user event */
	PEvent CreateUserEvent(event_callback_fn cb, void *arg) const;

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

	/** Wrapper for event_add for user event with timeout */
	void UserEventAddMs(PEvent ev, ms_time_t ms) const;

	/** Wrapper for event_add for user event with hacked timeout */
	void UserEventAdd(PEvent ev) const;

	/** Wrapper for set_event_priority, checks
	 * if event wasn't added to a base */
	void SetEventPriority(PEvent ev,
		const EventPriority priority) const;

	/** Manually trigger the event */
	void TriggerEvent(PEvent ev) const;

private:
	int PriorityToInt(const EventPriority priority) const;

#ifdef EVTHREAD_USE_PTHREADS
public:
	/** Activate pthread threading library locks - use to allow cross thread calls */
	static void EventThreadUsePthreads();

	/** Activate notifications */
	void EventThreadMakeBaseNotifiable() const;

	/** Get unique event id to identify it */
	unsigned long long ReserveEventId();

	/** Transfer event ownership to EventBase with given unique id */
	void ManageEvent(const unsigned long long id, PEvent ev);

	/** Release ownership of event from EventBase by id and get it */
	PEvent ReleaseEvent(const unsigned long long id);
#endif /* EVTHREAD_USE_PTHREADS */

private:
	PEvBase m_event_base; /**< event base for all created events */
	int m_priorities_num;	/**< number of priorities defined in event_base */
#ifdef EVTHREAD_USE_PTHREADS
	EventPoolManager m_event_pool_manager; /**< a manager instance to safely keep chosen events alive */
#endif /* EVTHREAD_USE_PTHREADS */
};


#ifdef EVTHREAD_USE_PTHREADS
/** Helper class representing async job callback argument.
 *
 * In libevent callback argument is void* so we are bound to
 * use static_cast. This class tries to at least address this
 * issue by making the casting step less error prone.
 * */
template<class ExpectedResultType>
struct EventIdWithJobResult {
	EventIdWithJobResult(unsigned long long id, std::shared_ptr<ExpectedResultType> result)
		:	event_id(id), job_result(result)
	{}
	EventIdWithJobResult() = delete;
	unsigned long long event_id; /**< event id from pool manager - use it with ReleaseEvent */
	std::shared_ptr<ExpectedResultType> job_result; /**< requested job result */
};

/** Private namespace of this module */
namespace {
/** Wait for future instance get() and generate managed event.
 *
 * This function is blocking and meant to be called in a thread.
 * It creates managed event with event id and future value as callback
 * arguments. It then triggers the user event.
 */
template<class Function, class... Args>
void wait_for_future(PEventBase evbase, event_callback_fn callback,
		std::shared_future<typename std::result_of<Function(Args...)>::type>&& pending_result)
{
	using JobReturnType = typename std::result_of<Function(Args...)>::type;
	using PJobReturnType = std::shared_ptr<JobReturnType>;
	PJobReturnType result(new JobReturnType(pending_result.get()));
	const unsigned long long id = evbase->ReserveEventId();
	EventIdWithJobResult<JobReturnType>* event_id_with_result =
		new EventIdWithJobResult<JobReturnType>(id, result);
	PEvent ev = evbase->CreateUserEvent(callback,static_cast<void*>(event_id_with_result));
	evbase->ManageEvent(id, ev);
	evbase->UserEventAdd(ev);
	evbase->TriggerEvent(ev);
}
}

/** Schedule an asynchronous job ending with user callback.
 *
 * WARNING: This is not efficient (2 threads are created).
 * This will work better with async future support in C++.
 *
 * The callback argument type is pointer to EventIdWithJobResult.
 * It is provided through event_callback_fn void* so dynamic_cast
 * is impossible. You need to use static_cast to get job result at the
 * moment. In future additional mechanism for casting result existence
 * is to be expected.
 *
 * This is not a class method because we would like to trigger
 * automatic template deduction. Thanks to this template
 * arguments will be deduced automatically.
 */
template<class Function, class... Args>
void EventBaseCreateAsyncJob(PEventBase evbase, event_callback_fn callback,
		Function&& f, Args&&... args)
{
	std::shared_future<typename std::result_of<Function(Args...)>::type>
		pending_result(std::async(std::launch::async, f, args...));
	std::thread pending_result_waiter(wait_for_future<Function,Args...>,
			evbase, callback, std::move(pending_result));
	pending_result_waiter.detach();
}
#endif /* EVTHREAD_USE_PTHREADS */

#endif
