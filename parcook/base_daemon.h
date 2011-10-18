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

#include <szarp_config.h>

#include <dmncfg.h>
#include <ipchandler.h>

const int DEFAULT_EXPIRE = 660;	/* 11 minutes */
const int DAEMON_INTERVAL = 10;

/** 
 * @brief Base class for line daemons
 *
 * This class provides core functionality for implementing
 * parcook line demons
 */
class BaseDaemon {
public:
	/** 
	 * @brief Constructs class with name
	 * 
	 * @param name this name will be used by logger
	 */
	BaseDaemon( const char* name );
	virtual ~BaseDaemon();

	/** 
	 * @brief Initialize class with command line arguments
	 * 
	 * @param argc arguments count
	 * @param argv arguments
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int Init(int argc, const char *argv[] , TSzarpConfig*sz_cfg = NULL , int dev_index = -1 );

	/** 
	 * @brief Wait for next cycle. 
	 */
	virtual void Wait( time_t interval = DAEMON_INTERVAL );

	/** 
	 * @brief Reads data from device
	 */
	virtual int Read() = 0;

	/** 
	 * @brief Transfer data to parcook
	 */
	virtual void Transfer();

	/** 
	 * @brief clears all parameters with 0
	 */
	void Clear();
	/** 
	 * @brief sets all parameters to specified value.
	 * 
	 * @param val value to set
	 */
	void Clear( short val );

	/** 
	 * @return daemon name
	 */
	const char* Name() { return name; }

protected:
	/** 
	 * @brief Initialize daemon
	 * 
	 * @param name daemon name -- used by logger
	 * @param argc arguments count
	 * @param argv arguments values
	 * @param sz_cfg szarp configuration, if null read from file
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int Init( const char*name , int argc, const char *argv[], TSzarpConfig* sz_cfg = NULL , int dev_index = -1 );
	/** 
	 * @brief Creates and initialize DaemonConfig member
	 * 
	 * @param name daemon name -- used by logger
	 * @param argc arguments count
	 * @param argv arguments values
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int InitConfig( const char*name , int argc, const char *argv[], TSzarpConfig*sz_cfg , int dev_index = -1 );
	/** 
	 * @brief Creates and initialize IPCHandler member
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int InitIPC();
	/** 
	 * @brief Defines signals handlers and masks
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int InitSignals();

	/** 
	 * @brief Parse daemon configuration 
	 *
	 * This function is called at initialization with 
	 * previously constructed DaemonConfig
	 * 
	 * @param cfg daemon configuration 
	 * 
	 * @return 0 on success, 1 otherwise
	 */
	virtual int ParseConfig(DaemonConfig * cfg) = 0;

	/** 
	 * @return length of parameters
	 */
	unsigned int Count();
	/** 
	 * @brief values setter
	 * 
	 * @param i index of parameter
	 * @param val value of parameter
	 */
	void Set( unsigned int i , short val );
	/** 
	 * @brief values getter
	 *
	 * @param i index of parameter
	 * 
	 * @return value of parameter
	 */
	short At( unsigned int i );

	DaemonConfig *cfg;
	IPCHandler   *ipc;
	time_t m_last;

	const char* name;
};

