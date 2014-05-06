/* 
  SZARP: SCADA software 

*/
/*
 * Line daemon

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Parcook and sender communication.
 * 
 * $Id$
 */

#include "ipchandler.h"

#ifndef MINGW32

#include <assert.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include "ipcdefines.h"
#include "msgtools.h"
#include "liblog.h"

IPCHandler::IPCHandler(DaemonConfig * cfg)
{
	assert(cfg != NULL);
	m_cfg = cfg;

	m_params_count = 0;
	m_sends_count = 0;
	TRadio* radio = cfg->GetDevice()->GetFirstRadio();
	for (TUnit* unit = radio->GetFirstUnit();
			unit; unit = radio->GetNextUnit(unit)) {
		m_params_count += unit->GetParamsCount();
		m_sends_count += unit->GetSendParamsCount();
	}

	if (m_params_count > 0) {
		m_read = (short *) calloc(m_params_count, sizeof(short));
		for (int i = 0; i < m_params_count; i++)
			m_read[i] = SZARP_NO_DATA;
	} else
		m_read = NULL;

	if (m_sends_count > 0) {
		m_send = (short *) calloc(m_sends_count, sizeof(short));
		for (int i = 0; i < m_sends_count; i++)
			m_send[i] = SZARP_NO_DATA;
	} else
		m_send = NULL;
}

int IPCHandler::Init()
{
	key_t key;
	if (m_cfg->GetSingle())
		return 0;

	key = ftok(m_cfg->GetLinexPath(), m_cfg->GetLineNumber());
	if (key == -1) {
		sz_log(0, "ftok() error (path: %s), errno %d", m_cfg->GetLinexPath(), errno);
		return 1;
	}
	m_shm_d = shmget(key, sizeof(short) * m_params_count, 00600);
	if (m_shm_d == -1) {
		sz_log(0, "shmget() error (key: %08x), errno %d", key, errno);
		return 1;
	}

	m_segment = (short *) shmat(m_shm_d, NULL, 0);
	if ( m_segment == (short *)(-1) ) {
		sz_log(0, "shmat() error, errno %d", errno);
		return 1;
	}

	key = ftok(m_cfg->GetParcookPath(), SEM_PARCOOK);
	if (key == -1) {
		sz_log(0, "ftok('%s', SEM_PARCOOK), errno %d", m_cfg->GetLinexPath(), errno);
		return 1;
	}
	m_sem_d = semget(key, SEM_LINE + 2 * m_cfg->GetLineNumber(), 00600);
	if (m_sem_d == -1) {
		sz_log(0, "semget(%x, %d) error, errno %d", 
				key, SEM_LINE + 2 * m_cfg->GetLineNumber(), errno);
		return 1;
	}

	if (m_sends_count <= 0)
		return 0;

	key = ftok(m_cfg->GetParcookPath(), MSG_SET);
	if (key == -1) {
		sz_log(0, "ftok('%s', MSG_SET), errno %d", m_cfg->GetLinexPath(), errno);
		return 1;
	}
	m_msgset_d = msgget(key, 00600);
	if (m_msgset_d == -1) {
		sz_log(0, "msgget() (MSG_SET) error, errno %d", errno);
		return 1;
	}

	return 0;
}


void IPCHandler::GoSender()
{
	if (m_send == NULL)
		return;
	if (m_cfg->GetSingle())
		return;

	tMsgSetParam msg;

	short* send = m_send;

	for (DaemonConfig::UnitInfo* unit = m_cfg->GetFirstUnitInfo();
		unit;
		unit = unit->GetNext()) {
		long type = unit->GetSenderMsgType();

		while (msgrcv(m_msgset_d, &msg, sizeof(msg.cont), type, IPC_NOWAIT)
		       == sizeof(msg.cont)) {
			if (msg.cont.param >= unit->GetSendParamsCount())
				continue;
			send[msg.cont.param] = msg.cont.value;
		}

		send += unit->GetSendParamsCount();

	}
}

void IPCHandler::GoParcook()
{
	if (m_read == NULL)
		return;
	if (m_cfg->GetSingle())
		return;

	struct sembuf semset[2];

	semset[0].sem_num = SEM_LINE + 2 * (m_cfg->GetLineNumber() - 1) + 1;
	semset[0].sem_op = 1;
	semset[1].sem_num = SEM_LINE + 2 * (m_cfg->GetLineNumber() - 1);
	semset[1].sem_op = 0;
	semset[0].sem_flg = semset[1].sem_flg = SEM_UNDO;
	semop(m_sem_d, semset, 2);

	memcpy(m_segment, m_read, m_params_count * sizeof(short));

	semset[0].sem_num = SEM_LINE + 2 * (m_cfg->GetLineNumber() - 1) + 1;
	semset[0].sem_op = -1;
	semset[0].sem_flg = SEM_UNDO;
	semop(m_sem_d, semset, 1);
}


#endif /* MINGW32 */ 

