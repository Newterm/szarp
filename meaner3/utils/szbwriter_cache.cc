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

#include "szbwriter_cache.h"

#include <cstring>

#include "liblog.h"
#include "szbase/szbbase.h"

SzProbeCache::SzProbeCache( int max_probes )
	: last_key(NULL) , last_param(NULL,NULL)
{
	last_param.second = new Values( max_probes );
}

SzProbeCache::~SzProbeCache()
{
	flush();
	if( last_key ) delete last_key;
	if( last_param.first ) delete last_param.first;
	delete last_param.second;
}

void SzProbeCache::add( const Key& k , const Value& v )
{
	if( !last_key || k != *last_key ) {
		sz_log(10,"Creating new file in TSaveParam in dir: %ls, name: %ls, probe: %d, year: %d, month: %d",k.dir.c_str(),k.name.c_str(),k.probe_length,k.year,k.month);
		if( last_key )
			sz_log(10,"         old file in TSaveParam in dir: %ls, name: %ls, probe: %d, year: %d, month: %d",last_key->dir.c_str(),last_key->name.c_str(),last_key->probe_length,last_key->year,last_key->month);

		flush(); // flush data to old file

		if( last_key ) delete last_key;
		last_key = new Key(k);

		if( last_param.first ) delete last_param.first;
		last_param.first = new TMMapParam( k.dir , k.name , k.year , k.month , k.probe_length );
	}

	Values& vs = *last_param.second;
	while( !vs.add( v.time , v.probe , k.probe_length ) ) flush();
}

void SzProbeCache::flush()
{
	flush( last_param , *last_key );
}

SzProbeCache::Values::Values( int n )
	: time(0) , length(0) , max_length(n)
{
	probes = new short[max_length];
}

bool SzProbeCache::Values::add( time_t t , short v , int probe_length )
{
	if( length == 0 ) time = t;

	sz_log(10,"Adding probe do cache - current time: %i, time: %i, value: %i, probe length: %i",szb_move_time( time , length , PT_CUSTOM , probe_length ),t,v,probe_length);

	if( length >= max_length ) return false;

	time_t curr_time = szb_move_time( time , length , PT_CUSTOM , probe_length );

	if( t > curr_time ) return false; // raport time gap
	if( t < curr_time ) return false; // override data. Return true otherwise

	sz_log(10,"Writing value %i to %u at %i",v,length,t);
	probes[length++] = v;

	return true;
}

void SzProbeCache::Values::clear()
{
	sz_log(10,"Clearing cache with length: %i",length);
	length = 0;
}

bool SzProbeCache::Values::clean()
{	return !length; }

SzProbeCache::Values::~Values()
{
	delete probes;
}

void SzProbeCache::flush( Param& p , const Key& k )
{
	if( p.second->clean() ) return;
//        if( p.first->WriteBuffered(
//                        k.dir , p.second->time , p.second->probes , p.second->length
//                        , NULL , 1 , 0 , k.probe_length , true ) )
	if( p.first->write( p.second->time , p.second->probes , p.second->length ) )
		throw failure( "Cannot write to buffer");
	p.second->clear();
}

bool operator==( const SzProbeCache::Key& a , const SzProbeCache::Key& b )
{
	return
		a.dir == b.dir && a.name == b.name &&
		a.probe_length == b.probe_length && 
		a.year == b.year && a.month == b.month;
}

bool operator!=( const SzProbeCache::Key& a , const SzProbeCache::Key& b )
{
	return !(a==b);
}

