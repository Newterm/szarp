/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/* 
 *
 * realdatablock
 * $Id$
 */

#ifndef __SZBASE_CACHABLE_DATABLOCK_H__
#define __SZBASE_CACHABLE_DATABLOCK_H__

#include <list>
#include <iostream>
#include "szbbuf.h"
#include "szbdatablock.h"
#include "boost/thread/mutex.hpp"
#include "boost/thread/recursive_mutex.hpp"

class CacheableDatablock;

struct cbe_str
{
	cbe_str(CacheableDatablock * b): block(b) {};
	CacheableDatablock * block;
};

typedef struct cbe_str CacheBlockEntry;

class RemoveBlockPredicate;

/** Base class for blocks with cache functionality */
class CacheableDatablock: public szb_datablock_t
{
	public:
		/** Constructor.
		 * @param szb_buffer_t owner of the block.
		 * @param p param.
		 * @param y year.
		 * @param m month.
		 */
		CacheableDatablock(szb_buffer_t * b, TParam * p, int y, int m);
		~CacheableDatablock();
		static std::wstring GetCacheRootDirPath(szb_buffer_t *buffer);
		static bool write_cache;
		static void ClearCache(szb_buffer_t *buffer);
		static void ResetCache(szb_buffer_t *buffer);
		static void ClearParamFromCache(szb_buffer_t *buffer, TParam *param);
	protected:
		void Cache();		/**< Caches the data to the cachefile. */
		bool LoadFromCache();	/**< Loads the data from the cachefile. */
		virtual bool IsCachable() {return this->fixedProbesCount == this->max_probes;};	
		std::wstring const cachepath;	/**< The path of the cachefile. */
		/** Checks if the cachefile is valid.
		 * @param probes number of probes stored in the cachefile.
		 */
		virtual bool IsCacheFileValid(int &probes, time_t *mdate = NULL);
	
		static std::list<CacheBlockEntry> cachequeue;
	private:
		friend class RemoveBlockPredicate;
		/** Creates directory structure for the cachefile.
		 * @return if the needed directory exists/was created.
		 */
		bool CreateCacheDirectory();	
		/** Creates path for the cachefile.
		 * @param p param.
		 * @param y year.
		 * @param m month.
		 * @return constructed path.
		 */
		std::wstring GetCacheFilePath(TParam * p, int y, int m);
		bool cached;
};

#endif //__SZBASE_CACHABLE_DATABLOCK_H__
