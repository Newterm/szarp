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

#include <wx/wx.h>
#include "ids.h"
#include "EkstraktorWidget.h"
#include "EkstraktorMainWindow.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "ekstraktor3.h"
#include "szmutex.h"

#include "geometry.h"
#include "cconv.h"

#include "../../resources/wx/icons/extr64.xpm"

bool is_correct_time(time_t time);

EkstraktorWidget::EkstraktorWidget(std::wstring ipk_prefix, wxString * geometry, std::pair<wxString, wxString> prober_address)
{
	int x, y, width, height;

	IPKContainer::Init(wxGetApp().GetSzarpDataDir().c_str(),
			   wxGetApp().GetSzarpDir().c_str(),
			   wxConfig::Get()->Read(_T("LANGUAGE"),
						 _T("pl")).c_str());

	Szbase::Init(wxGetApp().GetSzarpDataDir().c_str(), NULL);

	wxProgressDialog *prog =
	    new wxProgressDialog(_("SZARP Extractor v. 3.0."),
				 wxString(_("Starting program")) +
				 _T("        "),
				 101, NULL);
	prog->SetIcon(wxICON(extr64));

	is_ok = 0;

	if (geometry) {
		get_geometry(*geometry, &x, &y, &width, &height);
		delete geometry;
		geometry = NULL;
	} else {
		x = DEFAULT_POSITION_X;
		y = DEFAULT_POSITION_Y;
		width = DEFAULT_WIDTH;
		height = DEFAULT_HEIGHT;
	}

	prog->Update(0, _("Loading configuration"));
	prog->Fit();

	// don't load activity params
	IPKContainer::GetObject()->LoadConfig(ipk_prefix,std::wstring(),false);
	ipk = IPKContainer::GetObject()->GetConfig(ipk_prefix);

	xml_loaded = ipk != NULL;
	if (!xml_loaded) {
		delete prog;
		wxMessageBox(wxString::Format(_T("%s %s\n%s"),
					      _
					      ("Cannot load configuration for prefix: "),
					      ipk_prefix.c_str(),
					      _("It can be a prefix problem")),
			     wxString(_
				      ("Extractor: Cannot access configuration")),
			     wxOK | wxICON_ERROR);
		return;
	}

	prog->Update(0, _("Checking parameters in base"));
	prog->Fit();

	number_of_params = 0;
	int i = 0;
	int max =
	    ipk->GetParamsCount() + ipk->GetDefinedCount() +
	    ipk->GetDrawDefinableCount();

	TParam *prm = ipk->GetFirstParam();
	if (NULL == prm) {
		delete prog;
		wxMessageBox(wxString::
			     Format(_("Corrupted config %s\nNo params found"),
				    ipk_prefix.c_str()),
			     wxString(_("Extractor: Cannot load parameters")),
			     wxOK | wxICON_ERROR);
		return;
	}

	Szbase* szbase = Szbase::GetObject();
	szbase_buffer = szbase->GetBuffer(ipk_prefix);
	assert(szbase_buffer != NULL);
	if (prober_address != std::pair<wxString, wxString>()) {
		szbase->SetProberAddress(
			ipk_prefix.c_str(),
			prober_address.first.c_str(),
			prober_address.second.c_str());
		prober_configured = true;	
	} else {
		prober_configured = false;	
	}

	time_t tmp_date;
	first_date = -1;
	last_date = -1;

	for (; prm != NULL; prm = ipk->GetNextParam(prm)) {
		prog->Update(i++ * 100 / max);
		if (!prm->IsReadable())
			continue;
		tmp_date = szb_search_first(szbase_buffer, prm);
		if (!is_correct_time(tmp_date)) {
			continue;
		}
		if (first_date < 0) {
			first_date = szb_search(szbase_buffer, prm, tmp_date,	/* from */
						     -1,	/* to - all data */
						     1	/* search right */
			    );
		} else {
			if (tmp_date < first_date) {
				tmp_date =
				    szb_search(szbase_buffer, prm,
						    tmp_date, first_date, 1);
				if (tmp_date > 0 && tmp_date < first_date) {
					first_date = tmp_date;
				}
			}
		}
		tmp_date = szb_search_last(szbase_buffer, prm);
		assert(is_correct_time(tmp_date));
		if (last_date < 0) {
			last_date =
			    szb_search(szbase_buffer, prm, tmp_date,
					    first_date, -1);
		} else {
			if (tmp_date > last_date) {
				tmp_date =
				    szb_search(szbase_buffer, prm,
						    tmp_date, last_date, -1);
				if (tmp_date > 0 && tmp_date > last_date) {
					last_date = tmp_date;
				}
			}
		}
		number_of_params++;
	}			/* for each param */

	if (0 == number_of_params) {
		delete prog;
		wxMessageBox(wxString::
			     Format(_
				    ("Database empty or missing.\nNo params found."),
				    ipk_prefix.c_str()),
			     wxString(_("Extractor: Cannot load parameters")),
			     wxOK | wxICON_ERROR);
		return;
	}

	SetStartDate(first_date);
	SetStopDate(last_date);

	EkstraktorMainWindow *mainWindow =
	    new EkstraktorMainWindow(this, wxPoint(x, y),
				     wxSize(width, height));

	prog->Update(101);
	delete prog;

	mainWindow->SetTitle(wxString(ipk->GetTitle()));

	mainWindow->SetStartDate(first_date);
	mainWindow->SetFirstDatabaseDate(first_date);

	mainWindow->SetStopDate(last_date);
	mainWindow->SetLastDatabaseDate(last_date);

	wxString paramsnum = wxString::Format(_T("%d"), GetNumberOfParams());

	mainWindow->SetStatusText(paramsnum +
				  wxString(_(" parameters loaded")));
	is_ok = 1;
}

bool is_correct_time(time_t time)
{
	return (time <= (time_t) - 1 ? false : true);
}
