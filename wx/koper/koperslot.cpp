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
#include <wx/config.h>
#include <wx/confbase.h>
#include <exception>
#include <math.h>

#ifdef __WXMSW__
#include <GL/glu.h>
#endif

#include "cconv.h"
#include "koperslot.h"
#include "koperframe.h"
#include "kopervfetch.h"


enum { ID_TEXT = wxID_HIGHEST + 1,
	ID_BACKGROUND,
	ID_CONE,
	ID_FONT1,
	ID_FONT2,
	ID_FETCHTIMER, 
	ID_ANIMTIMER,
	ID_EXIT
};

const int KoperSlot::anim_step = 3;

BEGIN_EVENT_TABLE(KoperSlot, wxGLCanvas)
	EVT_RIGHT_DOWN(KoperSlot::OnMouseRightDown)
	EVT_LEFT_DOWN(KoperSlot::OnMouseLeftDown)
	EVT_MIDDLE_DOWN(KoperSlot::OnMouseMiddleDown)
	EVT_PAINT(KoperSlot::OnPaint)
	EVT_ERASE_BACKGROUND(KoperSlot::OnEraseBackground)
	EVT_MOTION(KoperSlot::OnMouseMotion)
	EVT_TIMER(ID_FETCHTIMER, KoperSlot::OnFetchTimer)
	EVT_TIMER(ID_ANIMTIMER, KoperSlot::OnAnimTimer)
	EVT_MENU_RANGE(ID_TEXT, ID_CONE, KoperSlot::OnChangeTexture)
	EVT_MENU_RANGE(ID_FONT1, ID_FONT2, KoperSlot::OnChangeFont)
	EVT_MENU(ID_EXIT, KoperSlot::OnExit)
END_EVENT_TABLE()

static const float black[] = { 0. , 0., 0.,  1. };
static const float white[]  = { 1., 1., 1., 1. };
static const float red[]  = { 1., 0., 0., 1. };


const float KoperSlot::cone_height = 32.f;
const float KoperSlot::cone_width = 16.f;

const int KoperSlot::value_font_size = 28;
const int KoperSlot::value_font_depth = 4;
const int KoperSlot::date_font_size = 8;
const int KoperSlot::date_font_depth = 4;

KoperSlot::KoperSlot(KoperFrame *parent, 
		const char* font_texture, 
		const char* background_texture, 
		const char *cone_texture,
		const char *font_path, 
		const char *prefix,
		KoperValueFetcher *fetcher) :

	wxGLCanvas(parent, (wxGLCanvas*)NULL, -1, wxPoint(0,0), parent->GetSize(), wxFULL_REPAINT_ON_RESIZE, _T("KOPERSLOT")),
	m_ftexdialog(this, _("Path to texture file"), _T("/opt/szarp/resources/wx/koper")),
	m_ffontdialog(this, _("Path to font"), _T("/usr/share/fonts/truetype/msttcorefonts")),
	m_fetchtimer(this, ID_FETCHTIMER),
	m_animationtimer(this, ID_ANIMTIMER),
	m_txt_tex_id(0), m_bg_tex_id(0),
	m_gl_init(false) {

	wxConfigBase *config = wxConfig::Get();

	SetCurrent();

	wxString str;
	m_valfont = NULL;
	if (!config->Read(_T("font_path"), &str) || !LoadFont(SC::S2A(str).c_str(), &m_valfont))
		if (!LoadFont(font_path, &m_valfont)) {
			sz_log(0, "Unable to load default font %s", font_path);
			throw std::exception();
		}



	if (!config->Read(_T("text_texture"), &str) || !LoadTexture(SC::S2A(str).c_str(), &m_txt_tex))
		if (!LoadTexture(font_texture, &m_txt_tex)) {
			sz_log(0, "Unable to load default text texture %s", font_texture);
			throw std::exception();
		}

	if (!config->Read(_T("background_texture"), &str) || !LoadTexture(SC::S2A(str).c_str(), &m_bg_tex))
		if (!LoadTexture(background_texture, &m_bg_tex)) {
			sz_log(0, "Unable to load default background texture %s", background_texture);
			throw std::exception();
		}

	if (!config->Read(_T("cone_texture"), &str) || !LoadTexture(SC::S2A(str).c_str(), &m_cone_tex))
		if (!LoadTexture(cone_texture, &m_cone_tex)) {
			sz_log(0, "Unable to load default cone texture %s", cone_texture);
			throw std::exception();
		}


#if 0
	m_datefont = new FTGLExtrdFont(font_path);
	m_datefont->FaceSize(date_font_size);
	m_datefont->Depth(date_font_depth);
	if (m_datefont->Error())
		assert(false);

	m_datefont->CharMap(ft_encoding_unicode);
#endif


	m_parent = parent;
	m_fetcher = fetcher;

	m_menu = new wxMenu();
	m_menu->Append(ID_TEXT, _("Change text texture"));
	m_menu->Append(ID_BACKGROUND, _("Change background texture"));
	m_menu->Append(ID_CONE, _("Change cone texture"));
	m_menu->Append(ID_FONT1, _("Change font"));
	m_menu->AppendSeparator();
	m_menu->Append(ID_EXIT, _("Close application"));
#if 0
	m_menu->Append(ID_FONT2, _T("Change date font"));
#endif

	m_anim = 0;

	m_angle = 0;
	m_morph_progress = 0;

	m_size = wxSize(180, 70);

	m_te = m_nte = NONE;

	m_v1 = m_v2 = m_nv1 = m_nv2 = SZB_NODATA;
	FetchData();
	m_fetchtimer.Start(60 * 1000);

	m_value_prec = fetcher->GetValuePrec();
	m_popup = new KoperPopup(m_parent, wxString(SC::A2S(prefix)), m_value_prec);
}

void KoperSlot::OnFetchTimer(wxTimerEvent &event) {
	FetchData();
	Refresh();
}

bool KoperSlot::LoadFont(const char* filename, FTFont **font) { 

	FTFont* f;
	f = new FTGLExtrdFont(filename);

	if (f->Error()) {
		delete f;
		return false;
	}

	f->FaceSize(value_font_size, 96);
	f->Depth(value_font_depth);
	f->CharMap(ft_encoding_unicode);

	*font = f;
	return true;

}

bool KoperSlot::LoadTexture(const char* filename, wxImage *img) {
	wxImage _img;
	_img.LoadFile(wxString(SC::A2S(filename)));

	if (!_img.Ok())
		return false;

	*img = _img;
	return true;
}

void KoperSlot::SetTexture(wxImage& img, unsigned& texid) {

	if (texid) 
		glDeleteTextures(1, &texid);

	glGenTextures(1, &texid);
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST | GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST | GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img.GetWidth(), img.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, img.GetData());

}


void KoperSlot::SetTextRotation() {
	if (m_anim >= 180)
		m_angle = 0;
	else if (m_anim > 90)
		m_angle = -180 - m_anim;
	else if (m_anim == 90) {
		m_angle = m_anim;
		m_v1 = m_nv1;
		m_v2 = m_nv2;
	} else 
		m_angle = m_anim;
}

void KoperSlot::SetConeMorphVal() {
	if (m_anim < 90 && m_te == m_nte)
		return;

	if (m_anim == 90) 
		m_te = m_nte;
	else if (m_anim > 90) 
		m_morph_progress = 180 - m_anim;
	else 
		m_morph_progress = m_anim;
}

void KoperSlot::OnAnimTimer(wxTimerEvent& event) {
	
	SetConeMorphVal();
	SetTextRotation();

	if (!(m_anim || m_trigger_anim)) {
		m_animationtimer.Stop();
		return;
	}
	m_trigger_anim = false;

	if (m_anim == 180)
		m_anim = 0;
	else
		m_anim += anim_step;

	Refresh();
}

void KoperSlot::OnMouseMiddleDown(wxMouseEvent &event) {
	PopupMenu(m_menu, event.m_x, event.m_y);
	return;
	if (m_te == NONE) {
		m_te = INCREASING;
		m_nte = DECREASING;
	} else {
		if (m_te == INCREASING) 
			m_nte = DECREASING;
		else
			m_nte = INCREASING;
	}

	m_trigger_anim = true;
	m_animationtimer.Start(50);
}

void KoperSlot::InitGL() {

	if (m_gl_init)
		return;

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	SetTexture(m_txt_tex, m_txt_tex_id);
	SetTexture(m_bg_tex, m_bg_tex_id);
	SetTexture(m_cone_tex, m_cone_tex_id);

	m_gl_init = true;


}

void KoperSlot::FetchData() {
	m_fetcher->AdjustToTime(time(NULL));
	m_nv1 = m_fetcher->Value(0);
	m_nv2 = m_fetcher->Value(1);
	m_t1 = m_fetcher->Time(0);
	m_t2 = m_fetcher->Time(1);


	m_nte = m_fetcher->GetTendency();
	
	bool start_anim = false;

	if (IS_SZB_NODATA(m_v1) || IS_SZB_NODATA(m_nv1)) {
		if (!(IS_SZB_NODATA(m_v1) && IS_SZB_NODATA(m_nv1)))
			start_anim = true;
	} else if (fabs(m_v1 - m_nv1) > 0.01)
		start_anim = true;

	if (IS_SZB_NODATA(m_v2) || IS_SZB_NODATA(m_nv2)) {
		if (!(IS_SZB_NODATA(m_v2) && IS_SZB_NODATA(m_nv2)))
			start_anim = true;
	} else if (fabs(m_v2 - m_nv2) > 0.01)
		start_anim = true;

	if (m_te != m_nte) 
		start_anim = true;

	if (start_anim) {
		m_trigger_anim = true;
		m_animationtimer.Start(50);
	}
}

void KoperSlot::TextExtent(FTFont *font, const char *str, float &w, float &h) {
	float llx, lly, llz, urx, ury, urz;
	glPushMatrix();
		font->BBox(str, llx, lly, llz, urx, ury, urz);
	glPopMatrix();
	w = urx - llx;
	h = ury - lly;
}

void KoperSlot::DrawText(const char* str, float angle) {

	glBindTexture(GL_TEXTURE_2D, m_txt_tex_id);
	glMaterialfv(GL_FRONT, GL_AMBIENT, white);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
	glMaterialfv(GL_FRONT, GL_SPECULAR, white);

	float ve = m_valfont->Advance(str);
	float llx, lly, llz, urx, ury, urz;
	m_valfont->BBox(str, llx, lly, llz, urx, ury, urz);
	glTranslatef(-ve, -lly , 0.f);
	glRotatef(angle, 1., 0., 0.);
	m_valfont->Render(str);

	glBindTexture(GL_TEXTURE_2D, 0);

}

void KoperSlot::OnPaint(wxPaintEvent& WXUNUSED(event)) {

	char str[50];

#define FTOA(val)								\
	{									\
	if (std::isnan(val))							\
		snprintf(str, sizeof(str), "b.d.");				\
	else 									\
		snprintf(str, sizeof(str), "%.*f", m_value_prec, (val));	\
	str[sizeof(str) - 1] = 0;						\
	}


	glMaterialfv(GL_FRONT, GL_EMISSION, black);

	wxPaintDC dc(this);
	StartDrawing();
	glTranslatef(0.f, 0.0f, -10.f);
	SetupLight();
	glTranslatef(0.f, 0.0f, -30.f);


	glPushMatrix();
	{
		float w, h;
		FTOA(m_v1);
		TextExtent(m_valfont, str, w, h);
		glTranslatef(m_size.GetWidth() / 2 - 3 * cone_width, float(m_size.GetHeight()) / 2 - h - 5.f, 0.f);
		DrawText(str, m_angle);
	}
	glPopMatrix();
	glPushMatrix();
	{
		FTOA(m_v2);
		glTranslatef(m_size.GetWidth() / 2 - 3 * cone_width, - float(m_size.GetHeight()) / 2 + 5.f , 0.f);
		DrawText(str, m_angle);
	}
	glPopMatrix();
	DrawCone();

	glTranslatef(0.f, 0.f, -20.f);
	DrawBackground();

	glFlush();
	glFinish();
	SwapBuffers();

}

void KoperSlot::DrawCone() {

	if (m_te == NONE) 
		return;

	glBindTexture(GL_TEXTURE_2D, m_cone_tex_id);


	glPushMatrix();
	{
		glTranslatef(m_size.GetWidth() / 2 - 1.5 * cone_width, - cone_height / 2, 0.f);

		GLUquadric* quadric = gluNewQuadric();
		gluQuadricTexture(quadric, GL_TRUE);

		float current_height = cone_height * (1 - (float)m_morph_progress / 90);

		float top_width;
		float bottom_width;
		
		if (m_te == DECREASING) {
			top_width = 0;
			bottom_width = cone_width * current_height / cone_height;
			glTranslatef(0.f, current_height, 0.f); 
			glRotatef(180, 1.f, 0.f, 0.f);
		} else {
			top_width = 0;
			bottom_width = cone_width * current_height / cone_height;
		}

		glRotatef(-90.f, 1.f, 0.f, 0.f); 
		gluCylinder(quadric, bottom_width, top_width, current_height, int(current_height), int(current_height));
		gluDeleteQuadric(quadric);
	}
	glPopMatrix();

}

void KoperSlot::StartDrawing() {
	SetCurrent();
	InitGL();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	const wxSize& size = m_parent->GetSize();
	glOrtho(-size.GetWidth() / 2, size.GetWidth() / 2, -size.GetHeight() / 2, size.GetHeight() / 2, 1, 100);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void KoperSlot::SetupLight() {

	float light1_position[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float light1_direction[] = { 0.f, 0.f, -1.f};
	glLightfv(GL_LIGHT1, GL_AMBIENT,  black);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  white);
	glLightfv(GL_LIGHT1, GL_SPECULAR, white);
	glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
	glLighti(GL_LIGHT1, GL_SPOT_CUTOFF , abs(m_anim - 90));
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, light1_direction);
	glEnable(GL_LIGHT1);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	glMaterialf(GL_FRONT, GL_SHININESS, 12.0);

}

void KoperSlot::DrawBackground() {

	int tiles = 40;
	int width = m_parent->GetSize().GetWidth();
	int height = m_parent->GetSize().GetHeight();

	glNormal3f(0.f, 0.f, 1.f);

	glMatrixMode(GL_MODELVIEW);
	{
		glBindTexture(GL_TEXTURE_2D, m_bg_tex_id);

		float tsx = float(width) / m_bg_tex.GetWidth();
		float tsy = float(height) / m_bg_tex.GetHeight();

		float px = -width / 2;
		float py = -height / 2;
		float ptx = 0;
		float pty = 0;

		for (int i = 0; i <= tiles; ++i) 
		for (int j = 0; j <= tiles; ++j) 
		{ 
			float x = -width / 2 + width * float(i) / tiles;
			float y = -height / 2 + height * float(j) / tiles;
			float tx = float(i) / tiles * tsx;
			float ty = float(j) / tiles * tsy;

			glBegin(GL_POLYGON);
				glTexCoord2f(ptx, ty); 
				glVertex2f(px, y);
				glTexCoord2f(tx, ty);
				glVertex2f(x, y);
				glTexCoord2f(tx, pty);
				glVertex2f(x, py);
				glTexCoord2f(ptx, pty);
				glVertex2f(px, py);
			glEnd();

			px = x;
			py = y;
			ptx = tx;
			pty = ty;
		}

	}
	glPopMatrix();

}


void KoperSlot::OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {
}

void KoperSlot::OnMouseRightDown(wxMouseEvent& event) {
	m_parent->OnMouseRightDown(event);
}

void KoperSlot::OnChangeFont(wxCommandEvent& event) {
	if (wxID_OK != m_ffontdialog.ShowModal())
		return;

	wxString path = m_ffontdialog.GetPath();
	if (!LoadFont(SC::S2A(path).c_str(), &m_valfont))
		wxMessageBox(_T("Failed to load font!"),_T("ERROR!"));
	else {
		wxConfigBase* config = wxConfigBase::Get();
		config->Write(_T("font_path"), path);
		config->Flush();
		Refresh();
	}


}


void KoperSlot::OnChangeTexture(wxCommandEvent& event) {
	if (wxID_OK != m_ftexdialog.ShowModal())
		return;

	wxString path = m_ftexdialog.GetPath();

	bool ok;
	unsigned* id;
	wxImage* img;
	wxString config_item;
	switch (event.GetId()) {
		case ID_BACKGROUND:
			config_item = _T("background_texture");
			id = &m_bg_tex_id;
			img = &m_bg_tex;
			break;
		case ID_TEXT:
			config_item = _T("text_texture");
			id = &m_txt_tex_id;
			img = &m_txt_tex;
			break;
		case ID_CONE:
			config_item = _T("cone_texture");
			id = &m_cone_tex_id;
			img = &m_cone_tex;
			break;
		default:
			assert(false);
	}

	ok = LoadTexture((SC::S2A(path).c_str()), img);

	if (!ok)
		wxMessageBox(_("File not loaded!"), _("ERROR!"));
	else {
		wxConfigBase *config = wxConfigBase::Get();
		config->Write(config_item, path);
		config->Flush();
		SetTexture(*img, *id);
		Refresh();
	}

}

void KoperSlot::OnMouseLeftDown(wxMouseEvent &event) {
	m_popup->SetValues(m_nv1, m_t1, m_nv2, m_t2);
	m_popup->Fit();
	m_popup->Raise();
	m_popup->SetFocus();
	m_popup->Show();
}


void KoperSlot::OnMouseMotion(wxMouseEvent& event) {
	if (event.RightIsDown() == FALSE)
		return;

	m_parent->OnMouseMotion(event);
}

void KoperSlot::OnExit(wxCommandEvent& WXUNUSED(event)) {
	int ret;

	ret = wxMessageBox(_("Do you want to terminate application?"), _T("Termination."), wxYES_NO | wxICON_QUESTION, m_parent);

	if (ret == wxYES)
		m_parent->Close(true);

}
	

