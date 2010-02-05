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
/* $Id: viszioTransparent.h 1 2009-11-17 21:44:30Z asmyk $
 *
 * viszio program
 * SZARP
 *
 * asmyk@praterm.com.pl
 */
 
#ifndef __configuration__
#define __configuration__
#ifdef WX_GCH
#include <wx_pch.h>
#else
#include <wx/wx.h>
#endif

#include <wx/menu.h>

#include "fetchparams.h"
#include "serverdlg.h"
#include "viszioParamadd.h"
#include "viszioTransparentFrame.h"
#include "parlist.h"
#include "cconv.h"
#include "authdiag.h"

class TransparentFrame;

class Configuration 
{
public:
	Configuration(wxString configurationName);
	~Configuration();
	static const int m_max_number_of_frames = 100;
    int m_current_amount_of_frames;
    TransparentFrame **m_all_frames;
    wxSize m_defaultSizeWithFrame;
    wxSize m_defaultSizeWithOutFrame;
    szParamFetcher *m_pfetcher;
    szProbeList m_probes;
    TSzarpConfig *m_ipk;
    wxString m_configuration_name;    
    wxConfig *wxconfig;
    long m_fontThreshold;
};


#endif // __configuration__
