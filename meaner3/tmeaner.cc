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


TMeaner::TMeaner(TStatus *status)
{
	assert (status != NULL);
	parcook = new TParcook();
	executer = new TExecute(status);
	config = new TSzarpConfig();
	params = NULL;
	params_len = 0;
	sparams = NULL;
	sparams_len = 0;
	this->status = status;
}

TMeaner::~TMeaner()
{
	delete parcook;
	delete config;
	if (params) {
		for (int i = 0; i < params_len; i++)
			delete params[i];
		delete[] params;
	}
	if (sparams) {
		for (int i = 0; i < sparams_len; i++)
			delete sparams[i];
		delete[] sparams;
	}
	delete executer;
	g_meaner = NULL;
}

int TMeaner::LoadConfig()
{
	char* c;
	int l;
	
	/* Search only for 'system wide' config file, exit on error. */
	libpar_init_with_filename(SZARP_CFG, 1);
	/* Check if base_format is set to 'szarpbase' */
	c = libpar_getpar(SZARP_CFG_SECTION, "base_format", 0);
	if ((c == NULL) || (strcmp(c, "szarpbase"))) {
		sz_log(0, "meaner3: set 'base_format' to 'szarpbase' in "SZARP_CFG" to\
 run meaner3");
		return 1;
	}
	free(c);
	
	char *_ipk_path = libpar_getpar(SZARP_CFG_SECTION, "IPK", 0);
	if (_ipk_path == NULL) {
		sz_log(0, "meaner3: set 'IPK' param in "SZARP_CFG" file");
		return 1;
	}
	ipk_path = SC::A2S(_ipk_path);
	free(_ipk_path);
	
	char* _data_dir = libpar_getpar(SZARP_CFG_SECTION, "datadir", 0);
	if (_data_dir  == NULL) {
		sz_log(0, "meaner3: set 'datadir' param in "SZARP_CFG" file");
		return 1;
	}
	data_dir = SC::A2S(_data_dir);
	free(_data_dir);

	c = libpar_getpar(SZARP_CFG_SECTION, "log_level", 0);
	if (c == NULL)
		l = 2;
	else {
		l = atoi(c);
		free(c);
	}
	
	c = libpar_getpar(SZARP_CFG_SECTION, "log", 0);
	if (c == NULL)
		c = strdup(PREFIX"/logs/meaner3");
	l = loginit(l, c);
	if (l < 0) {
		sz_log(0, "meaner3: cannot inialize log file %s, errno %d", c, errno);
		free(c);
		return 1;
	}
	free(c);

	c = libpar_getpar(SZARP_CFG_SECTION, "bodas_stdout", 0);
	if (c == NULL)
		c = strdup(PREFIX"/logs/bodas.stdout");

	if (parcook->LoadConfig() != 0)
		return 1;

	if (executer->LoadConfig() != 0)
		return 1;
	
	return 0;
}

int TMeaner::CheckBase()
{
	struct stat buf;
	struct statvfs buffs;
	int i;
	uid_t id, gid;

	/* search for data_dir directory */
	assert(!data_dir.empty());
	i = stat(SC::S2A(data_dir).c_str(), &buf);
	if (i < 0) { 
		sz_log(0, "meaner3: cannot stat data_dir '%ls', errno %d", 
				data_dir.c_str(), errno); 
		return 1;
	}
	if (!S_ISDIR(buf.st_mode)) {
		sz_log(0, "meaner3: data_dir '%ls' is not a directory", data_dir.c_str());
		return 1;
	}
	/* check for write permisions */
	id = geteuid();
	sz_log(5, "meaner3: running with effective user id %d", id);
	if (((id == buf.st_uid) || (id == 0))
			&& ((buf.st_mode & S_IRWXU) == S_IRWXU))
		goto perms_ok;
	gid = getegid();
	sz_log(5, "meaner3: running with effective group id %d", gid);
	if ((gid == buf.st_gid) && ((buf.st_mode & S_IRWXG) == S_IRWXG))
		goto perms_ok;
	if ((buf.st_mode & S_IRWXO) == S_IRWXO)
		goto perms_ok;
	sz_log(0, "meaner3: not enough permissions to use data dir '%ls'",
			data_dir.c_str());
perms_ok:
	/* check for free space on device */
	i = statvfs(SC::S2A(data_dir).c_str(), &buffs);
	if (i != 0) {
		sz_log(0, "meaner3: cannot stat data_dir '%ls' file system", 
				data_dir.c_str());
		return 1;
	}
	if (id == 0)
		i = buffs.f_bfree;
	else
		i = buffs.f_bavail;	/* blocks available for non-root users */
	if (i < MIN_BLOCKS_AV) {
		sz_log(0, "meaner3: not enough blocks available on file system (%d)",
				i);
		return 1;
	}
	sz_log(5, "meaner3: BaseCheck() OK");
	return 0;
}

int TMeaner::LoadIPK()
{
	TParam * p;
	int c = 0;
	
	xmlInitParser();
	
	int i = config->loadXML(ipk_path);
	if (i != 0) {
		sz_log(0, "meaner3: error loading configuration from file %ls",
				ipk_path.c_str());
		xmlCleanupParser();
		return 1;
	}
	sz_log(5, "meaner3: IPK file %ls successfully loaded", ipk_path.c_str());
	
	params_len = config->GetParamsCount() + config->GetDefinedCount();
	sparams_len = status->GetCount();
	params = new TSaveParam *[params_len];
	assert (params != NULL);
	sparams = new TSaveParam *[sparams_len];
	sz_log(5, "meaner3: total %d params found", params_len);
	for (p = config->GetFirstParam(), i = 0; p && (i < params_len);
			p = config->GetNextParam(p), i++)
	{
		if (p->IsInBase()) {
			sz_log(10, "meaner3: seting up parameter '%ls'", p->GetName().c_str());
			params[i] = new TSaveParam(p);
			c++;
		} else
			params[i] = NULL;
	}
	sz_log(5, "meaner3: %d params to save", c);
	/* configure status parameters */
	for (int j = 0 ; j < sparams_len; j++)
		sparams[j] = new TSaveParam(status->GetName((TStatus::ParamType)j));
	/* save stats */
	status->Set(TStatus::PT_PARAMS, config->GetParamsCount() + config->GetDefinedCount()
			+ config->GetDrawDefinableCount());
	status->Set(TStatus::PT_DEVPS, config->GetParamsCount());
	status->Set(TStatus::PT_DEFPS, config->GetDefinedCount());
	status->Set(TStatus::PT_DDPS, config->GetDrawDefinableCount());
	status->Set(TStatus::PT_BASEPS, c);
	
	xmlCleanupParser();

	return 0;
}

int TMeaner::InitSignals()
{
	int ret;
	struct sigaction sa;
	sigset_t block_mask;
	
	sigfillset(&block_mask);
	sigdelset(&block_mask, SIGKILL);
	sigdelset(&block_mask, SIGSTOP);
	/* set no-cleanup handlers for critical signals */
	sa.sa_handler = g_CriticalHandler;
	sa.sa_mask = block_mask;
	sa.sa_flags = 0;
	ret = sigaction(SIGFPE, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGQUIT, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGILL, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGSEGV, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGBUS, &sa, NULL);
	assert (ret == 0);
	/* cleanup handlers for termination signals */
	sa.sa_handler = g_TerminateHandler;
	sa.sa_flags = SA_RESTART;
	ret = sigaction(SIGTERM, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	assert (ret == 0);
	return 0;
}

int TMeaner::InitReader()
{
	return parcook->Init(params_len);
}
		
void TMeaner::InitExec()
{
	executer->GetReady();
}

void TMeaner::ReadParams()
{
	parcook->GetValues();
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

