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
 * $Id: xygraph.h 237 2009-12-21 11:35:22Z reksi0 $
 */

#include "config.h"

#ifdef HAVE_GLCANVAS
#ifdef HAVE_FTGL

#include <algorithm>
#include <sstream>

#include <wx/dcbuffer.h>
#include <wx/splitter.h>
#include <wx/glcanvas.h>
#include <FTGL/ftgl.h>
#include <GL/glu.h>

#include "szhlpctrl.h"

#include "cconv.h"
#include "szframe.h"

#include "ids.h"
#include "classes.h"

#include "cfgmgr.h"
#include "coobs.h"
#include "defcfg.h"
#include "drawtime.h"
#include "dbinquirer.h"
#include "database.h"
#include "dbmgr.h"
#include "draw.h"
#include "drawview.h"
#include "xydiag.h"
#include "xyzgraph.h"
#include "progfrm.h"
#include "timeformat.h"
#include "drawapp.h"

class XYZCanvas : public wxGLCanvas  {
	XYGraph *m_graph;
	wxGLContext *m_gl_context;
	GLUquadric* m_quad;	
	std::vector<float> m_vertices;
	std::vector<float> m_normals;
	std::vector<float> m_colors;
	bool m_update_arrays;
	wxTimer *m_timer;
	FTFont *m_font;

	static const size_t slices_no = 25; 
	
	void DrawFrameOfReference();
	void DrawAxis();
	void DrawColoredGraph();
	void DrawLineGraph();
	void UpdateArrays();
	void DrawPointInfo();
	size_t XZToIndex(size_t x, size_t z);

	float m_vrot, m_hrot;
	float m_x_disp, m_z_disp, m_y_disp;

	enum { WIRE, COLORED } m_graph_type;

	enum {
		UP_ROTATION,
		DOWN_ROTATION,
		FORWARD_MOVEMENT,
		BACKWARD_MOVEMENT,
		UP_MOVEMENT,
		DOWN_MOVEMENT,
		LEFT_ROTATION,
		RIGHT_ROTATION,
		LEFT_MOVEMENT,
		RIGHT_MOVEMENT,
		CURSOR_DOWN,
		CURSOR_UP,
		CURSOR_LEFT,
		CURSOR_RIGHT,
		NONE
	} m_movement;

	int m_cursor_x, m_cursor_z;
public:
	XYZCanvas(wxWindow *parent);	
	void OnPaint(wxPaintEvent & event);
	void OnSize(wxSizeEvent& event);
	void SetGraph(XYGraph *graph);
	void OnTimer(wxTimerEvent &e);
	void OnChar(wxKeyEvent &event);
	void OnKeyUp(wxKeyEvent &event);
	void DrawCursor();	
	DECLARE_EVENT_TABLE()
};

namespace {

	float cube_vertices[] = {
		//FRONT
		-0.3, -0.3, 0.3,
		-0.3, 0.3, 0.3,
		0.3, 0.3, 0.3,
		0.3, -0.3, 0.3,
		//LEFT
		0.3, 0.3, 0.3,
		0.3, 0.3, -0.3,
		0.3, -0.3, -0.3,
		0.3, -0.3, 0.3,
		//RIGHT
		-0.3, 0.3, 0.3,
		-0.3, 0.3, -0.3,
		-0.3, -0.3, -0.3,
		-0.3, -0.3, 0.3,
		//TOP
		-0.3, 0.3, 0.3,
		-0.3, 0.3, -0.3,
		0.3, 0.3, -0.3,
		0.3, 0.3, 0.3,
		//BOTTOM
		-0.3, -0.3, 0.3,
		-0.3, -0.3, -0.3,
		0.3, -0.3, -0.3,
		0.3, -0.3, 0.3,
		//BACK
		0.3, 0.3, -0.3,
		0.3, -0.3, -0.3,
		-0.3, -0.3, -0.3,
		-0.3, 0.3, -0.3,
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
		0, -1, 0,
		0, 0, -1,
		0, 0, -1,
		0, 0, -1,
		0, 0, -1
	};


}


XYZCanvas::XYZCanvas(wxWindow *parent) : wxGLCanvas(parent, wxID_ANY,
#ifdef __WXGTK__
		wxGetApp().GLContextAttribs(),
#else
		NULL,
#endif
		wxDefaultPosition, wxSize(300, 200), 0, _T("XYZCanvas")), m_graph(NULL), m_gl_context(NULL) {
	m_vertices.resize(3 * slices_no * slices_no);
	m_normals.resize(3 * slices_no * slices_no, 0);
	m_colors.resize(4 * slices_no * slices_no);
	m_movement = NONE;
	m_hrot = 30;
	m_vrot = -30;
	m_x_disp = 0;
	m_y_disp = 0;
	m_z_disp = -60;
	m_timer = new wxTimer(this, wxID_ANY);
	m_timer->Start(100);
	m_graph_type = COLORED;
	m_cursor_x = -1;
	m_cursor_z = -1;
	m_font = NULL;
}

void XYZCanvas::DrawFrameOfReference() {
	if (m_graph == NULL)
		return;

	float color[4] = {0, 0, 0, 1};

	wxColour c = m_graph->m_di[2]->GetDrawColor();
	color[0] = float(c.Red()) / 255;
	color[1] = float(c.Green()) / 255;
	color[2] = float(c.Blue()) / 255;
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color); 
	DrawAxis();

	glPushMatrix();
	glRotatef(90, 0, 1, 0);

	c = m_graph->m_di[0]->GetDrawColor();
	color[0] = float(c.Red()) / 255;
	color[1] = float(c.Green()) / 255;
	color[2] = float(c.Blue()) / 255;
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color); 
	DrawAxis();

	c = m_graph->m_di[1]->GetDrawColor();
	color[0] = float(c.Red()) / 255;
	color[1] = float(c.Green()) / 255;
	color[2] = float(c.Blue()) / 255;
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color); 
	glRotatef(-90, 1, 0, 0);
	DrawAxis();

	glPopMatrix();
}

void XYZCanvas::DrawAxis() {
	gluCylinder(m_quad, 0.1, 0.1, 24, 30, 1);
	glPushMatrix();
	glTranslatef(0, 0, 24);
	gluCylinder(m_quad, 0.3, 0.0, 0.5, 30, 10);
	glPopMatrix();
}

void XYZCanvas::OnPaint(wxPaintEvent& event) {
	if (m_gl_context == NULL)
		m_gl_context = wxGetApp().GetGLContext();

	if (m_font == NULL) {
		wxString font_file = 
#ifdef __WXMSW__
			wxGetApp().GetSzarpDir() + _T("\\resources\\fonts\\FreeSansBold.ttf");
#else
			_T("/usr/share/fonts/truetype/freefont/FreeSansBold.ttf");
#endif
		m_font = new FTGLTextureFont(const_cast<char*>(SC::S2A(font_file).c_str()));
		m_font->FaceSize(GetFont().GetPointSize() + 4);
		m_font->CharMap(ft_encoding_unicode);
	}

	assert(m_gl_context);

	wxPaintDC dc(this);

	m_gl_context->SetCurrent(*this);

	wxSize size = GetSize();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, size.GetWidth(), size.GetHeight());

	if (m_graph == NULL)
		return;

	glFrustum(-0.7, 0.7, -0.7, 0.7, 1, 100);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDepthFunc(GL_LEQUAL);
	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glTranslatef(m_x_disp, m_y_disp, m_z_disp);
	glRotatef(m_hrot, 1, 0, 0);
	glRotatef(m_vrot, 0, 1, 0);
	m_quad = gluNewQuadric();
	DrawFrameOfReference();
	gluDeleteQuadric(m_quad);
	switch (m_graph_type) {
		case COLORED:
			DrawColoredGraph();
			break;
		case WIRE:
			DrawLineGraph();
			break;
	}
	DrawCursor();

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, size.GetWidth(), 0, size.GetHeight(), 0.7, 2);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(3, size.GetHeight() - 1, -2); 
	DrawPointInfo();

	SwapBuffers();
}

void XYZCanvas::DrawPointInfo() {
	if (m_graph == NULL)
		return;
	size_t pi = XZToIndex(m_cursor_x, m_cursor_z);

	for (size_t i = 0; i < 3; i++) {
		double v = m_vertices[pi + i] / slices_no * (m_graph->m_dmax[i] - m_graph->m_dmin[i]) + m_graph->m_dmin[i];
		wxString str = wxString::Format(_T("%c: %.*f %s %s"),
				L'X' + i,
				m_graph->m_di[i]->GetPrec(),
				v,
				m_graph->m_di[i]->GetUnit().c_str(),
				m_graph->m_di[i]->GetShortName().c_str());
		wxColor c = m_graph->m_di[i]->GetDrawColor();
		float gc[] = {0, 0, 0, 1};
		gc[0] = float(c.Red()) / 255; 
		gc[1] = float(c.Green()) / 255; 
		gc[2] = float(c.Blue()) / 255; 
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, gc);
		FTBBox bbox = m_font->BBox(str.c_str());
		glTranslatef(0, -1.1 * (bbox.Upper().Yf() - bbox.Lower().Yf()), 0);
		m_font->Render(str.c_str());
	}
}

size_t XYZCanvas::XZToIndex(size_t x, size_t z) {
	return 3 * (x  + slices_no * z);
}

void XYZCanvas::UpdateArrays() {
	float xmin = m_graph->m_dmin[0];
	float xmax = m_graph->m_dmax[0];
	float ymin = m_graph->m_dmin[1];
	float ymax = m_graph->m_dmax[1];
	float zmin = m_graph->m_dmin[2];
	float zmax = m_graph->m_dmax[2];
	for (size_t x = 0; x < slices_no; x++) for (size_t z = 0; z < slices_no; z++) {
		int k = XZToIndex(x, z);
		m_vertices[k] = x;
		m_vertices[k + 2] = z;
	}

#if 0
	std::cout << "Index values: " << std::endl;
	for (size_t x = 0; x < slices_no; x++)  {
		for (size_t z = 0; z < slices_no; z++) {
			int k = XZToIndex(x, z);
			std::cout << "(" << x << "," << z << ")(" << m_vertices[k] << "," << m_vertices[k + 2] << ") ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
#endif
	
	std::vector<float> sums(slices_no * slices_no, 0);
	std::vector<int> counts(slices_no * slices_no, 0);

	for (size_t i = 0; i < m_graph->m_points_values.size(); i++) {
		int x = (m_graph->m_points_values[i].first[0] - xmin) / (xmax - xmin) * slices_no;
		int z = (m_graph->m_points_values[i].first[2] - zmin) / (zmax - zmin) * slices_no;
		int k = XZToIndex(x, z) / 3;
		sums[k] += m_graph->m_points_values[i].first[1];
		counts[k] += 1;
	}

	for (size_t x = 0; x < slices_no; x++) for (size_t z = 0; z < slices_no; z++) {
		int vi = XZToIndex(x, z);
		int si = vi / 3;
		float v;
		if (counts[si])
			v = sums[si] / counts[si];
		else
			v = ymin;
		m_vertices[vi + 1] = (v - ymin) / (ymax - ymin) * slices_no;
		m_colors[si * 4] = (v - ymin) / (ymax - ymin);
		m_colors[si * 4 + 1] = 0;
		m_colors[si * 4 + 2] = 1 - m_colors[si * 4];
		m_colors[si * 4 + 3] = 1;
	}

#if 0
	for (size_t x = 0; x < slices_no; x++) {
		for (size_t z = 0; z < slices_no; z++) {
			int k = XZToIndex(x, z);
			std::cout << m_vertices[k + 1] << "(" << m_colors[k] << "," << m_colors[k + 2] << ") ";
		}
		std::cout << std::endl;
	}
#endif

	for (size_t x = 0; x < slices_no - 2; x++) {
		size_t ppi = XZToIndex(x, 0);
		size_t pi = XZToIndex(x + 1, 0);
		bool up = false;
		size_t z = 1;
		while (z < slices_no - 1) {
			size_t i = XZToIndex(x + up ? 1 : 0, z);

			float v[3], w[3], r[3];

			v[0] = m_vertices[ppi] - m_vertices[pi];
			v[1] = m_vertices[ppi + 1] - m_vertices[pi + 1];
			v[2] = m_vertices[ppi + 2] - m_vertices[pi + 2];

			w[0] = m_vertices[i] - m_vertices[pi];
			w[1] = m_vertices[i + 1] - m_vertices[pi + 1];
			w[2] = m_vertices[i + 2] - m_vertices[pi + 2];

			r[0] = v[1] * w[2] - w[1] * v[2];
			r[1] = w[0] * v[2] - v[0] * w[2];
			r[2] = v[0] * w[1] - w[0] * v[1];
			
			if (up == false) {
				r[0] *= -1;
				r[1] *= -1;
				r[2] *= -1;
			}

			size_t iv[3] = {ppi, pi, i};
			for (size_t j = 0; j < 3; j++)
				for (size_t k = 0; k < 3; k++) 
					m_normals[iv[j] + k] += r[k];

			if (up == false)
				up = true;
			else {
				z += 1;
				up = false;
			}

			ppi = pi;
			pi = i;
		}
	}

	for (size_t i = 0; i < slices_no * slices_no - 1; i++) {
		size_t idx = 3 * i;	
		if (m_normals[idx] == 0 && m_normals[idx + 1] == 0 && m_normals[idx + 2] == 0) {
			m_normals[idx + 1] = 1;
			continue;
		}
		float vs = sqrt(m_normals[idx] * m_normals[idx]
				+ m_normals[idx + 1] * m_normals[idx + 1]
				+ m_normals[idx + 2] * m_normals[idx + 2]);
		m_normals[idx + 1] /= vs;
		m_normals[idx + 1] /= vs;
		m_normals[idx + 2] /= vs;
	}

	m_update_arrays = false;

}

void XYZCanvas::DrawLineGraph() {

	if (m_graph == NULL)
		return;

	if (m_update_arrays)
		UpdateArrays();

	size_t i, ci;
	
	glBegin(GL_LINES);
	for (size_t x = 0; x < slices_no - 2; x++) {
		i = XZToIndex(x, 0);
		ci = i / 3 * 4;
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ci]);
		glNormal3fv(&m_normals[i]);
		glVertex3fv(&m_vertices[i]);

		i = XZToIndex(x + 1, 0);
		ci = i / 3 * 4;
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ci]);
		glNormal3fv(&m_normals[i]);
		glVertex3fv(&m_vertices[i]);

		for (size_t z = 0; z < slices_no - 2; z++) {
			i = XZToIndex(x, z);
			ci = i / 3 * 4;
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ci]);
			glNormal3fv(&m_normals[i]);
			glVertex3fv(&m_vertices[i]);

			i = XZToIndex(x, z + 1);
			ci = i / 3 * 4;
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ci]);
			glNormal3fv(&m_normals[i]);
			glVertex3fv(&m_vertices[i]);

			i = XZToIndex(x, z + 1);
			ci = i / 3 * 4;
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ci]);
			glNormal3fv(&m_normals[i]);
			glVertex3fv(&m_vertices[i]);

			i = XZToIndex(x + 1, z + 1);
			ci = i / 3 * 4;
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ci]);
			glNormal3fv(&m_normals[i]);
			glVertex3fv(&m_vertices[i]);

		}
	}
	for (size_t z = 0; z < slices_no - 2; z++) {
		i = XZToIndex(slices_no - 2, z);
		ci = i / 3 * 4;
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ci]);
		glNormal3fv(&m_normals[i]);
		glVertex3fv(&m_vertices[i]);

		i = XZToIndex(slices_no - 2, z + 1);
		ci = i / 3 * 4;
		glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ci]);
		glNormal3fv(&m_normals[i]);
		glVertex3fv(&m_vertices[i]);
	}
	glEnd();

}

void XYZCanvas::DrawCursor() {
	if (m_cursor_x < 0)
		return;
	GLfloat col[] = { 1, 1, 1, .8};
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, col);
	size_t index = XZToIndex(m_cursor_x, m_cursor_z);
	glPushMatrix();
	glTranslatef(m_vertices[index], m_vertices[index + 1], m_vertices[index + 2]);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, cube_vertices);
	glNormalPointer(GL_FLOAT, 0, cube_normals);
	glDrawArrays(GL_QUADS, 0, 24);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glPopMatrix();
}

void XYZCanvas::DrawColoredGraph() {
	if (m_graph == NULL)
		return;

	if (m_update_arrays)
		UpdateArrays();
	
	for (size_t x = 0; x < slices_no - 2; x++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (size_t z = 0; z < slices_no - 1; z++) {
			size_t k = XZToIndex(x, z);
			size_t k1 = XZToIndex(x + 1, z);
			size_t ck = k / 3 * 4, ck1 = k / 3 * 4;
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ck]);
			glNormal3fv(&m_normals[k]);
			glVertex3fv(&m_vertices[k]);
			glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, &m_colors[ck1]);
			glNormal3fv(&m_normals[k1]);
			glVertex3fv(&m_vertices[k1]);
		}
		glEnd();
	}

}

void XYZCanvas::OnSize(wxSizeEvent &e) {
	Refresh();
}

void XYZCanvas::SetGraph(XYGraph *graph) {
	m_graph = graph;
	m_update_arrays = true;
	m_cursor_z = m_cursor_x = slices_no - 2;
	SetFocus();
	Refresh();
}

void XYZCanvas::OnChar(wxKeyEvent &event) {
	switch (event.GetKeyCode()) {
		case 'A':
			m_movement = LEFT_ROTATION;
			break;
		case 'a':
			m_movement = LEFT_MOVEMENT;
			break;
		case 'D':
			m_movement = RIGHT_ROTATION;
			break;
		case 'd':
			m_movement = RIGHT_MOVEMENT;
			break;
		case 'W':
			m_movement = UP_ROTATION;
			break;
		case 'w':
			m_movement = FORWARD_MOVEMENT;
			break;
		case 'e':
			m_movement = UP_MOVEMENT;
			break;
		case 'q':
			m_movement = DOWN_MOVEMENT;
			break;
		case 'S':
			m_movement = DOWN_ROTATION;
			break;
		case 's':
			m_movement = BACKWARD_MOVEMENT;
			break;
		case WXK_UP:
			m_movement = CURSOR_UP;
			break;
		case WXK_DOWN:
			m_movement = CURSOR_DOWN;
			break;
		case WXK_LEFT:
			m_movement = CURSOR_LEFT;
			break;
		case WXK_RIGHT:
			m_movement = CURSOR_RIGHT;
			break;
		default:
			break;
	}
}

void XYZCanvas::OnKeyUp(wxKeyEvent &event) {
	m_movement = NONE;
}

void XYZCanvas::OnTimer(wxTimerEvent&e) {
	switch (m_movement) {
		case UP_ROTATION:
			m_hrot += 1;
			break;
		case DOWN_ROTATION:
			m_hrot -= 1;
			break;
		case LEFT_ROTATION:
			m_vrot += 1;
			break;
		case RIGHT_ROTATION:
			m_vrot -= 1;
			break;
		case FORWARD_MOVEMENT:
			m_z_disp -= 1;
			break;
		case BACKWARD_MOVEMENT:
			m_z_disp += 1;
			break;
		case LEFT_MOVEMENT:
			m_x_disp -= 1;
			break;
		case RIGHT_MOVEMENT:
			m_x_disp += 1;
			break;
		case UP_MOVEMENT:
			m_y_disp += 1;
			break;
		case DOWN_MOVEMENT:
			m_y_disp -= 1;
			break;
		case CURSOR_LEFT:
			if (m_cursor_x == 0)
				return;
			m_cursor_x -= 1;
			break;
		case CURSOR_RIGHT:
			if (m_cursor_x == slices_no - 2)
				return;
			m_cursor_x += 1;
			break;
		case CURSOR_DOWN:
			if (m_cursor_z == slices_no - 2)
				return;
			m_cursor_z += 1;
			break;
		case CURSOR_UP:
			if (m_cursor_z == 0)
				return;
			m_cursor_z -= 1;
			break;
		case NONE:
			return;
	}
	Refresh();
}


BEGIN_EVENT_TABLE(XYZCanvas, wxGLCanvas)
	EVT_PAINT(XYZCanvas::OnPaint)
	EVT_SIZE(XYZCanvas::OnSize)
	EVT_TIMER(wxID_ANY, XYZCanvas::OnTimer)
    	EVT_KEY_UP(XYZCanvas::OnKeyUp)
    	EVT_CHAR(XYZCanvas::OnChar)
END_EVENT_TABLE()


XYZFrame::XYZFrame(wxString default_prefix, DatabaseManager *dbmanager, ConfigManager *cfgmanager, FrameManager *frame_manager) 
	: szFrame(NULL, wxID_ANY, _("XYZ Graph")), m_default_prefix(default_prefix), m_db_manager(dbmanager), m_cfg_manager(cfgmanager), m_frame_manager(frame_manager) {

	m_canvas = new XYZCanvas(this);

	wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	sizer->Add(m_canvas, 1, wxEXPAND | wxALL, 1);

	SetSizer(sizer);

	XYDialog* d = new XYDialog(this, 
			m_default_prefix,
			m_cfg_manager,
			m_db_manager,
			this);
	d->Show();

}

int XYZFrame::GetDimCount() {
	return 3;
}

void XYZFrame::SetGraph(XYGraph *graph) {
	m_canvas->SetGraph(graph);
	if (!IsShown())
		Show(true);
}

XYZFrame::~XYZFrame() {}



#endif
#endif
