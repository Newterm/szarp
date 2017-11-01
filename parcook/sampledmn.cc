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
 * @file sampledmn.cc
 * @brief Daemon for remote logging users activity.
 * @author Jakub Kotur <qba@newterm.pl>
 * @version 0.1
 * @date 2011-06-15
 */
/*
 @description_start
 @class 4

 @devices None -- always outputs 1
 @devices.pl ¿adne -- jedynie wypluwa 1
 @config None
 @config.pl ¿adna

 @description_end
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <liblog.h>

#include "base_daemon.h"

class SampleDaemon : public BaseDaemon {
public:
	SampleDaemon() : BaseDaemon("sampledmn")
	{
	}

	virtual ~SampleDaemon()
	{
	}

	virtual int Read()
	{
		for( unsigned int i=0 ; i<Count() ; ++i )
			Set( i , 1 );
		return 0;
	}

protected :
	virtual int ParseConfig(DaemonConfig * cfg)
	{
		return 0;
	}
};

int main(int argc, const char *argv[])
{
	SampleDaemon dmn;

	if( dmn.Init( argc , argv ) ) {
		sz_log(1,"Cannot start %s daemon",dmn.Name());
		return 1;
	}

	sz_log(2, "Starting %s",dmn.Name());

	for(;;)
	{
		dmn.Wait();
		dmn.Read();
		dmn.Transfer();
	}

	return 0;
}

