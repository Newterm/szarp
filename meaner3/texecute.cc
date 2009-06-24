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
 * bodas staff
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#include "texecute.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

//#ifndef TEMP_FAILURE_RETRY(expression)
#ifndef TEMP_FAILURE_RETRY
# define TEMP_FAILURE_RETRY(expression) \
  (__extension__							      \
    ({ long int __result;						      \
       do __result = (long int) (expression);				      \
       while (__result == -1L && errno == EINTR);			      \
       __result; }))							      \

#endif

#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>

#include "liblog.h"
#include "libpar.h"

#include "meaner3.h"

/** Name of shell used to execute commands */
#define SHELL "sh"

/** Mutex, set to 1 by signal handler when new cycle alarm expires. */
volatile sig_atomic_t g_next_cycle = 0;

/** States of automat for parsing crontab time */
typedef enum {PTAS_START, PTAS_DIGIT, PTAS_COMMA,
		PTAS_PERIOD, PTAS_ALL}  PT_AUTO_STATES;

/** Function set as a SIGCHLD signal handler, calles g_exec->SigChldHandler.
 * g_exec is a global pointer to program's TExecute object.
 * @param signum signal number
 */
RETSIGTYPE g_ChildHandler(int signum)
{
	g_exec->SigChldHandler(signum);
}

/** Function set as a SIGALRM signal handler, calles g_exec->SigAlrmHandler.
 * g_exec is a global pointer to program's TExecute object.
 * @param signum signal number
 */
RETSIGTYPE g_AlarmHandler(int signum)
{
	g_exec->SigAlrmHandler(signum);
}

TExecute::TExecute(TStatus *status)
{
	stdout_file = NULL;
	stderr_file = NULL;
	sections = NULL;
	alarms = NULL;
	this->status = status;
}

TExecute::~TExecute()
{
	if (stdout_file)
		free(stdout_file);
	if (stderr_file)
		free(stderr_file);
	if (sections) {
		/* signals should be blocked ! */
		TExecSection *s;
		TExecAlarm *a;
		struct timespec ts;
		
		for (s = sections; s; s = s->GetNext()) {
			if (s->GetPid() > 0) {
				sz_log(10, "execute: terminating process with pid %d",
						s->GetPid());
				kill(s->GetPid(), SIGTERM);
			}
		}
		/* wait half a second */
		ts.tv_sec = 0;
		ts.tv_nsec = 500000000;
		nanosleep(&ts, NULL);
		for (s = sections; s; s = s->GetNext()) {
			if (s->GetPid() > 0) {
				kill(s->GetPid(), SIGKILL);
				waitpid(s->GetPid(), NULL, 0);
			}
			delete s;
		}
		for (a = alarms; a; a = a->next)
			delete a;
	}
}

int TExecute::LoadConfig()
{
	char *sections;
	char *sect, *tmp;
	
	stdout_file = libpar_getpar(EXECUTE_SECTION, "execute_stdout", 0);
	if (stdout_file == NULL) 
		stdout_file = strdup(PREFIX"/logs/execute.stdout");
	stderr_file = libpar_getpar(EXECUTE_SECTION, "execute_stderr", 0);
	if (stderr_file == NULL) 
		stderr_file = strdup(PREFIX"/logs/execute.stderr");

	sections = libpar_getpar(EXECUTE_SECTION, "execute_sections", 0);
	if ((sections == NULL) || (!strcmp(sections, ""))) {
		sz_log(2, "execute: no sections to execute found");
		if (sections != NULL)
			free(sections);
		return 0;
	}
	
	tmp = sections;
	sz_log(10, "execute: sections is '%s'", sections);
	while ( (sect = strsep(&tmp, " ")) != NULL) {
		sz_log(10, "execute: loading section '%s'", sect);
		if (LoadSectionConfig(sect)) {
			free(sections);
			return 1;
		}
	}
	
	free(sections);
	return 0;
}

void TExecute::MarkToRun(time_t t)
{
	struct tm lt;
	TExecSection *s;
	
	localtime_r(&t, &lt);
	status->Reset(TStatus::PTE_TRSECS);
	status->Reset(TStatus::PTE_RSECS);
	status->Reset(TStatus::PTE_SECS);
	for (s = sections; s; s = s->GetNext()) {
		if ((s->GetFails() > 0) 
				|| s->CheckTime((lt.tm_wday + 6) % 7, 
					lt.tm_mon, 
					lt.tm_mday - 1,
					lt.tm_hour, 
					lt.tm_min / 10)) {
			s->MarkRun(1);
			status->Incr(TStatus::PTE_TRSECS);
		} else
			s->MarkRun(0);
		status->Incr(TStatus::PTE_SECS);
	}
	current = sections;
}

void TExecute::RunSections()
{
	for ( ; current; current = current->GetNext()) {
		if (!current->CheckRun())
			continue;
		status->Incr(TStatus::PTE_RSECS);
		current->Launch();
		if (current==NULL || !current->CheckParallel())
			return;
	}
}

void TExecute::SigChldHandler(int signum)
{
	int olderrno = errno;
	TExecSection *e;
	TExecAlarm *a;
	sz_log(10, "execute: SigChldHandler() called");
	
	while (1) {
		register int pid;
		int w, c, fail = 0, i;
		
		do {
			errno = 0;
			pid = waitpid (WAIT_ANY, &w, WNOHANG | WUNTRACED);
		} while (pid <= 0 && errno == EINTR);

		if (pid <= 0) {
			/* no more processes */
			errno = olderrno;
			return;
		}

		sz_log(10, "execute: child exited, pid %d", pid);

		/* find section for alarm */
		for (e = sections; e && (e->GetPid() != pid); e = e->GetNext())
			;
		assert (e != NULL);
	
		/* remove alarms for section */
		for (a = alarms; a; a = a->next) {
			if ((a->section == e) && (!a->isset))
				a->type = TExecAlarm::ALRM_REM;
		}

		/* process exit code */
		fail = 1;
		if (WIFEXITED(w)) {
			c = WEXITSTATUS(w);
			if (c == 0) {
				status->Incr(TStatus::PTE_OKSECS);
				i = 3;
				fail = 0;
			} else {
				i = 2;
				status->Incr(TStatus::PTE_ERRSECS);
			}
			sz_log(i, "execute: command '%s' (pid %d) returned with code %d", 
					e->GetCommand(),
					pid, c);
		} else if (WIFSIGNALED(w)) {
			sz_log(2, "execute: command '%s' (pid %d) terminated by signal %d",
					e->GetCommand(),
					pid, WTERMSIG(w));
			status->Incr(TStatus::PTE_INTSECS);
		} else if (WIFSTOPPED(w)) {
			sz_log(2, "execute: command '%s' (pid %d) stopped by signal %d",
					e->GetCommand(),
					pid, WSTOPSIG(w));
			status->Incr(TStatus::PTE_ERRSECS);
		} else {
			sz_log(2, "execute: command '%s' (pid %d) returned for unknown reason",
					e->GetCommand(), pid);
			status->Incr(TStatus::PTE_ERRSECS);
		}

		e->ClearPid();
		/* set retry */
		if (fail)
			e->CheckRetry();
		/* check next section */
		if (current == e) {
			current = current->GetNext();
			RunSections();
		}

	} /* while (1) */
}

void TExecute::SigAlrmHandler(int signum)
{
	int olderrno = errno;
	sz_log(10, "execute: SigAlrmHandler() called");
	assert (alarms != NULL);
	assert (alarms->isset == 1);
	TExecAlarm *tmp;

	switch (alarms->type) {
		case TExecAlarm::ALRM_NEWCL:
			g_next_cycle = 1;
			break;
		case TExecAlarm::ALRM_STRM:
			assert (alarms->section);
			if (alarms->section->GetPid() > 0)
				kill(alarms->section->GetPid(), SIGTERM);
			break;
		case TExecAlarm::ALRM_SKILL:
			assert (alarms->section);
			if (alarms->section->GetPid() > 0)
				kill(alarms->section->GetPid(), SIGKILL);
			break;
		case TExecAlarm::ALRM_REM:
			break;
	}
	tmp = alarms;
	alarms = alarms->next;
	/* if this was the end of cycle, we have to break outside
	 * the loop, so alarms will be reset */
	if (tmp->type != TExecAlarm::ALRM_NEWCL)
		NextAlarm();
	delete tmp;
	errno  = olderrno;
}

void TExecute::GetReady()
{
	time_t t;
	int ret;
	struct sigaction sa;
	sigset_t block_mask;
	
	g_next_cycle = 0;
	/* set signal handlers */
	sigfillset(&block_mask);
	sigdelset(&block_mask, SIGKILL);
	sigdelset(&block_mask, SIGSTOP);
	
	sa.sa_handler = g_AlarmHandler;
	sa.sa_mask = block_mask;
	sa.sa_flags = SA_RESTART;
	ret = sigaction(SIGALRM, &sa, NULL);
	assert (ret == 0);

	sa.sa_handler = g_ChildHandler;
	ret = sigaction(SIGCHLD, &sa, NULL);
	assert (ret == 0);
	
	/* alarm at the begining of next cycle */
	assert (alarms == NULL);
	time(&t);
	InsertAlarm(TExecAlarm::NewCycleAlarm(t));
	NextAlarm();
}

void TExecute::WaitForCycle()
{
	sigset_t sigmask, orig_sigmask;
	time_t t;

	sz_log(10, "execute: waiting for alarm signal");
	/* wait for signal */
	sigemptyset (&sigmask);
	sigaddset (&sigmask, SIGALRM);
	sigprocmask (SIG_BLOCK, &sigmask, &orig_sigmask);
			      
	while (!g_next_cycle) {
		sigsuspend (&orig_sigmask);
	}
	sigprocmask (SIG_UNBLOCK, &sigmask, NULL);

	g_next_cycle = 0;

	time(&t);
	InsertAlarm(TExecAlarm::NewCycleAlarm(t));
	NextAlarm();
}

int TExecute::RunCommand(char *command)
{
	pid_t pid;
	sigset_t mask;

	assert(command != NULL);
	pid = fork();

	if (pid == 0) {
		/* child process */
		int file, errfile;
		/* redirect stdout */
		file = TEMP_FAILURE_RETRY (open(stdout_file, 
					O_RDWR | O_CREAT | O_APPEND, 0644));
		if (file < 0) {
			sz_log(1, "execute: cannot redirect output for command '%s' - cannot open file '%s', errno %d",
					command, stdout_file, errno);
			_exit(EXIT_FAILURE);
		}
		if (dup2(file, STDOUT_FILENO) < 0) {
			sz_log(1, "execute: cannot redirect output for command '%s' - cannot duplicate fd %d, errno %d",
					command, file, errno);
			_exit(EXIT_FAILURE);
		}
		/* redirect stderr */
		errfile = TEMP_FAILURE_RETRY (open(stderr_file, 
					O_RDWR | O_CREAT | O_APPEND, 0644));
		if (errfile < 0) {
			sz_log(1, "execute: cannot redirect stderr for command '%s' - cannot open file '%s', errno %d",
					command, stderr_file, errno);
			_exit(EXIT_FAILURE);
		}
		if (dup2(errfile, STDERR_FILENO) < 0) {
			sz_log(1, "execute: cannot redirect stderr for command '%s' - cannot duplicate fd %d, errno %d",
					command, errfile, errno);
			_exit(EXIT_FAILURE);
		}

		// unblock all signals
		sigemptyset(&mask);
		sigprocmask(SIG_SETMASK, &mask, NULL);
		
		execlp(SHELL, SHELL, "-c", command, NULL);
		sz_log(1, "execute: error executing command '%s', errno is '%d'",
				command, errno);
		_exit(EXIT_FAILURE);
	}
	if (pid < 0) {
		/* error */
		sz_log(1, "execute: error forking, errno is %d", errno);
		return -1;
	}
	return pid;
}

void TExecute::InsertAlarm(TExecAlarm *alarm)
{
	assert (alarm != NULL);
	assert (alarm->next == NULL);

	TExecAlarm *tmp;

	if (alarms == NULL) {
		/* initialize empty list */
		alarms = alarm;
		return;
	}
	if (alarm->exptime < alarms->exptime) {
		alarm->next = alarms;
		alarms = alarm;
		if (alarms->next->isset) {
			/* replace current alarm */
			alarms->next->isset = 0;
			NextAlarm();
		}
		return;
	}
	if (!alarms->isset && (alarm->exptime < alarms->exptime)) {
		/* set alarm as first only if first is currently not active */
		alarm->next = alarms;
		alarms = alarm;
		return;
	}
	/* find correct position in list */
	for (tmp = alarms; tmp->next &&( tmp->next->exptime <= alarm->exptime); 
			tmp = tmp->next)
		;
	/* insert into list */
	alarm->next = tmp->next;
	tmp->next = alarm;
}

int TExecute::CreateSection(char *timestr, char *commandstr,
		char *retrystr, char *parallelstr,
		char *limitstr)
{
	char *tmp1, *tmp2;
	ExecTimeType wday, month, day, hour, min10;
	int retry = 0, parallel = 0, limit;
	TExecSection *section;

	assert(commandstr != NULL);
	assert(timestr != NULL);
	
	tmp1 = timestr;
	
	tmp2 = strsep(&tmp1, " ");
	if (tmp2 == NULL) {
		sz_log(1, "execute: parsing time: week-day string empty");
		return 1;	
	}
	if (parse_cron_time(tmp2, &wday, 7)) {
		sz_log(1, "execute: parsing time: error parsing week-day (%s)", tmp2);
		return 1;
	}

	tmp2 = strsep(&tmp1, " ");
	if (tmp2 == NULL) {
		sz_log(1, "execute: parsing time: month string empty");
		return 1;	
	}
	if (parse_cron_time(tmp2, &month, 12)) {
		sz_log(1, "execute: parsing time: error parsing month (%s)", tmp2);
		return 1;
	}
	
	tmp2 = strsep(&tmp1, " ");
	if (tmp2 == NULL) {
		sz_log(1, "execute: parsing time: day string empty");
		return 1;	
	}
	if (parse_cron_time(tmp2, &day, 31)) {
		sz_log(1, "execute: parsing time: error parsing day (%s)", tmp2);
		return 1;
	}
	
	tmp2 = strsep(&tmp1, " ");
	if (tmp2 == NULL) {
		sz_log(1, "execute: parsing time: hour string empty");
		return 1;	
	}
	if (parse_cron_time(tmp2, &hour, 23, 0)) {
		sz_log(1, "execute: parsing time: error parsing hour (%s)", tmp2);
		return 1;
	}
	
	tmp2 = strsep(&tmp1, " ");
	if (tmp2 == NULL) {
		sz_log(1, "execute: parsing time: minutes string empty");
		return 1;	
	}
	if (parse_cron_time(tmp2, &min10, 5, 0, 1)) {
		sz_log(1, "execute: parsing time: error parsing minutes (%s)", tmp2);
		return 1;
	}

	if (retrystr != NULL) {
		retry = strtol(retrystr, &tmp1, 0);
		if (*tmp1 != 0) {
			sz_log(1, "execute: error parsing retry string (%s)", retrystr);
			return 1;
		}
		if (retry < 0) {
			sz_log(1, "execute: 'retry' value must be non-negative (%d)", retry);
			return 1;
		}
	}

	if (limitstr != NULL) {
		limit = strtol(limitstr, &tmp1, 0);
		if (*tmp1 != 0) {
			sz_log(1, "execute: error parsing limit string (%s)", limitstr);
			return 1;
		}
		if (limit < - (BASE_PERIOD - 1)) {
			sz_log(1, "execute: limit value must be grater then -%d (%d)",
					BASE_PERIOD-1, limit);
			return 1;
		}
		if (limit < 0) {
			limit = BASE_PERIOD - limit;
		}
	} else
		limit = BASE_PERIOD - 1;

	if ((parallelstr != NULL) && !strcmp(parallelstr, "yes"))
		parallel = 1;

	section = new TExecSection(commandstr, wday, month, day, hour, min10,
			retry, parallel, limit);

	if (sections == NULL)
		sections = section;
	else
		sections->Append(section);
	
	return 0;
}

int TExecute::LoadSectionConfig(char *section)
{
	char *timestr, *command, *retry, *parallel, *limit;
	int ret;

	timestr = libpar_getpar(section, "time", 0);
	if (timestr == NULL) {
		sz_log(1, "execute: parametr 'time' not found in section '%s', section ignored", section);
		return 0;
	}

	command = libpar_getpar(section, "command_line", 0);
	if (command == NULL) {
		sz_log(1, "execute: parametr 'command_line' not found in section '%s', section ignored", section);
		free(timestr);
		return 0;
	}

	retry = libpar_getpar(section, "retry", 0);
	parallel = libpar_getpar(section, "parallel", 0);
	limit = libpar_getpar(section, "limit", 0);

	ret = CreateSection(timestr, command, retry, parallel, limit);
	
	free(timestr);
	free(command);
	if (retry)
		free(retry);
	if (parallel)
		free(parallel);
	if (limit)
		free(limit);
	
	if (ret != 0) {
		sz_log(1, "execute: error loading configuration for section '%s', section ignored",
				section);
	}
	return 0;
}

void TExecute::NextAlarm()
{
	time_t t;
	struct itimerval timer;
	int ret;
	TExecAlarm *a;

	/* skip null alarms */
	while (alarms && (alarms->type == TExecAlarm::ALRM_REM)) {
		a = alarms;
		alarms = alarms->next;
		delete a;
	}
	
	if (alarms == NULL)
		return;
	assert(alarms->isset == 0);
	
	time(&t);
	timer.it_interval.tv_sec = timer.it_interval.tv_usec = 0;
	/* if alarm already expired raise it as soon as possible */
	if ((timer.it_value.tv_sec = alarms->exptime - t) <= 0) {
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 10;
	} else
		timer.it_value.tv_usec = 0;
	alarms->isset = 1;
	/* set alarm */
	sz_log(10, "execute: setting alarm in %ld seconds", 
			timer.it_value.tv_sec);
	ret = setitimer(ITIMER_REAL, &timer, NULL);
	assert(ret == 0);
}

