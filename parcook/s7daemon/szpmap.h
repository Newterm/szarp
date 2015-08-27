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
#include "datatypes.h"

class SzParamMap 
{
public:
	typedef std::pair<unsigned long int, DataBuffer> ParamIndexData;
	typedef std::pair<unsigned long int, DataDescriptor> SendIndexData;

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
	
		bool isValid()
		{	
			if ((_addr == -1) || (_val_type.compare(std::string("")) == 0))
				return false;
			return true;

		}
		bool isLSW()
		{ return (_val_op.compare(std::string("lsw"))==0); }
		bool isMSW()
		{ return (_val_op.compare(std::string("msw"))==0); }
		bool isFloat()
		{ return (_val_type.compare(std::string("real"))==0); }
		int getAddr()
		{ return _addr; }
		std::string getType()
		{ return _val_type; }
		int getPrec()
		{ return _prec; }
		
		void setPrec(int prec)
		{ _prec = prec; }

	private:
		int _addr;
		std::string _val_type;
		std::string _val_op;
		int _prec;
	};

	bool ConfigureParamFromXml( unsigned long int idx, TParam* p, xmlNodePtr& node );
	bool ConfigureParamFromXml( unsigned long int idx, TSendParam* s, xmlNodePtr& node );
	SzParam ConfigureParamFromXml( unsigned long int idx, xmlNodePtr& node );
	
	void clearWriteBuffer() 
	{ _param_lsws.clear(); }
	void clearReadBuffer() 
	{ _send_lsws.clear(); }

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
	
	void setParamLSW( unsigned long int pid, DataBuffer data )
	{ _param_lsws[getAddr(pid)] = ParamIndexData(pid, data); }
	void setSendLSW( unsigned long int pid, DataDescriptor dd )
	{ _send_lsws[getAddr(pid)] = SendIndexData(pid, dd); }
	
	bool hasParamLSW( int addr ) 
	{ return ( _param_lsws.find(addr) != _param_lsws.end() ); }
	bool hasSendLSW( int addr ) 
	{ return ( _send_lsws.find(addr) != _send_lsws.end() ); }

	template <typename DataWriter>
	void writeFloat( unsigned long int pid, DataBuffer data, DataWriter write)
	{
		sz_log(10, "SzParamMap::writeFloat");

		float float_value;
		
		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&float_value);
		*pblock8 = data[1]; pblock8++;
		*pblock8 = data[0]; pblock8++;
		*pblock8 = _param_lsws[getAddr(pid) - 2].second[1]; pblock8++;
		*pblock8 = _param_lsws[getAddr(pid) - 2].second[0];

		uint32_t float_block = (uint32_t)(float_value * pow10(getPrec(pid)));
		int16_t lsw = (int16_t) float_block;
		int16_t msw = (int16_t) (float_block >> 16);

		sz_log(5, "Param value (msw float) id:%lu, val:%d", pid, msw);
		sz_log(5, "Param value (lsw float) id:%lu, val:%d", _param_lsws[getAddr(pid) - 2].first, lsw);

		write(pid, msw); 
		write(_param_lsws[getAddr(pid) - 2].first, lsw);
	}

	template <typename DataWriter>
	void writeInteger( unsigned long int pid, DataBuffer data, DataWriter write )
	{
		sz_log(10, "SzParamMap::writeInteger");

		int int_value;
		
		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&int_value);
		*pblock8 = data[1]; pblock8++;
		*pblock8 = data[0]; pblock8++;
		*pblock8 = _param_lsws[getAddr(pid) - 2].second[1]; pblock8++;
		*pblock8 = _param_lsws[getAddr(pid) - 2].second[0];

		uint32_t int_block = (uint32_t)(int_value);
		int16_t lsw = (int16_t) int_block;
		int16_t msw = (int16_t) (int_block >> 16);

		sz_log(5, "Param value (msw integer) id:%lu, val:%d", pid, msw);
		sz_log(5, "Param value (lsw integer) id:%lu, val:%d", _param_lsws[getAddr(pid) - 2].first, lsw);

		write(pid, msw); 
		write(_param_lsws[getAddr(pid) - 2].first, lsw);
	}

	template <typename DataWriter>
	void writeMultiParam( unsigned long int pid, DataBuffer data, DataWriter write )
	{ 
		sz_log(10, "SzParamMap::writeMultiParam");

		if (!hasParamLSW(getAddr(pid) - 2)) {
			sz_log(0, "Param addr:%d has msw and missing lsw", getAddr(pid));
			return;
		}
		
		if (isFloat(pid)) { writeFloat(pid,data,write); return; }

		writeInteger(pid,data,write);
		return;
	}
	
	template <typename DataWriter>
	void writeSingleParam( unsigned long int pid, DataBuffer data, DataWriter write )
	{ 
		sz_log(10, "SzParamMap::writeSingleParam");

		int16_t value = 0;
		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&value);		
		if (data.size() > 1) { 
			*pblock8 = data[1]; pblock8++; 
			*pblock8 = data[0]; 
		} else {
			*pblock8 = data[0]; 
		}

		sz_log(5, "Param value (single) id:%lu, val:%d", pid, value);
		write(pid, value); 
	}

	template <typename DataWriter>
	void WriteData(unsigned long int pid, DataBuffer data, DataWriter write )
	{
		sz_log(10, "SzParamMap::WriteData id:%lu",pid);

		if (isLSW(pid)) { setParamLSW(pid, data); return; }
		if (isMSW(pid)) { writeMultiParam(pid,data,write); return; }
		
		writeSingleParam(pid,data,write);
	}

	template <typename DataReader>
	void readFloat( unsigned long int pid, DataDescriptor desc, DataReader read)
	{
		sz_log(10, "SzParamMap::readInteger");

		SendIndexData sid = _send_lsws[getAddr(pid) - 2];
		unsigned long int lsw_pid = sid.first;

		int16_t lsw = read(_send_params[lsw_pid]);
		int16_t msw = read(_send_params[pid]);

		int int_value;
		uint16_t* pblock16 = reinterpret_cast<uint16_t*>(&int_value);
		*pblock16 = lsw; pblock16++;
		*pblock16 = msw;
		
		float float_value = (float)(int_value / pow10(getPrec(pid)));
		
		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&float_value);

		*(desc.first + 1) = *pblock8; pblock8++;
		*(desc.first + 0) = *pblock8; pblock8++;
		*(sid.second.first + 1) = *pblock8; pblock8++;
		*(sid.second.first + 0) = *pblock8; 
	}	

	template <typename DataReader>
	void readInteger( unsigned long int pid, DataDescriptor desc, DataReader read)
	{
		sz_log(10, "SzParamMap::readInteger");

		SendIndexData sid = _send_lsws[getAddr(pid)];
		unsigned long int lsw_pid = sid.first;

		int16_t lsw = read(_send_params[lsw_pid]);
		int16_t msw = read(_send_params[pid]);
	
		int int_value;
		uint16_t* pblock16 = reinterpret_cast<uint16_t*>(&int_value);
		*pblock16 = lsw; pblock16++;
		*pblock16 = msw;

		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&int_value);

		*(desc.first + 1) = *pblock8; pblock8++;
		*(desc.first + 0) = *pblock8; pblock8++;
		*(sid.second.first + 1) = *pblock8; pblock8++;
		*(sid.second.first + 0) = *pblock8; 
	}	

	template <typename DataReader>
	void readMultiParam( unsigned long int pid, DataDescriptor desc, DataReader read)
	{
		sz_log(10, "SzParamMap::readMultiParam");

		if (!hasSendLSW(getAddr(pid) - 2)) {
			sz_log(0, "Send addr:%d has msw and missing lsw", getAddr(pid));
			return;
		}
		
		if (isFloat(pid)) { readFloat(pid,desc,read); return; }

		readInteger(pid,desc,read);
		return;
	}

	template <typename DataReader>
	void readSingleParam( unsigned long int pid, DataDescriptor desc, DataReader read)
	{
		sz_log(10, "SzParamMap::readSingleParam");

		int16_t value = read(_send_params[pid]);
		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&value);		
		if (desc.first != desc.second) { 
			*(desc.first + 1) = *pblock8; pblock8++; 
			*(desc.first) = *pblock8;
		} else {
			*(desc.first) = *pblock8;
		}
	}

	template <typename DataReader>
	void ReadData( unsigned long int pid, DataDescriptor desc, DataReader read ) 
	{ 
		sz_log(10, "SzParamMap::ReadData id:%lu",pid);
		
		if (isLSW(pid)) { setSendLSW(pid,desc); return; }
		if (isMSW(pid)) { readMultiParam(pid,desc,read); return; }
		
		readSingleParam(pid,desc,read);
	}


private:
	/** Map param index in params.xml to SzParam */
	std::map<unsigned long int, SzParam> _params;
	std::map<unsigned long int, TSendParam*> _send_params;
	/** Map param address in params.xml to param index & data pair */ 
	std::map<int, ParamIndexData> _param_lsws;
	std::map<int, SendIndexData> _send_lsws;
};

#endif /*SZPMAP_H*/
