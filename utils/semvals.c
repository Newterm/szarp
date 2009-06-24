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
/*$Id$*/
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>


void usage(const char* progname) {
	printf("Wywolanie:\n"
		"%s sciezka [ilosc semaforow]\n"
		"Porgram wyswietla dla kazdego semafora jego aktualna\n"
		"wartosc, ilosc procesow czekajacych na zwiekszenie oraz\n"
		"wyzerowanie jego warosci. Domyslna ilosc semaforow to 12.\n"
		"Przyk³ad wywo³ania:\n"
		"%s /opt/szarp/swid/config/PTT.act\n",
		progname, progname
		);
	exit(1);
}

int main(int argc, char* argv[]) {
	if (argc < 2 || argc > 3) 
		usage(argv[0]);

	key_t key = ftok(argv[1],0);

	int sem_count = (argc == 2) ? 12 : atoi(argv[2]);

	int sem_des = semget(key, sem_count, 00666);
	if (sem_des < 0) {
		printf("Nie udalo sie pobrac semaforow %s\n", strerror(errno));
		return 0;
	}

	int i;

	printf("%10s%17s%17s%22s\n", "Numer", "wartosc", "czeka na zero", "czeka na zwiekszenie");
	for (i = 0; i < sem_count; i++) {
		int val = semctl(sem_des, i, GETVAL);
		int zerocnt = semctl(sem_des,i, GETZCNT);
		int incrcount = semctl(sem_des,i, GETNCNT);
		printf("%10d%17d%17d%22d\n", i, val, zerocnt, incrcount);
	}

	return 0;
}
