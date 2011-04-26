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
	{ public: failure( const std::string& msg ):std::ofstream::failure(msg) {} };


	/** 
	 * @brief Constructs szbase parameter.
	 *
	 * Holds every data needed to write data to one file.
	 * Assumes that maximum file size is
	 * 31 * 24 * probes_per_hour bytes.
	 *
	 * If any error occur it throws TMMapParam::failure exception.
	 * 
	 * @param dir file directory path
	 * @param name file name
	 * @param year year of probes in file
	 * @param month month of probes in file
	 * @param probe_length length of probes in seconds
	 */
	TMMapParam(	const std::wstring& dir ,
			const std::wstring& name ,
			int year , int month ,
			time_t probe_length );

	/** 
	 * @brief Clean up file.
	 *
	 * Unmap memory then truncate and close file. If truncate fail,
	 * destructor will shout in log and on stderr and try to close
	 * file anyway.
	 */
	virtual ~TMMapParam();

	/** 
	 * @brief Clean up file.
	 *
	 * Unmap memory then truncate and close file. If truncate fail,
	 * close will throw an exception so file will stay open. To force
	 * closing file simply delete object.
	 * 
	 * @return 0
	 */
	int close();

	/** 
	 * @brief Writes probes to file.
	 *
	 * Writes table of shorts to file using mmap. Index is calculated 
	 * with szb_probeind, so if this function never return less then
	 * 31*24*3600/probe_length this function is also safe.
	 *
	 * Otherwise assert fails.
	 * 
	 * @param t time of first probe
	 * @param probes table of probes
	 * @param len length of probes
	 * 
	 * @return 0
	 */
	int write( time_t t , short*probes , unsigned int len );
	
private:
	int fd;
	short*filemap;
	std::wstring path;
	unsigned int length;

	unsigned int file_size;
	time_t probe_length;
};

#endif /* __TMMAPPARAM_H__ */

