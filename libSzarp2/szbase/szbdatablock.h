/* 
  SZARP: SCADA software 

*/
/*
 * szbase - szbdatablock.h
 * $Id$
 * <pawel@praterm.com.pl>
 */

#ifndef __SZBDATABLOCK_H__
#define __SZBDATABLOCK_H__

#include <config.h>

#include <limits.h>
#include <time.h>
#include <assert.h>

#include "szbdefines.h"
#include "szarp_config.h"
#include "szbfile.h"
#include "szbdate.h"

#define IS_INITIALIZED if(this->initialized) throw std::wstring("Datablock not initialized properly");
#define NOT_INITIALIZED {this->initialized = false; return;}
#define DATABLOCK_CREATION_LOG_LEVEL 8
#define DATABLOCK_REFRESH_LOG_LEVEL 8
#define DATABLOCK_CACHE_ACTIONS_LOG_LEVEL 8
#define SEARCH_DATA_LOG_LEVEL 8
#define MAX_PROBES 4500

typedef struct szb_buffer_str szb_buffer_t;

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
	
		szb_buffer_t * const buffer;	/**< Pointer to the buffer which owns the block. */
	
		szb_datablock_t* hash_next;	/**< Linear hash next list element, NULL
						 * for last element. */
		szb_datablock_t* older;		/**< Next element of 'usage' list. */
		szb_datablock_t* newer;		/**< Previous element of 'usage' list. */
		int const year;			/**< Year for definable and combined. */
		int const month;		/**< Month. */
		TParam * const param;		/**< Param. */
	
		const int max_probes;		/**< Size of data block. */

		virtual const SZBASE_TYPE * GetData(bool refresh=true);	/**< Returns pointer to the block`s data.
						 * Before that the block is refreshed*/
		
		virtual void FinishInitialization() {};
		bool IsInitialized(){ return initialized; }; /**< If the initialization of the block has successed. */
		//int GetProbesBeforeLastAvDate();	/**< Returns number of probes before last available date. */
		time_t GetBlockBeginDate();		/**< Returns date of the first probe from the block. */
		time_t GetBlockLastDate();		/**< Returns date of the last probe from the block. */
		int GetFixedProbesCount() {return first_non_fixed_probe;};
		int GetFirstDataProbeIdx() {return first_data_probe_index;};
		int GetLastDataProbeIdx() {return last_data_probe_index;};
		virtual void Refresh() =0;	/**< Refreshes the stored data. */
		time_t GetBlockTimestamp() {return this->block_timestamp;};
		std::wstring GetBlockRelativePath();
		static std::wstring GetBlockRelativePath(TParam * param, int year, int month);
		std::wstring GetBlockFullPath();
		static std::wstring GetBlockFullPath(szb_buffer_t* buffer, TParam * param, int year, int month);

		void MarkAsUsed(){this->locator->Used();};

		BlockLocator * locator;

	protected:
		void AllocateDataMemory() {assert(!data); data = new SZBASE_TYPE[this->max_probes];};
		void FreeDataMemory() {assert(data); delete[] data;};
		bool initialized;		/**< Flag for the block`s successful initialization. */
		SZBASE_TYPE * data;		/**< Array of values stored in the block. */
		
		int first_non_fixed_probe;
		int first_data_probe_index;
		int last_data_probe_index;
		
		time_t last_update_time;
		time_t block_timestamp;
};

#endif

