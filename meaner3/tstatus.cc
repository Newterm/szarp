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
 * meaner3 - daemon writing data to base in SzarpBase format
 * SZARP
 * status parameters
 
 * Pawe³ Pa³ucha pawel@praterm.com.pl
 
 * $Id$
 */

#include "meaner3.h"
#include "tstatus.h"


const wchar_t * TStatus_ParamNames[TStatus::PT_SIZE] = {
	L"Status:Meaner3:program uruchomiony",
	L"Status:Meaner3:ilo\u015b\u0107 parametr\u00f3w",
	L"Status:Meaner3:ilo\u015b\u0107 parametr\u00f3w ze sterownik\u00f3w",
	L"Status:Meaner3:ilo\u015b\u0107 parametr\u00f3w definiowalnych",
	L"Status:Meaner3:ilo\u015b\u0107 parametr\u00f3w definiowalnych przegl\u0105daj\u0105cego",
	L"Status:Meaner3:ilo\u015b\u0107 parametr\u00f3w zapisywanych do bazy",
	L"Status:Meaner3:ilo\u015b\u0107 parametr\u00f3w zapisywanych niepustych",
	L"Status:Meaner3:ilo\u015b\u0107 parametr\u00f3w poprawnie zapisanych",
	L"Status:Meaner3:ilo\u015b\u0107 parametr\u00f3w zapisanych z b\u0142\u0119dem",
	L"Status:Execute:ilo\u015b\u0107 skonfigurowanych sekcji",
	L"Status:Execute:ilo\u015b\u0107 sekcji do uruchomienia w cyklu",
	L"Status:Execute:ilo\u015b\u0107 sekcji uruchomionych w cyklu",
	L"Status:Execute:ilo\u015b\u0107 sekcji zako\u0144czonych z sukcesem",
	L"Status:Execute:ilo\u015b\u0107 sekcji zako\u0144czonych z b\u0142\u0119dem",
	L"Status:Execute:ilo\u015b\u0107 sekcji przerwanych"
};

int TStatus_SaveShift[TStatus::PT_SIZE] = {
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	-1,
	-1,
	-1,
	-1,
	-1,
	-1
};

int TStatus_ResetParam[TStatus::PT_SIZE] = {
	0,
	0,
	0,
	0,
	0,
	0,
	1,
	1,
	1,
	0,
	0,
	0,
	1,
	1,
	1
};

TStatus::TStatus()
{
	firstTime = true;
	for (int i = 0; i < PT_SIZE; i++)
		Reset((ParamType)i);
	Set(PT_RUN, 1);
}

void TStatus::Reset(ParamType pt)
{
	Set(pt, 0);
}

void TStatus::ResetAll()
{
	for (int i = 0; i < PT_SIZE; i++)
		if (TStatus_ResetParam[i])
	 		Reset((ParamType)i);
	Set(PT_RUN, 1);
}

void TStatus::NextCycle()
{
	ResetAll();
	if (firstTime)
	{
		Set(PT_RUN, 0);
		firstTime = false;
	}
	else
		Set(PT_RUN, 1);
}

void TStatus::Set(ParamType pt, SZB_FILE_TYPE val)
{
	vals[pt] = val;
}


SZB_FILE_TYPE TStatus::Incr(ParamType pt)
{
	return ++vals[pt];
}

SZB_FILE_TYPE TStatus::Decr(ParamType pt)
{
	return --vals[pt];
}

int TStatus::GetCount()
{
	return PT_SIZE;
}

const wchar_t * TStatus::GetName(ParamType pt)
{
	return TStatus_ParamNames[pt];
}

int TStatus::WriteParam(ParamType pt, time_t t, TSaveParam *sp,
		const fs::wpath& datadir)
{
	return sp->Write(datadir, t + (BASE_PERIOD * TStatus_SaveShift[pt]),
			vals[pt], NULL);
}

