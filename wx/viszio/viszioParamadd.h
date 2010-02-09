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
/* $Id: viszioParamadd.h 1 2009-11-17 21:44:30Z asmyk $
 *
 * viszio program
 * SZARP
 *
 * asmyk@praterm.com.pl
 */
 
#ifndef __VISZIOPARAMADD_H__
#define __VISZIOPARAMADD_H__

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/spinctrl.h>
#include <szarp_config.h>
#include "parselect.h"
#include "parlist.h"



/**
 * Viszio add parameter dialog window 
 */
class szViszioAddParam : public wxDialog {
	DECLARE_CLASS(szViszioAddParam)
	DECLARE_EVENT_TABLE()
	TSzarpConfig *ipk;		/**< szarp config */
	szParSelect *ps;		/**< parameter select dialog */
	TParam *m_param;		/**< pointer to parameter */
public:
  struct {
    szProbe m_probe;		/**< returned data */
  }g_data;  
  /**
   * Add parameter dialog constructor
   * @param _ipk szarp config
   * @param parent parent window
   * @param id window id
   * @param title window title
   */
  szViszioAddParam(TSzarpConfig *_ipk,
      wxWindow *parent, wxWindowID id, const wxString &title);
  virtual ~szViszioAddParam();
protected:
  /** Invoked when button 'select param' has beend pressed  */
  void OnSelectParam(wxCommandEvent &ev);
  /** Transfers data to window */
  virtual bool TransferDataToWindow();
  /** Transfers data from window */
  virtual bool TransferDataFromWindow();
  /** 
  * Returns pointer to chosen parameter 
  * @return pointer to chosen parameter 
  */
  virtual TParam* GetParam() { return m_param; }
};


enum {
  ID_TC_PARNAME = wxID_HIGHEST + 1,
  ID_B_PARAM,
};

#endif //__VISZIOPARAMADD_H__
