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

#ifndef __KOPERAPP_H__
#define __KOPERAPP_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


// For compilers that support precompilation, includes "wx/wx.h".
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/snglinst.h>
#include <wx/cmdline.h>
#include <wx/image.h>
#else
#include <wx/wxprec.h>
#endif

#include "libpar.h"
#include "szapp.h"

#include "koperframe.h"

class KoperApp : public szApp {
protected:
	bool OnInit();
	int OnExit();
	bool ParseCommandLine();

	wxSingleInstanceChecker* m_app_inst;
	wxLocale m_locale;
	KoperFrame *m_frame;

};
#endif
