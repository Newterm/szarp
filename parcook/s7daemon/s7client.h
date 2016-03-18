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

#ifndef S7CLIENT_H 
#define S7CLIENT_H

/** 
 * @file s7client.h
 * @brief Hides all details of Snap7 communication.
 *
 * Translate requests for SZARP params into
 * S7 queries and translate raw data blocks
 * into valid SZARP data formats.
 *
 * @author Marcin Harasimczuk <hary@newterm.pl>
 * @version 0.1
 * @date 2015-08-12
 */

#include <snap7.h>
#include <string>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xmlutils.h"

#include "s7qmap.h"

#include "liblog.h"
#include "../base_daemon.h"

class S7Client 
{
public:
	static const int _default_rack;
	static const int _default_slot;
	static const std::string _default_address;

	S7Client() : _address(_default_address), _rack(_default_rack), _slot(_default_slot)
	{ _s7client = Cli_Create(); }
	
	~S7Client()
	{ Cli_Destroy(&_s7client); }

	bool ConfigureFromXml( xmlNodePtr& node );
	bool ConfigureParamFromXml( unsigned long int idx, TParam* p, xmlNodePtr& node );
	bool ConfigureParamFromXml( unsigned long int idx, TSendParam* p, xmlNodePtr& node );
	S7QueryMap::S7Param ConfigureParamFromXml( unsigned long int idx, xmlNodePtr& node );

	bool Connect();
	bool Reconnect();
	bool IsConnected();

	bool BuildQueries();

	template <typename ResponseProcessor>
	void ProcessResponse(ResponseProcessor proc)
	{ _s7qmap.ProcessResponse(proc); }

	template <typename DataAccessor>
	void AccessData(DataAccessor access)
	{ _s7qmap.AccessData(access); }

	bool ClearWriteNoDataFlags();

	bool QueryAll();
	bool AskAll();
	bool TellAll();
	bool DumpAll();

private:
	std::string _address;
	int _rack;
	int _slot;

	S7Object _s7client;
	S7QueryMap _s7qmap;
};

#endif /*S7CLIENT_H*/
