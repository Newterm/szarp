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

SzProbeCache::SzProbeCache( const std::wstring& d, int pl, int max_probes )
	: m_values(max_probes), m_dir(d), m_probe_length(pl), last_key(NULL)
{
}

SzProbeCache::~SzProbeCache()
{
	flush();
	if( last_key ) delete last_key;
	delete m_mmap_param;
	delete m_mmap_param_lsw;
	delete m_mmap_param_msw;
}

void SzProbeCache::add( const Key& k , const Value& v )
{
	if( !last_key || k != *last_key ) {
		sz_log(10,"Creating new file in TSaveParam in name: %ls, probe: %d, year: %d, month: %d",
				k.param->GetName().c_str(), k.is_double, k.year, k.month);
		if( last_key )
			sz_log(10,"         old file in TSaveParam in name: %ls, probe: %d, year: %d, month: %d",
					 last_key->param->GetName().c_str(), last_key->is_double, last_key->year, last_key->month);

		flush(); // flush data to old file

		if (last_key->is_double) {
			delete m_mmap_param_msw;
			delete m_mmap_param_lsw;
			
			std::wstring name1 = k.param->GetName() + L" msw";
			m_mmap_param_msw = new TMMapParam( m_dir , name1 , k.year , k.month , m_probe_length );

			std::wstring name2 = k.param->GetName() + L" lsw";
			m_mmap_param_lsw = new TMMapParam( m_dir , name2 , k.year , k.month , m_probe_length );
		}
		else {
			delete m_mmap_param;
			m_mmap_param = new TMMapParam( m_dir , k.param->GetName() , k.year , k.month , m_probe_length );
		}

		if( last_key ) delete last_key;
		last_key = new Key(k);
	}

	while( !m_values.add( v.time , v.probe , m_probe_length ) ) flush();
}

void SzProbeCache::flush()
{
	flush( *last_key );
}

void SzProbeCache::flush( const Key& k )
{
	if( m_values.empty() ) return;

//        if( p.first->WriteBuffered(
//                        k.dir , p.second->time , p.second->probes , p.second->length
//                        , NULL , 1 , 0 , k.probe_length , true ) )

	double prec = pow10(last_key->param->GetPrec());

	if (last_key->is_double) {
		short* sp_lsw = new short[m_values.length];
		short* sp_msw = new short[m_values.length];

		int v = 0;
		unsigned *pv = (unsigned *)&v;

		for (size_t i = 0; i < m_values.length; i++) {
			v = rint(prec * m_values.probes[i]);
			sp_lsw[i] = *pv & 0xffff;
			sp_msw[i] = *pv >> 16;
		}
		if( m_mmap_param_lsw->write( m_values.time , sp_lsw , m_values.length ) )
			throw failure( "Cannot write to buffer");

		if( m_mmap_param_msw->write( m_values.time , sp_msw , m_values.length ) )
			throw failure( "Cannot write to buffer");

		delete sp_lsw;
		delete sp_msw;
	}
	else {
		short* sp = new short[m_values.length];

		for (size_t i = 0; i < m_values.length; i++) {
			sp[i] = rint(prec * m_values.probes[i]);
		}

		if( m_mmap_param->write( m_values.time , sp , m_values.length ) )
			throw failure( "Cannot write to buffer");

		delete sp;
	}

	m_values.clear();
}

SzProbeCache::Values::Values( int n )
	: time(0) , length(0) , max_length(n)
{
	probes = new double[max_length];
}

bool SzProbeCache::Values::add( time_t t , double v , int probe_length )
{
	if( length == 0 ) time = t;

	sz_log(10,"Adding probe do cache - current time: %li, time: %li, value: %f, probe length: %i",
			szb_move_time( time , length , PT_CUSTOM , probe_length ), t, v, probe_length);

	if( length >= max_length ) return false;

	time_t curr_time = szb_move_time( time , length , PT_CUSTOM , probe_length );

	if( t > curr_time ) return false; // raport time gap
	if( t < curr_time ) return false; // override data. Return true otherwise

	sz_log(10,"Writing value %f to %u at %li", v, length, t);
	probes[length++] = v;

	return true;
}


void SzProbeCache::Values::clear()
{
	sz_log(10,"Clearing cache with length: %i",length);
	length = 0;
}

bool SzProbeCache::Values::empty()
{	return !length; }


SzProbeCache::Values::~Values()
{
	delete probes;
}

bool operator==( const SzProbeCache::Key& a , const SzProbeCache::Key& b )
{
	return	a.param == b.param &&
		a.is_double == b.is_double &&
		a.year == b.year &&
		a.month == b.month;
}

bool operator!=( const SzProbeCache::Key& a , const SzProbeCache::Key& b )
{
	return !(a==b);
}

