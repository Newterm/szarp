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
 * $Id$
 */

#ifndef EXTRACTION_H
#define EXTRACTION_H

#include "szbextr/extr.h"
#include "szarp_config.h"
#include <assert.h>
#include <vector>

struct extr_arguments {
        int debug_level;
        std::wstring date_format;
        time_t start_time;
        time_t end_time;
        int year;
        int month;
        SZARP_PROBE_TYPE probe;
        int probe_length;
        int raw;
        int openoffice;
        int csv;
        int xml;
        std::wstring output;
        std::wstring delimiter;
        int progress;
	std::vector<std::wstring> params;
        std::wstring no_data;
        int empty;
	std::wstring oo_script;
	wchar_t dec_delim;
};

/** Returns structure with extraction arguments initialized to default 
 * values.
 */
struct extr_arguments getExtrDefaultArguments();

/** Performs extraction.
 * @param my_args extraction arguments
 * @param progress_printer function used to print extraction progress,
 * NULL means no progress printing
 * @param progress_data parameter passed to progress_printer function
 * @param cancel_lval pointer to cancel controll value, ignored if NULL
 * @param ipk configuration info
 * @param szbase_buffer buffer for accessing database
 * @return return code from SzbExtractor::Extract() method, -1 on internal
 * function method (error opening file)
 */
int extract(
	struct extr_arguments my_args, 
	void* (*progress_printer)(int, void*),
	void* progress_data,
	int *cancel_lval,
	TSzarpConfig *ipk,
	szb_buffer_t *szbase_buffer
);

#endif

