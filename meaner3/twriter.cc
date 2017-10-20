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
 * twriter.cc - common code for writing probes to database
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 */

#include "twriter.h"
#include "conversion.h"

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "liblog.h"
#include "libpar.h"


TWriter::TWriter(ProbesType type)
{
	parcook = new TParcook(type);
	config = new TSzarpConfig();
	params = NULL;
	params_len = 0;
}

TWriter::~TWriter()
{
	delete parcook;
	delete config;
	if (params) {
		for (int i = 0; i < params_len; i++)
			delete params[i];
		delete[] params;
	}
}

int TWriter::LoadConfig(const char *section, const char* datadir_param)
{
	char* c;
	int l;
	
	/* Search only for 'system wide' config file, exit on error. */
	libpar_init();
	
	char *_ipk_path = libpar_getpar(section, "IPK", 0);
	if (_ipk_path == NULL) {
		sz_log(1, "TWriter::LoadConfig(): set 'IPK' param in szarp.cfg file");
		return 1;
	}
	ipk_path = SC::L2S(_ipk_path);
	free(_ipk_path);
	
	char* _data_dir = libpar_getpar(section, datadir_param, 0);
	if (_data_dir  == NULL) {
		sz_log(1, "TWriter::LoadConfig(): set '%s' param in szarp.cfg file", datadir_param);
		return 1;
	}
	data_dir = SC::L2S(_data_dir);
	free(_data_dir);

	c = libpar_getpar(section, "log_level", 0);
	if (c == NULL)
		l = 2;
	else {
		l = atoi(c);
		free(c);
	}
	
	c = libpar_getpar(section, "log", 0);
	l = sz_loginit(l, c);
	if (c != NULL)
		free(c);

	if (parcook->LoadConfig() != 0)
		return 1;

	return 0;
}

int TWriter::CheckBase(unsigned long req_free_space)
{
	struct stat buf;
	struct statvfs buffs;
	int i;
	uid_t id, gid;

	/* search for data_dir directory */
	assert(!data_dir.empty());
	i = stat(SC::S2A(data_dir).c_str(), &buf);
	if (i < 0) { 
		sz_log(1, "TWriter::CheckBase(): cannot stat data_dir '%s', errno %d", 
				SC::S2A(data_dir).c_str(), errno); 
		return 1;
	}
	if (!S_ISDIR(buf.st_mode)) {
		sz_log(1, "TWriter::CheckBase(): data_dir '%s' is not a directory", SC::S2A(data_dir).c_str());
		return 1;
	}
	/* check for write permisions */
	id = geteuid();
	sz_log(5, "TWriter::CheckBase(): running with effective user id %d", id);
	if (((id == buf.st_uid) || (id == 0))
			&& ((buf.st_mode & S_IRWXU) == S_IRWXU))
		goto perms_ok;
	gid = getegid();
	sz_log(5, "TWriter::CheckBase(): running with effective group id %d", gid);
	if ((gid == buf.st_gid) && ((buf.st_mode & S_IRWXG) == S_IRWXG))
		goto perms_ok;
	if ((buf.st_mode & S_IRWXO) == S_IRWXO)
		goto perms_ok;
	sz_log(1, "TWriter::CheckBase(): not enough permissions to use data dir '%s'",
			SC::S2A(data_dir).c_str());
	return 1;
perms_ok:
	/* check for free space on device */
	i = statvfs(SC::S2A(data_dir).c_str(), &buffs);
	if (i != 0) {
		sz_log(1, "TWriter::CheckBase(): cannot stat data_dir '%s' file system", 
				SC::S2A(data_dir).c_str());
		return 1;
	}
	if (id == 0)
		i = buffs.f_bfree;
	else
		i = buffs.f_bavail;	/* blocks available for non-root users */
	if (i * buffs.f_bsize < req_free_space) {
		sz_log(1, "TWriter::CheckBase(): not enough blocks available on file system (%d)",
				i);
		return 1;
	}
	sz_log(5, "TWriter::CheckBase(): OK");
	return 0;
}

int TWriter::LoadIPK()
{
	TParam * p;
	int c = 0;
	
	xmlInitParser();
	
	int i = config->loadXML(ipk_path);
	if (i != 0) {
		sz_log(1, "TWriter::LoadIPK(): error loading configuration from file %s",
				SC::S2A(ipk_path).c_str());
		xmlCleanupParser();
		return -1;
	}
	sz_log(5, "TWriter::LoadIPK(): IPK file %s successfully loaded", SC::S2A(ipk_path).c_str());
	
	params_len = config->GetParamsCount() + config->GetDefinedCount();
	params = new TSaveParam *[params_len];
	assert (params != NULL);
	sz_log(5, "TWriter::LoadIPK(): total %d params found", params_len);
	for (p = config->GetFirstParam(), i = 0; p && (i < params_len);
			p = p->GetNextGlobal(), i++)
	{
		if (p->IsInBase()) {
			sz_log(10, "TWriter::LoadIPK(): setting up parameter '%s'", SC::S2A(p->GetName()).c_str());
			params[i] = new TSaveParam(p);
			c++;
		} else
			params[i] = NULL;
	}
	sz_log(5, "TWriter::LoadIPK(): %d params to save", c);

	xmlCleanupParser();

	return c;
}

int TWriter::InitSignals(void (*critical_handler)(int), void (*term_handler)(int))
{
	int ret;
	struct sigaction sa;
	sigset_t block_mask;
	
	sigfillset(&block_mask);
	sigdelset(&block_mask, SIGKILL);
	sigdelset(&block_mask, SIGSTOP);
	/* set no-cleanup handlers for critical signals */
	sa.sa_handler = critical_handler;
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
	sa.sa_handler = term_handler;
	sa.sa_flags = SA_RESTART;
	ret = sigaction(SIGTERM, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	assert (ret == 0);
	return 0;
}

int TWriter::InitReader()
{
	return parcook->Init(params_len);
}
		
void TWriter::ReadParams()
{
	parcook->GetValues();
}


