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
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <liblog.h>

#include "base_daemon.h"
#include "custom_assert.h"

BaseDaemon::BaseDaemon( const char* name )
	: m_cfg(NULL), m_last(0) , name(name) 
{
}

BaseDaemon::~BaseDaemon()
{
}

int BaseDaemon::Init(int argc, const char *argv[], TSzarpConfig* sz_cfg , int dev_index )
{
	return Init(name, argc, argv, sz_cfg, dev_index);
}

int BaseDaemon::Init( const char*name , int argc, const char *argv[], TSzarpConfig* sz_cfg , int dev_index )
{
	return InitConfig(name,argc,argv,sz_cfg,dev_index)
	    || InitIPC()
	    || InitSignals();
}

int BaseDaemon::InitConfig( const char* name , int argc, const char *argv[] , TSzarpConfig* sz_cfg , int dev_index )
{
	m_cfg = new DaemonConfig(name);
	ASSERT( m_cfg );

	if( m_cfg->Load(&argc, const_cast<char**>(argv), 1, sz_cfg, dev_index) ) {
		sz_log(1,"Cannot load dmn cfg");
		return 1;
	}

	if( ParseConfig(m_cfg) ) {
		return 1;
	}

	return 0;
}

int BaseDaemon::InitIPC()
{
	try {
		auto _ipc = std::unique_ptr<IPCHandler>(new IPCHandler(*m_cfg)); 
		ipc = _ipc.release();
	} catch (const std::exception& e) {
		sz_log(1,"Cannot init ipc %s", e.what());
		return 1;
	}

	return 0;
}

RETSIGTYPE terminate_handler(int signum)
{
	sz_log(2, "signal %d caught, exiting", signum);
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
	ASSERT(ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	ASSERT(ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	ASSERT(ret == 0);

	return 0;
}

void BaseDaemon::Wait( time_t interval )
{
	time_t t;
	t = time(NULL);
	
	if (t - m_last < interval ) {
		int i = interval - (t - m_last);
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
	ipc->GoSender();
}

void BaseDaemon::Set( unsigned int i , short val )
{
	sz_log(10, "Setting param no %d to value: %d", i, val);
	ipc->m_read[i] = val;
}

short BaseDaemon::At( unsigned int i )
{
	return ipc->m_read[i];
}

short BaseDaemon::Send( unsigned int i )
{
	return ipc->m_send[i];
}

void BaseDaemon::Clear()
{
	memset(ipc->m_read,0,sizeof(short)*Count());
}

void BaseDaemon::Clear( short val )
{
	for( int i=Count()-1 ; i>=0 ; --i ) ipc->m_read[i] = val;
}

unsigned int BaseDaemon::Count()
{
	return ipc->m_params_count;
}

