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
#include "../common/parlist.cpp"

void* progress_update(int progress, void* prog) {
	((wxProgressDialog*) prog)->Update(progress);
	return NULL;
}
 
using namespace SC;
/*  
{
	std::wstring W2S(const wxString& c) {
		return (std::wstring(c.ToStdWstring()));
	}
}
*/
EkstraktorWidget::EkstraktorWidget(std::wstring ipk_prefix, wxString * geometry, std::pair<wxString, wxString> prober_address, bool sz4) : sz4(sz4)
{
	int x, y, width, height;

	IPKContainer::Init(SC::W2S(wxString::FromUTF8(wxGetApp().GetSzarpDataDir().mb_str())),
				SC::W2S(wxString::FromUTF8(wxGetApp().GetSzarpDir().mb_str())),
			   SC::W2S(wxString::FromUTF8(wxConfig::Get()->Read(_T("LANGUAGE"),
						 _T("pl")).mb_str())));

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

	IPKContainer::GetObject()->LoadConfig(ipk_prefix,std::wstring());
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

	IPKContainer* ipkc = IPKContainer::GetObject();
	if (!sz4) {
		Szbase::Init(SC::W2S(wxString::FromUTF8(wxGetApp().GetSzarpDataDir().mb_str())), NULL);

		Szbase* szbase = Szbase::GetObject();
		extr = new SzbExtractor(ipkc, szbase);

		szbase_buffer = szbase->GetBuffer(ipk_prefix);
		assert(szbase_buffer != NULL);

		if (prober_address != std::pair<wxString, wxString>()) {
			szbase->SetProberAddress(
				ipk_prefix.c_str(),
				SC::W2S(wxString::FromUTF8(prober_address.first.mb_str())),
				SC::W2S(wxString::FromUTF8(prober_address.second.mb_str())));
			prober_configured = true;	
		} else {
			prober_configured = false;	
		}
	} else {
		Szbase::Init(SC::W2S(wxString::FromUTF8(wxGetApp().GetSzarpDataDir().mb_str())), NULL);
		extr = new SzbExtractor(ipkc, new sz4::base(SC::W2S(wxString::FromUTF8(wxGetApp().GetSzarpDataDir().mb_str())),
							ipkc));
		prober_configured = false;
	}

	first_date = -1;
	last_date = -1;

	extr->SetProgressWatcher(progress_update, (void*)prog);

	extr->ExtractStartEndTime(ipk, first_date, last_date, number_of_params);
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

	sz4 = false;
}

EkstraktorWidget::~EkstraktorWidget()
{	
	/* 
	 * hary: 29.07.2015: Free memory at least for SzbExtractor 
	 * @TODO: Fix more memory leaks in this class...
	 */
	delete extr;
}
