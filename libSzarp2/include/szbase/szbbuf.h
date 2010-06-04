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
#ifndef __SZBBUF_H__
#define __SZBBUF_H__

#include <config.h>

#include <limits.h>
#include <time.h>
#include <assert.h>

#include <vector>
#include <list>
#include <map>

#include <tr1/unordered_map>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "szbdefines.h"
#include "szarp_config.h"
#include "szbfile.h"
#include "szbdate.h"
#include "szbcancelhandle.h"

using std::tr1::unordered_map;

class TParam;
class TSzarpConfig;
class Szbase;

typedef class szb_buffer_str szb_buffer_t;

class szb_block_t;

class BlockLocator
{
	public:
		BlockLocator(szb_buffer_t* buff, szb_block_t* b);
		~BlockLocator();
		void Used();
		szb_block_t * block;
	private:
		szb_buffer_t* buffer;
		BlockLocator* newer;
		BlockLocator* older;
		BlockLocator* next_same_param;
		BlockLocator* prev_same_param;
};

class szb_block_t {
public:
	szb_buffer_t * const buffer;
	TParam * const param;
	const time_t start_time;
	const time_t end_time;
	int fixed_probes_count;
	BlockLocator *locator;
	virtual SZB_BLOCK_TYPE GetBlockType() = 0;
	time_t GetStartTime();
	time_t GetEndTime();
	int GetFixedProbesCount();	
	void MarkAsUsed();
	virtual const SZBASE_TYPE* GetData(bool refresh = true) = 0;
	szb_block_t(szb_buffer_t *buffer, TParam* param, time_t start_time, time_t end_time);
	virtual ~szb_block_t();
};

/** Base class for datablocks.
 * This class realizes basic functionality od datablock. */
class szb_datablock_t : public szb_block_t {
public:
	/** Constructor.
	 * @param szb_buffer_t owner of the block.
	 * @param p param.
	 * @param y year.
	 * @param m month.
	 */
	szb_datablock_t(szb_buffer_t * b, TParam * p, int y, int m); 
	/** Destructor. */
	virtual ~szb_datablock_t();

	int const year;			/**< Year for definable and combined. */
	int const month;		/**< Month. */

	const int max_probes;		/**< Size of data block. */

	virtual const SZBASE_TYPE * GetData(bool refresh=true);	/**< Returns pointer to the block`s data.
					 * Before that the block is refreshed*/
	
	virtual void FinishInitialization() {};
	bool IsInitialized(){ return initialized; }; /**< If the initialization of the block has successed. */
	int GetFirstDataProbeIdx() {return first_data_probe_index;};
	int GetLastDataProbeIdx() {return last_data_probe_index;};
	virtual void Refresh() =0;	/**< Refreshes the stored data. */
	time_t GetBlockTimestamp() {return this->block_timestamp;};
	std::wstring GetBlockRelativePath();
	static std::wstring GetBlockRelativePath(TParam * param, int year, int month);
	std::wstring GetBlockFullPath();
	static std::wstring GetBlockFullPath(szb_buffer_t* buffer, TParam * param, int year, int month);
	SZB_BLOCK_TYPE GetBlockType();

protected:
	void AllocateDataMemory();
	void FreeDataMemory();
	bool initialized;		/**< Flag for the block`s successful initialization. */
	SZBASE_TYPE * data;		/**< Array of values stored in the block. */
	
	int first_data_probe_index;
	int last_data_probe_index;
	
	time_t last_update_time;
	time_t block_timestamp;
};

class szb_probeblock_t : public szb_block_t {
public:
	double* data;
	long last_query_id;

	static const int probes_per_block = SZBASE_PROBES_IN_BLOCK;

	int GetParamDataFromServer(time_t start, time_t end, TParam* param);
	virtual void FetchProbes() = 0;
	virtual void Refresh();
	virtual SZB_BLOCK_TYPE GetBlockType();

	virtual const SZBASE_TYPE* GetData(bool refresh = true);
	szb_probeblock_t(szb_buffer_t *buffer, TParam* param, time_t start_time, time_t end_time);
	virtual ~szb_probeblock_t();
};


time_t
szb_get_last_av_date(szb_buffer_t * buffer);

time_t
szb_definable_meaner_last(szb_buffer_t * buffer);

typedef boost::tuple<TParam*, time_t> BufferKey;

class TupleComparer {
public:
	bool operator()(const BufferKey s1, const BufferKey s2) const
	{
		return s1 == s2;
	}
};

class TupleHasher {
	public:
		size_t operator()(const BufferKey s) const;
};

class ProberConnection;

/**
 * Buffer structure:
 * - buffer contains linear hash table of blocks
 * - each block  contains data  from one file
 * - each block remembers it's state, for example if file is open, if it is
 *   up-to-date and so on, it  may contain file descriptor (if file is opened).
 * - blocks are linked in bidrectional 'usage list' - it is used to manage
 *   blocks according to last access time
 * - buffer has pointers to first and last elements of usage lists.
 */
class szb_buffer_str {
private:
	friend class BlockLocator;

	unordered_map< BufferKey, szb_datablock_t*, TupleHasher, TupleComparer > datastorage;
	unordered_map< BufferKey, szb_probeblock_t*, TupleHasher, TupleComparer > probestorage;

	std::map< TParam* , BlockLocator* > paramindex;
#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
	std::vector<TParam*> optimized_params;
#endif
#endif
	BlockLocator* newest_block;
	BlockLocator* oldest_block;

	int blocks_c;		/**< Number of blocks in buffer. */
	int max_blocks;		/**< Maximum number of blocks allowed,
						-1 means no limit (default value). */

	void freeBlocks();
	int locked;		/**< flag, if not 0 blocks are not removed from buffer */

public:
	void Lock();
	void Unlock();
	bool PrepareConnection();
#ifndef NO_LUA
#if LUA_PARAM_OPTIMISE
	void AddExecParam(TParam *param);
	void RemoveExecParam(TParam* param);
#endif
#endif
	void AddBlock(szb_block_t* block);
	void DeleteBlock(szb_block_t* block);
	szb_buffer_str(int size);
	~szb_buffer_str();

	int last_err;		/**< code of last error */
	std::wstring last_err_string;	/**< textual descripton of last error*/


	std::wstring rootdir;	    /**< root directory for base */
	std::wstring prefix;	    /**< prefix for base */

	std::wstring meaner3_path; /**< path to status (for speed) */

	time_t first_av_date;	/**< first available date for definable params */

	int first_av_year;	/**< first available year - for speedup */
	int first_av_month;	/**< first available month - for speedup */

	TParam * first_param;	/** First param in base - for first/last 
				  available date */
	TParam * meaner3_param;	/** Param 'program uruchomiony' - for searching
				  last available date */

	Szbase *szbase;		/**< parent szbase object*/

	ProberConnection* prober_connection; /**< @see ProberConnection*/

	time_t configurationDate;

	time_t GetMeanerDate() {return szb_definable_meaner_last(this);};

	time_t GetConfigurationDate();

	std::wstring GetConfigurationFilePath();
	std::wstring GetSzbaseStampFilePath();

	void ClearParamFromCache(TParam* param);
	void ClearCache();
	void Reset();
	
	bool cachepoison;

	long last_query_id;

	time_t last_av_date;

	szb_datablock_t* FindDataBlock(TParam * param, int year, int month);

	szb_probeblock_t* FindProbeBlock(TParam * param, time_t start_time);
};


/** Creates new buffer.
 * @param object to parent @see Szbase object
 * @param directory path to base root directory
 * @param num maximum number of blocks in buffer
 * @param ipk pointer to configuration
 * @return pointer to allocated buffer, NULL if no memory is available or
 * num is less then 1
 */
szb_buffer_t *
szb_create_buffer(Szbase *szbase, const std::wstring& directory, int num, TSzarpConfig * ipk);

/** Destroys buffer. Closes all opened files (flushing data), frees 
 * memory for all data blocks and for buffer. 
 * @param buffer pointer to buffer to destroy, if NULL nothing happens
 * @return 0 on success, negative error code on error (for example on 
 * 'close()').
 */
int
szb_free_buffer(szb_buffer_t * buffer);

/** Resets buffer. Frees all loaded blocks.
 * @param buffer pointer to buffer
 */
void
szb_reset_buffer(szb_buffer_t * buffer);

/** Locks buffer. Prevents removing block from cache.
 * @param buffer pointer to buffer.
 */
void
szb_lock_buffer(szb_buffer_t * buffer);

/** Unlocks buffer. Allows removing blocks from cache.
 * Removes old block if number of block is greater
 * then max_blocks.
 * @param buffer pointer to buffer.
 */
void
szb_unlock_buffer(szb_buffer_t * buffer);

/** Sets how base filenames should look like. Filenames are created from
 * parameter name, year and month. 
 * @param buffer buffer for which we set filenames, may not be NULL
 * @param formatstring format for file names, arguments for format are: 
 * parameter name, year and month, default is something like 
 * %s.%04d%02d.szb, local copy of this value is created, if NULL default value
 * will be used
 * @param extraspace length of filename, except for parameter name length,
 * default is 12
 */
void
szb_set_format(szb_buffer_t * buffer, const std::wstring& formatstring, int extraspace);

/** Returns pointer to block for given param, year and month. 
 * @param buffer pointer to szbase buffer context
 * @param param parameter names, encoded by latin2szb
 * @param year year (4 digits)
 * @param month month (from 1 to 12)
 * @return pointer to data block, NULL if file for block does not exists
 * or couldn't be loaded, buffer->last_err is set to error code
 */
szb_datablock_t *
szb_get_datablock(szb_buffer_t * buffer, TParam * param, int year, int month);

szb_probeblock_t *
szb_get_probeblock(szb_buffer_t * buffer, TParam * param, time_t time);

szb_block_t *
szb_get_block(szb_buffer_t * buffer, TParam * param, time_t time, SZB_BLOCK_TYPE block_type);

/** Searches for block in buffer. Does not load blocks, only searches within
 * already loaded blocks.
 * @param buffer buffer pointer, mey not be NULL
 * @param param name of parameter encoded by latin2szb
 * @param year year (4 digits)
 * @param month month (from 1 to 12)
 * @return pointer to block found, NULL if not found
 */
szb_datablock_t * 
szb_find_block(szb_buffer_t * buffer, TParam * param, int year, int month);

/** Inserts new block into buffer. Block for given file must not exists
 * in buffer.
 * @param buffer pointer to szbase buffer
 * @param block pointer to data block to insert
 */
void
szb_insert_block(szb_buffer_t * buffer, szb_datablock_t* block);

/** Free given block (only one). */
void
szb_free_block(szb_datablock_t * block);

/** Return data from base for given parameter and time.
 * @param buffer data base buffer
 * @param param full name of parameter
 * @param time data date and time
 * @return data value, SZB_NODATA if data is not available
 */
SZBASE_TYPE
szb_get_data(szb_buffer_t * buffer, TParam * param, time_t time);

/** Return average value of param in given time period. Time period start at
 * first given date, end before second. It is more efficient not to look
 * for data beyond first and last date for param.
 * @param buffer pointer to data base buffer
 * @param param parameter
 * @param start_time begining of time period (inclusive)
 * @param end_time end of time period (exclusive)
 * @param psum pointer for sum of probes in period
 * @param pcount pointer for number of no no-data probes in period
 * @return average value, SZB_NODATA if no value was found in given time
 * period
 */
SZBASE_TYPE
szb_get_avg(szb_buffer_t * buffer, TParam * param,
	time_t start_time, time_t end_time, double * psum = NULL, int * pcount = NULL, SZARP_PROBE_TYPE pt = PT_MIN10, bool *isFixes = NULL);

/** Return value of probe of given type.
 * @param buffer pointer to data base buffer
 * @param param name of parameter
 * @param t time of probe, may be converted according to probe type
 * @param probe type of probe
 * @param custom_probe length in seconds for custom probe
 * @return probe value, may be SZB_NODATA
 */
SZBASE_TYPE
szb_get_probe(szb_buffer_t * buffer, TParam * param, time_t t,
	SZARP_PROBE_TYPE probe, int custom_probe = 0);


/** Return values of param from given period. Fills in the given buffer
 * with values of param.
 * @param buffer pointer to data base buffer
 * @param param name of parameter
 * @param start_time begining of time period (inclusive)
 * @param end_time end of time period (exclusive)
 * @param retbuf pointer to buffer, enough memory must be allocated - at
 * least (end_time - start_time) / SZBASE_DATA_SPAN * sizeof(SZBASE_TYPE)
 */
void
szb_get_values(szb_buffer_t * buffer, TParam * param,
		time_t start_time, time_t end_time, SZBASE_TYPE * retbuf, bool *isFixed = NULL);

/** Searches for not-null data in given time period.
 * @param buffer base buffer
 * @param param parameter
 * @param start search starts from that time
 * @param end search ends when reaching given time even if no data is found,
 * if value is (time_t)(-1) all data base is searched
 * @param direction direction of searching, less then 0 means left, greater then
 * 0 means right, 0 means that only start date is checked
 * @return -1 if no data was found, else date of first not empty record
 */
time_t
szb_search(szb_buffer_t * buffer, TParam * param,
		time_t start, time_t end, int direction, SZARP_PROBE_TYPE probe = PT_MIN10, SzbCancelHandle * c_handle = NULL);

/** Return time of begining of first file in base for given param.
 * It is the time of first record in base, but it doesn't 
 * check for empty records. If you want to know first time for parameter,
 * use szb_search_data(buffer, param, 0, -1, 1)
 * @param buffer pointer to buffer context
 * @param param name of parameter
 * @return time of first record in base for parameter, -1 of param doesn't 
 * exists in base.
 */
time_t
szb_search_first(szb_buffer_t * buffer, TParam * param);

/** Return time of end of last file in base for given param.
 * It is the time of last record in base, but it doesn't 
 * check for empty records. If you want to know last time for parameter,
 * use szb_search_data(buffer, param, time(), -1, -1)
 * @param buffer pointer to buffer context
 * @param param name of parameter
 * @return time of last record in base for parameter, -1 of param doesn't 
 * exists in base.
 */
time_t
szb_search_last(szb_buffer_t * buffer, TParam * param);

szb_probeblock_t* szb_create_probe_block(szb_buffer_t *buffer, TParam *param, time_t time);


#endif

