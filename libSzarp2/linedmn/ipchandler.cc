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

IPCHandler::IPCHandler(DaemonConfigInfo* cfg): units(cfg->GetUnits())
{
	const auto& ipc_info = cfg->GetIPCInfo();
	single = cfg->GetSingle();
	line_no = cfg->GetLineNumber();
	m_params_count = cfg->GetParamsCount();
	m_sends_count = cfg->GetSendsCount();

	if (!single) {
		InitIPC(ipc_info);
	}
}


void IPCHandler::InitIPC(const IPCInfo& ipc_info)
{
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

	key_t key;
	const auto& linex_key = ipc_info.linex_path.c_str();
	key = ftok(linex_key, line_no);
	if (key == -1) {
		sz_log(1, "ftok() error (path: %s), errno %d", m_cfg->GetLinexPath(), errno);
		throw std::runtime_error("ftok() error");
	}
	m_shm_d = shmget(key, sizeof(short) * m_params_count, 00600);
	if (m_shm_d == -1) {
		sz_log(1, "shmget() error (key: %08x), errno %d", key, errno);
		throw std::runtime_error("shmget() error");
	}

	m_segment = (short *) shmat(m_shm_d, NULL, 0);
	if ( m_segment == (short *)(-1) ) {
		sz_log(1, "shmat() error, errno %d", errno);
		throw std::runtime_error("shmat() error");
	}

	const auto& parcook_key = ipc_info.parcook_path.c_str();
	key = ftok(parcook_key, SEM_PARCOOK);
	if (key == -1) {
		sz_log(1, "ftok('%s', SEM_PARCOOK), errno %d", m_cfg->GetLinexPath(), errno);
		throw std::runtime_error("ftok() error");
	}
	m_sem_d = semget(key, SEM_LINE + 2 * line_no, 00600);
	if (m_sem_d == -1) {
		sz_log(1, "semget(%x, %d) error, errno %d", 
				key, SEM_LINE + 2 * m_cfg->GetLineNumber(), errno);
		throw std::runtime_error("shmget() error");
	}

	if (m_sends_count <= 0)
		return;

	key = ftok(parcook_key, MSG_SET);
	if (key == -1) {
		sz_log(1, "ftok('%s', MSG_SET), errno %d", m_cfg->GetLinexPath(), errno);
		throw std::runtime_error("ftok() error");
	}
	m_msgset_d = msgget(key, 00600);
	if (m_msgset_d == -1) {
		sz_log(1, "msgget() (MSG_SET) error, errno %d", errno);
		throw std::runtime_error("msgget() error");
	}
}



void IPCHandler::GoSender()
{
	if (m_send == NULL || single)
		return;

	tMsgSetParam msg;

	short* send = m_send;

	for (const auto& unit: units) {
		while (msgrcv(m_msgset_d, &msg, sizeof(msg.cont), unit->GetSenderMsgType(), IPC_NOWAIT)
		       == sizeof(msg.cont))
		{
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
	if (single)
		return;

	struct sembuf semset[2];

	semset[0].sem_num = SEM_LINE + 2 * (line_no - 1) + 1;
	semset[0].sem_op = 1;
	semset[1].sem_num = SEM_LINE + 2 * (line_no - 1);
	semset[1].sem_op = 0;
	semset[0].sem_flg = semset[1].sem_flg = SEM_UNDO;
	semop(m_sem_d, semset, 2);

	memcpy(m_segment, m_read, m_params_count * sizeof(short));

	semset[0].sem_num = SEM_LINE + 2 * (line_no - 1) + 1;
	semset[0].sem_op = -1;
	semset[0].sem_flg = SEM_UNDO;
	semop(m_sem_d, semset, 1);
}


#endif /* MINGW32 */ 

