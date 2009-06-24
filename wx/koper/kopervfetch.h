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
 * koper - Kalkulator Premii
 * SZARP
 
 * Dariusz Marcinkiewicz
 *
 * $Id$
 */
#ifndef __KOPERVALFETCH_H__
#define __KOPERVALFETCH_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <vector>
#include <utility>
#include "crontime.h"
#include "liblog.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"

struct TimeSpec {
	ExecTimeType month;
	ExecTimeType day;
	ExecTimeType hour;
	ExecTimeType minute;
};
		

enum Tendency {
	INCREASING,
	DECREASING,
	NONE 
};

class KoperValueFetcher {
	struct ValueInfo {
		TimeSpec ts;
		struct tm t;	
		double val;
		double ival;
		time_t lft;
		Tendency tendency;
	};

	szb_buffer_t* m_szbbuf;
	TSzarpConfig* m_config;
	TParam *m_param;
	bool m_init;
	std::vector<ValueInfo> m_vt;

	bool Setup(std::vector<std::string>& vs, time_t ct);
	bool SetTime(time_t ct);
	bool ParseTime(TimeSpec *s, const std::string& t);
	SZBASE_TYPE GetHoursum(time_t st, time_t en);
public:
	KoperValueFetcher();
	int GetValuePrec();
	bool Init(const std::string& szarp_dir,
		const std::string& prefix,
		const std::string& param_name,
		const std::string& date1,
		const std::string& date2);
	void AdjustToTime(time_t t);
	Tendency GetTendency();
	double Value(size_t index);
	const struct tm& Time(size_t index);
	~KoperValueFetcher();
};

#endif
