/* 
  SZARP: SCADA software 

*/
/*
 * Line daemon

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Communication with parcook and sender.
 * 
 * $Id$
 */

#ifndef __IPCHANDLER_H__
#define __IPCHANDLER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef MINGW32

#include "dmncfg.h"
#include "ipcdefines.h"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "ipcdefines.h"
#include "msgtools.h"
#include "liblog.h"


/**
 * Handles communication with parcook and sender.
 */
class IPCHandler {
public:
	IPCHandler(DaemonConfigInfo* cfg);

	void InitIPC(const IPCInfo&);

	/**
	* Get data from sender.
	*/
	void GoSender();

	/**
	* Puts data to parcook shared memory segment.
	*/
	void GoParcook();

protected:
	const std::vector<UnitInfo*> units;

	int m_shm_d;
	int m_sem_d;
	int m_msgset_d;

	bool single;
	short *m_segment;

	int line_no;

public:
	int m_params_count;
	short *m_read;
	int m_sends_count;
	short *m_send;
};

#endif /* MINGW32 */

#endif /* __IPCHANDLER_H__ */

