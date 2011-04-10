#ifndef __SZBWRITER_CACHE_H__

#define __SZBWRITER_CACHE_H__

#include <map>
#include <string>
#include <locale>
#include <fstream>

#include "tsaveparam.h"

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
	 * Data that can specify param or are constns for param,
	 * based on szbwriter.cc file are directory, param name
	 * and probe length, year and month.
	 */
	struct Key {
		Key( const std::wstring& d , const std::wstring& n ,
				int p , int y , int m )
			: dir(d) , name(n) , probe_length(p) , year(y) , month(m) {}
		const std::wstring& dir;
		const std::wstring& name;
		int probe_length;
		int year;
		int month;

//                void operator=(  Key& b )
//                {
//                        dir = b.dir;
//                        name = b.name;
//                        probe_length = b.probe_length;
//                        year = b.year;
//                        month = b.month;
//                }
	};

	/** 
	 * @brief values needed to write one probe
	 */
	struct Value {
		Value( short p , time_t t ) 
			: probe(p) , time(t) {}
		short probe;
		time_t time;	
	};

	typedef std::ofstream::failure failure;

	/** 
	 * @brief Initilize class with limitations.
	 *
	 * Largest limitation that make sense is 31 * 24 * 6 = 4464,
	 * but tests shows that 2048 is better value.
	 *
	 * @param probes_num number of probes within one param
	 * to be cached
	 * @param params_num number of params to be cached.
	 * After reaching this number, biggest param will be written
	 * to file, and deleted.
	 */
	SzProbeCache( int probes_num = 2048 );

	/** 
	 * @brief Clean up object and write cached data to database
	 */
	virtual ~SzProbeCache();

	/** 
	 * @brief Adds probe to param. If necessary creates new param.
	 * 
	 * When there is probe missing (time gap) its filled with
	 * SZB_FILE_NODATA special value.
	 * 
	 * @param k param to wich value should be added
	 * @param v 
	 */
	void add( const Key& k , const Value& v);

	/** 
	 * @brief Write cached param to database.
	 * 
	 * @param k
	 */
	void flush();
	
private:
	struct Values {
		Values( int n );
		virtual ~Values();
		bool add( time_t t , short v , int probe_length );
		void clear();
		bool clean();

		short*probes;
		time_t time;
		unsigned int length;
		unsigned int max_length;
	};

	typedef std::pair<TSaveParam*,Values*> Param;

	/**
	 * @see flush( Key k )
	 */
	void flush( Param& p , const Key& k );

	Key*last_key;
	Param last_param;
};

bool operator==( const SzProbeCache::Key& a , const SzProbeCache::Key& b );
bool operator!=( const SzProbeCache::Key& a , const SzProbeCache::Key& b );

#endif /* __SZBWRITER_CACHE_H__ */

