/* 
 * SZARP: SCADA software 
 *
 * Copyright (C) 
 * 2011 - Jakub Kotur <qba@newterm.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */
/** 
 * @file basedmn.cc
 * @brief Implementatio of BaseDaemon class
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-16
 */

#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <liblog.h>

#include "basedmn.h"

BaseDaemon::BaseDaemon( const char* name )
	: m_last(0) , name(name) 
{
}

BaseDaemon::~BaseDaemon()
{
}

int BaseDaemon::Init(int argc, const char *argv[]) 
{
	return Init(name,argc,argv);
}

int BaseDaemon::Init( const char*name , int argc, const char *argv[] )
{
	return InitConfig(name,argc,argv)
	    || InitIPC()
	    || InitSignals();
}

int BaseDaemon::InitConfig( const char*name , int argc, const char *argv[] )
{
	assert( cfg = new DaemonConfig(name) );

	if( cfg->Load(&argc, const_cast<char**>(argv)) ) {
		return 1;
	}

	if( ParseConfig(cfg) ) {
		return 1;
	}

	return 0;
}

int BaseDaemon::InitIPC()
{
	assert( ipc = new IPCHandler(cfg) );

	if( !cfg->GetSingle() && ipc->Init() ) {
		return 1;
	}

	return 0;
}

RETSIGTYPE terminate_handler(int signum)
{
	sz_log(2, "signal %d cought, exiting", signum);
	signal(signum, SIG_DFL);
	raise(signum);
}

int BaseDaemon::InitSignals()
{
	int ret;
	struct sigaction sa;
	sigset_t block_mask;

	sigfillset(&block_mask);
	sigdelset(&block_mask, SIGKILL);
	sigdelset(&block_mask, SIGSTOP);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = terminate_handler;
	ret = sigaction(SIGTERM, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	assert (ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	assert (ret == 0);

	return 0;
}

void BaseDaemon::Wait()
{
	time_t t;
	t = time(NULL);
	
	if (t - m_last < DAEMON_INTERVAL) {
		int i = DAEMON_INTERVAL - (t - m_last);
		while (i > 0) {
			i = sleep(i);
		}
	}
	m_last = time(NULL);
	return;
}

void BaseDaemon::Transfer()
{
	ipc->GoParcook();
}

void BaseDaemon::Set( unsigned int i , short val )
{
	ipc->m_read[i] = val;
}

short BaseDaemon::At( unsigned int i )
{
	return ipc->m_read[i];
}

unsigned int BaseDaemon::Length()
{
	return ipc->m_params_count;
}

