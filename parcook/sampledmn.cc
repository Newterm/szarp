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

class SampleDaemon {
public:
	SampleDaemon(BaseDaemon& _base_dmn) {
		_base_dmn.setCycleHandler([this](BaseDaemon& base_dmn){ Read(base_dmn); });
	}

	void Read(BaseDaemon& base_dmn)
	{
		const auto params_count = base_dmn.getDaemonCfg().GetParamsCount();
		for (unsigned int i=0; i < params_count; ++i)
			base_dmn.getIpc().setRead(i, 1);
		base_dmn.getIpc().publish();
	}
};

int main(int argc, const char *argv[])
{
	BaseDaemonFactory::Go<SampleDaemon>(argc, argv, "kamsdmn");
	return 0;
}

