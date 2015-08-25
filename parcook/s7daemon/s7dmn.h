/* 
 * SZARP: SCADA software 
 *
 * Copyright (C) 
 * 2015 - Marcin Harasimczuk <hary@newterm.pl>
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

#ifndef S7DAEMON_H
#define S7DAEMON_H

/** 
 * @file s7dmn.h
 * @brief Extends BaseDaemon and lends its database write utility method to
 * lower level modules (S7 client and SZARP param map).
 *
 * Configures both the S7 controller client and SZARP params map. Implements
 * main read/write sequence and loop.
 *
 * @author Marcin Harasimczuk <hary@newterm.pl>
 * @version 0.1
 * @date 2015-08-25
 */

#include "s7client.h"
#include "szpmap.h"

class S7Daemon : public BaseDaemon
{
public:
	S7Daemon() : BaseDaemon("s7dmn") {};
	virtual ~S7Daemon() {};

	virtual int Read();
	virtual void Transfer();

protected:
	virtual int ParseConfig(DaemonConfig * cfg);
	
	S7Client _client;
	SzParamMap _pmap;

};

#endif /*S7DAEMON_H*/
