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

#ifndef __SZBWRITER_CACHE_H__

#define __SZBWRITER_CACHE_H__

#include <string>
#include <fstream>

#include "szarp_config.h"
#include "tsaveparam.h"
#include "tmmapparam.h"


/** 
 * @brief Class that cache params and write them with delay.
 *
 * Data are flushed while specified probes number were collected
 * or if file would be changed.
 */
class SzProbeCache {
public:
	/** 
	 * @brief type that specify param.
	 *
	 * Data that can specify param or are constants for param,
	 * based on szbwriter.cc file are directory, param name
	 * and probe length, year and month.
	 */
	struct Key {
		Key( const std::wstring& n , bool is_dbl , int y , int m )
			: name(n) , is_double(is_dbl) , year(y) , month(m) {}
		std::wstring name;
		bool is_double;
		int year;
		int month;
	};

	/** 
	 * @brief values needed to write one probe
	 */
	struct Value {
		Value( double p , time_t t ) 
			: probe(p) , time(t) {}

		double probe;
		time_t time;	
	};

	/** 
	 * @brief failure exception that is thrown by SzProbeCache when 
	 * writing to file failed
	 */
	class failure : public std::ofstream::failure
	{ public: failure( const char*msg ) : std::ofstream::failure(msg) {} };

	/** 
	 * @brief Initialize class with limitations.
	 *
	 * Largest limitation that make sense is 31 * 24 * 6 = 4464,
	 * but tests shows that 2048 is better value.
	 *
	 * @param probes_num number of probes within one param
	 * to be cached. After reaching that number, probes are flushed
	 * into the file.
	 */
	SzProbeCache( TSzarpConfig* ipk, const std::wstring& d, time_t pl, int probes_num = 2048 );

	/** 
	 * @brief Clean up object and write cached data to database
	 */
	virtual ~SzProbeCache();

	/** 
	 * @brief Adds probe to param. If necessary creates new param.
	 * 
	 * When there is probe missing (time gap) data are flushed to file.
	 *
	 * It creates new parameter if file has changed, so file name,
	 * directory or probe month differ from last time
	 *
	 * If new time is older than last time this function does nothing.
	 * 
	 * @param k param to which value should be added
	 * @param v value that should be added
	 */
	void add( const Key& k , const Value& v);

	/** 
	 * @brief Write cached probes to database, and clear current
	 * buffer.
	 */
	void flush();
	
private:
	struct Values {
		Values( int n );
		virtual ~Values();
		bool add( time_t t , double v , int probe_length );

		bool empty();
		void clear();

		double* probes;
		time_t time;
		unsigned int length;
		unsigned int max_length;
	};

	/**
	 * @see flush( Key k )
	 */
	void flush( const Key& k );


	TSzarpConfig * m_ipk;
	Values m_values;


	const std::wstring m_dir;
	int m_probe_length;

	Key* last_key;

	TMMapParam * m_mmap_param;
	TMMapParam * m_mmap_param_lsw;
	TMMapParam * m_mmap_param_msw;
};

bool operator==( const SzProbeCache::Key& a , const SzProbeCache::Key& b );
bool operator!=( const SzProbeCache::Key& a , const SzProbeCache::Key& b );

#endif /* __SZBWRITER_CACHE_H__ */

