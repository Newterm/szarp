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
	XYZFrame *m_parent;
	XYGraph *m_graph;
	wxGLContext *m_gl_context;
	GLUquadric* m_quad;	
	std::vector<float> m_vertices;
	enum POINT_TYPE { IN_TRIANGLE, SINGLE};
	std::vector<POINT_TYPE> m_point_types;
	std::vector<float> m_triangles;
	std::vector<unsigned char> m_triangles_colors;
	std::vector<size_t> m_triangles_indexes;
	std::vector<float> m_single_quads;
	std::vector<unsigned char> m_single_quads_colors;
	std::vector<size_t> m_single_quads_indexes;
	std::vector<float> m_normals;
	std::vector<unsigned char> m_colors;
	wxTimer *m_timer;
	FTFont *m_font;

	float m_camera_x, m_camera_y, m_camera_z;
	float m_camera_angle_x, m_camera_angle_y;

	bool m_right_down;

	float m_start_angle_x, m_start_angle_y;
	float m_graph_angle_x, m_graph_angle_y;

	int m_start_mouse_x;
	int m_start_mouse_y;

	static const size_t slices_no = 100; 
	
	void DrawFrameOfReference();
	void DrawAxis(const wxString& short_name);
	void DrawColoredGraph();
	void DrawLineGraph();
	void UpdateArrays();
	void DrawPointInfo();
	void MoveCameraX(float disp);
	void MoveCameraY(float disp);
	void MoveCameraZ(float disp);
	void SetProjectionMatrix(const wxSize &size);
	void Camera();
	size_t XZToIndex(size_t x, size_t z);

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
	XYZCanvas(XYZFrame *parent);	
	~XYZCanvas();	
	void MoveCameraForward();
	void OnPaint(wxPaintEvent & event);
	void OnSize(wxSizeEvent& event);
	void SetGraph(XYGraph *graph);
	void OnTimer(wxTimerEvent &e);
	void OnChar(wxKeyEvent &event);
	void OnMouseLeftDown(wxMouseEvent &event);
	void OnMouseWheel(wxMouseEvent &event);
    	void OnMouseRightDown(wxMouseEvent &event);
    	void OnMouseRightUp(wxMouseEvent &event);
	void OnMouseMotion(wxMouseEvent &event);
	void OnMouseLeaveWindow(wxMouseEvent &event);
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


XYZCanvas::XYZCanvas(XYZFrame *parent) : wxGLCanvas(parent, wxID_ANY,
#ifdef __WXGTK__
		wxGetApp().GLContextAttribs(),
#else
		NULL,
#endif
		wxDefaultPosition, wxSize(300, 200), 0, _T("XYZCanvas")), m_parent(parent), m_graph(NULL), m_gl_context(NULL) {
	m_vertices.resize(3 * slices_no * slices_no);
	m_point_types.resize(3 * slices_no * slices_no, SINGLE);
	m_normals.resize(3 * slices_no * slices_no, 0);
	m_colors.resize(4 * slices_no * slices_no);
	m_movement = NONE;
	m_camera_angle_x = 0;
	m_camera_angle_y = 0;
	m_camera_x = 0;
	m_camera_y = 0;
	m_camera_z = slices_no * 2;
	m_graph_angle_x = 30;
	m_graph_angle_y = -30;
	m_timer = new wxTimer(this, wxID_ANY);
	m_timer->Start(100);
	m_graph_type = COLORED;
	m_cursor_x = -1;
	m_cursor_z = -1;
	m_font = NULL;
	m_right_down = false;
}

void XYZCanvas::DrawFrameOfReference() {
	if (m_graph == NULL)
		return;

	float color[4] = {0, 0, 0, 1};

	wxColour c = m_graph->m_di[2]->GetDrawColor();
	color[0] = float(c.Red()) / 255;
	color[1] = float(c.Green()) / 255;
	color[2] = float(c.Blue()) / 255;
	glColor4fv(color); 
	DrawAxis(m_graph->m_di[2]->GetShortName());

	glPushMatrix();
	glRotatef(90, 0, 1, 0);

	c = m_graph->m_di[0]->GetDrawColor();
	color[0] = float(c.Red()) / 255;
	color[1] = float(c.Green()) / 255;
	color[2] = float(c.Blue()) / 255;
	glColor4fv(color); 
	DrawAxis(m_graph->m_di[0]->GetShortName());

	c = m_graph->m_di[1]->GetDrawColor();
	color[0] = float(c.Red()) / 255;
	color[1] = float(c.Green()) / 255;
	color[2] = float(c.Blue()) / 255;
	glColor4fv(color); 
	glRotatef(-90, 1, 0, 0);
	DrawAxis(m_graph->m_di[1]->GetShortName());

	glPopMatrix();
}

void XYZCanvas::DrawAxis(const wxString& short_name) {
	//gluCylinder(m_quad, 0.1, 0.1, 1.2 * slices_no, 30, 1);
	gluCylinder(m_quad, 0.5, 0.5, 1.2 * slices_no, 30, 1);
	glPushMatrix();
	glTranslatef(0, 0, 1.2 * slices_no);
	gluCylinder(m_quad, 1.5, 0.0, 5, 30, 10);
	glTranslatef(0, 0, 10);

	double m[16];
	glGetDoublev(GL_TRANSPOSE_MODELVIEW_MATRIX, m);
	m[0] = 1; m[1] = 0; m[2] = 0;
	m[4] = 0; m[5] = 1; m[6] = 0;
	m[8] = 0; m[9] = 0; m[10] = 1;
	m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
	double x = m[3];
	double y = m[7];
	double z = m[11];
	glLoadTransposeMatrixd(m);
	glRotatef(180 / M_PI * atan(x / -z), 0, -1, 0);
	glRotatef(180 / M_PI * atan(y / -z), 1, 0, 0);
	glTranslatef(-2.5, 0.0, 0.0);

	FTBBox bbox = m_font->BBox(short_name.c_str());
	float w = bbox.Upper().Xf() - bbox.Lower().Xf();
	glScalef(5 / w, 5 / w, 1);
	//fprintf(stdout, "width: %f\n", bbox.Upper().Xf() - bbox.Lower().Xf());
	m_font->Render(short_name.c_str());
	glPopMatrix();
}

void XYZCanvas::MoveCameraX(float disp) {
	float dx = disp * cos(m_camera_angle_x / 180 * M_PI) * cos(m_camera_angle_y / 180 * M_PI);
	float dy = disp * sin(m_camera_angle_x / 180 * M_PI);
	float dz = -disp * cos(m_camera_angle_x / 180 * M_PI) * sin(m_camera_angle_y / 180 * M_PI);

	//std::cout << "(" << m_camera_angle_x << "," << m_camera_angle_y << ") (" << dx << "," << dy << "," << dz << ")" << std::endl;

	m_camera_x += dx;
	m_camera_y += dy;
	m_camera_z += dz;
}

void XYZCanvas::MoveCameraY(float disp) {
	float dx = disp * sin(m_camera_angle_x / 180 * M_PI) * sin(m_camera_angle_y / 180 * M_PI);
	float dy = disp * cos(m_camera_angle_x / 180 * M_PI);
	float dz = -disp * sin(m_camera_angle_x / 180 * M_PI) * cos(m_camera_angle_y / 180 * M_PI);

	//std::cout << "(" << m_camera_angle_x << "," << m_camera_angle_y << ") (" << dx << "," << dy << "," << dz << ")" << std::endl;

	m_camera_x += dx;
	m_camera_y += dy;
	m_camera_z += dz;
}

void XYZCanvas::MoveCameraZ(float disp) {
	float dx = disp * cos(m_camera_angle_x / 180 * M_PI) * sin(m_camera_angle_y / 180 * M_PI);
	float dy = disp * sin(m_camera_angle_x / 180 * M_PI);
	float dz = -disp * cos(m_camera_angle_x / 180 * M_PI) * cos(m_camera_angle_y / 180 * M_PI);

	//std::cout << "(" << m_camera_angle_x << "," << m_camera_angle_y << ") (" << dx << "," << dy << "," << dz << ")" << std::endl;

	m_camera_x += dx;
	m_camera_y += dy;
	m_camera_z += dz;
}

void XYZCanvas::Camera() {
	glRotatef(m_camera_angle_y, 0, 1, 0);
	glRotatef(m_camera_angle_x, -1, 0, 0);

	glTranslatef(-m_camera_x, -m_camera_y, -m_camera_z);

	glRotatef(m_graph_angle_x, 1, 0, 0);
	glRotatef(m_graph_angle_y, 0, 1, 0);

}

void XYZCanvas::SetProjectionMatrix(const wxSize &size) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, size.GetWidth(), size.GetHeight());
	glFrustum(-0.5 * size.GetWidth() / size.GetHeight(), 0.5 * size.GetWidth() / size.GetHeight(), -0.5, 0.5, 1, 500);
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

	SetProjectionMatrix(size);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DITHER);
	glShadeModel(GL_SMOOTH);
	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Camera();

	float light_position[] = { slices_no / 2, slices_no * 3, slices_no / 2, 0};
	float light_direction[] = { 0, -1, 0 };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
	glEnable(GL_LIGHT0);
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

	glDisable(GL_LIGHTING);

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
		glColor4fv(gc);
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
		if (counts[si]) {
			v = sums[si] / counts[si];
			m_vertices[vi + 1] = (v - ymin) / (ymax - ymin) * slices_no;
			m_colors[si * 4] = (v - ymin) / (ymax - ymin) * 255;
			m_colors[si * 4 + 1] = 0;
			m_colors[si * 4 + 2] = 255 - m_colors[si * 4];
			m_colors[si * 4 + 3] = 200;
		} else {
			m_vertices[vi + 1] = nan("");
		}
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

#if 0
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
			
			if (up) {
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
		m_normals[idx + 0] /= vs;
		m_normals[idx + 1] /= vs;
		m_normals[idx + 2] /= vs;

	}
#endif
	m_triangles.resize(0);
	m_triangles_colors.resize(0);
	m_triangles_indexes.resize(0);
	m_single_quads.resize(0);
	m_single_quads_colors.resize(0);
	m_single_quads_indexes.resize(0);
	m_point_types.resize(slices_no * slices_no, SINGLE);

	for (size_t x = 0; x < slices_no - 1; x++) for (size_t z = 1; z < slices_no - 1; z++)  {
		size_t i = XZToIndex(x, z);
		if (isnan(m_vertices[i + 1]))
			continue;

		size_t neighbours[] = {
				XZToIndex(x, z + 1),
				XZToIndex(x + 1, z + 1),
				XZToIndex(x + 1, z + 1),
				XZToIndex(x + 1, z),
				XZToIndex(x + 1, z),
				XZToIndex(x + 1, z - 1),
				XZToIndex(x + 1, z - 1),
				XZToIndex(x, z - 1),
				XZToIndex(x, z + 1),
				XZToIndex(x + 1, z),
				XZToIndex(x + 1, z),
				XZToIndex(x, z - 1),
			};

		for (size_t j = 0; j < sizeof(neighbours) / sizeof(neighbours[0]) / 2; j += 2) {
			size_t ji = neighbours[j], ki = neighbours[j + 1];
			if (!isnan(m_vertices[ji + 1]) && !isnan(m_vertices[ki + 1])) {
				m_point_types[i / 3] = m_point_types[ji / 3] = m_point_types[ki / 3] = IN_TRIANGLE;

				m_triangles_indexes.push_back(i);
				m_triangles_indexes.push_back(ji);
				m_triangles_indexes.push_back(ki);

				m_triangles.push_back(m_vertices[i]);
				m_triangles.push_back(m_vertices[i + 1]);
				m_triangles.push_back(m_vertices[i + 2]);
				
				m_triangles.push_back(m_vertices[ji]);
				m_triangles.push_back(m_vertices[ji + 1]);
				m_triangles.push_back(m_vertices[ji + 2]);
				
				m_triangles.push_back(m_vertices[ki]);
				m_triangles.push_back(m_vertices[ki + 1]);
				m_triangles.push_back(m_vertices[ki + 2]);
					
				m_triangles_colors.push_back(m_colors[i / 3 * 4]);
				m_triangles_colors.push_back(m_colors[i / 3 * 4 + 1]);
				m_triangles_colors.push_back(m_colors[i / 3 * 4 + 2]);
				m_triangles_colors.push_back(m_colors[i / 3 * 4 + 4]);

				m_triangles_colors.push_back(m_colors[ji / 3 * 4]);
				m_triangles_colors.push_back(m_colors[ji / 3 * 4 + 1]);
				m_triangles_colors.push_back(m_colors[ji / 3 * 4 + 2]);
				m_triangles_colors.push_back(m_colors[ji / 3 * 4 + 4]);

				m_triangles_colors.push_back(m_colors[ki / 3 * 4]);
				m_triangles_colors.push_back(m_colors[ki / 3 * 4 + 1]);
				m_triangles_colors.push_back(m_colors[ki / 3 * 4 + 2]);
				m_triangles_colors.push_back(m_colors[ki / 3 * 4 + 3]);
			}
		}

		if (m_point_types[i / 3] == SINGLE) {
			m_single_quads_indexes.push_back(i);
			for (size_t m = 0; m < sizeof(cube_vertices) / sizeof(float) / 3; ++m)  {

				m_single_quads.push_back(m_vertices[i] + 0.8 * cube_vertices[m * 3]);
				m_single_quads.push_back(m_vertices[i + 1] + 0.8 * cube_vertices[m * 3 + 1]);
				m_single_quads.push_back(m_vertices[i + 2] + 0.8 * cube_vertices[m * 3 + 2]);

				m_single_quads_colors.push_back(m_colors[i / 3 * 4]);
				m_single_quads_colors.push_back(m_colors[i / 3 * 4 + 1]);
				m_single_quads_colors.push_back(m_colors[i / 3 * 4 + 2]);
				m_single_quads_colors.push_back(m_colors[i / 3 * 4 + 3]);
			}
		}
	}

}

void XYZCanvas::DrawLineGraph() {

	if (m_graph == NULL)
		return;

	size_t i, ci;
	
	glBegin(GL_LINES);
	for (size_t x = 0; x < slices_no - 2; x++) {
		i = XZToIndex(x, 0);
		ci = i / 3 * 4;
		glColor4ubv(&m_colors[ci]);
		glNormal3fv(&m_normals[i]);
		glVertex3fv(&m_vertices[i]);

		i = XZToIndex(x + 1, 0);
		ci = i / 3 * 4;
		glColor4ubv(&m_colors[ci]);
		glNormal3fv(&m_normals[i]);
		glVertex3fv(&m_vertices[i]);

		for (size_t z = 0; z < slices_no - 2; z++) {
			i = XZToIndex(x, z);
			ci = i / 3 * 4;
			glColor4ubv(&m_colors[ci]);
			glNormal3fv(&m_normals[i]);
			glVertex3fv(&m_vertices[i]);

			i = XZToIndex(x, z + 1);
			ci = i / 3 * 4;
			glColor4ubv(&m_colors[ci]);
			glNormal3fv(&m_normals[i]);
			glVertex3fv(&m_vertices[i]);

			i = XZToIndex(x, z + 1);
			ci = i / 3 * 4;
			glColor4ubv(&m_colors[ci]);
			glNormal3fv(&m_normals[i]);
			glVertex3fv(&m_vertices[i]);

			i = XZToIndex(x + 1, z + 1);
			ci = i / 3 * 4;
			glColor4ubv(&m_colors[ci]);
			glNormal3fv(&m_normals[i]);
			glVertex3fv(&m_vertices[i]);

		}
	}
	for (size_t z = 0; z < slices_no - 2; z++) {
		i = XZToIndex(slices_no - 2, z);
		ci = i / 3 * 4;
		glColor4ubv(&m_colors[ci]);
		glNormal3fv(&m_normals[i]);
		glVertex3fv(&m_vertices[i]);

		i = XZToIndex(slices_no - 2, z + 1);
		ci = i / 3 * 4;
		glColor4ubv(&m_colors[ci]);
		glNormal3fv(&m_normals[i]);
		glVertex3fv(&m_vertices[i]);
	}
	glEnd();

}

void XYZCanvas::DrawCursor() {
	if (m_cursor_x < 0)
		return;
	GLfloat col[] = { 1, 1, 1, .8};
	glColor4fv(col);
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

	float normal[] = {0, 1, 0};
	glNormal3fv(normal);

	if (m_triangles.size()) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, &m_triangles[0]);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, &m_triangles_colors[0]);
		glDrawArrays(GL_TRIANGLES, 0, m_triangles.size() / 3);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

	if (m_single_quads.size()) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, &m_single_quads[0]);
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, &m_single_quads_colors[0]);
		glDrawArrays(GL_QUADS, 0, m_single_quads.size() / 3);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}

#if 0
	bool drawing_triangle = false;

	for (size_t x = 0; x < slices_no - 2; x++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (size_t z = 0; z < slices_no - 1; z++) {
			size_t k = XZToIndex(x, z);
			size_t k1 = XZToIndex(x + 1, z);
			size_t ck = k / 3 * 4, ck1 = k / 3 * 4;
			glColor4ubv(&m_colors[ck]);
			glNormal3fv(&m_normals[k]);
			glVertex3fv(&m_vertices[k]);
			glColor4ubv(&m_colors[ck1]);
			glNormal3fv(&m_normals[k1]);
			glVertex3fv(&m_vertices[k1]);
		}
		glEnd();
	}
#endif

}

void XYZCanvas::OnSize(wxSizeEvent &e) {
	Refresh();
}

void XYZCanvas::SetGraph(XYGraph *graph) {
	int x, z;
	m_graph = graph;
	UpdateArrays();
	for (x = slices_no - 2; x >= 0; x--)
		for (z = slices_no - 2; z >= 0; z--) 
			if (!isnan(m_vertices[XZToIndex(x, z) + 1])) {
				m_cursor_z = z;
				m_cursor_x = x;
				goto out;
			}
out:
	if (x < 0) {
		wxMessageBox(_("No common data found"), _("There is no common data for these params for given time period"), wxICON_HAND);
		delete m_graph;
		m_graph = NULL;
		m_parent->GetNewGraph();
		return;
	}
		
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
		case 's':
			m_movement = BACKWARD_MOVEMENT;
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
		case 'w':
			m_movement = FORWARD_MOVEMENT;
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
	int nx, nz;

	switch (m_movement) {
		case UP_ROTATION:
			m_camera_angle_x += 1;
			break;
		case DOWN_ROTATION:
			m_camera_angle_x -= 1;
			break;
		case LEFT_ROTATION:
			m_camera_angle_y -= 1;
			break;
		case RIGHT_ROTATION:
			m_camera_angle_y += 1;
			break;
		case FORWARD_MOVEMENT:
			MoveCameraZ(1);
			break;
		case BACKWARD_MOVEMENT:
			MoveCameraZ(-1);
			break;
		case LEFT_MOVEMENT:
			MoveCameraX(-1);
			break;
		case RIGHT_MOVEMENT:
			MoveCameraX(1);
			break;
		case UP_MOVEMENT:
			MoveCameraY(1);
			break;
		case DOWN_MOVEMENT:
			MoveCameraY(-1);
			break;
		case CURSOR_LEFT:
			if (m_cursor_x == 0)
				return;
			nx = m_cursor_x - 1;
			while (nx >= 0 && isnan(m_vertices[XZToIndex(nx, m_cursor_z) + 1]))
				nx -= 1;
			if (nx < 0)
				return;
			m_cursor_x = nx;
			break;
		case CURSOR_RIGHT:
			if (m_cursor_x == slices_no - 2)
				return;
			nx = m_cursor_x + 1;
			while (nx < (int)slices_no - 1 && isnan(m_vertices[XZToIndex(nx, m_cursor_z) + 1]))
				nx += 1;
			if (nx == slices_no - 1)
				return;
			m_cursor_x = nx;
			break;
		case CURSOR_DOWN:
			if (m_cursor_z == slices_no - 2)
				return;
			nz = m_cursor_z + 1;
			while (nz < (int)slices_no - 1 && isnan(m_vertices[XZToIndex(m_cursor_x, nz) + 1]))
				nz += 1;
			if (nz == slices_no - 1)
				return;
			m_cursor_z = nz;
			break;
		case CURSOR_UP:
			if (m_cursor_z == 0)
				return;
			nz = m_cursor_z - 1;
			while (nz >= 0 && isnan(m_vertices[XZToIndex(m_cursor_x, nz) + 1]))
				nz -= 1;
			if (nz < 0)
				return;
			m_cursor_z = nz;
			break;
		case NONE:
			return;
	}
	Refresh();
}

void XYZCanvas::OnMouseLeftDown(wxMouseEvent &event) {
	if (m_graph == NULL)
		return;

	wxSize size = GetSize();

	m_gl_context->SetCurrent(*this);

	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);

	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_FLAT);
	glDisable(GL_BLEND);
	glDisable(GL_DITHER);
	glClearColor(0, 0, 0, 0);
	glEnable(GL_DEPTH_TEST);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_COLOR_MATERIAL);

	SetProjectionMatrix(size);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Camera();

	unsigned char color[4] = { 0, 0, 0, 255 };
	glBegin(GL_TRIANGLES);
	for (size_t i = 0; i < m_triangles_indexes.size(); i += 3) {
		color[0] = (i / 3) % 256;
		color[1] = i / 3 / 256; 
		color[2] = i / 3 / 256 / 256 * 2 + 1;
		glColor4ubv(color);
		glVertex3fv(&m_vertices[m_triangles_indexes[i]]);
		glVertex3fv(&m_vertices[m_triangles_indexes[i + 1]]);
		glVertex3fv(&m_vertices[m_triangles_indexes[i + 2]]);
	}
	glEnd();

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, &m_single_quads[0]);
	for (size_t i = 0; i < m_single_quads_indexes.size(); i++) {
		color[0] = m_single_quads_indexes[i] % 256;
		color[1] = m_single_quads_indexes[i] / 256;
		color[2] = m_single_quads_indexes[i] / 256 / 256 * 2;
		glColor4ubv(color);
		glDrawArrays(GL_QUADS, i * 24, 24);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glFinish();

#if 0
	wxImage img(size.GetWidth(), size.GetHeight());
	unsigned char* inversed = (unsigned char*) malloc(size.GetWidth() * size.GetHeight() * 3);
	unsigned char* regular = (unsigned char*) malloc(size.GetWidth() * size.GetHeight() * 3);
	glReadPixels(0, 0, size.GetWidth(), size.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, inversed);
	for (int i = 0; i < size.GetHeight(); i++)
		for (int j = 0; j < size.GetWidth(); j++) {
			regular[size.GetWidth() * i * 3 + j * 3] = inversed[size.GetWidth() * (size.GetHeight() - i) * 3 + j * 3];
			regular[size.GetWidth() * i * 3 + j * 3 + 1] = inversed[size.GetWidth() * (size.GetHeight() - i) * 3 + j * 3 + 1];
			regular[size.GetWidth() * i * 3 + j * 3 + 2] = inversed[size.GetWidth() * (size.GetHeight() - i) * 3 + j * 3 + 2];
		}
	img.SetData(regular);
	img.SaveFile(_T("a.bmp"));
	free(inversed);
#endif


	unsigned char points_colors[25 * 3 * 3];
	unsigned char point_color[3];
	size_t cx = 2;
	size_t cy = 2;
	glReadPixels(event.GetX() - 2, size.GetHeight() - event.GetY() - 2, 5, 5, GL_RGB, GL_UNSIGNED_BYTE, points_colors);
	//middle point
	point_color[0] = points_colors[3 * 5 * cy + 3 * cx];
	point_color[1] = points_colors[3 * 5 * cy + 3 * cx + 1];
	point_color[2] = points_colors[3 * 5 * cy + 3 * cx + 2];
	cx = 0;
	cy = 0;
	while (point_color[0] == 0 && point_color[1] == 0 && point_color[2] == 0) {
		point_color[0] = points_colors[3 * 5 * cy + 3 * cx];
		point_color[1] = points_colors[3 * 5 * cy + 3 * cx + 1];
		point_color[2] = points_colors[3 * 5 * cy + 3 * cx + 2];
		if (++cx == 5) {
			if (++cy == 5)
				return;
			cx = 0;
		}
	}
	int index = 0;
	if (point_color[2] & 1) {
		size_t i = 3 * (point_color[0] + size_t(point_color[1]) * 256 + size_t(point_color[2] / 2 * 256 * 256));

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glShadeModel(GL_SMOOTH);
		glBegin(GL_TRIANGLES);
			color[0] = 255; color[1] = 0; color[2] = 0;
			glColor4ubv(color);
			glVertex3fv(&m_vertices[m_triangles_indexes[i]]);
			color[0] = 0; color[1] = 255; color[2] = 0;
			glColor4ubv(color);
			glVertex3fv(&m_vertices[m_triangles_indexes[i + 1]]);
			color[0] = 0; color[1] = 0; color[2] = 255;
			glColor4ubv(color);
			glVertex3fv(&m_vertices[m_triangles_indexes[i + 2]]);
		glEnd();
		glFinish();

		unsigned char tv[3];
		glReadPixels(event.GetX(), size.GetHeight() - event.GetY(), 1, 1, GL_RGB, GL_UNSIGNED_BYTE, tv);
		if (tv[1] > tv[0])
			if (tv[2] > tv[1])
				index = m_triangles_indexes[i + 2];
			else
				index = m_triangles_indexes[i + 1];
		else
			if (tv[2] > tv[0])
				index = m_triangles_indexes[i + 2];
			else
				index = m_triangles_indexes[i];

	} else 
		index = point_color[0] + 256 * size_t(point_color[1]) + 256 * 256 * size_t(point_color[2] / 2);

	index /= 3;

	m_cursor_z = index / slices_no;
	m_cursor_x = index % slices_no;
	Refresh();
}

void XYZCanvas::OnMouseWheel(wxMouseEvent &event) {
	MoveCameraZ(event.m_wheelRotation / event.m_wheelDelta);
	Refresh();
}

void XYZCanvas::OnMouseRightDown(wxMouseEvent &event) {
	m_right_down = true;
	m_start_angle_x = m_graph_angle_x;
	m_start_angle_y = m_graph_angle_y;

	m_start_mouse_x = event.GetX();
	m_start_mouse_y = event.GetY();
}

void XYZCanvas::OnMouseRightUp(wxMouseEvent &event) {
	m_right_down = false;
}

void XYZCanvas::OnMouseMotion(wxMouseEvent &event) {
	if (m_right_down == false)
		return;

	int x = event.GetX();
	int y = event.GetY();

	int dx = x - m_start_mouse_x;
	int dy = y - m_start_mouse_y;

	wxSize size = GetSize();

	m_graph_angle_y = m_start_angle_y + 360 * dx / (size.GetWidth() / size.GetHeight()) / size.GetWidth();
	m_graph_angle_x = m_start_angle_x + 360 * dy / size.GetHeight();

	//std::cout << "(" << m_start_angle_x << " " << m_start_angle_y << ") -> (" << m_camera_angle_x << " " << m_camera_angle_y << ")" << std::endl;

	Refresh();
}

void XYZCanvas::OnMouseLeaveWindow(wxMouseEvent &event) {
	m_right_down = false;
#if 0
	if (m_right_down) {	
		m_graph_angle_x = m_start_angle_x;
		m_graph_angle_y = m_start_angle_y;
		Refresh();
	}
#endif
}

XYZCanvas::~XYZCanvas() {
	delete m_timer;
}


BEGIN_EVENT_TABLE(XYZCanvas, wxGLCanvas)
	EVT_PAINT(XYZCanvas::OnPaint)
	EVT_SIZE(XYZCanvas::OnSize)
	EVT_TIMER(wxID_ANY, XYZCanvas::OnTimer)
    	EVT_KEY_UP(XYZCanvas::OnKeyUp)
    	EVT_CHAR(XYZCanvas::OnChar)
    	EVT_LEFT_DOWN(XYZCanvas::OnMouseLeftDown)
    	EVT_RIGHT_DOWN(XYZCanvas::OnMouseRightDown)
    	EVT_RIGHT_UP(XYZCanvas::OnMouseRightUp)
	EVT_MOTION(XYZCanvas::OnMouseMotion)
	EVT_LEAVE_WINDOW(XYZCanvas::OnMouseLeaveWindow)
	EVT_MOUSEWHEEL(XYZCanvas::OnMouseWheel)
END_EVENT_TABLE()


XYZFrame::XYZFrame(wxString default_prefix, DatabaseManager *dbmanager, ConfigManager *cfgmanager, FrameManager *frame_manager) 
	: szFrame(NULL, wxID_ANY, _("XYZ Graph")), m_default_prefix(default_prefix), m_db_manager(dbmanager), m_cfg_manager(cfgmanager), m_frame_manager(frame_manager) {

	m_canvas = new XYZCanvas(this);

	wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	sizer->Add(m_canvas, 1, wxEXPAND | wxALL, 1);

	SetSizer(sizer);

	m_xydialog = new XYDialog(this, 
			m_default_prefix,
			m_cfg_manager,
			m_db_manager,
			this);
	m_xydialog->Show();
}

int XYZFrame::GetDimCount() {
	return 3;
}

void XYZFrame::SetGraph(XYGraph *graph) {
	m_canvas->SetGraph(graph);
	if (!IsShown())
		Show(true);
}

void XYZFrame::GetNewGraph() {
	m_xydialog->Show();
}

XYZFrame::~XYZFrame() {}

#endif
#endif
