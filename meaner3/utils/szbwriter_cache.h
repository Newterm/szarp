#ifndef __SZBWRITER_CACHE_H__

#define __SZBWRITER_CACHE_H__

#include <map>
#include <string>
#include <locale>
#include <fstream>

#include "tsaveparam.h"

/** 
 * @brief Class that cache params and write them after specified 
 * probes collected
 */
class SzProbeCache {
public:
	/** 
	 * @brief type that specify param.
	 *
	 * Data that can specify param or are constns for param,
	 * based on szbwriter.cc file * are directory, param name
	 * and probe length.
	 */
	struct Key {
		Key( const std::wstring& d , const std::wstring& n , int p )
			: dir(d) , name(n) , probe_length(p)
		{ // FIXME: speed this up
			std::locale loc;
			const std::collate<wchar_t>& coll =
				std::use_facet<std::collate<wchar_t> >(loc);
			std::wstring tmp = dir + name;
			hash_code = coll.hash( tmp.c_str() , tmp.c_str() + tmp.length() );
		}
		const std::wstring& dir;
		const std::wstring& name;
		int probe_length;

		long hash_code;
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
	 * @brief Initilize class with limitations. -1 means 
	 * limitations -- write while deleted
	 *
	 * @todo: unlimited probes caching not implemented
	 * 
	 * @param probes_num number of probes within one param
	 * to be cached
	 * @param params_num number of params to be cached.
	 * After reaching this number, biggest param will be written
	 * to file, and deleted.
	 */
	SzProbeCache( int probes_num , int params_num );

	/** 
	 * @brief Clean up object and write cached data to database
	 */
	virtual ~SzProbeCache();

	/** 
	 * @brief Adds probe to param. If necessary creates new param.
	 * 
	 * When there is probe missing (time gap) actual cache is
	 * written to database.
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
	void flush( const Key& k );
	
private:
	struct Values {
		Values( int n );
		virtual ~Values();
		bool add( time_t t , short v , int probe_length );
		void clear();

		short*probes;
		time_t time;
		unsigned int length;
		unsigned int max_length;
	};

	typedef std::pair<TSaveParam*,Values*> Param;
	typedef std::map<Key, Param> ParamsMap;

	/**
	 * @see flush( Key k )
	 */
	void flush( Param& p , const Key& k );

	int max_probes , max_params;
	ParamsMap params;
};

bool operator<( const SzProbeCache::Key& a , const SzProbeCache::Key& b );

#endif /* __SZBWRITER_CACHE_H__ */

