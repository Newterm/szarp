/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/*
 * meaner3 - daemon writing data to base in SzarpBase format
 * SZARP
 * parcook.cc - communication with parcook
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#include "tparcook.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <assert.h>
#include <errno.h>

#include "liblog.h"
#include "libpar.h"
#include "ipcdefines.h"

#include "meaner3.h"

/** structure with IPC identifiers needed to connect to parcook */
typedef struct {
	key_t ipc_ftok_sem;	/**< ftok key for semaphors */
	key_t ipc_ftok_shm;	/**< ftok key for shared memory segments */
	int ipc_id_sem1;	/**< id fo semaphore 1 */
	int ipc_id_sem2;	/**< id for semaphore 2 */
} IPCIds;

/** ipc identifiers for min10 and probe segment types */
IPCIds IPCParams[ProbesType_last] = {
	{ 
		SEM_PARCOOK,
	       	SHM_MIN10,
		SEM_MIN10 + 1,
		SEM_MIN10,
	},
	{
		SEM_PARCOOK,
		SHM_PROBE,
		SEM_PROBE + 1,
		SEM_PROBE, 
	}
};

TParcook::TParcook(ProbesType type):
	probes_type(type)
{
	parcook_path = NULL;
	shm_desc = sem_desc = 0;
	probes_count = 0;
	copied = NULL;
}

TParcook::~TParcook()
{
	if (parcook_path)
		free(parcook_path);
	if (copied)
		free(copied);
}

int TParcook::LoadConfig()
{
	parcook_path = libpar_getpar(SZARP_CFG_SECTION, "parcook_path", 0);
	if (parcook_path == NULL) {
		sz_log(0, "TParcook::LoadConfig(): set 'parcook_path' param in " SZARP_CFG " file");
		return 1;
	}
	return 0;
}


int TParcook::Init(int probes_count)
{
	key_t shm_key;
	key_t sem_key;
	const int max_attempts_no = 60;

	assert(probes_count > 0);
	
	this->probes_count = probes_count;
	copied = (short int *) malloc (probes_count * sizeof(short int));
	if (copied == NULL) {
		sz_log(0, "TParcook::Init(): not enough memory for probes table, errno %d",
				errno);
		return 1;
	}
	shm_key = ftok(parcook_path, IPCParams[probes_type].ipc_ftok_shm);
	if (shm_key == -1) {
		sz_log(0, "TParcook::Init(): ftok() for shared memory key failed, errno %d, \
path '%s'",
			errno, parcook_path);
		return 1;
	}
	sem_key = ftok(parcook_path, IPCParams[probes_type].ipc_ftok_sem);
	if (sem_key == -1) {
		sz_log(0, "TParcook::Init(): ftok() for semaphore key failed, errno %d, \
path '%s'",
			errno, parcook_path);
		return 1;
	}
	shm_desc = shmget(shm_key, 1, 00600);
	sem_desc = semget(sem_key, 2, 00600);
	for (int i = 0; ((shm_desc == -1) || (sem_desc == -1)) && (i < max_attempts_no-1); ++i)
	{
		sleep(1);
		shm_desc = shmget(shm_key, 1, 00600);
		sem_desc = semget(sem_key, 2, 00600);
	}
	if (shm_desc == -1) {
		sz_log(0, "TParcook::Init(): error getting parcook shared memory identifier, \
errno %d, key %d",
			errno, shm_key);
		return 1;
	}
	if (sem_desc == -1) {
		sz_log(0, "TParcook::Init(): error getting parcook semaphor identifier, \
errno %d, key %d",
			errno, sem_key);
		return 1;
	}
	
	return 0;
}

void TParcook::GetValues()
{
	short int* probes;	/**< attached probe table */
	struct sembuf sems[2];

	/* block signals */
	g_signals_blocked = 1;
	/* enter semaphore, set undo for possible exit */
	sems[0].sem_num = IPCParams[probes_type].ipc_id_sem1;
	sems[0].sem_op = 0;
	sems[0].sem_flg = SEM_UNDO;
	sems[1].sem_num = IPCParams[probes_type].ipc_id_sem2;
	sems[1].sem_op = 1;
	sems[1].sem_flg = SEM_UNDO;
	if (semop(sem_desc, sems, 2) == -1) {
		sz_log(0, "TParcook::GetValues(): cannot open parcook semaphore (is parcook runing?), errno %d, exiting", errno);
		g_signals_blocked = 0;
		/* we use non-existing signal '0' */
		g_TerminateHandler(0);
	}
	/* attach segment */
retry:
	probes = (short int *) shmat(shm_desc, 0, SHM_RDONLY);
	if (probes == (void*)-1) {
		if (errno == EINTR)
			goto retry;
		sz_log(0, "TParcook::GetValues(): cannot attach parcook memory segment (is parcook runing?), errno %d, exiting", errno);
		g_signals_blocked = 0;
		g_TerminateHandler(0);
	}
	/* copy values */
	memcpy(copied, probes, probes_count * sizeof (short int) );
	sz_log(10, "TParcook::GetValues(): %d params copied", probes_count);
	/* detach segment */
	if (shmdt(probes) == -1) {
		sz_log(0, "TParcook::GetValues(): cannot detaches parcook memory segment, errno %d, exiting", errno);
		g_signals_blocked = 0;
		g_TerminateHandler(0);
	}
	/* release semaphore */
	sems[0].sem_num = IPCParams[probes_type].ipc_id_sem2;
	sems[0].sem_op = -1;
	sems[0].sem_flg = SEM_UNDO;
	if (semop(sem_desc, sems, 1) == -1) {
		sz_log(0, "TParcook::GetValues(): cannot release parcook semaphore, errno %d, exiting", errno);
		g_signals_blocked = 0;
		/* we use non-existing signal '0' */
		g_TerminateHandler(0);
	}
	/* unblock signals */
	g_signals_blocked = 0;
	if (g_should_exit) {
		g_TerminateHandler(0);
	}
}
		
short int TParcook::GetData(int i)
{
	return copied[i];
}

