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

#include "ids.h"
#include "classes.h"
#include "cconv.h"
#include "cfgmgr.h"
#include "drawtime.h"
#include "drawdnd.h"
#include "dbinquirer.h"
#include "database.h"
#include "drawobs.h"
#include "draw.h"
#include "coobs.h"
#include "drawsctrl.h"
#include "drawswdg.h"
#include "glgraphs.h"
#include "conversion.h"
#include "drawapp.h"
#include "graphsutils.h"

#ifdef HAVE_GLCANVAS
#ifdef HAVE_FTGL

BEGIN_EVENT_TABLE(GLGraphs, wxWindow)
    EVT_PAINT(GLGraphs::OnPaint)
    EVT_LEFT_DOWN(GLGraphs::OnMouseLeftDown)
    EVT_LEFT_DCLICK(GLGraphs::OnMouseLeftDClick)
    EVT_LEAVE_WINDOW(GLGraphs::OnMouseLeftUp)
    EVT_LEFT_UP(GLGraphs::OnMouseLeftUp)
    EVT_IDLE(GLGraphs::OnIdle)
    EVT_SIZE(GLGraphs::OnSize)
    EVT_SET_FOCUS(GLGraphs::OnSetFocus)
    EVT_ERASE_BACKGROUND(GLGraphs::OnEraseBackground)
    EVT_TIMER(wxID_ANY, GLGraphs::OnTimer)
END_EVENT_TABLE()


bool GLGraphs::_gl_ready = false;

GLuint GLGraphs::_circle_list = 0;

GLuint GLGraphs::_line_list = 0;

GLuint GLGraphs::_back1_tex = 0;

GLuint GLGraphs::_line1_tex = 0;

FTFont* GLGraphs::_font = NULL;

namespace {

	float cube_vertices[] = {
		//FRONT
		-6, -6, 0,
		-6, 6, 0,
		6, 6, 0,
		6, -6, 0,
		//LEFT
		6, 6, 0,
		6, 6, -1,
		6, -6, -1,
		6, -6, 0,
		//RIGHT
		-6, 6, 0,
		-6, 6, -1,
		-6, -6, -1,
		-6, -6, 0,
		//TOP
		-6, 6, 0,
		-6, 6, -1,
		6, 6, -1,
		6, 6, 0,
		//BOTTOM
		-6, -6, 0,
		-6, -6, -1,
		6, -6, -1,
		6, -6, 0

	};

	float cube_normals[] = {
		0, 0, 1,
		0, 0, 1,
		0, 0, 1,
		0, 0, 1,
		1, 0, 0,
		1, 0, 0,
		1, 0, 0,
		1, 0, 0,
		-1, 0, 0,
		-1, 0, 0,
		-1, 0, 0,
		-1, 0, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, -1, 0,
		0, -1, 0,
		0, -1, 0,
		0, -1, 0
	};


}


GLGraphs::GLGraphs(wxWindow *parent, ConfigManager *cfg) : wxGLCanvas(parent, wxID_ANY,
#ifdef __WXGTK__
		wxGetApp().GLContextAttribs(),
#else
		NULL,
#endif
		wxDefaultPosition, wxSize(300, 200), 0, _T("GLCanvas")) {

	m_draws_wdg = NULL;
	m_lists_compiled = false; 
	m_cfg_mgr = cfg;
	m_recalulate_margins = false;
	m_draw_current_draw_name = false;
	m_refresh = false;
	m_widget_initialized = false;

	SetInfoDropTarget* dt = new SetInfoDropTarget(this);
	SetDropTarget(dt);

	/* Set minimal size of widget. */
	SetSizeHints(300, 200);

	m_timer = new wxTimer(this, wxID_ANY);

	m_gl_context = NULL;

}

void GLGraphs::Init() {
	if (m_widget_initialized == false) {
		m_screen_margins.leftmargin = 36;
		m_screen_margins.rightmargin = 10;
		m_screen_margins.infotopmargin = 7;

		m_screen_margins.bottommargin = GetFont().GetPointSize() + 4 + 4;
		m_screen_margins.topmargin = GetFont().GetPointSize() + 4 + 3 + 6;

		m_widget_initialized = true;
	}
}

void GLGraphs::OnIdle(wxIdleEvent & event)
{
	if (m_refresh) {
		m_refresh = false;
		wxGLCanvas::Refresh();
	}
}

void GLGraphs::OnEraseBackground(wxEraseEvent & WXUNUSED(event))
{ 
}

void GLGraphs::SetUpScene() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, m_size.GetWidth(), m_size.GetHeight());

	float _near = 80;
	float _far = 100.0;
	float near_width = _near / _far * m_size.GetWidth();
	float near_height = _near / _far * m_size.GetHeight();

	glFrustum(-near_width / 2, near_width / 2, -near_height / 2, near_height / 2, _near, _far);
	//glFrustum(-near_width / 2, near_width / 2, near_height / 2, -near_height / 2, near, far);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDepthFunc(GL_LEQUAL);
	glClearColor(0, 0, 0, 0);
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLGraphs::DrawYAxis() {
	float white[] = { 1, 1, 1, 1};
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white); glMaterialfv(GL_FRONT, GL_SPECULAR, white);

	glBegin(GL_QUADS);
		glVertex3f(m_screen_margins.leftmargin - 1, m_screen_margins.bottommargin, 0);
		glVertex3f(m_screen_margins.leftmargin - 1, m_size.GetHeight(), 0);
		glVertex3f(m_screen_margins.leftmargin + 1, m_size.GetHeight(), 0);
		glVertex3f(m_screen_margins.leftmargin + 1, m_screen_margins.bottommargin, 0);

		glVertex3f(m_screen_margins.leftmargin - 8, m_size.GetHeight() - 10, 0);
		glVertex3f(m_screen_margins.leftmargin - 8, m_size.GetHeight() - 8, 0);
		glVertex3f(m_screen_margins.leftmargin - 1, m_size.GetHeight(), 0);
		glVertex3f(m_screen_margins.leftmargin - 1, m_size.GetHeight() - 2, 0);

		glVertex3f(m_screen_margins.leftmargin + 1, m_size.GetHeight() - 2, 0);
		glVertex3f(m_screen_margins.leftmargin + 1, m_size.GetHeight(), 0);
		glVertex3f(m_screen_margins.leftmargin + 8, m_size.GetHeight() - 8, 0);
		glVertex3f(m_screen_margins.leftmargin + 8, m_size.GetHeight() - 10, 0);
	glEnd();

}

void GLGraphs::DestroyPaneDisplayList() {
	if (m_lists_compiled) {
		glDeleteLists(m_back_pane_list, 1);
		m_lists_compiled = false;
	}
}

void GLGraphs::SetLight() {

	GLfloat specular_vals[] = { .4, .4, .4, 1};
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular_vals);

	//GLfloat max_light[] = { .8, .8, .8, 1.0};
	GLfloat max_light[] = { 1, 1, 1, 1.0};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, max_light);

	int sel = m_draws_wdg->GetSelectedDrawIndex();

	float light_position[] = { 0, 0, 400, 1 };

	if (sel >= 0) {
		Draw *d = m_draws[sel];
		int cidx = d->GetCurrentIndex();
		if (cidx >= 0) {
			const ValuesTable& vt = d->GetValuesTable();
			double x = GetX(cidx, d->GetValuesTable().size());
			double y = GetY(vt[cidx].val, d->GetDrawInfo());
			light_position[0] = x;
			light_position[1] = y;
		}
	}

	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

}

void GLGraphs::GeneratePaneDisplayList() {

	if (m_size.IsFullySpecified() == false)
		return;

	if (m_lists_compiled == true)
		return;

	int sel = m_draws_wdg->GetSelectedDrawIndex();

	if (sel < 0)
		return;

	m_back_pane_list = glGenLists(1);	
	glNewList(m_back_pane_list, GL_COMPILE);

	int width = m_size.GetWidth() - m_screen_margins.leftmargin - m_screen_margins.rightmargin;
	int height = m_size.GetHeight() - m_screen_margins.topmargin - m_screen_margins.bottommargin;
	const int tile_size = 8;
	PeriodType pt = m_draws[sel]->GetPeriod();
	int pane_width = width * TimeIndex::PeriodMult[pt] / m_draws_wdg->GetNumberOfValues();
	int cx = pane_width / tile_size;
	int cy = height / tile_size;

	float py = 0;
	float x, y;
#if 0
	const float texture_width = 128, texture_height = 128;
	glBindTexture(GL_TEXTURE_2D, _back1_tex);
#endif
	for (int j = 0; j <= cy; j++) {
		float px = 0;
		y = float(j) * height / cy ;
#if 0
		glNormal3f(0, 0, 1);
#endif
		glBegin(GL_QUAD_STRIP);
#if 0
			glTexCoord2f(px / texture_width, (height - y) / texture_height); 
#endif
			glVertex2f(px, y);
#if 0
			glTexCoord2f(px / texture_width, (height - py) / texture_height); 
#endif
			glVertex2f(px, py);
			for (int i = 0; i < cx; i++)  {
				x = float(i) * pane_width / cx;
#if 0
				glTexCoord2f(x / texture_width, (height - y) / texture_height); 
#endif
				glVertex2f(x, y);
#if 0
				glTexCoord2f(x / texture_width, (height - py) / texture_height);
#endif
				glVertex2f(x, py);
			}
#if 0
			glTexCoord2f(pane_width / texture_width, (height - y) / texture_height); 
#endif
			glVertex2f(pane_width, y);
#if 0
			glTexCoord2f(pane_width / texture_width, (height - py) / texture_height); 
#endif
			glVertex2f(pane_width, py);
		glEnd();
		py = y;
	}
#if 0
	glBindTexture(GL_TEXTURE_2D, 0);
#endif

	glEndList();

	m_lists_compiled = true;

}

void GLGraphs::DrawBackground() {
	int sd = m_draws_wdg->GetSelectedDrawIndex();
	if (sd < 0)
		return;

	Draw *draw = m_draws[sd];
	GLfloat almost_dark[] = { 0.5, 0.5, 0.5, 0.5 };
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, almost_dark); glMaterialfv(GL_FRONT, GL_SPECULAR, almost_dark);

	size_t pc = draw->GetValuesTable().size();
	size_t i = 0;
	int sign = 1;
	while (i < pc) {
		if (sign > 0) {
			double x = float(i) * (m_size.GetWidth() - m_screen_margins.leftmargin - m_screen_margins.rightmargin) / pc + m_screen_margins.leftmargin;
			glPushMatrix();
				glTranslatef(x, m_screen_margins.bottommargin, 0);
				glCallList(m_back_pane_list);
			glPopMatrix();
		}
		sign *= -1;
		i += TimeIndex::PeriodMult[draw->GetPeriod()];
	}
		
}

float GLGraphs::GetX(int i, const size_t& pc) {
	float res = float(m_size.GetWidth() - m_screen_margins.leftmargin - m_screen_margins.rightmargin)  / pc;
	return i * res + res / 2 + m_screen_margins.leftmargin;
}

float GLGraphs::GetY(double value, DrawInfo *di) {
	double max = di->GetMax();
	double min = di->GetMin();

	double dmax = max - min;
	double dmin = 0;

	double smin = 0;
	double smax = 0;
	double k = 0;

	int sc = di->GetScale();
	if (sc) {
		smin = di->GetScaleMin() - min;
		smax = di->GetScaleMax() - min;
		assert(smax > smin);
		k = sc / 100. * (dmax - dmin) / (smax - smin);
		dmax += (k - 1) * (smax - smin);
	}

	double dif = dmax - dmin;
	if (dif <= 0) {
		wxLogError(_T("%s %f %f"), di->GetName().c_str(), min, max);
		assert(false);
	}

	double dvalue = value - min;

	double scaled = 
		wxMax(dvalue - smax, 0) +
		wxMax(wxMin(dvalue - smin, smax - smin), 0) * k +
		wxMax(wxMin(dvalue - dmin, smin), 0);

	int height = m_size.GetHeight() - m_screen_margins.topmargin - m_screen_margins.bottommargin;

	float ret = height * scaled / dif + m_screen_margins.bottommargin;

	ret = wxMin(ret, m_size.GetHeight() - m_screen_margins.topmargin);

	ret = wxMax(ret, m_screen_margins.bottommargin);
		
	return ret;
}

bool GLGraphs::AlternateColor(Draw *d, int idx) {
	if (d->GetSelected() == false)
		return false;

	if (d->GetDoubleCursor() == false)
		return false;

	const Draw::VT& vt = d->GetValuesTable();

	int ss = idx + vt.m_view.Start();

	bool result = (ss >= vt.m_stats.Start()) && (ss <= vt.m_stats.End());

	return result;

}

void GLGraphs::DrawGraph(Draw *d) {
	if (d->GetEnable() == false
			&& m_graphs_states.at(d->GetDrawNo()).fade_state
				== GraphState::STILL)
		return;

	DrawInfo *di = d->GetDrawInfo();

	const wxColour &wc = di->GetDrawColor();

	size_t pc = d->GetValuesTable().size();

	float alpha_val = m_graphs_states.at(d->GetDrawNo()).fade_level;
	GLfloat col[12] = { 
		float(wc.Red()) / 255, float(wc.Green()) / 255, float(wc.Blue()) / 255, 1 * alpha_val,
		float(wc.Red()) / 255 / 2 , float(wc.Green()) / 255 / 2, float(wc.Blue()) / 255, 0.5 * alpha_val,
		float(wc.Red()) / 255 / 4 , float(wc.Green()) / 255 / 4, float(wc.Blue()) / 255 / 4, 0.25 * alpha_val
	};
	GLfloat alternate_col[12] = { 1., 1., 1., 1,  0.5, 0.5 , 0.5, 0.5, 0.25, 0.25, 0.25};

	float line_height = 2;
	if (d->GetSelected() && d->GetDoubleCursor())
		line_height = 4;
	GLfloat *cc = &col[0];

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cc); glMaterialfv(GL_FRONT, GL_SPECULAR, cc);

	bool prev_data = false;
	double py = 0, px = 0;

	bool is_alternate = false;
	bool alt_color_set = false;

	glNormal3f(0, 0, 1);

	glBindTexture(GL_TEXTURE_2D, _line1_tex);
	for (size_t i = 0; i < pc; i++) {
		if (!d->GetValuesTable().at(i).IsData()) {
			prev_data = false;
			continue;
		}

		float x = GetX(i, pc);
		float y = GetY(d->GetValuesTable().at(i).val, di);

		bool pa = AlternateColor(d, i);

		if (is_alternate && pa) {
			if  (!alt_color_set) {
				alt_color_set = true;
				cc = alternate_col;
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cc); glMaterialfv(GL_FRONT, GL_SPECULAR, cc);
			}
		} else {
			if (alt_color_set) {
				alt_color_set = false;
				cc = col;
				glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cc); glMaterialfv(GL_FRONT, GL_SPECULAR, cc);
			}
		}


#if 1
		glPushMatrix();
			glTranslatef(x, y, 0);
			glScalef(line_height, line_height, 1);
			glCallList(_circle_list);
		glPopMatrix();
#endif

		if (prev_data) {
			glPushMatrix();
			{
				float length = sqrtf((x - px) * (x - px) + (y - py) * (y - py));  

				glTranslatef(px, py, 0);
				glRotatef(180.f / M_PI * atan2(y - py, x - px), 0, 0, 1);
#if 0
				glScalef(length, line_height + 2, 1);
				glCallList(m_line_list);
#endif
				glBindTexture(GL_TEXTURE_2D, _line1_tex);
				glBegin(GL_QUADS);
					glTexCoord2f(0, 0);
					glVertex3f(0, line_height + 2, 0);
					glTexCoord2f(1, 0);
					glVertex3f(length, line_height + 2, 0);
					glTexCoord2f(1, 1);
					glVertex3f(length, -line_height - 2, 0);
					glTexCoord2f(0, 1);
					glVertex3f(0, -line_height - 2, 0);
				glEnd();
				glBindTexture(GL_TEXTURE_2D, 0);

			}
			glPopMatrix();

		}

		px = x;
		py = y;
		prev_data = true;
		is_alternate = pa;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

}

void GLGraphs::DrawGraphs() {
	int sel = m_draws_wdg->GetSelectedDrawIndex();
	if (sel < 0)
		return;

	for (size_t i = 0; i <= m_draws.size(); i++) {
		 
		size_t j = i;
		if ((int) j == sel) 
			continue;
		if (j == m_draws.size())	
			j = sel;

		Draw* d = m_draws[j];

		DrawGraph(d);

		if ((int) j == sel)
			DrawCursor(d);
	}

}

void GLGraphs::DrawCursor(Draw *d) {

	int i = d->GetCurrentIndex();
	if (i < 0)
		return;

	if (!d->GetValuesTable().at(i).IsData())
		return;

	float x = GetX(i, d->GetValuesTable().size());
	float y = GetY(d->GetValuesTable().at(i).val, d->GetDrawInfo());

	GLfloat col[] = { 1, 1, 1, .8};
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, col); glMaterialfv(GL_FRONT, GL_SPECULAR, col);

	glPushMatrix();
		glTranslatef(x, y, 0);
		glTranslatef(0, 0, 1);
		glNormal3f(0, 0, 1);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		int quadrant = 0;
		if (x > m_size.GetWidth() / 2)
			quadrant = 1;
		if (y > m_size.GetHeight() / 2)
			quadrant |= 2;

		glVertexPointer(3, GL_FLOAT, 0, cube_vertices);
		glNormalPointer(GL_FLOAT, 0, cube_normals);

		const int front = 0;
		const int left = 4;
		const int right = 8;
		const int top = 12;
		const int bottom = 16;

		switch (quadrant) {
			case 0: //LEFT-DOWN
				glDrawArrays(GL_QUADS, bottom, 4);
				glDrawArrays(GL_QUADS, left, 4);
				glDrawArrays(GL_QUADS, top, 4);
				glDrawArrays(GL_QUADS, right, 4);
				break;
			case 1: //RIGHT-DOWN
				glDrawArrays(GL_QUADS, bottom, 4);
				glDrawArrays(GL_QUADS, right, 4);
				glDrawArrays(GL_QUADS, top, 4);
				glDrawArrays(GL_QUADS, left, 4);
				break;
			case 2: //LEFT-UP
				glDrawArrays(GL_QUADS, bottom, 4);
				glDrawArrays(GL_QUADS, left, 4);
				glDrawArrays(GL_QUADS, top, 4);
				glDrawArrays(GL_QUADS, right, 4);
				break;
			case 3: //RIGHT-UP
				glDrawArrays(GL_QUADS, right, 4);
				glDrawArrays(GL_QUADS, top, 4);
				glDrawArrays(GL_QUADS, bottom, 4);
				glDrawArrays(GL_QUADS, left, 4);
				break;
		}

		glDrawArrays(GL_QUADS, front, 4);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
	glPopMatrix();


}

void GLGraphs::GenerateCircleDisplayList() {

	_circle_list = glGenLists(1);	
	
	glNewList(_circle_list, GL_COMPILE);
	{
		glBegin(GL_TRIANGLE_FAN);
		{
			glBindTexture(GL_TEXTURE_2D, _line1_tex);
			glNormal3f(0, 0, 1);
			int slices = 8;

			glTexCoord2f(1, 0.5);
			glVertex3f(0, 0, 0);

			glTexCoord2f(1, 1);
			glVertex3f(1, 0, 0);
			for (int i = 1; i < slices; i++) {
				float x = cos(2 * M_PI * i / slices);
				float y = sin(2 * M_PI * i / slices);
				glTexCoord2f(1, 1);
				glVertex3f(x, y, 0);
			}
			glVertex3f(1, 0, 0);

			glBindTexture(GL_TEXTURE_2D, 0);
		}
		glEnd();
	}
	glEndList();
}

void GLGraphs::GenerateLineList() {
	_line_list = glGenLists(1);	
	
	glNewList(_line_list, GL_COMPILE);
	{
		glBindTexture(GL_TEXTURE_2D, _line1_tex);
		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex3f(0, 1, 0);
			glTexCoord2f(1, 0);
			glVertex3f(1, 1, 0);
			glTexCoord2f(1, 1);
			glVertex3f(1, -1, 0);
			glTexCoord2f(0, 1);
			glVertex3f(0, -1, 0);
		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glEndList();

}

void GLGraphs::DrawRemarksTriangles() {
	Draw *d = m_draws_wdg->GetSelectedDraw();
	const Draw::VT& vt = d->GetValuesTable();

	glBegin(GL_TRIANGLES);

	float white[] = { 1, 0, 0, 1};
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white);
	glMaterialfv(GL_FRONT, GL_SPECULAR, white);

	for (size_t i = 0; i < vt.size(); i++)
		if (vt.at(i).m_remark) { 
			int x = GetX(i, vt.size());
			glVertex3f(x, m_size.GetHeight() - m_screen_margins.topmargin, 0);
			glVertex3f(x - 5, m_size.GetHeight() - m_screen_margins.topmargin + 5, 0);
			glVertex3f(x + 5, m_size.GetHeight() - m_screen_margins.topmargin + 5, 0);
		}
	glEnd();
}
	
void GLGraphs::OnPaint(wxPaintEvent & WXUNUSED(event))
{
	if (m_draws_wdg == NULL || !IsShown())
		return;
	if (m_gl_context == NULL)
		m_gl_context = wxGetApp().GetGLContext();
	wxPaintDC dc(this);
	DoPaint();
}

void GLGraphs::DrawCurrentParamName() {

	DrawInfo *di = m_draws_wdg->GetCurrentDrawInfo();
	if (di == NULL)
		return;

	const wxColor& wc = di->GetDrawColor();

	const float gc[] = { float(wc.Red()) / 255, float(wc.Green()) / 255, float(wc.Blue()) / 255, 1};

	int fs = _font->FaceSize();
	_font->FaceSize(fs * 1.25);

	wxString text = m_cfg_mgr->GetConfigTitles()[di->GetBasePrefix()] + _T(":") + di->GetParamName();

	FTBBox bbox = _font->BBox(text.c_str());
	float tw = bbox.Upper().Xf() - bbox.Lower().Xf();
	float th = bbox.Upper().Yf() - bbox.Lower().Yf();

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gc); glMaterialfv(GL_FRONT, GL_SPECULAR, gc);
			
	glPushMatrix();
		glTranslatef(m_size.GetWidth() / 2 - tw / 2, m_size.GetHeight() / 2 - th / 2, 0);
		_font->Render(text.c_str());
	glPopMatrix();

	_font->FaceSize(fs);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GLGraphs::DoPaint() {

	GLGraphs::m_gl_context->SetCurrent(*this);

	InitGL();

	Init();

	RecalculateMargins();

	GeneratePaneDisplayList();
	
	SetUpScene();

	glPushMatrix();
	{
		glTranslatef(-m_size.GetWidth() / 2, -m_size.GetHeight() / 2, 0);

		SetLight();

		glPushMatrix();
		{
			glTranslatef(0, 0, -100);
			DrawBackground();
			DrawYAxis();
			DrawXAxis();
			Draw *d = m_draws_wdg->GetSelectedDraw();
			if (d) {
				DrawYAxisVals(d);
				DrawXAxisVals(d);
				DrawUnit(d);
			}
			DrawShortNames();
			DrawRemarksTriangles();
			DrawSeasonLimits();
		}
		glPopMatrix();

		glPushMatrix();
		{
			glTranslatef(0, 0, -100);
			DrawGraphs();
			if (m_draw_current_draw_name) 
				DrawCurrentParamName();
		}
		glPopMatrix();
	}
	glPopMatrix();


	glFlush();
	SwapBuffers();
}

void GLGraphs::Deselected(int i) {
}

void GLGraphs::RecalculateMargins() {
	if (!m_recalulate_margins)
		return;

	Draw *d = m_draws_wdg->GetSelectedDraw();
	if (d == NULL)
		return;

	DrawInfo *di = d->GetDrawInfo();

	wxString max = di->GetValueStr(di->GetMax(), _T(""));
	wxString min = di->GetValueStr(di->GetMin(), _T(""));
	wxString unit = di->GetUnit();

	float max_width = _font->Advance(max.c_str());
	float min_width = _font->Advance(min.c_str());
	float unit_width = _font->Advance(unit.c_str());

	m_screen_margins.leftmargin = wxMax(wxMax(max_width, min_width) + 3, unit_width + 6);

	m_recalulate_margins = false;

	glBindTexture(GL_TEXTURE_2D, 0);

}

void GLGraphs::Selected(int i) {
	m_recalulate_margins = true;
	Refresh();
}

void GLGraphs::OnSize(wxSizeEvent & WXUNUSED(event))
{
	int w, h;
	GetClientSize(&w, &h);
	if (m_size.GetWidth() == w && m_size.GetHeight() == h)
		return;
	m_size = wxSize(w, h);
	DestroyPaneDisplayList();
	Refresh();
}

int GLGraphs::GetIndex(size_t i,  int x) const {
	int w = m_size.GetWidth();
    
	x -= m_screen_margins.leftmargin;
	w -= m_screen_margins.leftmargin + m_screen_margins.rightmargin;
    
	int index = x * m_draws[i]->GetValuesTable().size() / w;

	return index;
}

void GLGraphs::GetDistance(size_t draw_index, int x, int y, double &d, wxDateTime &time) {
	d = INFINITY;
	time = wxInvalidDateTime;
	/* this defines search radius for 'direct hit' */

	int h = m_size.GetHeight();
	
	/* forget about top and bottom margins */
	h -= m_screen_margins.bottommargin + m_screen_margins.topmargin;
	/* get X coordinate */
	int index = GetIndex(draw_index, x);

	const Draw::VT& vt = m_draws[draw_index]->GetValuesTable();
    
	int i, j;
	for (i = j = index; i >= 0 || j < (int)vt.size(); --i, ++j) {
		double d1 = INFINITY;
		double d2 = INFINITY;

		if (i >= 0 && i < (int)vt.size() && vt.at(i).IsData()) {
			float xi = GetX(i, vt.size());
			float yi = GetY(vt[i].val, m_draws[draw_index]->GetDrawInfo());
			d1 = (x - xi) * (x - xi) + (y - yi) * (y - yi);
		}

		if (j >= 0 && j < (int)vt.size() && vt.at(j).IsData()) {
			float xj = GetX(j, vt.size());
			float yj = GetY(vt[j].val, m_draws[draw_index]->GetDrawInfo());
			d2 = (x - xj) * (x - xj) + (y - yj) * (y - yj);
		}

		if (!std::isfinite(d1) && !std::isfinite(d1))
			continue;

		if (!std::isfinite(d2) || d1 < d2) {
			d = d1;
			time = m_draws[draw_index]->GetTimeOfIndex(i);
		} else {
			d = d2;
			time = m_draws[draw_index]->GetTimeOfIndex(j);
		}

		break;

	}

}

void GLGraphs::OnMouseLeftDown(wxMouseEvent & event) {

	/* check for 'move cursor left' event */
	if (event.GetX() < m_screen_margins.leftmargin) {
		m_draws_wdg->MoveCursorLeft();
		return;
	}

	/* check for 'move cursor right' event */
	if (event.GetX() > (m_size.GetWidth() - m_screen_margins.rightmargin)) {
		m_draws_wdg->MoveCursorRight();
		return;
	}

	int index = m_draws_wdg->GetSelectedDrawIndex();
	if (index < 0)
		return;

	wxDateTime ct = m_draws_wdg->GetSelectedDraw()->GetCurrentTime();
	if (!ct.IsValid())
		return;

	int x = event.GetX();
	int y = m_size.GetHeight() - event.GetY();

	int sri = -1;

	if (y >= m_size.GetHeight() - m_screen_margins.topmargin && y <= m_size.GetHeight() - m_screen_margins.topmargin + 10) {
		int sdx = -1;

		const ValuesTable& vt = m_draws_wdg->GetSelectedDraw()->GetValuesTable();

		for (size_t i = 0; i < vt.size(); i++)
			if (vt.at(i).m_remark) {
				int rx = GetX(i, vt.size());

				if (rx > x + 5)
					break;
				int d = std::abs(x - rx);

				if (d <= 5) {
					if (sri == -1 || sdx > d) {
						sri = i;
						sdx = d;
					}
				}
			}
	}

	if (sri >= 0) {
		m_draws_wdg->ShowRemarks(sri);
		return;
	}

	struct VoteInfo {
		double dist;
		wxDateTime time;
	};

	VoteInfo infos[m_draws.size()];

	for (size_t i = 0; i < m_draws.size(); ++i) {

		VoteInfo & in = infos[i];
		in.dist = -1;

		if (m_draws[i]->GetEnable())
			GetDistance(i, x, y, in.dist, in.time);
	}

	double min = INFINITY;
	int j = -1;

	int selected_draw = m_draws_wdg->GetSelectedDrawIndex();

	for (size_t i = 1; i <= m_draws.size(); ++i) {
		size_t k = (i + selected_draw) % m_draws.size();

		VoteInfo& in = infos[k];
		if (in.dist < 0)
			continue;

		if (in.dist < 6 * 6 && in.time == ct) {
			j = k;
			break;
		}

		if ((int)k == selected_draw && in.dist == 0) {
			j = k;
			break;
		}

		if (min > in.dist) {
			j = k;
			min = in.dist;
		}
	}

	if (j == -1) 
		return;

	if (selected_draw != j && infos[j].dist < 1000)
		m_draws_wdg->SelectDraw(j, true, infos[j].time);
	else
		m_draws_wdg->SelectDraw(selected_draw, true, infos[selected_draw].time);

}

void GLGraphs::OnMouseLeftUp(wxMouseEvent & event)
{
	m_draws_wdg->StopMovingCursor();
}

void GLGraphs::OnMouseLeftDClick(wxMouseEvent & event)
{
	if (m_draws.size() == 0)
		return;

	if (m_draws_wdg->GetNoData())
		return;

	/* get widget size */
	int w, h;
	GetClientSize(&w, &h);

	/* check for 'move screen left' event */
	if (event.GetX() < m_screen_margins.leftmargin) {
		m_draws_wdg->MoveScreenLeft();
		return;
	}

	/* check for 'move screen right' event */
	if (event.GetX() > w - m_screen_margins.rightmargin) {
		m_draws_wdg->MoveScreenRight();
		return;
	}
}

void GLGraphs::OnSetFocus(wxFocusEvent& event) {
}

void GLGraphs::OnMouseRightDown(wxMouseEvent &event) {
	if (m_draws_wdg->GetSelectedDrawIndex() == -1)
		return;

	Draw *d = m_draws[m_draws_wdg->GetSelectedDrawIndex()];
	DrawInfo *di = d->GetDrawInfo();

	SetInfoDataObject wido(di->GetBasePrefix(), di->GetSetName(), d->GetPeriod(), d->GetCurrentTime().GetTicks(), m_draws_wdg->GetSelectedDrawIndex());
	wxDropSource ds(wido, this);
	ds.DoDragDrop(0);
}

wxDragResult GLGraphs::SetSetInfo(wxCoord x, 
		wxCoord y, 
		wxString window,
		wxString prefix, 
		time_t time,
		PeriodType pt, 
		wxDragResult def) {
	return m_draws_wdg->SetSetInfo(window, prefix, time, pt, def);
}

void GLGraphs::SetFocus() {
	wxWindow::SetFocus();
}

void GLGraphs::InitGL() {

	if (_gl_ready == false) {
		wxString font_file = 
#ifdef __WXMSW__
			wxGetApp().GetSzarpDir() + _T("\\resources\\fonts\\FreeSansBold.ttf");
#else
			_T("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf");
#endif
		_font = new FTGLTextureFont(const_cast<char*>(SC::S2A(font_file).c_str()));
		_font->FaceSize(GetFont().GetPointSize() + 4);
		_font->CharMap(ft_encoding_unicode);

                wxImage img;
		wxString b1_texture_file = wxGetApp().GetSzarpDir();
#ifndef MINGW32
		b1_texture_file += _T("resources/wx/images/draw_background1_tex.bmp");
#else   
		b1_texture_file += _T("resources\\wx\\images\\draw_background1_tex.bmp");
#endif
		img.LoadFile(b1_texture_file);

		size_t pixels_count = img.GetWidth() * img.GetHeight();
		std::vector<unsigned char> rgba(3 * pixels_count);
		for (size_t i = 0; i < pixels_count; i++) {
			rgba[i * 3] = img.GetData()[i * 3];
			rgba[i * 3 + 1] = img.GetData()[i * 3 + 1];
			rgba[i * 3 + 2] = img.GetData()[i * 3 + 2];
		}

		glGenTextures(1, &_back1_tex);
		glBindTexture(GL_TEXTURE_2D, _back1_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST | GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST | GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, img.GetWidth(), img.GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, &rgba[0]);
		glBindTexture(GL_TEXTURE_2D, 0);

		unsigned char line_texture[8][1][4];
		line_texture[0][0][0] = 64;  line_texture[0][0][1] = 64; line_texture[0][0][2] = 64; line_texture[0][0][3] = 64; 
		line_texture[1][0][0] = 128;  line_texture[1][0][1] = 128; line_texture[1][0][2] = 128; line_texture[1][0][3] = 128; 
		line_texture[2][0][0] = 255;  line_texture[2][0][1] = 255; line_texture[2][0][2] = 255; line_texture[2][0][3] = 255; 
		line_texture[3][0][0] = 255;  line_texture[3][0][1] = 255; line_texture[3][0][2] = 255; line_texture[3][0][3] = 255; 
		line_texture[4][0][0] = 255;  line_texture[4][0][1] = 255; line_texture[4][0][2] = 255; line_texture[4][0][3] = 255; 
		line_texture[5][0][0] = 255;  line_texture[5][0][1] = 255; line_texture[5][0][2] = 255; line_texture[5][0][3] = 255; 
		line_texture[6][0][0] = 128;  line_texture[6][0][1] = 128; line_texture[6][0][2] = 128; line_texture[6][0][3] = 128; 
		line_texture[7][0][0] = 64;  line_texture[7][0][1] = 64; line_texture[7][0][2] = 64; line_texture[7][0][3] = 64; 

		glGenTextures(1, &_line1_tex);
		glBindTexture(GL_TEXTURE_2D, _line1_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST | GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST | GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, 1, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, line_texture);
		glBindTexture(GL_TEXTURE_2D, 0);

		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glShadeModel(GL_FLAT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glLineWidth(1);

#if 0
		glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);
#endif

		glNormal3f(0, 0, 1);

		GenerateCircleDisplayList();
		GenerateLineList();

		_gl_ready = true;
	}

	
}

namespace {
const int PeriodMarkShift[PERIOD_T_LAST] = {0, 0, 1, 3, 3, 0};
}


void GLGraphs::DrawXAxis() {
	float white[] = { 1, 1, 1, 1};
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white); glMaterialfv(GL_FRONT, GL_SPECULAR, white);

	glBegin(GL_QUADS);
		glVertex3f(m_screen_margins.leftmargin, m_screen_margins.bottommargin - 2, 0);
		glVertex3f(m_screen_margins.leftmargin, m_screen_margins.bottommargin + 0, 0);
		glVertex3f(m_size.GetWidth(), m_screen_margins.bottommargin, 0);
		glVertex3f(m_size.GetWidth(), m_screen_margins.bottommargin - 2, 0);

		glVertex3f(m_size.GetWidth() - 8, m_screen_margins.bottommargin + 8, 0);
		glVertex3f(m_size.GetWidth() - 8, m_screen_margins.bottommargin + 10, 0);
		glVertex3f(m_size.GetWidth(), m_screen_margins.bottommargin + 2, 0);
		glVertex3f(m_size.GetWidth(), m_screen_margins.bottommargin, 0);

		glVertex3f(m_size.GetWidth() - 8, m_screen_margins.bottommargin - 10, 0);
		glVertex3f(m_size.GetWidth() - 8, m_screen_margins.bottommargin - 8, 0);
		glVertex3f(m_size.GetWidth(), m_screen_margins.bottommargin, 0);
		glVertex3f(m_size.GetWidth(), m_screen_margins.bottommargin - 2, 0);
	glEnd();

}

void GLGraphs::DrawXAxisVals(Draw *draw) {
	float white[] = { 1, 1, 1, 1};
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white); glMaterialfv(GL_FRONT, GL_SPECULAR, white);

	int i = PeriodMarkShift[draw->GetPeriod()];
	int size = draw->GetValuesTable().size();
    
	const float tick_height = 5;
	wxDateTime prev_date;
	while (i < size) {
		float x = GetX(i, size);
		glBegin(GL_QUADS);
			glVertex3f(x, m_screen_margins.bottommargin, 0);
			glVertex3f(x, m_screen_margins.bottommargin + tick_height, 0);
			glVertex3f(x + 1, m_screen_margins.bottommargin + tick_height, 0);
			glVertex3f(x + 1, m_screen_margins.bottommargin, 0);
		glEnd();

		wxDateTime date = draw->GetTimeOfIndex(i);
	
		/* Print date */
		wxString datestring = get_date_string(draw->GetPeriod(), prev_date, date);

		glPushMatrix();
		{
			FTBBox bbox = _font->BBox(datestring.c_str()); float width = bbox.Upper().Xf() - bbox.Lower().Xf();
			float height = bbox.Upper().Yf() - bbox.Lower().Yf();

			glTranslatef(x - width / 2, m_screen_margins.bottommargin - 4 - height, 0);
			_font->Render(datestring.c_str());
		}
		glPopMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);

		i += TimeIndex::PeriodMult[draw->GetPeriod()];
		prev_date = date;
	}
    
}

void GLGraphs::PeriodChanged(Draw *draw, PeriodType period) {
	if (draw->GetSelected())
		DestroyPaneDisplayList();

	Refresh(); 
}

void GLGraphs::DrawYAxisVals(Draw *draw) {
	float white[] = { 1, 1, 1, 1};
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, white); glMaterialfv(GL_FRONT, GL_SPECULAR, white);

	double min = draw->GetDrawInfo()->GetMin();
	double max = draw->GetDrawInfo()->GetMax();
	double dif = max - min;

	if (dif <= 0) {
		wxLogInfo(_T("%s %f %f"), draw->GetDrawInfo()->GetName().c_str(), min, max);
		assert(false);
	}

	//procedure for calculating distance between marks stolen from SzarpDraw2
	double x = dif;
	double step;
	int i = 0;

	if (x < 1)
		for (;x < 1; x *=10, --i);
	else
		for (;(int)x / 10; x /=10, ++i);

	if (x <= 1.5)
		step = .1;
	else if (x <= 3.)
		step = .2;
	else if (x <= 7.5)
		step = .5;
	else
		step = 1.;

	double acc = 1;

	int prec = draw->GetDrawInfo()->GetPrec();
				
	for (int p = prec; p > 0; --p)
		acc /= 10;

	double factor  = (i > 0) ? 10 : .1;

	for (;i; i -= i / abs(i))
		step *= factor;

	if (step < acc)
		step = acc;

	int h = m_size.GetHeight();

	h -= m_screen_margins.bottommargin + m_screen_margins.topmargin;

	for (double val = max; (min - val) < acc; val -= step) {

		float y = GetY(val, draw->GetDrawInfo());

		glBegin(GL_QUADS);
			glVertex3f(m_screen_margins.leftmargin - 10, y - 2, 0);
			glVertex3f(m_screen_margins.leftmargin - 10, y, 0);
			glVertex3f(m_screen_margins.leftmargin, y, 0);
			glVertex3f(m_screen_margins.leftmargin, y - 2, 0);
		glEnd();

		wxString sval = draw->GetDrawInfo()->GetValueStr(val, _T(""));

		FTBBox bbox = _font->BBox(sval.c_str());
		float width = bbox.Upper().Xf() - bbox.Lower().Xf();
		float height = bbox.Upper().Yf() - bbox.Lower().Yf();

		glPushMatrix();
			glTranslatef(m_screen_margins.leftmargin - width - 3, y - 2 - height, 0);
			_font->Render(sval.c_str());
		glPopMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);
	}

}

void GLGraphs::DrawUnit(Draw *d) {
	const wxString& s = d->GetDrawInfo()->GetUnit();

	float width = _font->Advance(s.c_str());
	glPushMatrix();
		glTranslatef(m_screen_margins.leftmargin - width - 6, m_size.GetHeight() - m_screen_margins.topmargin + 3, 0);
		_font->Render(s.c_str());
	glPopMatrix();
}

void GLGraphs::DrawShortNames() {
	if (m_draws.size() < 1)
		return ;

	glPushMatrix(); 
	{
		glTranslatef(m_screen_margins.leftmargin + 10, m_size.GetHeight() - m_screen_margins.topmargin + 9, 0);

		float x = 0;

		for (size_t i = 0; i < m_draws.size(); i++) {

			if (!m_draws[i]->GetEnable())
				continue;

			DrawInfo *info = m_draws[i]->GetDrawInfo();

			const wxColor& wc = info->GetDrawColor();
			const float gc[] = { float(wc.Red()) / 255, float(wc.Green()) / 255, float(wc.Blue()) / 255, 1};

			FTBBox bbox = _font->BBox(info->GetShortName().c_str());
			float width = bbox.Upper().Xf() - bbox.Lower().Xf();

			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gc); glMaterialfv(GL_FRONT, GL_SPECULAR, gc);
			
			glPushMatrix();
				glTranslatef(x, 0, 0);
				_font->Render(info->GetShortName().c_str());
			glPopMatrix();
			x += width + 5;
		}

	}
	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, 0);
}


void GLGraphs::DrawSeasonLimitInfo(Draw *d, int i, int month, int day, bool summer) {

	float x1 = GetX(i, d->GetValuesTable().size());
	float x2 = GetX(i + 1, d->GetValuesTable().size());

	float x = (x1 + x2) / 2;

	float color[] = {0, 0, 0, 1};
	if (summer)
		color[2] = 1;
	else
		color[0] = 1;

	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color); glMaterialfv(GL_FRONT, GL_SPECULAR, color);

	glBegin(GL_QUADS);
	{
		glVertex3f(x + 1, m_screen_margins.bottommargin + 1, 0);
		glVertex3f(x + 1, m_size.GetHeight() - m_screen_margins.topmargin - 1, 0);
		glVertex3f(x + 3, m_size.GetHeight() - m_screen_margins.topmargin - 1, 0);
		glVertex3f(x + 3, m_screen_margins.bottommargin + 1, 0);
	}
	glEnd();

	wxString str;
	if (summer)
		str = wxString(_("summer season"));
	else
		str = wxString(_("winter season"));

	str += wxString::Format(_T(" %d "), day);

	switch (month) {
		case 1:
			str += _("january");
			break;
		case 2:
			str += _("february");
			break;
		case 3:
			str += _("march");
			break;
		case 4:
			str += _("april");
			break;
		case 5:
			str += _("may");
			break;
		case 6:
			str += _("june");
			break;
		case 7:
			str += _("july");
			break;
		case 8:
			str += _("august");
			break;
		case 9:
			str += _("septermber");
			break;
		case 10:
			str += _("october");
			break;
		case 11:
			str += _("november");
			break;
		case 12:
			str += _("december");
			break;
		default:
			assert(false);
	}

	float letter_height = GetFont().GetPointSize() + 4;
	float letter_w_width = _font->Advance(_T("W"));


	float ty = m_size.GetHeight() - m_screen_margins.topmargin - GetFont().GetPointSize() - 4 - 4;
	for (size_t i = 0; i < str.Len(); ++i) {
		wxString letter = str.Mid(i, 1);
		if (letter == _T(" ")) {
			ty -= letter_height - 4;
			continue;
		}
		float width = _font->Advance(letter.c_str());
		glPushMatrix();
		{
			glTranslatef(x + 2 + (letter_w_width - width) / 2, ty, 0);
			_font->Render(letter.c_str());
		}
		glPopMatrix();
		ty -= letter_height - 4;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

}

void GLGraphs::DrawSeasonLimits() {
	Draw* draw = m_draws_wdg->GetSelectedDraw();
	if (draw == NULL)
		return;

	DrawsSets *cfg = m_cfg_mgr->GetConfigByPrefix(draw->GetDrawInfo()->GetBasePrefix());

	std::vector<SeasonLimit> limits = get_season_limits_indexes(cfg, draw);
	for (std::vector<SeasonLimit>::iterator i = limits.begin(); i != limits.end(); i++)
		DrawSeasonLimitInfo(draw, i->index, i->month, i->day, i->summer);

}


void GLGraphs::OnTimer(wxTimerEvent& event) {
	bool refresh = false;
	bool restart_timer = false;

	for (size_t i = 0; i < m_graphs_states.size(); i++)
		switch (m_graphs_states[i].fade_state) {
			case GraphState::FADING:
				refresh = true;
				m_graphs_states[i].fade_level -= 0.2;
				if (m_graphs_states[i].fade_level <= 0) {
					m_graphs_states[i].fade_level = 0;
					m_graphs_states[i].fade_state = GraphState::STILL;
				} else
					restart_timer = true;
				break;
			case GraphState::EMERGING:
				refresh = true;
				m_graphs_states[i].fade_level += 0.2;
				if (m_graphs_states[i].fade_level >= 1) {
					m_graphs_states[i].fade_state = GraphState::STILL;
					m_graphs_states[i].fade_level = 1;
				} else
					restart_timer = true;
				break;
			case GraphState::STILL:
				break;
		}

	if (refresh)
		DoPaint();

	if (restart_timer)
		m_timer->Start(100, wxTIMER_ONE_SHOT);
}

void GLGraphs::StartDrawingParamName() {
	m_draw_current_draw_name = true;
	Refresh();
}

void GLGraphs::StopDrawingParamName() {
	m_draw_current_draw_name = false;
	Refresh();
}

void GLGraphs::NewRemarks(Draw *d) {
	Refresh();
}

GLGraphs::~GLGraphs() {
}

void GLGraphs::EnableChanged(Draw *draw) {
	bool enable = draw->GetEnable();
	int i = draw->GetDrawNo();
	if (enable)
		m_graphs_states.at(i).fade_state = GraphState::EMERGING;
	else
		m_graphs_states.at(i).fade_state = GraphState::FADING;

	Refresh();
	m_timer->Start(100, wxTIMER_ONE_SHOT);
}

GLGraphs::GraphState::GraphState() {
	fade_state = STILL;
	fade_level = 1;
}

void GLGraphs::DrawSelected(Draw *draw) {
	Refresh();
}

void GLGraphs::DrawInfoChanged(Draw *draw) { 
	if (draw->GetSelected()) {
		m_draws.resize(0);

		DrawsController* controller = draw->GetDrawsController();
		for (size_t i = 0; i < controller->GetDrawsCount(); i++)
			m_draws.push_back(controller->GetDraw(i));

		m_graphs_states.resize(m_draws.size());
		for (size_t i = 0; i < m_graphs_states.size(); i++) {
			m_graphs_states[i].fade_state = GraphState::STILL;
			m_graphs_states[i].fade_level = 1;
		}

		m_recalulate_margins = true;
		Refresh();
	}
}

#endif
#endif
