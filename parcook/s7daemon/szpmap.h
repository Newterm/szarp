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

#ifndef SZPMAP_H
#define SZPMAP_H

/** 
 * @file szpmap.h
 * @brief Handles SZARP params configuration and conversion to SZARP format.
 *
 * Translate query response data blocks into valid SZARP database writes.
 *
 * @author Marcin Harasimczuk <hary@newterm.pl>
 * @version 0.1
 * @date 2015-08-25
 */

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <utility>

#include "liblog.h"
#include "xmlutils.h"
#include "conversion.h"

#include "../base_daemon.h"

class SzParamMap 
{
public:
	typedef std::pair<unsigned long int, std::vector<uint8_t>> ParamIndexData;

	class SzParam 
	{
	public:
		SzParam() : _addr(-1), _val_type(""), _val_op(""), _prec(0) {}
		SzParam(int addr,
			std::string vtype,
			std::string vop,
			int prec) : 
			
			_addr(addr),
			_val_type(vtype),
			_val_op(vop),
			_prec(prec)
		{}

		bool isLSW()
		{ return (_val_op.compare(std::string("lsw"))==0); }
		bool isMSW()
		{ return (_val_op.compare(std::string("msw"))==0); }
		int getAddr()
		{ return _addr; }
		std::string getType()
		{ return _val_type; }
		int getPrec()
		{ return _prec; }

	private:
		int _addr;
		std::string _val_type;
		std::string _val_op;
		int _prec;
	};

	bool ConfigureParamFromXml( unsigned long int idx, TParam* p, xmlNodePtr& node );
	
	void clearData() 
	{ _lower_words.clear(); }

	bool isLSW(unsigned long int pid)
	{ return _params[pid].isLSW(); }
	bool isMSW(unsigned long int pid)
	{ return _params[pid].isMSW(); }
	bool isFloat(unsigned long int pid)
	{ return (_params[pid].getType().compare(std::string("real"))==0); }

	int getAddr(unsigned long int pid)
	{ return _params[pid].getAddr(); }
	std::string getType(unsigned long int pid)
	{ return _params[pid].getType(); }
	int getPrec(unsigned long int pid)
	{ return _params[pid].getPrec(); }
	
	void setLSW( unsigned long int pid, std::vector<uint8_t> data )
	{ _lower_words[getAddr(pid)] = ParamIndexData(pid, data); }
	
	bool hasLSW( int addr ) 
	{ return ( _lower_words.find(addr) != _lower_words.end() ); }

	template <typename DataWriter>
	void writeFloat( unsigned long int pid, std::vector<uint8_t> data, DataWriter write)
	{
		float float_value;
		
		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&float_value);
		*pblock8 = data[1]; pblock8++;
		*pblock8 = data[0]; pblock8++;
		*pblock8 = _lower_words[getAddr(pid) - 2].second[1]; pblock8++;
		*pblock8 = _lower_words[getAddr(pid) - 2].second[0];

		uint32_t float_block = (uint32_t)(float_value * pow10(getPrec(pid)));
		int16_t lsw = (int16_t) float_block;
		int16_t msw = (int16_t) (float_block >> 16);

		write(pid, msw); 
		write(_lower_words[getAddr(pid) - 2].first, lsw);
	}

	template <typename DataWriter>
	void writeInteger( unsigned long int pid, std::vector<uint8_t> data, DataWriter write )
	{
		int int_value;
		
		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&int_value);
		*pblock8 = data[1]; pblock8++;
		*pblock8 = data[0]; pblock8++;
		*pblock8 = _lower_words[getAddr(pid) - 2].second[1]; pblock8++;
		*pblock8 = _lower_words[getAddr(pid) - 2].second[0];

		uint32_t int_block = (uint32_t)(int_value);
		int16_t lsw = (int16_t) int_block;
		int16_t msw = (int16_t) (int_block >> 16);

		write(pid, msw); 
		write(_lower_words[getAddr(pid) - 2].first, lsw);
	}

	template <typename DataWriter>
	void writeMultiParam( unsigned long int pid, std::vector<uint8_t> data, DataWriter write )
	{ 
		if (!hasLSW(getAddr(pid) - 2)) {
			sz_log(0, "Param addr:%d has msw and missing lsw", getAddr(pid));
			return;
		}
		
		if (isFloat(pid)) { writeFloat(pid,data,write); return; }

		writeInteger(pid,data,write);
		return;
	}
	
	template <typename DataWriter>
	void writeSingleParam( unsigned long int pid, std::vector<uint8_t> data, DataWriter write )
	{ 
		int16_t value = 0;
		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&value);		
		if (data.size() > 1) { 
			*pblock8 = data[1]; pblock8++; 
			*pblock8 = data[0]; 
		} else {
			*pblock8 = data[0]; 
		}
		write(pid, value); 
	}

	template <typename DataWriter>
	void WriteData(unsigned long int pid, std::vector<uint8_t> data, DataWriter write )
	{
		if (isLSW(pid)) { setLSW(pid, data); return; }
		if (isMSW(pid)) { writeMultiParam(pid,data,write); return; }
		
		writeSingleParam(pid,data,write);
		return;
	}

private:
	/** Map param index in params.xml to SzParam */
	std::map<unsigned long int, SzParam> _params;
	/** Map param address in params.xml to param index & data pair */ 
	std::map<int, ParamIndexData> _lower_words;
};

#endif /*SZPMAP_H*/
