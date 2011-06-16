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
 * @file basedmn.h
 * @brief Base class for line daemons.
 * 
 * Contains basic functions that may be useful.
 *
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-16
 */

#include <ctime>

#include <dmncfg.h>
#include <ipchandler.h>

const int DEFAULT_EXPIRE = 660;	/* 11 minutes */
const int DAEMON_INTERVAL = 10;

class BaseDaemon {
public:
	BaseDaemon( const char* name );
	virtual ~BaseDaemon();


	int Init(int argc, const char *argv[]);

	/** 
	 * @brief Wait for next cycle. 
	 */
	virtual void Wait();

	virtual void Read() = 0;
	virtual int ParseConfig(DaemonConfig * cfg) = 0;

	virtual void Transfer();

	const char* Name() { return name; }

protected:
	int Init( const char*name , int argc, const char *argv[] );
	virtual int InitConfig( const char*name , int argc, const char *argv[] );
	virtual int InitIPC();
	virtual int InitSignals();

	unsigned int Length();
	void Set( unsigned int i , short val );
	short At( unsigned int i );

	DaemonConfig *cfg;
	IPCHandler   *ipc;
	time_t m_last;

	const char* name;
};

