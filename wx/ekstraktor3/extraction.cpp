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

#include <time.h>
#include <errno.h>
#include <iostream>

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
 
#include "extraction.h"
#include "cconv.h"

struct extr_arguments getExtrDefaultArguments()
{
struct extr_arguments arguments;

        arguments.debug_level = 2;
        arguments.date_format = L"%Y-%m-%d %H:%M";
        arguments.start_time = -1;
        arguments.end_time = -1;
        arguments.year = -1;
        arguments.month = -1;
        arguments.probe = PT_MIN10;
        arguments.probe_length = 0;
        arguments.raw = 0;
        arguments.openoffice = 0;
        arguments.csv= 0;
        arguments.xml = 0;
        arguments.delimiter = L";";
        arguments.progress = 1;
        arguments.empty = 0;
	arguments.dec_delim = 0;

	return arguments;

}

int extract(struct extr_arguments arguments, 
		void* (*progress_printer)(int, void*), 
		void* progress_data,
		int* cancel_lval,
		TSzarpConfig *ipk, szb_buffer_t *szb)
{
	FILE *output = NULL;
	SzbExtractor* extr = new SzbExtractor(Szbase::GetObject());
	assert (extr != NULL);

        for (unsigned int i = 0; i < arguments.params.size(); i++) {
		arguments.params[i].szb = szb;
		arguments.params[i].prefix = ipk->GetPrefix();
	}

        if (arguments.year < 0)
                extr->SetPeriod(arguments.probe, arguments.start_time,
                                arguments.end_time, arguments.probe_length);
        else
                extr->SetMonth(arguments.probe, arguments.year,
                                arguments.month, arguments.probe_length);
        if (extr->SetParams(arguments.params) 
			> 0) {
                return -1;
        }

        extr->SetNoDataString(arguments.no_data);
        extr->SetEmpty(arguments.empty);
	extr->SetDecDelim(arguments.dec_delim);

	assert (!arguments.output.empty());
        if (!arguments.openoffice) {
		output = fopen(SC::S2A(arguments.output).c_str(), "w");
		if (output == NULL) {
			wxMessageBox(
			_("Couldn't open file ") +  wxString(arguments.output) + _("\n") +
			wxString::Format(_("Error %d (%s)"), errno, strerror(errno)), 
					_("Error"), wxOK);
			return -1;
		}
        }
	
	if (arguments.progress && progress_printer) {
                extr->SetProgressWatcher(progress_printer, progress_data);
        }
	extr->SetCancelValue(cancel_lval);
        SzbExtractor::ErrorCode ret;

        if (arguments.openoffice)
                ret = extr->ExtractToOpenOffice(arguments.output);
        else if (arguments.xml)
                ret = extr->ExtractToXML(output);
        else
                ret = extr->ExtractToCSV(output, arguments.delimiter);
	if (output != NULL)
		fclose(output);
	delete extr;

	return (int) ret;
}

