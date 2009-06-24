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
 * draw3 
 * SZARP
 
 *
 * $Id$
 */

#ifndef __PROGFRM_H__
#define __PROGFRM_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/**dialog displaying progress of some operation*/
class ProgressFrame : public wxDialog {
	/**widgt showing progress bar*/
	wxGauge *m_gauge;
	public:
	ProgressFrame(wxWindow* parent);
	/**sets value of progress, value shall be withing range (1;100)*/
	void SetProgress(int progress);
	virtual ~ProgressFrame();
};

#endif
