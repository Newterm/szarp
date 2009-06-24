/* 
  SZARP: SCADA software 

*/
/* 
 * szbase - szbcancelhandle
 * $Id$
 * <schylek@praterm.com.pl>
 */

#include "szbcancelhandle.h"

SzbCancelHandle::SzbCancelHandle(): m_stop_flag(false), m_timeout(-1), m_operation_start(-1) {

}

void SzbCancelHandle::SetStopFlag() {
	boost::mutex::scoped_lock datalock(m_flag_mutex);
	m_stop_flag = true;
}

bool SzbCancelHandle::IsStopFlag() {
	boost::mutex::scoped_lock datalock(m_flag_mutex);
	if(m_operation_start == -1) {
		m_operation_start = time(NULL);
	} else {
		if(time(NULL) - m_operation_start > m_timeout){
			return true;
		}
	}
	
	return m_stop_flag;
}

void SzbCancelHandle::SetTimeout(time_t timeout) {
	boost::mutex::scoped_lock datalock(m_flag_mutex);
	m_timeout = timeout;
}

void SzbCancelHandle::Start() {
	boost::mutex::scoped_lock datalock(m_flag_mutex);
	m_operation_start = time(NULL);
}

void SzbCancelHandle::Reset() {
	boost::mutex::scoped_lock datalock(m_flag_mutex);
	m_operation_start = -1;
	m_stop_flag = false;

}


