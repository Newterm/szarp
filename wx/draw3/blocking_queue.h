#ifndef __BLOCKING_QUEUE__
#define __BLOCKING_QUEUE__

#include <mutex>
#include <stack>
#include <condition_variable>


template <class T>
class BlockingQueue {
public:
	void push(const T& item);
	void push(T&& item);

	T pop();
	void pop(T& item);

	bool empty() const
	{ return q.empty();	}

private:
	std::stack<T> q;
	std::condition_variable cv;
	std::mutex queue_lock;
};

#endif /*  blocking_queue */
