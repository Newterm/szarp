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
 
#ifndef _KONTRADD_H
#define _KONTRADD_H

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
//#include "parcalc.h"

//#include "kontroler.h"


/** Kontroler: add par */
class szViszioAddParam : public wxDialog {
DECLARE_CLASS(szViszioAddParam)
DECLARE_EVENT_TABLE()
  /** szarp config */
  TSzarpConfig *ipk;
  /** */
  szParSelect *ps;
//  szParCalc *pc;
  /** */
  TParam *m_param;
  /** */
//  wxString m_value_min;
//  wxString m_value_max;
public:
  /** returned data */
  struct {
    szProbe m_probe;
  }g_data;
  /**
   * @param _ipk szarp config
   */
  szViszioAddParam(TSzarpConfig *_ipk,
      wxWindow *parent, wxWindowID id, const wxString &title);
  virtual ~szViszioAddParam();
protected:
  /** */
  void OnSelectParam(wxCommandEvent &ev);
  /** */
  virtual bool TransferDataToWindow();
  /** */
  virtual bool TransferDataFromWindow();
  virtual TParam* GetParam() { return m_param; }
};

/*
 * IDs
 */
enum {
  ID_TC_PARNAME = wxID_HIGHEST + 1,
  ID_B_PARAM,
  ID_B_FORMULA,
  ID_CB_VALID,
  ID_TC_VALMIN,
  ID_TC_VALMAX,
  ID_SC_PREC,
  ID_SC_GROUP,
  ID_CH_DTYPE,
  ID_CH_ALARM
};

#endif //_KONTRADD_H
