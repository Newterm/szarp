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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/lexical_cast.hpp>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <snap7.h>

#include <map>
#include <set>

#include "conversion.h"
#include "liblog.h"
#include "xmlutils.h"
#include "custom_assert.h"
#include "base_daemon.h"

class s7map_key
{
public:
	enum Type { SHORT, INTEGER, FLOAT };

	s7map_key(s7map_key::Type vt, int addr)
		: vtype(vt), address(addr) {} ;

	int get_address() const { return address; };
	int end_address() const
	{
		switch (vtype) {
			case SHORT:
				return address + 1;
			case INTEGER:
			case FLOAT:
				return address + 3;
		};
		return -1;
	}

	bool operator<(const s7map_key& other) const
	{
		return (end_address() < other.address);
	}

	bool operator>(const s7map_key& other) const
	{
		return ( address > other.end_address() );
	}

	bool operator==( const s7map_key& other ) const
	{
		return (address == other.address && vtype == other.vtype);
	}


	bool operator!=( const s7map_key& other) const
	{
		return vtype != other.vtype || address != other.address;
	}

	enum Type vtype;
	int address;
};

class s7map_val
{
public:
	s7map_val() : lsw_ind(-1), msw_ind(-1), lsw_param(nullptr), msw_param(nullptr) {};
	

	void set_msw_ind(int ind) { msw_ind = ind; };
	void set_lsw_ind(int ind) { lsw_ind = ind; };

	int lsw_ind;
	int msw_ind;

	IPCParamInfo* lsw_param;
	IPCParamInfo* msw_param;
};


class Snap7Daemon
{
public:
	typedef std::map<s7map_key, s7map_val> s7_db_map;
	typedef std::pair<s7map_key, s7map_val> s7_db_pair;

	Snap7Daemon(BaseDaemon& base_dmn) {
		s7client = Cli_Create();
		ParseConfig(base_dmn.getDaemonCfg());
		base_dmn.setCycleHandler([this](BaseDaemon& base_dmn){ Read(base_dmn); });
	};

	/** 
	 * @brief Reads data from device
	 */
	void Read(BaseDaemon& base_dmn);
protected:
	void ParseConfig(const DaemonConfigInfo& cfg);

	void ConfigureParam(int param_ind, IPCParamInfo* p);

	int ReadDB(BaseDaemon& base_dmn, int db, s7_db_map& ops);
	int DBVal(BaseDaemon& base_dmn, const s7map_key& vkey, s7map_val& vval, int start, char* data);

	S7Object s7client;
	std::string address;
	int rack;
	int slot;

	std::map<int, s7_db_map> m_params_map;

	char buffer[512];
};

void Snap7Daemon::ParseConfig(const DaemonConfigInfo& cfg)
{
	auto device = cfg.GetDeviceInfo();
	address = device->getAttribute<std::string>("extra:address");
	rack = device->getAttribute<int>("extra:rack", 0);
	slot = device->getAttribute<int>("extra:slot", 2);

	sz_log(9, "Configured with address: %s, rack: %d, slot: %d", address.c_str(), rack, slot);

	int i = 0;
	for (auto unit: cfg.GetUnits()) {
		for (auto param: unit->GetParams()) {
			ConfigureParam(i++, param);
		}
	}
}

void Snap7Daemon::ConfigureParam(int ind, IPCParamInfo* p)
{
	int db = p->getAttribute<int>("extra:db");
	s7_db_map& vmap = m_params_map[db];

	int address = p->getAttribute<int>("extra:address");
	auto val_type = p->getAttribute<std::string>("extra:val_type");
	auto val_op = p->getAttribute<std::string>("extra:val_op");

	sz_log(9, "ConfigureParam param %s db: %d, address: %d, val_type: %s",
		       	SC::S2U(p->GetName()).c_str(),
			db,
			address,
			val_type.c_str());

	if (val_type == "short") {
		s7map_key nkey(s7map_key::SHORT, address);
		auto it = vmap.find( nkey ) ;
		if (it == vmap.end()) {
			s7map_val& mval = vmap[nkey];
			mval.set_lsw_ind(ind - 1);
			mval.lsw_param = p;
		} else {
			std::string error_str = "Could not configure param, overlaping mapping for db: ";
			error_str += std::to_string(db);
			error_str += ", addr: ";
			error_str += std::to_string(address);
			error_str += ", type: ";
			error_str += val_type;
			throw std::runtime_error(std::move(error_str));
		}
	} else {
		s7map_key::Type vt;
	       	if (val_type == "integer") {
			vt = s7map_key::INTEGER;
		} else if (val_type == "float") {
			vt = s7map_key::FLOAT;
		} else {
			throw std::runtime_error("Could not configure param (unrecognized val_type)");
		}

		s7map_key nkey(vt, address);
		auto iret = vmap.find( nkey );
		if (iret != vmap.end() && iret->first != nkey) {
			throw std::runtime_error("Could not configure param (overlaping mappings in configuration)");
		}

		s7map_val& mval = vmap[nkey];

		if (val_op == "LSW") {
			mval.set_lsw_ind(ind - 1);
			mval.lsw_param = p;
		} else if (val_op == "MSW" ) {
			mval.set_msw_ind(ind - 1);
			mval.msw_param = p;
		} else {
			throw std::runtime_error("Could not configure param (wrong argument for attributte val_op)");
		}
	}
}

int Snap7Daemon::DBVal(BaseDaemon& base_dmn, const s7map_key& vkey, s7map_val& vval, int start, char* data)
{
	short* psv;
	int* piv;
	float* pfv;
	float fv;
	switch(vkey.vtype) {
		case s7map_key::SHORT:
			psv = reinterpret_cast<short*>(&data[vkey.address - start]);
			if (vval.lsw_ind != -1)
				base_dmn.getIpc().setRead(vval.lsw_ind, *psv);
			return 0;
		case s7map_key::INTEGER:
			piv = reinterpret_cast<int*>(&data[vkey.address - start]);
			if (vval.lsw_ind != -1) {
				base_dmn.getIpc().setRead(vval.lsw_ind, *piv & 0xFFFF);
			}
			if (vval.msw_ind != -1) {
				base_dmn.getIpc().setRead(vval.lsw_ind, ((*piv) >> 16) & 0xFFFF);
			}
			return 0;
		case s7map_key::FLOAT:
			pfv = reinterpret_cast<float*>(&data[vkey.address - start]);
			if (vval.lsw_param)
				fv = (*pfv) * exp10(vval.lsw_param->GetPrec());
			else {
				sz_log(1, "ERROR: no param configured for value");
				return 1;
			}
			piv = reinterpret_cast<int*>(&fv);
			if (vval.lsw_ind != -1) {
				base_dmn.getIpc().setRead(vval.lsw_ind, *piv & 0xFFFF);
			}
			if (vval.msw_ind != -1) {
				base_dmn.getIpc().setRead(vval.lsw_ind, ((*piv) >> 16) & 0xFFFF);
			}
			return 0;
		default:
			return 1;
	}
}

int Snap7Daemon::ReadDB(BaseDaemon& base_dmn, int db, s7_db_map& ops)
{
	int start_address = -1;
	int end_address = -1;

	for (size_t i = 0; i < 256; i++) {
		buffer[i] = 0;
	}

	for (auto it = ops.begin(); it != ops.end(); it++) {
		if (start_address == -1 || it->first.get_address() < start_address)
			start_address = it->first.get_address();

		if (end_address == 0 || end_address < it->first.end_address())
			end_address = it->first.end_address();
	}



	if (Cli_DBRead(s7client, db, start_address, end_address - start_address, (void*)buffer)) {
		sz_log(2, "Cli_DBRead returned error");
		// TODO error
		return 1;
	}


	for (auto it = ops.begin(); it != ops.end(); it++) {
		int r = DBVal(base_dmn, it->first, it->second, start_address, buffer);
		if (r)
			return r;
	}


	return 0;
}

void Snap7Daemon::Read(BaseDaemon& base_dmn)
{
	int ret;
	int connected = 0;
	for (auto it = m_params_map.begin(); it != m_params_map.end(); it++) {
		if (!Cli_GetConnected(s7client, &connected) || connected) {
			ret = Cli_ConnectTo(s7client, address.c_str(), rack, slot);
			if (ret) {
				sz_log(2, "Cannot connect to: %s", address.c_str());
				return;
			}
		}

		ret = ReadDB(base_dmn, it->first, it->second);
		if (ret) {
			return;
		}
	}


	return;
}

int main(int argc, const char *argv[])
{
	BaseDaemonFactory::Go<Snap7Daemon>(argc, argv, "snap7dmn");
	return 0;
}
