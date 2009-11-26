/* 
  SZARP: SCADA software 

*/
/*
 * Liczenie parametrow definiowalnych - nowa wersja
 * $Id$
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "liblog.h"

// #include "szarp_config.h"

#include "szbbuf.h"
#include "szbname.h"
#include "szbfile.h"
#include "szbdate.h"

#include "definabledatablock.h"

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/operations.hpp"

namespace fs = boost::filesystem;

int	szb_definable_error;	// kod ostatniego bledu
unsigned char szb_calcul_no_data;

double
szb_definable_choose_function(double funid, double *parlst);

#ifdef DEBUG_PV
#include "stdarg.h"
void
debug_str(char * msg_str, ...)
{
    va_list args;
    fprintf(stderr, "DEBUG: ");
    fprintf(stderr, msg_str, args);
}
#endif

#define DEFINABLE_STACK_SIZE 200

static szb_custom_function custom_functions[26];

void
szb_register_custom_function(char symbol, szb_custom_function function) {
	int index = symbol - 'A';

	if (index < 0 || index > (int)(sizeof(custom_functions) / sizeof(szb_custom_function))) 
		assert(false);

	custom_functions[index] = function;			

}

static bool
szb_execute_custom_function(char symbol, SZBASE_TYPE* stack, short *sp, int year, int month, int probe_n) {
	int index = symbol - 'A';
	szb_custom_function func;

	if (index < 0 
		|| index > (int)(sizeof(custom_functions) / sizeof(szb_custom_function)) 
		|| (func = custom_functions[index]) == NULL)
		return false;

	struct tm t;
	probe2local(probe_n, year, month, &t);
	(*func)(&t, stack, sp);
	return true;

}

/** Calculates value of one probe in block.
 * @param stack array usesd as stack
 * @param cache array of blocks used in formula calculation.
 * @param formula formula.
 * @param probe_n probe index.
 * @param param_cnt number of params in formula.
 * @param param IPK parameter object
 * @return calculated value.
 */
SZBASE_TYPE
szb_definable_calculate(szb_buffer_t *buffer, SZBASE_TYPE * stack, szb_datablock_t ** cache,
	const std::wstring& formula, int probe_n, int param_cnt, int year, int month, TParam* param)
{

	short sp = 0;
	const wchar_t *chptr = formula.c_str();
	int it = 0;	// licznik, do indeksowania parametrow

	szb_definable_error = 0;

	if (formula.empty()) {
		szb_definable_error = 1;
		sz_log(0, "Invalid, NULL formula");
		return SZB_NODATA;
	}

	do {
		if (iswdigit(*chptr)) {	
			chptr = wcschr(chptr, L' ');
	
			// check stack size
			if (sp >= DEFINABLE_STACK_SIZE) {
				sz_log(0,
					"Nastapilo przepelnienie stosu przy liczeniu formuly %ls",
					formula.c_str());
				return SZB_NODATA;
			}
	
			if (it >= param_cnt) {
				sz_log(0, "FATAL: in szb_definable_calculate: it > param_cnt");
				sz_log(0, " param: %ls", param->GetName().c_str());
				sz_log(0, " formula: %ls", formula.c_str());
				sz_log(0, " it: %d, param_cnt: %d, num: %d",
					it, param_cnt, param->GetNumParsInFormula());
				sz_log(0, " chptr: %ls", chptr);
				return NULL;
			}
	
			// put probe value on stack
			if (cache[it] != NULL) {
				const SZBASE_TYPE * data = cache[it]->GetData(false);
				if(!IS_SZB_NODATA(data[probe_n]))
					stack[sp++] = data[probe_n] * pow(10, cache[it]->param->GetPrec());
				else
					stack[sp++] = SZB_NODATA;
			} else {
				TParam** fc = param->GetFormulaCache();
				if (fc[it]->GetType() == TParam::P_LUA
					&& fc[it]->GetFormulaType() == TParam::LUA_AV) {

					time_t t = probe2time(probe_n, year, month);

					stack[sp++] = szb_get_data(buffer, fc[it], t);
					if (!IS_SZB_NODATA(stack[sp]))
						stack[sp] = stack[sp] * pow(10, cache[it]->param->GetPrec());
				} else {
					stack[sp++] = SZB_NODATA;
				}
			}
	
			it++;
		} else {
			SZBASE_TYPE tmp;
			short par_cnt;
			int i1, i2;
			switch (*chptr) {
				case L'&':
					if (sp < 2)	/* swap */
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp - 1]) || IS_SZB_NODATA(stack[sp - 2]))
						return SZB_NODATA;
					tmp = stack[sp - 1];
					stack[sp - 1] = stack[sp - 2];
					stack[sp - 2] = tmp;
					break;		
				case L'!':		
					if (IS_SZB_NODATA(stack[sp - 1]))
						return SZB_NODATA;

					// check stack size
					if (DEFINABLE_STACK_SIZE <= sp) {
					sz_log(0,
						"Przepelnienie stosu dla formuly %ls, w funkcji '!'",
						formula.c_str());
					return SZB_NODATA;
					}

					stack[sp] = stack[sp - 1];	/* duplicate */
					sp++;
					break;
				case L'$':
					if (sp-- < 2)	/* function call */
						return SZB_NODATA;
					par_cnt = (short) rint(stack[sp - 1]);
					if (sp < par_cnt + 1)
						return SZB_NODATA;
					for (int i = sp - 2; i >= sp - par_cnt - 1; i--)
						if (IS_SZB_NODATA(stack[i]))
							return SZB_NODATA;

					stack[sp - par_cnt - 1] =
					szb_definable_choose_function(stack[sp], &stack[sp - par_cnt - 1]);

					sp -= par_cnt;
					break;		
				case L'#':
					// check stack size
					if (DEFINABLE_STACK_SIZE <= sp) {
						sz_log(0,
							"Przepelnienie stosu dla formuly %ls, przy odkladaniu stalej: %lf",
							formula.c_str(), wcstod(++chptr, NULL));
						return SZB_NODATA;
					}
		
					stack[sp++] = wcstod(++chptr, NULL);
					chptr = wcschr(chptr, L' ');
					break;
				case L'?':
					if (*(++chptr) == L'f') {	
					/* warunek <par1> <par2> <cond> if
					* jesli <cond> != 0 zostawia <par1> 
					* w innym wypadku zostawia <par2>  
					*/
						if (sp-- < 3)
							return SZB_NODATA;

						if (0 == stack[sp])
							stack[sp - 2] = stack[sp - 1];
						sp--;
					}
					break;
				case L'+':
					if (sp-- < 2)
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
						return SZB_NODATA;
					stack[sp - 1] += stack[sp];
					break;
				case L'-':
					if (sp-- < 2)
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
						return SZB_NODATA;
					stack[sp - 1] -= stack[sp];
					break;
				case L'*':
					if (sp-- < 2)
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
						return SZB_NODATA;
					stack[sp - 1] *= stack[sp];
					break;
				case L'/':
					if (sp-- < 2)
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1] ))
						return SZB_NODATA;
					if (stack[sp] != 0.0)
						stack[sp - 1] /= stack[sp];
					else {
						sz_log(4, "WARRNING: szb_definable_calculate: dzielenie przez zero (i: %d)", probe_n);
							return SZB_NODATA;
					}
					break;
				case L'>':
					if (sp-- < 2)	/* wieksze */
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
						return SZB_NODATA;
					if (stack[sp - 1] > stack[sp])
						stack[sp - 1] = 1;
					else
						stack[sp - 1] = 0;
					break;
				case L'<':
					if (sp-- < 2)	/* mniejsze */
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
						return SZB_NODATA;
					if (stack[sp - 1] < stack[sp])
						stack[sp - 1] = 1;
					else
						stack[sp - 1] = 0;
					break;
				case L'~':
					if (sp-- < 2)	/* rowne */
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
						return SZB_NODATA;
					if (stack[sp - 1] == stack[sp])
						stack[sp - 1] = 1;
					else
						stack[sp - 1] = 0;
					break;
				case L':':
					if (sp-- < 2)	/* slowo */
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1]))
						return SZB_NODATA;
					i1 = (SZB_FILE_TYPE) rint(stack[sp - 1]);
					i2 = (SZB_FILE_TYPE) rint(stack[sp]);
					stack[sp - 1] = (SZBASE_TYPE) ((i1 << 16) | i2);
					break;
				case L'^':
					if (sp-- < 2)	/* potega */
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp]) || IS_SZB_NODATA(stack[sp - 1] ))
						return SZB_NODATA;
	
					// fix - PP 
					if (stack[sp] == 0.0) {
						// a^0 == 1 
						stack[sp - 1] = 1;
					} else if (stack[sp - 1] >= 0.0) {
						stack[sp - 1] = pow(stack[sp - 1], stack[sp]);
					} else {
						sz_log(4, "WARRNING: szb_definable_calculate: wyk�adnik pot�gi < 0 (i: %d)", probe_n);
						return SZB_NODATA;
					}
					break;
				case L'N':
					if (sp-- < 2)	/* Sprawdzenie czy SZB_NODATA */
						return SZB_NODATA;
					if (IS_SZB_NODATA(stack[sp - 1]))
						stack[sp - 1] = stack[sp];
					break;
				case L'X':
					// check stack size
					if (DEFINABLE_STACK_SIZE <= sp) {
						sz_log(0,
							"Przepelnienie stosu dla formuly %ls, w funkcji X",
							formula.c_str());
						return SZB_NODATA;
					}
					stack[sp++] = SZB_NODATA;
					break;
				case L'S':
					if (DEFINABLE_STACK_SIZE > sp) {
						TSzarpConfig *config = param->GetSzarpConfig();
						assert(config);
						const TSSeason *seasons;
						if ((seasons = config->GetSeasons())) {
							struct tm t;
							probe2local(probe_n, year, month, &t);
							stack[sp++] = seasons->IsSummerSeason(&t) ? 1 : 0;
							break;
						}
					} else {
						sz_log(0,
							"Przepelnienie stosu dla formuly %ls, w funkcji S",
							formula.c_str());
						return SZB_NODATA;
					} //Falls through if there is no seasons defined
				default:
					if (iswspace(*chptr))
						break;
					if (!szb_execute_custom_function(*chptr, stack, &sp, year, month, probe_n))
						return SZB_NODATA;
					break;
			}
		}
	} while (chptr && *(++chptr) != 0);

	if (szb_definable_error)
	return SZB_NODATA;

	if (sp-- < 0) {
		sz_log(5, "ERROR: szb_definable_calculate: sp-- < 0");
		szb_definable_error = 1;
		return SZB_NODATA;
	}

	if (IS_SZB_NODATA(stack[0])) {
		sz_log(10, "WARRNING: szb_definable_calculate: stack[0] == SZB_NODATA");
		return SZB_NODATA;
	}

	return stack[0];
}

/** Search first available probe in given period.
 * @param buffer pointer to cache buffer
 * @param param  parameter
 * @param start_time begin of period
 * @param end_time   end of period
 * @param direction  direction of search
 * @return date of first available probe
 */
time_t
szb_definable_search_data(szb_buffer_t * buffer, TParam * param, 
	time_t start_time, time_t end_time, int direction)
{
#ifdef KDEBUG
	sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: %s, s:%ld, e:%ld, d:%d",
		param->GetName(),
		(unsigned long int) start_time, (unsigned long int) end_time,
		direction);
#endif

	time_t first_available = buffer->first_av_date; 
	time_t last_available = szb_search_last(buffer, param);

#ifdef KDEBUG
	sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: fad:%ld, lad:%ld",
		(unsigned long int) first_available,
		(unsigned long int) last_available);
#endif

	if (param->IsConst()) {
		if (start_time >= first_available && start_time <= last_available) {
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", start_time);
#endif
			return start_time;
		}

		switch (direction) {
			case -1:
				if (end_time < 0 || end_time < first_available)
					end_time = first_available;
				if (end_time < 0 || end_time > last_available) {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
					return -1;
				}

				if (start_time < 0 || start_time > last_available)
					start_time = last_available;
				if (start_time >= 0 && start_time > end_time) {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", start_time);
#endif
					return start_time;
				} else {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
					return -1;
				}
				break;
			case 1:
				if (end_time < 0 || end_time > last_available)
					end_time = last_available;
				if (end_time < first_available) {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
					return -1;
				}

				if (start_time < 0 || start_time < first_available)
					start_time = first_available;
				if (start_time < 0 || start_time > last_available) {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
					return -1;
				} else {
#ifdef KDEBUG
					sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", start_time);
#endif
					return start_time;
				}
			default:
#ifdef KDEBUG
				sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
				return -1;
		}
	}

#ifdef DEBUG
	debug_str("ndef.c (641): last_available = %s\n", ctime(&last_available));
#endif

	if ((start_time >= first_available && start_time <= last_available)
			&& !IS_SZB_NODATA(szb_get_data(buffer, param, start_time))) {
#ifdef KDEBUG
		sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", start_time);
#endif
		return start_time;
	}

	time_t time = -1;

	switch (direction) {
		case -1: // search left
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_definable_search_last: searching left");
#endif
			if (end_time < 0 || end_time < first_available)
				end_time = first_available;
			if (end_time < 0 || end_time > last_available) {
#ifdef KDEBUG
				sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
				return -1;
			}

			if (start_time < 0 || start_time > last_available)
				start_time = last_available;
			if (start_time >= 0 && start_time > end_time) {
				int probe_n, month = 0, year = 0;
				int new_year, new_month;
				szb_datablock_t * block = NULL;

				szb_time2my(start_time, &year, &month);
#ifdef KDEBUG
				sz_log(9, "szb_definable_search_data: s_t:%ld, e_t:%ld",
					(unsigned long int) start_time, (unsigned long int) end_time);
#endif
				const SZBASE_TYPE * data = NULL;
				for (time = start_time; time >= end_time; time -= SZBASE_PROBE) {
					szb_time2my(time, &new_year, &new_month);
					if (block == NULL || new_month != month || new_year != year) {
						block = szb_get_block(buffer, param, new_year, new_month);
						if (block != NULL)
							data = block->GetData();
						if (NULL == block || block->GetFirstDataProbeIdx() < 0) {
							time = probe2time(0, new_year, new_month);
							continue;
						}
						year = new_year;
						month = new_month;
					}
					/* check for data at index */
					probe_n = szb_probeind(time);
		
					assert(!IS_SZB_NODATA(data[block->GetFirstDataProbeIdx()]));
					assert(!IS_SZB_NODATA(data[block->GetLastDataProbeIdx()]));
		
					if(probe_n > block->GetLastDataProbeIdx()) { //If index is after last data jump to last data in next iteration
						time = probe2time(block->GetLastDataProbeIdx() + 1, new_year, new_month);
						continue;
					}
					if(probe_n < block->GetFirstDataProbeIdx()) { //If index is before first data jump to the previous block in next iteration
						time = probe2time(0, new_year, new_month);
						continue;
					}
		
					if ( !IS_SZB_NODATA(data[probe_n]) ) {
#ifdef KDEBUG
						sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", time);
#endif
						return time;
					}
				}
			}
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
			return -1;		
		case 1: // search right
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "S: szb_definable_search_data: searching right");
#endif
			if (end_time < 0)
				end_time = last_available;

			if (end_time < first_available) {
#ifdef KDEBUG
				sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
				return -1;
			}

			if (start_time < 0 || start_time < first_available)
				start_time = first_available;
			if (start_time < 0 || start_time > last_available) {
#ifdef KDEBUG
				sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
				return -1;
			}

			if (end_time >= 0) {
				int probe_n, month = 0, year = 0;
				int new_year, new_month;
				szb_datablock_t * block = NULL;

				szb_time2my(start_time, &year, &month);

				const SZBASE_TYPE * data = NULL;
				for (time = start_time; time <= end_time; time += SZBASE_PROBE) {
					szb_time2my(time, &new_year, &new_month);
					if (block == NULL || new_month != month || new_year != year) {
						block = szb_get_block(buffer, param, new_year, new_month);
						if (NULL != block)
							data = block->GetData();
						if (NULL == block || block->GetFirstDataProbeIdx() < 0) {
							time = probe2time(szb_probecnt(new_year, new_month) - 1, new_year, new_month);
							continue;
						}
						data = block->GetData();
						year = new_year;
						month = new_month;
					}
					probe_n = szb_probeind(time);
					assert(probe_n >= 0);
			
					assert(!IS_SZB_NODATA(data[block->GetFirstDataProbeIdx()]));
					assert(!IS_SZB_NODATA(data[block->GetLastDataProbeIdx()]));
			
					if(probe_n > block->GetLastDataProbeIdx()) { //If index is after last data jump to the next block
						time = probe2time(block->max_probes - 1, new_year, new_month);
						continue;
					}
					if(probe_n < block->GetFirstDataProbeIdx()) { //If index is before first data jump first data in next iteration
						time = probe2time(block->GetFirstDataProbeIdx() - 1, new_year, new_month);
						continue;
					}
			
					if (!IS_SZB_NODATA(block->GetData()[probe_n])) {
#ifdef KDEBUG
						sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return %ld", time);
#endif
						return time;
					}
				}
			}
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
			return -1;
		default:
#ifdef KDEBUG
			sz_log(SEARCH_DATA_LOG_LEVEL, "szb_definable_search_data: return -1 ");
#endif
			return -1;
	}

	return time;
}

#define MAX_FID 8


double
szb_fixo(double *parlst)
{
    double Fi[3][3][33] = {
	{
	    {1.00, 0.97, 0.95, 0.92, 0.89, 0.87, 0.84, 0.82, 0.79, 0.76, 0.74, 0.71,
		0.68, 0.66, 0.63, 0.61, 0.58, 0.55, 0.53, 0.50, 0.47, 0.45, 0.42, 0.39,
		0.37, 0.34, 0.32, 0.29, 0.26, 0.24, 0.21, 0.18, 0.16},
	    {1.04, 1.01, 0.99, 0.96, 0.93, 0.90, 0.88, 0.85, 0.82, 0.79, 0.77, 0.74,
		0.71, 0.68, 0.65, 0.63, 0.60, 0.57, 0.55, 0.52, 0.49, 0.47, 0.44, 0.41,
		0.38, 0.36, 0.33, 0.30, 0.27, 0.25, 0.22, 0.19, 0.16},
	    {1.07, 1.04, 1.01, 0.99, 0.96, 0.93, 0.90, 0.87, 0.84, 0.82, 0.79, 0.76,
		0.73, 0.70, 0.68, 0.65, 0.62, 0.59, 0.56, 0.53, 0.51, 0.48, 0.45, 0.42,
		0.39, 0.37, 0.34, 0.31, 0.28, 0.25, 0.23, 0.20, 0.17}
	},
	{
	    {0.99, 0.96, 0.93, 0.91, 0.88, 0.85, 0.82, 0.80, 0.77, 0.74, 0.71, 0.69,
		0.66, 0.63, 0.60, 0.58, 0.55, 0.52, 0.49, 0.47, 0.44, 0.41, 0.38, 0.35,
		0.33, 0.30, 0.27, 0.24, 0.22, 0.19, 0.16, 0.13, 0.11},
	    {1.03, 1.00, 0.97, 0.94, 0.91, 0.89, 0.86, 0.83, 0.80, 0.77, 0.74, 0.71,
		0.68, 0.66, 0.63, 0.60, 0.57, 0.54, 0.51, 0.48, 0.45, 0.43, 0.40, 0.37,
		0.34, 0.31, 0.28, 0.25, 0.22, 0.20, 0.17, 0.14, 0.11},
	    {1.06, 1.03, 1.00, 0.97, 0.94, 0.91, 0.88, 0.85, 0.82, 0.79, 0.76, 0.73,
		0.70, 0.67, 0.65, 0.62, 0.59, 0.56, 0.53, 0.50, 0.47, 0.44, 0.41, 0.38,
		0.35, 0.32, 0.29, 0.26, 0.23, 0.20, 0.17, 0.14, 0.11}
	},
	{
	    {0.98, 0.95, 0.92, 0.89, 0.86, 0.84, 0.81, 0.78, 0.75, 0.72, 0.69, 0.66,
		0.63, 0.60, 0.57, 0.55, 0.52, 0.49, 0.46, 0.43, 0.40, 0.37, 0.34, 0.31,
		0.29, 0.26, 0.23, 0.20, 0.17, 0.14, 0.11, 0.06, 0.05},
	    {1.02, 0.99, 0.98, 0.98, 0.90, 0.87, 0.84, 0.81, 0.78, 0.75, 0.72, 0.69,
		0.66, 0.63, 0.60, 0.57, 0.54, 0.51, 0.48, 0.43, 0.42, 0.39, 0.36, 0.33,
		0.30, 0.27, 0.24, 0.21, 0.18, 0.15, 0.12, 0.09, 0.06},
	    {1.05, 1.02, 0.99, 0.96, 0.92, 0.89, 0.86, 0.83, 0.80, 0.79, 0.74, 0.71,
		0.68, 0.65, 0.61, 0.58, 0.55, 0.51, 0.49, 0.44, 0.43, 0.42, 0.37, 0.34,
		0.31, 0.27, 0.24, 0.21, 0.18, 0.15, 0.12, 0.09, 0.05}
	}
    };
  
    short x = (short)rint(parlst[0]);	/* Pogoda */
    short y = (short)rint(parlst[1]);	/* Wiatr */
    short z = (short)rint(parlst[2]);	/* Tzewn */
    
    if (x < 0)
	x = 0;
    if (x > 2)
	x = 2;
    if (y < 3)
	y = 0;
    else if (y < 8)
	y = 1;
    else
	y = 2;
    if (z < -20)
	z = -20;
    if (z > 12)
	z = 12;
    z += 20;

    return Fi[x][y][z];
}

double
szb_tab_reg_zas(double *parlst)
{
  double Tz[81] = {
      65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.7,
      66.4, 67.7, 68.9, 70.2, 71.4, 72.7, 73.9, 75.1, 76.3, 77.6,
      78.8, 80.0, 81.2, 82.4, 83.6, 84.8, 86.0, 87.2, 88.4, 89.6,
      90.7, 91.9, 93.0, 94.2, 95.3, 96.5, 97.6, 98.8, 99.9, 101.1,
      102.2, 103.3, 104.4, 105.6, 106.7, 107.8, 108.9, 110.0, 111.1,
      112.3, 113.4, 114.5, 115.5, 116.6, 117.7, 118.8, 119.9, 121.0,
      122.1, 123.2, 124.3, 125.4, 126.5, 127.6, 128.7, 129.9, 131.0, 
      132.1, 133.1, 134.2, 135.3, 136.4, 137.5, 138.6, 139.7, 140.8,
      141.9, 143.0, 144.0, 145.1, 146.2
  };

  short x = (short) rint(parlst[0] * 100.0);

  if (x < 20)
      x = 20;
  if (x > 100)
      x=100;
  x -= 20;
  
  return Tz[x];
}  

double
szb_tab_reg_pow(double *parlst)
{
    double Tp[81] = {
	49.3, 48.6, 47.9, 47.2, 46.4, 45.7, 45.0, 44.3, 43.5, 43.1,
	42.6, 42.9, 43.2, 44.2, 45.2, 45.8, 46.3, 46.9, 47.4, 47.9,
	48.4, 49.0, 49.5, 50.0, 50.4, 51.0, 51.5, 52.0, 52.5, 53.0,
	53.4, 53.9, 54.4, 54.9, 55.4, 55.9, 56.3, 56.8, 57.3, 57.7,
	58.1, 58.6, 59.1, 59.6, 60.0, 60.5, 60.9, 61.4, 61.8, 62.3,
	62.7, 63.1, 63.5, 64.0, 64.4, 64.8, 65.2, 65.7, 66.1, 66.5,
	66.9, 67.4, 67.8, 68.2, 68.6, 69.1, 69.5, 69.9, 70.3, 70.7,
	71.1, 71.6, 72.0, 72.4, 72.7, 73.2, 73.6, 74.0, 74.4, 74.8,
	75.1
    };
    
    short x=(short)rint(parlst[0]*100.0);
    if (x < 20)
	x = 20;
    else if (x > 100)
	x = 100;
    
    x -= 20;

    return Tp[x];
} 

double
szb_positive(double *parlst)
{
    return (parlst[0] > 0.0 ? parlst[0] : 0.0);
}

double
szb_sum_existing(double *parlst)
{
    double sum = 0;
    unsigned char valid = 0;
    int max = (int) rint(parlst[0]);

    for(int i = 1; i <= max; i++)
	if (parlst[i] > -1e20) {
	    sum += parlst[i];
	    valid += 1;
	}
    
    if (valid)
	szb_calcul_no_data = 0;
    else
	szb_calcul_no_data = 1; 

    return sum;
}

double
szb_cnt_existing(double *parlst)
{
    double sum = 0;
    int max = (int) rint(parlst[0]);
    
    for(int i = 1; i<= max; i++)
	if (parlst[i] >- 1e20)
	    sum += 1;

    szb_calcul_no_data = 0;

    return sum;
}

double
szb_water_tank_suwa(double *parlst)
{
    double H2Vtab[22]={
	0.0,  0.8,  2.4,  4.4,  6.6,  9.0, 11.6, 14.4, 17.2,
	20.0, 23.0, 25.8, 28.8, 31.6, 34.4, 37.0, 39.4, 41.6,
	43.6, 45.2, 46.0, 46.0
    };

    if (parlst[0] < 0.0)
	parlst[0] = 0.0;
    else if (parlst[0] > 100.0)
	parlst[0] = 100.0;
    
    int ind = (int) rint(floor(parlst[0] / 5.0));
    double res = (H2Vtab[ind + 1] - H2Vtab[ind]) / 5.0 * (parlst[0] - (double)ind * 5.0) + H2Vtab[ind];
    return res * 10;
}

double
szb_bit_of_log(double *parlst)
{
    int val = (int) rint(parlst[0]);
    int sh = (int) rint(parlst[1]);
    int res = ((val&(1<<sh))!=0);
    /*  printf("BitOfLog(%d,%d)=%d\n", val, sh, res);  */
    return (double)res;
}

double (*szb_def_fun_table[MAX_FID])(double*) = {
    szb_fixo, szb_tab_reg_zas, szb_tab_reg_pow, szb_positive,
    szb_sum_existing, szb_cnt_existing, szb_water_tank_suwa,
    szb_bit_of_log
};

double
szb_definable_choose_function(double funid, double *parlst)
{
    ushort fid = (ushort) funid;

    if (fid >= MAX_FID)
	return (0.0);

    return ((*(szb_def_fun_table[fid])) (parlst));
}

DefinableDatablock::DefinableDatablock(szb_buffer_t * b, TParam * p, int y, int m): CacheableDatablock(b, p, y, m)
{
	#ifdef KDEBUG
	sz_log(DATABLOCK_CREATION_LOG_LEVEL, "D: DefinableDatablock::DefinableDatablock(%s, %d.%d)", param->GetName(), year, month);
	#endif

	time_t updatetime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	this->AllocateDataMemory();

	if (year < this->buffer->first_av_year)
		NOT_INITIALIZED;

	if (year == this->buffer->first_av_year && month < this->buffer->first_av_month)
		NOT_INITIALIZED;

	if (LoadFromCache())
	{
		Refresh();
		return;
	}

	this->lastUpdateTime = updatetime;
	
	// local copy for readability
	const std::wstring& formula = this->param->GetDrawFormula();
	int num_of_params = this->param->GetNumParsInFormula();

	#ifdef KDEBUG
	sz_log(9, "  f: %ls, n: %d", formula.c_str(), num_of_params);
	#endif

	szb_datablock_t * dblocks[num_of_params];

	// prevent removing blocks from cache
	szb_lock_buffer(this->buffer);

	int probes_to_compute;
	if (num_of_params > 0) {
		probes_to_compute = this->GetBlocksUsedInFormula(dblocks, this->fixedProbesCount);
	}
	else {
		if(this->lastUpdateTime > this->GetBlockLastDate())
			probes_to_compute = this->fixedProbesCount = this->max_probes;
		else if(this->lastUpdateTime < this->GetBlockBeginDate()) {
			NOT_INITIALIZED;
		} else
			probes_to_compute = this->fixedProbesCount = szb_probeind(szb_search_last(buffer, param)) + 1;
	}

	assert(this->fixedProbesCount <= this->max_probes);

	if(probes_to_compute <= 0)
		NOT_INITIALIZED;

	/* if N is used or no params in formula we must calculate probes to last_av_date */
// 	if (param->IsNUsed() || 0 == num_of_params) {
// 		this->probes_c = GetProbesBeforeLastAvDate();
// 	}

	double pw = pow(10, param->GetPrec());

	SZBASE_TYPE  stack[DEFINABLE_STACK_SIZE]; // stack for calculatinon of formula

#ifdef KDEBUG
	sz_log(10, "V szb_definable_calculate_block: probes_to_compute: %d, max: %d, N? %s",
		probes_to_compute, this->max_probes, param->IsNUsed() ? "YES" : "NO");
#endif	

	int i = 0;
	for (; i < probes_to_compute; i++) {
		this->data[i] = szb_definable_calculate(b, stack, dblocks, formula, i, num_of_params, year, month, param) / pw;

		if(!IS_SZB_NODATA(this->data[i])) {
			if(this->firstDataProbeIdx < 0)
				this->firstDataProbeIdx = i;
			this->lastDataProbeIdx = i;
		}

		if (0 != szb_definable_error) { // error
			sz_log(1,
				"D: szb_definable_calculate_block: error %d for param: %ls date: %d.%d",
				szb_definable_error, this->param->GetName().c_str(), this->year, this->month);
			szb_unlock_buffer(this->buffer);
			NOT_INITIALIZED;
		}
	}

	if(this->firstDataProbeIdx >= 0) {
		assert(!IS_SZB_NODATA(this->data[this->GetFirstDataProbeIdx()]));
		assert(!IS_SZB_NODATA(this->data[this->GetLastDataProbeIdx()]));
	}

	for (; i < this->max_probes; i++) {
		this->data[i] = SZB_NODATA;
	}

	szb_unlock_buffer(this->buffer);

	return;
}

int
DefinableDatablock::GetBlocksUsedInFormula(szb_datablock_t ** dblocks, int &fixedprobes)
{
	int probes = 0;
	int num_of_params = this->param->GetNumParsInFormula();
	TParam ** f_cache = this->param->GetFormulaCache();

	fixedprobes = max_probes;

	for (int i = 0; i < num_of_params; i++) {

		dblocks[i] = szb_get_block(this->buffer, f_cache[i], this->year, this->month);

		if(dblocks[i] == NULL) {
			fixedprobes = 0;
			continue;
		}
		dblocks[i]->Refresh();

		switch (f_cache[i]->GetType()) {
			case TParam::P_REAL:
			case TParam::P_COMBINED:
			case TParam::P_DEFINABLE:
			case TParam::P_LUA:
				probes = probes > dblocks[i]->GetLastDataProbeIdx() + 1 ? probes : dblocks[i]->GetLastDataProbeIdx() + 1;
				fixedprobes = fixedprobes < dblocks[i]->GetFixedProbesCount() ? fixedprobes : dblocks[i]->GetFixedProbesCount();
				this->block_timestamp = this->block_timestamp > dblocks[i]->GetBlockTimestamp() ? this->block_timestamp : dblocks[i]->GetBlockTimestamp();
				break;
		}
	}

	if(probes < fixedprobes)
		probes = fixedprobes;

	if(this->GetBlockLastDate() < this->lastUpdateTime) {
		probes = max_probes;
	} else if (this->GetBlockBeginDate() < this->lastUpdateTime) {
		int tmp = szb_probeind(szb_search_last(buffer, this->param)) + 1;
		if(tmp > probes)
			probes = tmp;
	}

	return probes;
}

void
DefinableDatablock::Refresh()
{
	// block is full - no more probes can be load
	if (this->fixedProbesCount == this->max_probes)
		return;

	time_t updatetime = szb_round_time(buffer->GetMeanerDate(), PT_MIN10, 0);

	if (this->lastUpdateTime == updatetime)
		return;

	this->lastUpdateTime = updatetime;

	sz_log(DATABLOCK_REFRESH_LOG_LEVEL, "DefinableDatablock::Refresh() '%ls'", this->GetBlockRelativePath().c_str());

	// local copy for readability
	const std::wstring& formula = this->param->GetDrawFormula();
	int num_of_params = this->param->GetNumParsInFormula();

	szb_datablock_t * dblocks[num_of_params];

	// prevent removing blocks from cache
	szb_lock_buffer(this->buffer);

	int new_probes_c = 0;
	int new_fixed_probes;

	if (num_of_params > 0) {
		new_probes_c = GetBlocksUsedInFormula(dblocks, new_fixed_probes);
	}
	else {
		if(this->lastUpdateTime > this->GetBlockLastDate())
			new_probes_c = new_fixed_probes = this->max_probes;
		else
			new_probes_c = new_fixed_probes = szb_probeind(szb_search_last(buffer, param)) + 1;
	}
	
	assert(new_probes_c > 0);

// 	if (num_of_params == 0 || this->param->IsNUsed()) {
// 		new_probes_c = GetProbesBeforeLastAvDate();
// 	}

	assert(new_fixed_probes >= this->fixedProbesCount);

// 	if (new_fixed_probes <= this->fixedProbesCount) {
// 		// size havent change - nothing to calculate;
// 		szb_unlock_buffer(buffer);
// 		return; 
// 	}

	SZBASE_TYPE stack[DEFINABLE_STACK_SIZE]; // stack for calculatinon of formula

	double pw = pow(10, this->param->GetPrec());

	if(this->firstDataProbeIdx >=fixedProbesCount)
		firstDataProbeIdx = -1;
	if(this->lastDataProbeIdx >=fixedProbesCount)
		lastDataProbeIdx = -1;

	for (int i = this->fixedProbesCount; i < new_probes_c; i++) {
		this->data[i] = szb_definable_calculate(buffer, stack, dblocks, formula, i, num_of_params, this->year, this->month, this->param) / pw;

		if(!IS_SZB_NODATA(this->data[i])) {
			if(this->firstDataProbeIdx < 0 || i < this->firstDataProbeIdx)
				this->firstDataProbeIdx = i;
			this->lastDataProbeIdx = i;
		}

		if (0 != szb_definable_error) { // error
			sz_log(1,
				"D: szb_definable_calculate_block: error %d for param: %ls date: %d.%d",
				szb_definable_error, this->param->GetName().c_str(), this->year, this->month);
			break;
		}
	}

	if (this->firstDataProbeIdx >= 0) {
		assert(!IS_SZB_NODATA(this->data[this->GetFirstDataProbeIdx()]));
		assert(!IS_SZB_NODATA(this->data[this->GetLastDataProbeIdx()]));
	}

	this->fixedProbesCount = new_fixed_probes;

	szb_unlock_buffer(buffer);

	return;
}

bool DefinableDatablock::IsCachable()
{
	TParam** params = this->param->GetFormulaCache();
	for(int i = 0; i < this->param->GetNumParsInFormula(); i++){
		if(!TestParam(params[i], this->year, this->month, this->block_timestamp)){
			return false;
		}
	}
	return true;
}

bool
DefinableDatablock::IsCacheFileValid(int &probes, time_t *mdate) {
	time_t ts;

	if(!CacheableDatablock::IsCacheFileValid(probes, &ts))
		return false;

	TParam** params = this->param->GetFormulaCache();

	for(int i = 0; i < this->param->GetNumParsInFormula(); i++){
		if(!TestParam(params[i], this->year, this->month, ts))
			return false;
	}

	if (mdate != NULL)
		*mdate = ts;

	return true;
}

bool
DefinableDatablock::TestParam(TParam *p, int year, int month, time_t t) {
	switch (p->GetType()) {
		case TParam::P_REAL:
			{
				std::wstring tmppath = szb_datablock_t::GetBlockFullPath(buffer, p, year, month);

				try {
					time_t mt;

					if (fs::exists(tmppath)) {
						try {
							mt = fs::last_write_time(tmppath);
						} catch (fs::wfilesystem_error &e) {
							sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
								"DefinableDatablock::TestParam: cannot retrive modification date for: '%ls'", 
								tmppath.c_str());
							return false;
						}

						if (mt >= t) {
							sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL,
									"DefinableDatablock::TestParam: referenced param is newer: '%ls'", 
									tmppath.c_str());
							return false;
						} else {
							sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL,
									"DefinableDatablock::TestParam: referenced file is valid: '%ls'", 
									tmppath.c_str());
							return true;
						}
					}
					sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
							"DefinableDatablock::TestParam: no file: '%ls'",
							tmppath.c_str());

					tmppath = ((fs::wpath(tmppath)).remove_leaf()).string(); //now we`ll check directory
					if (fs::exists(tmppath)) {
						try {
							mt = fs::last_write_time(tmppath);
						} catch (fs::wfilesystem_error) {
							sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL, 
								"DefinableDatablock::TestParam: cannot retrive modification date for: '%ls'", 
								tmppath.c_str());
							return false;
						}

						if (mt >= t) {
							sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL,
									"DefinableDatablock::TestParam: directory of referenced param is newer: '%ls'",
									tmppath.c_str());
							return false;
						} else {
							sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL,
									"DefinableDatablock::TestParam: directory referenced file is valid: '%ls'", 
									tmppath.c_str());
							return true;
						}
					}

					return false;

				} catch (fs::wfilesystem_error &e) {
					sz_log(DATABLOCK_CACHE_ACTIONS_LOG_LEVEL,
							"DefinableDatablock: filesystem_error %ls",
							tmppath.c_str());
					return true;
				}
				break;
			}
		case TParam::P_COMBINED:
		case TParam::P_DEFINABLE:
			{
				TParam** params = p->GetFormulaCache();
				for(int i = 0; i < p->GetNumParsInFormula(); i++){
					if(!TestParam(params[i], this->year, this->month, t))
						return false;
				}
			}
			break;
		case TParam::P_LUA:
			{
				if (p->GetFormulaType() == TParam::LUA_AV)
					return false;
				else
					return true;
			}
			break;
	}
	return true;

}
