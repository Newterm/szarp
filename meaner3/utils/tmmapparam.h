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
 */

#ifndef __TMMAPPARAM_H__

#define __TMMAPPARAM_H__

#include <string>
#include <fstream>

class TMMapParam {
public:
	class failure : public std::ofstream::failure
	{ public: failure( const std::string& msg ) : std::ofstream::failure(msg) {} };


	TMMapParam(	const std::wstring& dir ,
			const std::wstring& name ,
			int year , int month ,
			time_t probe_length );

	virtual ~TMMapParam();

	int write( time_t t , short*probes , unsigned int len );
	
private:
	int fd;
	short*filemap;
	std::wstring path;

	unsigned int file_size;
	time_t probe_length;
};

#endif /* __TMMAPPARAM_H__ */

