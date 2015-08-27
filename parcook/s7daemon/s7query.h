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

#ifndef S7QUERY_H
#define S7QUERY_H

/** 
 * @file s7query.h
 * @brief Hide details of a single query to S7 controller.
 *
 * Hold all arguments needed by query and store query data
 * response. Implement merging logic on query level.
 *
 * @author Marcin Harasimczuk <hary@newterm.pl>
 * @version 0.1
 * @date 2015-08-25
 */

#include <snap7.h>

#include <vector>
#include <tuple>
#include <cstdint>

#include "liblog.h"
#include "datatypes.h"

class S7Query 
{
public:
	typedef std::tuple<bool,int,int,int> QueryKey;
	S7Query():
		_area(-1),
		_db_num(-1),
		_start(-1),
		_amount(-1),
		_w_len(-1),
		_write(false)
	{}

	void merge( S7Query& query );
	void appendId( int id );
	bool isValid();
	bool ask( S7Object& client );
	bool tell( S7Object& client );
	void dump();
	int nextAddress();
	unsigned int typeSize();
	
	template <typename ResponseProcessor>
	void ProcessResponse(ResponseProcessor proc) 
	{
		DataBuffer buff;
		std::vector<int> ids = _ids;
		for (unsigned int offset = 0; (offset + typeSize()) <= _data.size(); offset += typeSize()) {
			for (unsigned int idx = 0; idx < typeSize(); idx++) 
				buff.push_back(_data[offset + idx]);
			proc(ids.front(), buff);
			ids.erase(ids.begin());
			buff.clear();
		}
	}
	
 	template <typename DataAccessor>
	void AccessData(DataAccessor access) 
	{	
		std::vector<int> ids = _ids;
		for (unsigned int offset = 0; (offset + typeSize()) <= _data.size(); offset += typeSize()) {
			DataIterator data_begin = _data.begin() + offset;
			DataIterator data_end =  data_begin + typeSize() - 1;

			access(ids.front(), DataDescriptor(data_begin, data_end));

			ids.erase(ids.begin());
		}
	}

	void setArea( int area )
	{ _area = area; }
	void setDbNum( int db_num )
	{ _db_num = db_num; }
	void setStart( int start )
	{ _start = start; }
	void setAmount( int amount )
	{ _amount = amount; }
	void setWordLen( int w_len )
	{ _w_len = w_len; }
	void setWriteQuery( bool write )
	{ _write = write; }

	bool isWriteQuery()
	{ return _write; }

	int getStart()
	{ return _start; }

	bool hasData()
	{ return !_data.empty(); }

	QueryKey getKey() 
	{ return QueryKey(_write,_area,_w_len,_db_num); }
	
	friend bool operator<(const S7Query& l, const S7Query& r) 
	{ return l._start < r._start; }
	
	void build()
	{ _data.resize(_amount * typeSize()); }

private:
	int _area;
	int _db_num;
	int _start;
	int _amount;
	int _w_len;

	bool _write;
	
	std::vector<int> _ids;
	DataBuffer _data;
};


#endif /*S7QUERY_H*/
