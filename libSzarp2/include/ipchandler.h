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

/**
 * Handles communication with parcook and sender.
 */
class           IPCHandler {
      public:
	/**
	 * @param cfg pointer to daemon config info */
	IPCHandler(DaemonConfig * cfg);
	/** Initializes shared memory, semaphores and message queues.
	 * @return 0 on success, 1 on error
	 */
	int             Init();
	/**
	 * Get data from sender.
	 */
	void            GoSender();
	/**
	 * Puts data to parcook shared memory segment.
	 */
	void            GoParcook();
      protected:
	                DaemonConfig * m_cfg;
				/**< pointer to daemon config info */
	int             m_shm_d;/**< shared memory descriptor */
	int             m_sem_d;/**< semaphore descriptor */
	int             m_msgset_d;
				/**< descriptor of queue for reading params
				  from sender */
	short          *m_segment;
				/**< pointer to attached segment */
      public:
	int             m_params_count;
				/**< number of params to read */
	short          *m_read;	/**< buffer for params read*/
	int             m_sends_count;
				/**< number of params to send */
	short          *m_send;	/**< buffer for params to send */
};

#endif /* MINGW32 */

#endif /* __IPCHANDLER_H__ */

