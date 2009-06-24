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
/* $Id$ */

/*
 * Ma³e proxy dla rs'a (a w³a¶ciwie dla dwóch dowolny urz±dzeñ znakowych:
 * umo¿liwia komunikacje dwóch stron z logowaniem przebiegu dialogu.
 *
 * (c) Ecto 2003, Praterm
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define BUFSIZE 1024

/**
 * Otwiera linie (port szeregowy) do komunikacji i inicjalizuje
 * jego parametry
 * \param line nazwa pliku-u¿±dzenia
 * \returns deskryptor pliku
 */
int open_line(char *line) {
  int linedes;
  struct termio rsconf;

  fprintf(stderr, "[%s] ", line);
  if ((linedes = open(line, O_RDWR|O_NDELAY)) <0) {
    perror("open");
    return -1;
  }else {
    fprintf(stderr, "opened\n");
  }

  ioctl(linedes, TCGETA, &rsconf);

  rsconf.c_iflag = 0;
  rsconf.c_oflag = 0;
  rsconf.c_cflag = B19200|CS8|CLOCAL|CREAD;
  rsconf.c_lflag = 0;
  rsconf.c_cc[4] = 0;
  rsconf.c_cc[5] = 0;
  
  ioctl(linedes, TCSETA, &rsconf);

  return linedes;
}

int main(int argc, char *argv[]) {
  char *line1 = "/dev/ttyA11", *line2 = "/dev/ttyA12", *logfile="log";
  int linedes1, linedes2;
  fd_set rfds;
  FILE *logfile_fl;

  if ( ((linedes1 = open_line(line1)) <0) ||
       ((linedes2 = open_line(line2)) <0) ||
       ((logfile_fl = fopen(logfile, "w")) ==NULL) ) {
    fprintf(stderr, "initialize failed\n");
    return -1;
  }

  FD_ZERO(&rfds);
  FD_SET(linedes1, &rfds);
  FD_SET(linedes2, &rfds);

  while (1) {
    int cg, cr;
    fd_set rfds_local = rfds;
    static char readbuf[BUFSIZE];

    if ((cg = select(16, &rfds_local, NULL, NULL, NULL)) <0) {
      perror("select");
    }

    if (cg ==2)
      fprintf(stderr, "possibly collision\n");

    if ((cg >0) && FD_ISSET(linedes1, &rfds_local)) {
      if ((cr = read(linedes1, readbuf, sizeof(readbuf))) <0) {
        fprintf(stderr, "[%s] ", line1);
        perror("read");
      }else {
        register int i;

        printf("[1->2] %d bytes\n", cr);

	fprintf(logfile_fl, "[1->2] (%03d bytes) ", cr);
	for (i=0; i <cr; i++)
          fprintf(logfile_fl, " 0x%02hx", (unsigned char)readbuf[i]);
	fprintf(logfile_fl, "\n");
	fflush(logfile_fl);

        write(linedes2, readbuf, cr);
      }
      cg--;
    }
    
    if ((cg >0) && FD_ISSET(linedes2, &rfds_local)) {
      if ((cr = read(linedes2, readbuf, sizeof(readbuf))) <0) {
        fprintf(stderr, "[%s] ", line2);
        perror("read");
      }else {
        register int i;

        printf("[2->1] %d bytes\n", cr);

	fprintf(logfile_fl, "[2->1] (%03d bytes) ", cr);
	for (i=0; i <cr; i++)
          fprintf(logfile_fl, " 0x%02hx", (unsigned char)readbuf[i]);
	fprintf(logfile_fl, "\n");
	fflush(logfile_fl);

        write(linedes1, readbuf, cr);
      }
      cg--;
    }

  }

  return 0;
}
