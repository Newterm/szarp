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
#include <limits>

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
		SzParam() : 
			_addr(-1), 
			_val_type(""), 
			_val_op(""), 
			_prec(0),
			_min(std::numeric_limits<double>::lowest()),
			_max(std::numeric_limits<double>::max()),
			_has_min(false),
			_has_max(false)
		{}

		SzParam(int addr,
			std::string vtype,
			std::string vop,
			int prec) : 
			
			_addr(addr),
			_val_type(vtype),
			_val_op(vop),
			_prec(prec),
			_min(std::numeric_limits<double>::lowest()),
			_max(std::numeric_limits<double>::max()),
			_has_min(false),
			_has_max(false)
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
		double getMin()
		{ return _min; }
		double getMax()
		{ return _max; }
		bool hasMin()
		{ return _has_min; }
		bool hasMax()
		{ return _has_max; }

		void setPrec(int prec)
		{ _prec = prec; }
		void setMin(double min)
		{ _min = min; }
		void setMax(double max)
		{ _max = max; }
		void setHasMin(bool has_min)
		{ _has_min = has_min; }
		void setHasMax(bool has_max)
		{ _has_max = has_max; }

	private:
		int _addr;
		std::string _val_type;
		std::string _val_op;
		int _prec;
		double _min;
		double _max;
		bool _has_min;
		bool _has_max;
	};

	bool ConfigureParamFromXml( unsigned long int idx, TParam* p, xmlNodePtr& node );
	bool ConfigureParamFromXml( unsigned long int idx, TSendParam* s, xmlNodePtr& node );
	SzParam ConfigureParamFromXml( unsigned long int idx, xmlNodePtr& node );
	
	/** Used for testing only */
	void AddParam( unsigned long int idx, SzParam param );

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

	double getMin(unsigned long int pid)
	{ return _params[pid].getMin(); }
	double getMax(unsigned long int pid)
	{ return _params[pid].getMax(); }
	
	bool hasMin(unsigned long int pid)
	{ return _params[pid].hasMin(); }
	bool hasMax(unsigned long int pid)
	{ return _params[pid].hasMax(); }

	/**
	 * Floating point comparison is not an easy problem. 
	 * We instead try to use an approach similar to what
	 * SZARP always does with floating point when writing
	 * to DB. Using epsilon would be a risk of allowing
	 * a value that looks out of range into the szbase.
	 *
	 * ex. max = 1.1111, value = 1.1110999999...
	 * With epislon 0.0000001 value would be accepted
	 * but from SZARP perspective these are different
	 * numbers 11111 and 11110 (because of cropping).
	 */
	bool out_of_range(double value, long int pid) {
		// For integers use default precision for comparison 
		int prec = getPrec(pid);
		long int_value = (long)(value * pow10(prec));
		if (hasMin(pid)) {
			long min = (long)(getMin(pid) * pow10(prec));
			if (int_value < min) return true;
		}
		if (hasMax(pid)) {
			long max = (long)(getMax(pid) * pow10(prec));
			if (int_value > max) return true;
		}
		return false;
	}	

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
		sz_log(10, "SzParamMap::writeMultiParam(%lu)",pid);

		if (!hasParamLSW(getAddr(pid) - 2)) {
			sz_log(1, "Param addr:%d has msw and missing lsw", getAddr(pid));
			return;
		}
		
		if (isFloat(pid)) { writeFloat(pid,data,write); return; }

		writeInteger(pid,data,write);
		return;
	}
	
	template <typename DataWriter>
	void writeSingleParam( unsigned long int pid, DataBuffer data, DataWriter write )
	{ 
		sz_log(10, "SzParamMap::writeSingleParam(%lu)", pid);

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
	bool readFloat( unsigned long int pid, DataDescriptor desc, DataReader read)
	{
		sz_log(10, "SzParamMap::readFloat(%lu)", pid);

		SendIndexData sid = _send_lsws[getAddr(pid) - 2];
		unsigned long int lsw_pid = sid.first;

		int16_t lsw = read(lsw_pid);
		int16_t msw = read(pid);
		
		sz_log(10, "Send value (msw integer) id:%lu, val:%d", pid, msw);
		sz_log(10, "Send value (lsw integer) id:%lu, val:%d", lsw_pid, lsw);
	
		if ((lsw == SZARP_NO_DATA) || (msw == SZARP_NO_DATA)) {
			sz_log(10, "Param lsw id:%lu msw id:%lu  is no data - aborting query write", 
					lsw_pid, pid);
			return false;
		}

		int int_value;
		uint16_t* pblock16 = reinterpret_cast<uint16_t*>(&int_value);
		*pblock16 = lsw; pblock16++;
		*pblock16 = msw;
		
		float float_value = (float)(int_value / pow10(getPrec(pid)));
		
		sz_log(10, "Send value (float) id:%lu, val:%f", lsw_pid, float_value);
		
		/* Use min/max from lsw and msw param configuration together */
		if (out_of_range(float_value, lsw_pid) || 
				out_of_range(float_value, pid))
		{
			sz_log(10, "Param id:%lu value:%f is out of limits:", lsw_pid, float_value);
			sz_log(10, "[%f,%f]", getMin(lsw_pid), getMax(lsw_pid));
			sz_log(10, "[%f,%f]", getMin(pid), getMax(pid));
			sz_log(10, "Aborting query write");
			return false;
		}

		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&float_value);

		*(desc.first + 1) = *pblock8; pblock8++;
		*(desc.first + 0) = *pblock8; pblock8++;
		*(sid.second.first + 1) = *pblock8; pblock8++;
		*(sid.second.first + 0) = *pblock8; 

		return true;
	}	

	template <typename DataReader>
	bool readInteger( unsigned long int pid, DataDescriptor desc, DataReader read)
	{
		sz_log(10, "SzParamMap::readInteger(%lu)", pid);

		SendIndexData sid = _send_lsws[getAddr(pid) - 2];
		unsigned long int lsw_pid = sid.first;

		int16_t lsw = read(lsw_pid);
		int16_t msw = read(pid);
	
		sz_log(10, "Send value (msw integer) id:%lu, val:%d", pid, msw);
		sz_log(10, "Send value (lsw integer) id:%lu, val:%d", lsw_pid, lsw);

		if ((lsw == SZARP_NO_DATA) || (msw == SZARP_NO_DATA)) {
			sz_log(10, "Param lsw id:%lu msw id:%lu  is no data - aborting query write", 
					lsw_pid, pid);
			return false;
		}

		int int_value;
		uint16_t* pblock16 = reinterpret_cast<uint16_t*>(&int_value);
		*pblock16 = lsw; pblock16++;
		*pblock16 = msw;

		sz_log(10, "Send value (int ) id:%lu, val:%d", lsw_pid, int_value);

		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&int_value);

		/* Use min/max from lsw and msw param configuration together */
		if (out_of_range(int_value, lsw_pid) ||
				out_of_range(int_value, pid))
		{
			sz_log(10, "Param id:%lu value:%d is out of limits:", lsw_pid, int_value);
			sz_log(10, "[%f,%f]", getMin(lsw_pid), getMax(lsw_pid));
			sz_log(10, "[%f,%f]", getMin(pid), getMax(pid));
			sz_log(10, "Aborting query write");
			return false;
		}

		*(desc.first + 1) = *pblock8; pblock8++;
		*(desc.first + 0) = *pblock8; pblock8++;
		*(sid.second.first + 1) = *pblock8; pblock8++;
		*(sid.second.first + 0) = *pblock8; 

		return true;
	}	

	template <typename DataReader>
	bool readMultiParam( unsigned long int pid, DataDescriptor desc, DataReader read)
	{
		sz_log(10, "SzParamMap::readMultiParam(%lu)", pid);

		if (!hasSendLSW(getAddr(pid) - 2)) {
			sz_log(1, "Send addr:%d has msw and missing lsw", getAddr(pid));
			return false;
		}
		
		if (isFloat(pid)) { return readFloat(pid,desc,read); }

		return readInteger(pid,desc,read);
	}

	template <typename DataReader>
	bool readSingleParam( unsigned long int pid, DataDescriptor desc, DataReader read)
	{
		sz_log(10, "SzParamMap::readSingleParam(%lu)", pid);

		int16_t value = read(pid);
		
		if (value == SZARP_NO_DATA) {
			sz_log(10, "Param id:%lu is no data - aborting query write", pid);
			return false;
		}

		if (out_of_range(value, pid)) {
			sz_log(10, "Param id:%lu value:%d is out of limits:", pid, value);
			sz_log(10, "[%f,%f]", getMin(pid), getMax(pid));
			sz_log(10, "Aborting query write");
			return false;
		}

		uint8_t* pblock8 = reinterpret_cast<uint8_t*>(&value);		
		if (desc.first != desc.second) { 
			*(desc.first + 1) = *pblock8; pblock8++; 
			*(desc.first) = *pblock8;
		} else {
			*(desc.first) = *pblock8;
		}

		return true;
	}

	template <typename DataReader>
	bool ReadData( unsigned long int pid, DataDescriptor desc, DataReader read ) 
	{ 
		sz_log(10, "SzParamMap::ReadData id:%lu",pid);
		
		if (isLSW(pid)) { setSendLSW(pid,desc); return true; }
		if (isMSW(pid)) { return readMultiParam(pid,desc,read); }
		
		return readSingleParam(pid,desc,read);
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
