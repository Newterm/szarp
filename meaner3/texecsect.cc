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

#include "texecsect.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "liblog.h"

#include "meaner3.h"
#include "texecute.h"

TExecSection::TExecSection(char *command,
		ExecTimeType wday,
		ExecTimeType month,
		ExecTimeType day,
		ExecTimeType hour,
		ExecTimeType min10,
		int retry,
		int parallel,
		int limit)
{
	this->command = strdup(command);
	this->wday = wday;
	this->month = month;
	this->day = day;
	this->hour = hour;
	this->min10 = min10;
	this->retry = retry;
	this->parallel = parallel;
	this->limit = limit;
	
	next = NULL;
	fails = 0;
	pid = 0;
	torun = 0;
	
	sz_log(10, "execute: section created, command '%s', time is %o %o %o %o %o, retry %d, parallel %d, limit %d", 
			command, wday, month, day, hour, min10, retry, parallel,
			limit);
}

TExecSection::~TExecSection()
{
	if (command)
		free(command);
}

void TExecSection::Append(TExecSection *section)
{
	assert (section != NULL);

	TExecSection *tmp = this;

	while (tmp->next != NULL)
		tmp = tmp->next;

	tmp->next = section;
}

pid_t TExecSection::GetPid()
{
	return pid;
}

void TExecSection::ClearPid()
{
	assert (pid != 0);
	pid = 0;
}

char *TExecSection::GetCommand()
{
	return command;
}

int TExecSection::GetFails()
{
	return fails;
}

TExecSection *TExecSection::GetNext()
{
	return next;
}

void TExecSection::CheckRetry()
{
	if (retry > 0) {
		if (fails < retry)
			fails++;
		else
			fails = 0;
	}
}

void TExecSection::MarkRun(int torun)
{
	this->torun = torun;
}

int TExecSection::CheckRun()
{
	return (torun && (pid == 0));
}

int TExecSection::CheckTime(int wday, int month, int mday, int hour, int min10)
{
	sz_log(10, "execute: CheckTime(%d, %d, %d, %d, %d) <-> %o %o %o %o %o",
			wday, month, mday, hour, min10,
			this->wday, this->month, this->day,
			this->hour, this->min10);
	if (((this->wday & (1 << wday)) == 0) ||
			((this->month & (1 << month)) == 0) ||
			((this->day & (1 << mday)) == 0) ||
			((this->hour & (1 << hour)) == 0) ||
			((this->min10 & (1 << min10)) == 0))
		return 0;
	else
		return 1;
}

int TExecSection::CheckParallel()
{
	return parallel;
}

void TExecSection::Launch()
{
	sigset_t sigmask, oldsigmask;
	time_t t;
	
	assert (pid == 0);
	/* block signals while manipulating alarms list */
	sigemptyset (&sigmask);
	sigaddset (&sigmask, SIGALRM);
	sigaddset (&sigmask, SIGCHLD);
	sigaddset (&sigmask, SIGTERM);
	sigprocmask (SIG_BLOCK, &sigmask, &oldsigmask);

	sz_log(10, "execute: running command '%s'", command);
	time(&t);
	pid = g_exec->RunCommand(command);
	if (pid > 0)
		SetAlarm(t);
			      
	/* unblock signals */
	sigprocmask (SIG_SETMASK, &oldsigmask, NULL);
}

void TExecSection::SetAlarm(time_t t)
{
	assert (limit >= 0);
	if (limit == 0)
		return;
	t = t - (t % BASE_PERIOD) + limit;
	g_exec->InsertAlarm(TExecAlarm::NewTermAlarm(t-1, this));
	g_exec->InsertAlarm(TExecAlarm::NewKillAlarm(t, this));
}

