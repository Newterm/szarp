#include "blocking_queue.h"

#include <functional>

template <class T>
void BlockingQueue<T>::push(const T& item)
{
	std::unique_lock<std::mutex> mlock(queue_lock);
	q.push(item);
	mlock.unlock();
	cv.notify_all();
}

template <class T>
void BlockingQueue<T>::push(T&& item)
{
	std::unique_lock<std::mutex> mlock(queue_lock);
	q.push(std::move(item));
	mlock.unlock();
	cv.notify_all();
}

template <class T>
T BlockingQueue<T>::pop()
{
	std::unique_lock<std::mutex> mlock(queue_lock);
	while(q.empty())
	{
		cv.wait(mlock);
	}
	auto item = q.top();
	q.pop();
	return item;
}

template <class T>
void BlockingQueue<T>::pop(T& item)
{
	std::unique_lock<std::mutex> mlock(queue_lock);
	while(q.empty())
	{
		cv.wait(mlock);
	}
	item = q.top();
	q.pop();
}

template class BlockingQueue<std::function<void()>>;
