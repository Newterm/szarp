/* 
  SZARP: SCADA software 

*/
/*
 * szbase - szbcancelhandle.h
 * $Id$
 * <schylek@praterm.com.pl>
 */

#ifndef __SZBCANCELHANDLE_H__
#define __SZBCANCELHANDLE_H__

#include <boost/thread/thread.hpp>
#include <time.h>

class SzbCancelHandle {
	public:
		SzbCancelHandle();
		void SetStopFlag();
		bool IsStopFlag();
		void SetTimeout(time_t timeout);
		void Start();
		void Reset();
	private:
		boost::mutex m_flag_mutex;
		bool m_stop_flag;
		time_t m_timeout;
		time_t m_operation_start;
};

#endif //__SZBCANCELHANDLE_H__

