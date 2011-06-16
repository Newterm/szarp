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
 * tmeaner.cc
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#include "meaner3.h"
#include "tmeaner.h"
#include "conversion.h"

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>

#include "liblog.h"
#include "libpar.h"


TMeaner::TMeaner(TStatus *status) : TWriter(min10)
{
	assert (status != NULL);
	executer = new TExecute(status);
	config = new TSzarpConfig( true );
	sparams = NULL;
	sparams_len = 0;
	this->status = status;
}

TMeaner::~TMeaner()
{
	if (sparams) {
		for (int i = 0; i < sparams_len; i++)
			delete sparams[i];
		delete[] sparams;
	}
	delete executer;
	g_meaner = NULL;
}

int TMeaner::LoadConfig(const char *section)
{
	if (TWriter::LoadConfig(section) != 0) {
		return 1;
	}
	
	if (executer->LoadConfig() != 0)
		return 1;
	
	return 0;
}

int TMeaner::LoadIPK()
{
	int c = TWriter::LoadIPK();
	if (c == -1) {
		return 1;
	}
	/* configure status parameters */
	sparams_len = status->GetCount();
	sparams = new TSaveParam *[sparams_len];
	for (int j = 0 ; j < sparams_len; j++)
		sparams[j] = new TSaveParam(status->GetName((TStatus::ParamType)j));
	/* save stats */
	status->Set(TStatus::PT_PARAMS, config->GetParamsCount() + config->GetDefinedCount()
			+ config->GetDrawDefinableCount());
	status->Set(TStatus::PT_DEVPS, config->GetParamsCount());
	status->Set(TStatus::PT_DEFPS, config->GetDefinedCount());
	status->Set(TStatus::PT_DDPS, config->GetDrawDefinableCount());
	status->Set(TStatus::PT_BASEPS, c);
	
	return 0;
}

void TMeaner::InitExec()
{
	executer->GetReady();
}

void TMeaner::WriteParams()
{
	time_t t;
	int err = 0, ok = 0;

	time(&t);
	t = t - (t % BASE_PERIOD);

	/* block signals */
	g_signals_blocked = 1;
	
	for (int i = 0; i < params_len; i++) {
		if (params[i] != NULL) {
			sz_log(10,"TMeaner: writing param %ls with value %d",params[i]->GetName().c_str() , parcook->GetData(i));
			if (params[i]->Write(fs::wpath(data_dir), t, parcook->GetData(i),
						status) != 0) {
				err++;
			} else {
				ok++;
			}
		}
	}
	sz_log(5, "meaner3: writing params: %d ok, %d errors",
			ok, err);
	status->Set(TStatus::PT_OKPS, ok);
	status->Set(TStatus::PT_ERRPS, err);
	for (int i = 0; i < sparams_len; i++) {
		if (status->WriteParam((TStatus::ParamType)i, t, sparams[i], data_dir) != 0)
			sz_log(1, "meaner3: error writing status param with index %d", i);
	}
	if (ok == 0) 
		sz_log(1, "meaner3: no params written, increase log level and consult previous messages");
	else if (err > 0)
		sz_log(1, "meaner3: %d error while writing params", err);
	/* unblock signals */
	g_signals_blocked = 0;
	if (g_should_exit) {
		/* block signals */
		sigset_t sigmask;
		sigfillset(&sigmask);
		sigdelset(&sigmask, SIGKILL);
		sigdelset(&sigmask, SIGSTOP);
		sigprocmask(SIG_SETMASK, &sigmask, NULL);
		/* call terminate handler */
		g_TerminateHandler(0);
		/* unblock terminate signal raised by g_TerminateHandler */
		sigemptyset(&sigmask);
		sigaddset(&sigmask, SIGTERM);
		sigprocmask(SIG_UNBLOCK, &sigmask, NULL);
	}
}
		
TExecute *  TMeaner::GetExec()
{
	return executer;
}

