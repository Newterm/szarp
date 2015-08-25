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

#ifndef S7QMAP_H 
#define S7QMAP_H

/** 
 * @file s7qmap.h
 * @brief Manage a map of queries to S7 controller.
 *
 * Reads SZARP configuration to create a map of sorted
 * and merged queries.
 *
 * @author Marcin Harasimczuk <hary@newterm.pl>
 * @version 0.1
 * @date 2015-08-25
 */

#include <snap7.h>

#include <map>
#include <vector>
#include <string>

#include "s7query.h"

class S7QueryMap 
{
public:
	typedef std::vector<S7Query> QueryVec;

	class S7Param 
	{
	public:
		S7Param() :
			s7DBNum(-1),
			s7DBType(""),
			s7Addr(-1),
			s7ValType(""),
			s7BitNumber(-1)
		{}

		S7Param(int num, 
			std::string type, 
			int addr, 
			std::string vtype, 
			int bnum)  :

			s7DBNum(num),
			s7DBType(type),
			s7Addr(addr),
			s7ValType(vtype),
			s7BitNumber(bnum)
		{}

		int s7DBNum; /**Database number*/
		std::string s7DBType; /**Database type*/
		int s7Addr; /**Address in database*/
		std::string s7ValType; /**Value type*/
		int s7BitNumber; /**Bit number*/
	};

	S7Query QueryFromParam( S7Param param );
	bool AddQuery( unsigned long int idx, S7Param param );

	void Sort();
	void Merge();

	bool AskAll(S7Object& client);
	bool DumpAll();
	
	template <typename DataProcessor>
	void ProcessData(DataProcessor proc)
	{ 
		for (auto vk_q = _queries.begin(); vk_q != _queries.end(); vk_q++)
			for (auto q = vk_q->second.begin(); q != vk_q->second.end(); q++)
				q->ProcessData(proc);
	}

private:
	void MergeBucket( S7Query::QueryKey key, QueryVec& qvec );

	std::map<S7Query::QueryKey,QueryVec> _queries;
};

#endif /*S7QMAP_H*/
