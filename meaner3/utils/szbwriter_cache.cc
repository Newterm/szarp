#include "szbwriter_cache.h"

#include <cstring>
#include <iostream>

#include "liblog.h"
#include "szbase/szbbase.h"

SzProbeCache::SzProbeCache( int probes_num , int params_num )
	: max_probes(probes_num) , max_params(params_num)
{
}

SzProbeCache::~SzProbeCache()
{
	for( ParamsMap::iterator i = params.begin() ; i != params.end() ; ++i )
	{
		flush( i->second , i->first );
		delete i->second.first; // delete TSaveParam
		delete i->second.second;
	}
}

void SzProbeCache::add( const Key& k , const Value& v )
{
	ParamsMap::iterator pi = params.find(k);

	if( pi == params.end() )
		pi = params.insert(
			std::pair<Key,Param>(
				k, Param(
					new TSaveParam(k.name,false),
					new Values(max_probes)
					)
				)
			).first;

	Values& vs = *pi->second.second;
	if( ! vs.add( v.time , v.probe , k.probe_length ) )
	{
		flush( pi->second , pi->first );
		vs.add( v.time , v.probe , k.probe_length );
	}
}

void SzProbeCache::flush( const Key& k )
{
	ParamsMap::iterator pi = params.find(k);
	if( pi == params.end() ) return;

	flush( pi->second , k );
}

SzProbeCache::Values::Values( int n )
	: time(0) , length(0) , max_length(n)
{
	probes = new short[max_length];
}

bool SzProbeCache::Values::add( time_t t , short v , int probe_length )
{
	if( length == 0 ) time = t;

	sz_log(10,"Adding probe do cache - current time: %i, time: %i, value: %i",szb_move_time( time , length , PT_CUSTOM , probe_length ),t,v);

	if( length >= max_length ) return false;

	time_t curr_time = szb_move_time( time , length , PT_CUSTOM , probe_length );

	// TODO: replace this loop with memcpy
	while( t > curr_time && length < max_length ) 
	{
		curr_time = szb_move_time( curr_time, 1, PT_CUSTOM, probe_length );
		if( t <= curr_time ) break;
		probes[length++] = SZB_FILE_NODATA;
	}

	if( t != curr_time ) return false;

	probes[length++] = v;

	return true;
}

void SzProbeCache::Values::clear()
{
	sz_log(10,"Clearing cache with length: %i",length);
	length = 0;
}

SzProbeCache::Values::~Values()
{
	delete probes;
}

void SzProbeCache::flush( Param& p , const Key& k )
{
	if( p.first->WriteBuffered(
			k.dir , p.second->time , p.second->probes , p.second->length
			, NULL , 1 , 0 , k.probe_length ) )
		throw failure( "Cannot write to buffer");
	p.second->clear();
}

bool operator<( const SzProbeCache::Key& a , const SzProbeCache::Key& b )
{
	return a.hash_code < b.hash_code;
}

