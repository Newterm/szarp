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
 * Very Restricted Shell
 * 
 * Praterm S.A.
 * Stanis³aw Sawa <ecto@praterm.com.pl> 2002
 * Pawe³ Pa³ucha <pawel@praterm.com.pl> 2005
 * 
 * Pozwala na:
 * - czytanie rsyncem z ¶cie¿ek zaczynaj±cych siê na /opt/szarp
 * - pisanie rsyncem do ¶cie¿ek zaczynaj±cych siê na /opt/szarp wszystkim 
 *   userom poza RO_USER
 * - odpalenie 'pustego' shella z komend± 'exit'
 * - odpalenie '/bin/sh -c' powoduj±ce wej¶cie w tryb 'keep-alive'
 * W ¶cie¿kach do rsynca sprawdzana jest obecno¶æ '..' - je¿eli wyst±pi, to
 * shell nie pozwala na transfer.
 * Komenda w trybie interaktywnym mo¿e mieæ do 255 znaków. Wszystkie wpisy
 * stringów w logach s± obciête to 254 znaków.
 * 
 * Logowanie jest do sysloga:
 * - 'interactive session...' - rozpoczêcie sesji interaktywnej
 * - 'batch session...' - rozpoczêcie sesji wsadowej
 * - 'sesion closed' - zamkniêcie sesji
 * - 'cmd:...' - pe³na zawarto¶æ komendy wydanej przez w sesji wsadowej
 * - 'INTERACTIVE...' - komendy wydawane przez usera w trakcie sesji
 *   interaktywnej - testowanie usera albo próba w³amania
 * - 'KEEPALIVE...' - sesja 'keep-alive' - stary tunel albo próba w³amania
 * - 'ILLEGALCOMMAND...' - komenda inna ni¿ rsync lub keep-alive - albo kto¶
 *   próbuje ¶ci±gaæ CVS'a albo co¶ kombinuje
 * - 'ILLEGALRSYNC...' - albo co¶ siê nie zgadza z wersj± rsync'a, albo kto¶
 *   co¶ kombinuje (np. próbuje u¿ywaæ '..' w ¶cie¿ce)
 * - 'READONLY...' - kto¶ próbuje co¶ zapisywaæ z usera RO_USER
 * - 'RSYNC...' - kto¶ zrobi³ sobie rsync'a
 * - 'SCRIPTER...' - kto¶ ¶ci±gn±³ sobie skrypt do szarp_scripter'a
 *
 * Wersja z debugowaniem (vrsh_d) tworzy dla ka¿dej sesji w katalogu /tmp plik
 * o nazwie vrsh.PID zawieraj±cy informacjê o argumentach wywo³ania i
 * ¶rodowisku.
 *
 * $Id$
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdlib.h>
#include <syslog.h>

#include "tokens.h"

#define SH_PATH "/bin/sh"

/// ¶cie¿ka z/do której mo¿na robiæ rsync'a
#define ALLOW_PATH "/opt/szarp"

/// user read-only
#define RO_USER	"szarpcvs"

/// komunikat powitalny
#define TOS "\
Shell masz bardzo ograniczony ;-)\n\
"
/// menu z dostêpnymi komendami
#define MENU "\
Dostêpne komendy:\n\
exit - koniec pracy - wyj¶cie\n\
"

/// znak zachêty
#define PROMPT "% "


/// tryb debug - decyzja w czasie kompilacji
#ifdef DEBUG
/// globalny uchwyt do pliku z debugiem
FILE           *statusfl;

/** Wypisuje status (linia poleceñ, ¶rodowisko). */
void
openstatus(int argc, char *argv[], char *envp[])
{
    char            fname[20];
    int             i;

    sprintf(fname, "/tmp/vrsh.%d", getpid());

    statusfl = fopen(fname, "w");
    fprintf(statusfl, "**ARG**\n");
    for (i = 0; i < argc; i++)
	fprintf(statusfl, "[%d]: %s\n", i, argv[i]);
    fprintf(statusfl, "\n**ENV**\n");
    for (i = 0; envp[i] != NULL; i++)
	fprintf(statusfl, "%s\n", envp[i]);

    fprintf(statusfl, "\n**SESSION**\n");
}

/** Logowanie komend trybu interaktywnego */
void
logstatus(char *s)
{
    static int      i = 0;
    fprintf(statusfl, "%d> %s\n", ++i, s);
}

/** Zamyka plik status */
void
closestatus()
{
    fclose(statusfl);
}
#endif // DEBUG

#define LINEBUF 255

#define GETLINE \
for ( i=0; (i <LINEBUF) && ((ch=getchar()) !=EOF) && (ch!='\n'); i++)\
  buf[i] = (char)ch;\
buf[i] = '\0';\
if (ch ==EOF)\
  break;\
if ((i ==LINEBUF) && ((char)ch !='\n'))\
  while (ch!='\n')\
    ch=getchar();

/*** Tryb interaktywny */
void
interactive()
{
    printf(TOS);
    // mainloop
    printf("%s", MENU);
    while (1) {
	static char     buf[LINEBUF + 1];
	int             i = 0;
	int             ch;

	printf("%s", PROMPT);
	// getline
	GETLINE;
#ifdef DEBUG
	logstatus(buf);
#endif				// DEBUG
	syslog(LOG_NOTICE, "INTERACTIVE USER %.254s@[%.254s] CMD: %.254s", 
			getenv("USER"), getenv("SSH_CLIENT"),
			buf);
	// wychodzimy
	if (strncmp("exit", buf, 4) == 0) {
	    break;
#if 0
	    // ssh - przyk³ad co mo¿na zrobiæ by uruchomiæ program
	} else if (strncmp("ssh", buf, 3) == 0) {
	    pid_t           pid;
#define HOSTLEN 40
#define UNAMELEN 10
	    static char     host[HOSTLEN],
	                    uname[UNAMELEN];
	    // dialog z userem
	    printf("Podaj nazwê hosta docelowego: ");
	    GETLINE;
	    strncat(host, buf, HOSTLEN);
	    printf("Podaj nazwê u¿ytkownika: ");
	    GETLINE;
	    strncat(uname, buf, UNAMELEN);
	    if ((pid = fork()) == 0) {
		execl(SSH_PATH, "ssh", "-e", "none", "-l", uname, host,
		      NULL);
		perror("execl");
	    } else
		waitpid(pid, NULL, 0);
	    // ppp
#endif				// 0
	} else
	    printf("unregistered command\n");
    }
}

int batch_rsync(int argc, char *argv[]) {
	int tokc = 0;
	char **toks;
	/* czy user chce czytaæ pliki */
	int send = 0;
	/* czy user odpala szarp_scripter'a */
	int scripter = 0;
	
	// teraz w teorii powinni¶my grzecznie odpaliæ to co jest w
	// argv[2],
	// ale to jest biiig dziura (chiechie, orginalne scp ma to samo)
	// musimy odpowiednio poci±æ argv[2] 

	if (strncmp(argv[2], "/bin/sh -c", 10) == 0) {
	    // dowcip: zak³adamy, ¿e jak kto¶ chce shella do dajemy mu
	    // keepaliva
	    // TODO trzeba to bedzie w przysz³o¶ci wywaliæ po sprawdzeniu ¿e
	    // nikt z tego nie korzysta
	    syslog(LOG_NOTICE, "KEEPALIVE USER %.254s@[%.254s]", getenv("USER"), 
			    getenv("SSH_CLIENT"));
	    while (1) {
		printf(".+.\n");
		fflush(stdout);
		sleep(5);
	    }
	}

	if (strncmp(argv[2], "rsync", 5) != 0) {
		syslog(LOG_WARNING, "ILLEGALCOMMAND rsync expected");
		goto end;
	}
	// sprawdzamy co rsyncujemy, spodziewamy siê czego¶ w postaci:
	// rsync --server --sender -vlogDtprz . /opt/szarp/prefix
	tokenize(argv[2], &toks, &tokc);
	if (tokc < 5) {
		syslog(LOG_WARNING, "ILLEGALRSYNC not enough tokens (%d)", tokc);
		goto end;
	}
	if (strncmp(toks[0], "rsync", 5) != 0) {
		syslog(LOG_WARNING, "ILLEGALRSYNC rsync expected");
		goto end;
	}
	if (strncmp(toks[1], "--server", 8) != 0) {
		syslog(LOG_WARNING, "ILLEGALRSYNC --server expected");
		goto end;
	}
	if (strncmp(toks[2], "--sender", 8) != 0) {
		if (strncmp(RO_USER, getenv("USER"), strlen(RO_USER)) == 0) {
			syslog(LOG_WARNING, "READONLY user, --sender expected");
			goto end;
		}
	} else {
		send = 1;
	}
	if (strncmp(toks[tokc-2], ".", 1) != 0) {
		syslog(LOG_WARNING, "ILLEGALRSYNC . expected");
		goto end;
	}
	/* sprawdzamy czy to mo¿e szarp_scripter */
	if ((send == 1) 
			&& (strncmp(toks[tokc-1], "scripts/", 8) == 0)
			&& (strncmp(getenv("USER"), RO_USER, strlen(RO_USER)) == 0)) {
		scripter = 1;
	} else if (strncmp(toks[tokc-1], ALLOW_PATH, strlen(ALLOW_PATH)) != 0) {
		syslog(LOG_WARNING, "ILLEGALRSYNC "ALLOW_PATH" expected");
		goto end;
	}
	// zabezpieczenie ¿eby nie wyskakiwaæ z /opt/szarp
	if (strstr(toks[tokc-1], "..") != NULL) {
		syslog(LOG_WARNING, "ILLEGALRSYNC cannot use '..' in path");
		goto end;
	}
	if (scripter) {
		syslog(LOG_NOTICE, "SCRIPTER USER %.254s@[%.254s] AT '%.254s'", 
				getenv("USER"), 
				getenv("SSH_CLIENT"), 
				toks[tokc-1]);
	} else {
		syslog(LOG_NOTICE, "RSYNC USER %.254s@[%.254s] %s DIR '%.254s'", 
				getenv("USER"), 
				getenv("SSH_CLIENT"), 
				send ? "FROM" : "TO",
				toks[tokc-1]);
	}
	tokenize(NULL, &toks, &tokc);
	
	return 1;
end:
	return 0;
}

int batch_stat(int argc, char *argv[]) {
	int tokc = 0;
	char **toks;
	int ret = 0;

	tokenize(argv[2], &toks, &tokc);

	if (tokc != 3) {
		goto end;
	}

	if (strcmp(toks[0], "stat")) {
		goto end;
	}

	if (strcmp(toks[1], "--format=%Y")) {
		syslog(LOG_WARNING, "second param must be --format=%%Y");
		goto end;
	}

	if (strncmp(toks[2], "/opt/szarp/", 11)) {
		syslog(LOG_WARNING, "not a szarp base dir");
		goto end;
	}

	
	char *c = toks[2] + strlen("/opt/szarp/");
	for (;*c;c++) 
		switch (*c) {
			case '/':
			case '_':
			case '.':
			case 'a'...'z':
			case 'A'...'Z':
			case '0'...'9':
				break;
			default: 
				syslog(LOG_WARNING, "illegal char '%c'", *c);
				goto end;
				break;

		}

	ret = 1;

end:
	if (tokc)
		tokenize(NULL, &toks, &tokc);
	return ret;
}

/** Tryb wsadowy */
int 
batch(int argc, char *argv[])
{
	pid_t           pid;
	// tutaj parsujemy to co mo¿na naszemu shellowi zadaæ
	if (strcmp(argv[1], "-c") != 0) 
		return 255;

	syslog(LOG_NOTICE, "cmd: %.254s", argv[2]);

	char* c = argv[2];

	for (; *c; ++c) 
		switch (*c) {
			case '&':
			case ';':
			case '$':
			case '|':
			case '`':
				syslog(LOG_WARNING, "cmd: %.254s rejected because looks suspicious", argv[2]);
				return 255;
			default:
				break;
		}
	
	if (!(batch_stat(argc, argv) 
		|| batch_rsync(argc, argv)))
		return 255;
	
	// uruchamiamy komende
	if ((pid = fork()) == 0) {
		execl(SH_PATH, "/bin/sh", "-c", argv[2], NULL);
		perror("execl");
		return 255;
	} else {
		int status;
		waitpid(pid, &status, 0);
		return WEXITSTATUS(status);
	}

}

int
main(int argc, char *argv[], char *env[])
{
#ifdef DEBUG
    openstatus(argc, argv, env);
#endif // DEBUG
    openlog("vrsh", LOG_PID, LOG_LOCAL0);

    int ret = 0;
    
    if (strncmp(argv[0], "-vrsh", 5) == 0) {
	syslog(LOG_WARNING, "interactive session %.254s@[%.254s]",
	       getenv("USER"), getenv("SSH_CLIENT"));
	interactive();
    } else {
	syslog(LOG_NOTICE, "batch session %.254s@[%.254s]",
	       getenv("USER"), getenv("SSH_CLIENT"));
	ret = batch(argc, argv);
    }

#ifdef DEBUG
    closestatus();
#endif // DEBUG
    syslog(LOG_NOTICE, "session closed");
    closelog();

    return ret;
}

