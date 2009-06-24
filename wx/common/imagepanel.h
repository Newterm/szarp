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
 * SZARP

 *
 * $Id$
 */
#ifndef __IMAGEPANEL_H__
#define __IMAGEPANEL_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class ImagePanel: public wxPanel
{
public:
	ImagePanel(wxWindow *pParent, const wxBitmap& bitmap)
		:wxPanel(pParent, -1), m_Bitmap(bitmap){
		SetClientSize(m_Bitmap.GetWidth(), m_Bitmap.GetHeight());
	}

	~ImagePanel(){};

private:
	/**Bitmap that is drawn on the panel*/
	wxBitmap m_Bitmap;

	DECLARE_EVENT_TABLE()

	/**Draws bitmap*/
	void OnPaint(wxPaintEvent& event){
		wxPaintDC PaintDc(this);
		PaintDc.DrawBitmap(m_Bitmap, 0, 0, TRUE);
	}
};


#endif //__IMAGEPANEL_H__
