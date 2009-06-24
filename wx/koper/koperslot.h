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
 
 * Darek Marcinkiewicz
 *
 * $Id$
 */

#ifndef __KOPERSLOT_H__
#define __KOPERSLOT_H__

// For compilers that support precompilation, includes "wx/wx.h".
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/timer.h>
#include <wx/sizer.h>
#include <wx/image.h>
#include <wx/filedlg.h>
#include <wx/glcanvas.h>
#include <wx/menu.h>
#else
#include <wx/wxprec.h>
#endif

#include <FTGL/ftgl.h>

#include "kopervfetch.h"
#include "koperpopup.h"

class KoperFrame;

class KoperSlot : public wxGLCanvas {
protected:
	void OnPaint(wxPaintEvent& paint);
	void InitGL();
	void OnEraseBackground(wxEraseEvent& erase);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseMiddleDown(wxMouseEvent& event);
	void OnMouseMotion(wxMouseEvent& event);
	void OnChangeTexture(wxCommandEvent &event);
	void OnChangeFont(wxCommandEvent &event);
	void OnFetchTimer(wxTimerEvent& event);
	void OnAnimTimer(wxTimerEvent& event);
	void OnExit(wxCommandEvent& event);
	void DrawCone();
	void SetConeMorphVal();
	void SetTextRotation();
	void DrawBackground();
	void SetupLight();
	void StartDrawing();
	void FetchData();
	void TextExtent(FTFont *font, const char* str, float &w, float &h);
	void DrawText(const char* str, float angle);
	bool LoadTexture(const char* filename, wxImage *img);
	bool LoadFont(const char* filename, FTFont **font);
	void SetTexture(wxImage& img, unsigned& texid);

	wxFileDialog m_ftexdialog;
	wxFileDialog m_ffontdialog;
	wxTimer m_fetchtimer,m_animationtimer;
	unsigned m_txt_tex_id, m_bg_tex_id, m_cone_tex_id;

	bool m_gl_init;
	wxImage m_txt_tex;
	wxImage m_bg_tex;
	wxImage m_cone_tex;
	KoperPopup *m_popup;
	FTFont *m_valfont;
#if 0
	FTFont *m_datefont;
#endif
	wxMenu *m_menu;

	KoperFrame *m_parent;
	KoperValueFetcher *m_fetcher;
	wxSize m_size;

	struct tm m_t1, m_t2;

	Tendency m_te, m_nte;

	double m_v1,m_v2;
	double m_nv1,m_nv2;

	bool m_trigger_anim;
	int m_anim;

	int m_angle;
	int m_morph_progress;
	int m_value_prec;

	static const int anim_step;

	static const float cone_width;
	static const float cone_height;

	static const int value_font_size;
	static const int value_font_depth;
	static const int date_font_size;
	static const int date_font_depth;
public:
	KoperSlot(KoperFrame* parent, 
			const char *text_texture, 
			const char *background_texture, 
			const char *cone_texture,
			const char *font_path, 
			const char *prefix,
			KoperValueFetcher* fetcher);
	DECLARE_EVENT_TABLE()
};



#endif
