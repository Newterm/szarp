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
#ifndef __DRAWVIEW_H__
#define __DRAWVIEW_H__
/*
 * draw3 program
 * SZARP
 
 
 * $Id$
 */

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifdef __WXMSW__
#include <windows.h>
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#endif

/**I hope this all non cross-platform stuff will be removed when 
 * next version of wxWidgets is relased so I won't bother to document it.*/
class SDC {
#ifdef __WXGTK__
	GdkGC *m_gc;
	GdkBitmap *m_gdkbmp;
#else
	HDC m_gc;
	HBITMAP m_mswbmp;
	HBITMAP m_defbmp;
	wxPen m_pen;
	bool m_selected;
	void Select();
#endif
	public:
	SDC();
#ifdef __WXGTK__
	void SetObjects(GdkGC *gc, GdkBitmap* bmp);
#else
	void SetObjects(HDC gc, HBITMAP bmp);
#endif
	void SetLineWidth(int width);
	void SetWhiteForeground();
	void SetBlackForeground();
	void DrawPoint(int x, int y);
	void DrawCircle(int x, int y, int r);
	void DrawLine(int x1, int y1, int x2, int y2);
	void DrawRectangle(int x, int y, int w, int h);
	void DrawRectangle(const wxRect &r);
	void Clear();
#ifdef __WXMSW__
	void Deselect();
#endif

};


/*Abstract class representing object being some kind of Draw view,
 * that can be blitted onto @see WxGraphs.  Implements @see DrawObserver 
 * thus is notfied about any changes in observed @see Draw.*/
class View {
protected:

	/** @see Draw object*/
	Draw *m_draw;

	/** Return poition of i'th probe from current @see draw object
	 * @param dc device context we draw with
	 * @param i index of probe's position to find coorrdinates for*/
	void GetPointPosition(wxDC* dc, int i, int *x, int *y) const;

	/**Returns size of the DC region we draw with*/
	virtual void GetSize(int *w, int *h) const = 0;

public:
	View() : m_draw(NULL) {}

	/**@return wxDC on which view is drawn*/
	virtual wxDC* GetDC() = 0;

	/**Asocciates bitmap with dc and clear the bitmap, sets Null pen and brush
	 * @param dc dc to associate with bitmap
	 * @param bitmap bitmap to asscociate with dc :)
	 * @param w witdth of bitmap
	 * @param h height of bitmap*/
	void ClearDC(wxMemoryDC *dc, wxBitmap *bitmap, int w, int h);

        /** Get X coordinate coresponding to current date and with respect
         * to period time set. 
         * @param date selected date (GMT)
         * @return absolute X coordinate within View, can be negative or
         * greater then widget width if date is outside period shown.
         */
	long int GetX(int i) const;

        /** Get Y coordinate coresponding to given value.
         * @param value value for which Y coordinate is caclulaed 
         * @return absolute Y coordinate within View
         */
	long int GetY(double value) const;

	/**Sets size of the view
	 * @param w width of view
	 * @param h height of view*/
	virtual void SetSize(int w, int h) = 0;

        /** This table describes shifts for time marks on time axis. */
	static const int PeriodMarkShift[PERIOD_T_LAST];

	virtual ~View() {}

protected:
        /** Left margin width in pixels (for printing vertical axis' values). */
	int m_leftmargin;

        /** Left margin width in pixels (for printing vertical axis' values). */
	int m_rightmargin;

	/**< Top margin width in pixels. */
	int m_topmargin;

	/**< Bottom margin width in pixels. */
	int m_bottommargin;

};

class BackgroundDrawer : public View {
protected:
	/** Draws background 'stripes'*/
	void DrawBackground(wxDC* dc);

	/** Draws time axis
	 * @param arrow_width width of the arrow
	 * @param arrow_height 'heigt' of arrow
	 * @param tick_height height of ticks on the axis
	 * @param line_width width of line*/
	void DrawTimeAxis(wxDC *dc, int arrow_width, int arrow_height, int tick_height, int line_width = 1);


	/** Draws vertical axis. Meaning of values if analogical to these of @see DrawTimeAxis*/
	void DrawYAxis(wxDC *dc, int arrow_width, int arrow_height, int line_width = 1);

	void DrawYAxisVals(wxDC *dc, int tick_len, int line_width = 1);
	 
	/** @return colour of time axis (and all other axes as a matter of fact...*/
	virtual const wxColour& GetTimeAxisCol() = 0;
	
	/** @return colour of 'even' stripes*/
	virtual const wxColour& GetBackCol1() = 0;

	/** @return colour of 'odd' stripes*/
	virtual const wxColour& GetBackCol2() = 0;

	/** @return size of region of DC we draw*/
	virtual void GetSize(int *w, int *h) const = 0;

	/** draws unit of current param*/
	void DrawUnit(wxDC *dc, int x, int y);

	/** @return font that is used for text writing*/
	virtual wxFont GetFont() const = 0;
public:
	BackgroundDrawer();
	virtual ~BackgroundDrawer() {}

};


/**Draws background of current @see Draw*/
class BackgroundView : public BackgroundDrawer {
	/**@see WxGraphs*/
	WxGraphs* m_graphs;

	/**dc used to draw @see BackgroundView*/
	wxMemoryDC *m_dc;
	/**Bitmap where the result of drawing is stored*/
	wxBitmap *m_bmp;

	/**@return font for text drawing*/
	virtual wxFont GetFont() const;

        static const wxColour timeaxis_col;  /**< Color of time axis. */

        static const wxColour back1_col, back2_col;
                                /**< Colors for background stripes. */

	/**colour for axes printing*/
	const wxColour& GetTimeAxisCol();

	/**colour of 'odd' stripes*/
	const wxColour& GetBackCol1();

	/**colour of 'even' stripes*/
	const wxColour& GetBackCol2();

	/**return size of region of DC we use for drawing*/
	virtual void GetSize(int *w, int *h) const;

	/**@see ConfigManager*/
	ConfigManager *m_cfg_mgr;

	/**remark flag bitmap*/
	wxBitmap m_remark_flag_bitmap;

	/**daws flags at point where remarks are located*/
	void DrawRemarksFlags(wxDC *dc);
public:
	BackgroundView(WxGraphs *widget, ConfigManager *cfg_mgr);

	/**Redraws view*/
	void Attach(Draw *draw);

	/**Clears background*/
	void Detach(Draw *draw);

	/**Redraws view*/
	void DrawInfoChanged(Draw *draw);

	/**Redraws view*/
	void NewRemarks(Draw *draw);

	/**Redraws view*/
	void PeriodChanged(Draw *draw, PeriodType period);

	/**Redraws view*/
	void ScreenMoved(Draw* draw, const wxDateTime &start_date);

	/**Resizes view*/
	virtual void SetSize(int w, int h);

	/**@return DC onto which View is drawn*/
	virtual wxDC* GetDC();

	/**draws winter summer season limits*/
	void DrawSeasonLimits(wxDC *dc);

	/**daws w vertical line denoting seasons change*/
	void DrawSeasonLimitInfo(wxDC *dc, int x, int month, int day, bool summer);

	/**Redraws whole view*/
	void DoDraw(wxDC *dc);

	void SetMargins(int left, int right, int top, int bottom) { m_leftmargin = left, m_rightmargin = right, m_topmargin = top, m_bottommargin = bottom; }

	int GetRemarkClickedIndex(int x, int y);

	virtual ~BackgroundView();

};

class GraphDrawer : public View {
	int m_circle_radius;
protected:
	/**return size of region of DC we use for drawing*/
	virtual void GetSize(int *w, int *h) const = 0;

	/**draw all points of graph
	 * @param dc device context to draw with
	 * @param sdc supplementary context to draw with
	 * @param point_width width of point in pixels*/
	void DrawAllPoints(wxDC *dc, SDC *sdc, int point_width = 1);

	/**@return true if for point at given index alternate colour shall be used*/
	virtual bool AlternateColor(int idx) = 0;
        static const wxColour alt_color;
                                /**< Alternate graph color 
				 * (used when drawing points and lines in double cursor mode*/

public:	
	GraphDrawer(int circle_radius);
};


/**Class resposible for rendering graphs onto screen*/
class GraphView : public GraphDrawer {
	/**@see WxGraphs*/
	WxGraphs* m_graphs;
protected:
	/**dc used to draw @see BackgroundView*/
	wxMemoryDC *m_dc;
	/**Bitmap where the result of drawing is stored*/
	wxBitmap *m_bmp;
	/**supplemental device context*/
	SDC m_sdc;

#ifdef __WXGTK__
	/**platform specific drawing context used for drawing onto wxMask*/
	GdkGC *m_gc;

	/**platform specific extracted from wxMask*/
	GdkBitmap *m_gdkbmp;
#else
	/**platform specific drawing context used for drawing onto wxMask*/
	HDC m_gc;

	/**platform specific extracted from wxMask*/
	HBITMAP m_mswbmp;
#endif

	/**Draws point represeting probe at give index if observer @see Draw.
	 * If adjacent probes are present line connecting them with this point is drawn
	 * @param index index of porbe in Draw vales table
	 * @param region if not NULL part of the of @see m_dc affected by this operation is 
	 * stored(union operation with it's current content) in this region
	 * @param maybe_repaint_cursor if set to true and as result of this draw cursor 
	 * is damaged causes repaint of cursor*/
	void DrawPoint(int index, wxRegion *region = NULL, bool maybe_repaint_cursor = true); 

	/**Draws cursor surrounding point that represents probe value localted at given
	 * postition in observed @see Draw values table.
	 * @param index index of porbe in Draw vales table
	 * @param clear if set to true the cursor is actually cleared(removed)
	 * @param region if not NULL part of the of @see m_dc affected by this operation is 
	 * stored(union operation with it's current content) in this region*/
	void DrawCursor(int index, bool clear, wxRegion *region = NULL);

	/** Get cursor index corresponding to point with given 
	 * x-coordinate.
	 * @param x window-relative x cooridinate
	 * @return index of position corresponding to given x cooridinate
	 */
	int GetIndex(int x) const;
	
	/**@return true is alternate color shall be used while drawing point and given index*/
	virtual bool AlternateColor(int idx);

	/**return size of region of dc used for drawing*/
	virtual void GetSize(int *w, int *h) const;

	/**repaints view*/
	virtual void EnableChanged(Draw *draw);

	void DrawDot(int x, int y, wxDC *dc, SDC *sdc, wxRegion* region = NULL);

public:
	GraphView(WxGraphs *widget);

	void SetMargins(int left, int right, int top, int bottom) { m_leftmargin = left, m_rightmargin = right, m_topmargin = top, m_bottommargin = bottom; }

	/**Repaints view*/
	void Attach(Draw *draw);

	/**Clears view*/
	void Detach(Draw *draw);

	/**Repaints view*/
	void DrawInfoChanged(Draw *draw);

	/**Repaints view*/
	void PeriodChanged(Draw *draw, PeriodType period);

	/**repaints view*/
	virtual void FilterChanged(Draw *draw);

	/**Repaints view*/
	void ScreenMoved(Draw* draw, const wxDateTime &start_date);

	/**Moves cursor
	 * @param pi previous cursor poitiion, may be -1 if cursor was not visible previously
	 * @param ni new (current cursor position)
	 * @param d actual displacement of cursor (may be different than ni - pi)*/
	void CurrentProbeChanged(Draw* draw, int pi, int ni, int d);

	/**Draw points all cursor position*/
	void DrawAll();

	/**Draws point representing data probe
	 * @param i index into observer @see Draw data table*/
	void NewData(Draw* draw, int i);

	/**Resizes view*/
	virtual void SetSize(int w, int h);

	/**return current dc*/
	virtual wxDC* GetDC();

	/**Returns distance from given point in the view to nearest drawn data probe
	 * @param x x coordinate of point
	 * @param y y coordinate of point
	 * @param d output param, calculated distance
	 * @param time output param, time returned distance refers to*/
	void GetDistance(int x, int y, double &d, wxDateTime &time) const;

	/**repaints cursor*/
	void Refresh();

	virtual ~GraphView();

	/**< Size of cursor rectangle*/
	static const int cursorrectanglesize;


};

#endif

