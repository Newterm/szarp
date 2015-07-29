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

#ifndef __EKSTRAKTOR_WIDGET_H__
#define __EKSTRAKTOR_WIDGET_H__

#include <wx/wx.h>
#include <szarp_config.h>
#include "szbase/szbbase.h"
#include <time.h>


class EkstraktorMainWindow;
class SzbExtractor;

/* The main application widget */

class EkstraktorWidget 
{
	time_t first_date; // first date in database
	time_t last_date; // last date in database

	time_t start_date; // start date
	time_t stop_date; // stop date

	int number_of_params; // number of all parameters for configuration

	SZARP_PROBE_TYPE probe_type; // sample frequency

	EkstraktorMainWindow * mainWindow; // main window

	bool minimized; // if we minimize the output file length

	bool xml_loaded; // true if configuration has been loaded

	std::wstring szbase_datadir; // data directory

	szb_buffer_t *szbase_buffer;

	TSzarpConfig *ipk;
	int is_ok;	/**< 1 when object is properly initialized, 
			  0 otherwise */
	bool prober_configured;
	bool sz4;
	SzbExtractor* extr;
	public:
		EkstraktorWidget(std::wstring ipk_prefix, wxString *geometry, std::pair<wxString, wxString> prober_address, bool sz4);
		~EkstraktorWidget();
		
		int GetNumberOfParams() { return number_of_params; }
		void SetNumberOfParams(int params) { number_of_params = params; }

		time_t GetFirstDate() { return first_date; }
		
		time_t GetLastDate() { return last_date; }

		time_t GetStartDate() { return start_date; }
		void SetStartDate(time_t sd) { start_date = sd; }

		time_t GetStopDate() { return stop_date; }
		void SetStopDate(time_t sd) { stop_date = sd; }

		SZARP_PROBE_TYPE GetProbeType() { return probe_type; }
		void SetProbeType(SZARP_PROBE_TYPE probe) { probe_type = probe; }

		void SetMinimized(bool min) { minimized = min; }
		bool GetMinimized() { return minimized; }

		szb_buffer_t* GetSzbaseBuffer() { return szbase_buffer; }

		bool IsConfigLoaded() { return xml_loaded && is_ok; } 

		std::wstring GetSzbaseDataDir() { return szbase_datadir; }

		TSzarpConfig *GetIpk() { return ipk; }
		
		bool IsProberConfigured() { return prober_configured; }
		
		bool IsSz4() { return sz4; }

		void SetSz4(bool sz4) { this->sz4 = sz4; }

		SzbExtractor* GetExtractor() { return this->extr; }
};

#endif // __EKSTRAKTOR_WIDGET_H__
