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
 * draw3 SZARP
 
 *
 * $Id$
 */

#ifndef __XYGRAPH_H__
#define __XYGRAPH_H__

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <utility>
#include <deque>
#include <vector>

#include <wx/frame.h>
#include <wx/grid.h>

/**Paints XY graph onto DC*/
class XYGraphPainter {
	/**margins*/
	int m_left_margin;
	int m_right_margin;
	int m_top_margin;
	int m_bottom_margin;

	/**width of region grph in painted on*/
	int m_width;
	/**height of region grph in painted on*/
	int m_height;

	/**Painted graph*/
	XYGraph* m_graph;

	/**Calculates number of ticks on axis given maximum, minimum values and param precision*/
	int FindNumberOfSlices(double max, double min, DrawInfo *di);

	public:
	XYGraphPainter(int m_left_margin,
			int m_right_margin,
			int m_top_margin,
			int m_bottom_margin);


	/**Sets size of drawing region
	 * @param w width of region
	 * @param w height of region*/
	void SetSize(int w, int h);

	/**Draws short names of params
	 * @param xdisp disatnce of graph description text from vertical axis
	 * @param ypos y coordinate of start of graph description*/
	void DrawDrawsInfo(wxDC *dc, int xdisp, int ypos);

	/**Sets @see XYGraph description to be painted*/
	void SetGraph(XYGraph *graph);

	/**Get coordinates of point of i'th index*/
	void GetPointPosition(int i, int *x, int *y);

	/**Draws X axis.
	 * @param dc device contex to paint with
	 * @param arrow_width width of the arrow
	 * @param arrow_height height of the arrow
	 * @param mark_height height of the arrow*/
	void DrawXAxis(wxDC *dc, int arrow_width, int arrow_height, int mark_height);

	/**Draws Y axis.
	 * @param dc device contex to paint with
	 * @param arrow_width width of the arrow
	 * @param arrow_height height of the arrow
	 * @param mark_height height of the arrow*/
	void DrawYAxis(wxDC *dc, int arrow_width, int arrow_height, int mark_width);

	/**Draws unit of X-axis graph
	 * @param xdisp distance of unit string from horizontal axis
	 * @param ydisp distance of unit string from horizontal axis*/
	void DrawXUnit(wxDC *dc, int xdisp, int ydisp);

	/**Draws unit of Y-axis graph
	 * @param xdisp distance of unit string from vertical axis
	 * @param ydisp distance of unit string from vertical axis*/
	void DrawYUnit(wxDC *dc, int xdisp, int ydisp);

	/**Draw actual graph*/
	void DrawGraph(wxDC *dc);

};

/**Main XY grap frame*/
class XYFrame : public szFrame, public XFrame {
	/**Panel holding widgets*/
	XYPanel *m_panel;
	/**Dialog for xy graph parameters retrieval*/
	XYDialog *m_dialog;
	/**Hides window (or destroys it when event can not be vetoed)*/
	virtual void OnClose(wxCloseEvent &event);
	/**Close menu item handler*/
	void OnCloseMenu(wxCommandEvent &event);
	/**Changes graph parameters*/
	void OnGraphChange(wxCommandEvent &event);
	/**Prints graph*/
	void OnPrint(wxCommandEvent &event);
	/**Prints graph*/
	void OnPrintPageSetup(wxCommandEvent &event);
	/**Displays print preview of xy graph*/
	void OnPrintPreview(wxCommandEvent &event);
	/**Zooms grap out*/
	void OnZoomOut(wxCommandEvent &event);

	void OnHelp(wxCommandEvent &event);
	/**prefix of configuration users initially chooses draws from*/
	wxString m_default_prefix;
	/**@see DatabaseManager*/
	DatabaseManager *m_db_manager;
	/**@see ConfigManager*/
	ConfigManager *m_cfg_manager;
public:
	/**Sets parameters of displayed graphs*/
	virtual void SetGraph(XYGraph *graph);

	virtual int GetDimCount();

	XYFrame(wxString default_prefix, DatabaseManager *dbmanager, ConfigManager *cfgmanager, TimeInfo time, FrameManager *frame_manager);

	virtual ~XYFrame();
	DECLARE_EVENT_TABLE()
};

class XYPanel : public wxPanel {
	/**prefix of configuration users initially chooses draws from*/
	wxString m_default_prefix;
	/**@see XYGraphWidget. Widget displaying xy graph*/
	XYGraphWidget *m_graph;
	/**@see XYPointInfo. Widget used for displaying current point info*/
	XYPointInfo *m_point_info;
	/**@see DatabaseManager*/
	DatabaseManager *m_database_manager;
	/**@see ConfigManager*/
	ConfigManager *m_config_manager;

	public:
	/**Prints (or displays print preview) of graph*/
	void Print(bool preview);
	/**Sets XYGraph parameters object*/
	void SetGraph(XYGraph *graph);
	/**Zooms out*/
	void ZoomOut();
	XYPanel(wxWindow *parent, DatabaseManager *database_manager, ConfigManager *config_manager, FrameManager *frame_manager, wxString default_prefix);
	//DECLARE_EVENT_TABLE()
};	

/**KD tree for fast points lookup*/
class KDTree {
public:
	/**Tree entry*/
	struct KDTreeEntry {
		/**Index of point in @see XYGraph structure*/
		size_t i;
		/**Point position*/
		wxPoint p;
	};

	/**Tree nodes visotr*/
	class Visitor {
	public:
		virtual void Visit(const KDTreeEntry &entry) = 0;
		virtual ~Visitor() = 0;
	};

	/**Orientation crosscut - horizontal or vertical*/
	enum Pivot { 
		XPIVOT,
		YPIVOT
	}; 

	/**Range of plane covered node*/
	struct Range {
		/**Upper left corner*/
		wxPoint first;
		/**Lower right corner*/
		wxPoint second;
		Range(const wxPoint& s, const wxPoint &e);
		Range();
		/**@return intersectionof two ranges*/
		Range Intersect(const Range &range) const;
		/**@return true if given point lays withing current range boundary*/
		bool IsContained(const wxPoint &point) const;
		/**Split this range vertically along y coordiate*/
		void SplitH(int y, Range &up, Range &down) const;
		/**Split this range horizontally along x coordiate*/
		void SplitV(int x, Range &left, Range &right) const;
		/**Calculates distance from give point to this plane*/
		double Distance(const wxPoint &p) const;
		/**@return true if this range is empty*/
		bool IsEmpty() const;
	};

	/**@see KDTreeEntry*/
	KDTreeEntry m_entry;
	/**Left subtree*/
	KDTree *m_left;
	/**Right subtree*/
	KDTree *m_right;
	/**@see Range*/
	Range m_range;
	/**@see Pivot*/
	Pivot m_pivot;
	KDTree(const KDTreeEntry &entry, KDTree *left, KDTree *right, const Range &range, Pivot p);

	/**Constructs a KDTree
	 * @param vals tree entries from which tree is to be constructed
	 * @param range range covered by this tree
	 * @param pivot type of crosscur of root node*/
	static KDTree* Construct(std::deque<KDTreeEntry*> vals, const Range& range, Pivot pivot);
	/**Compares x coordinates of two points*/
	static bool XCoordCmp(const KDTreeEntry* p1, const KDTreeEntry* p2);
	/**Compares y coordinates of two points*/
	static bool YCoordCmp(const KDTreeEntry* p1, const KDTreeEntry* p2);
public:
	/**Construct a KDTree given list of entries and tree range coordinates*/
	static KDTree* Construct(std::deque<KDTreeEntry*>&, int xmin, int xmax, int ymin, int ymax);
	/**Returs nearest point to given one.
	 * @param p point to find nearest entry
	 * @param r output param, nearest found entry
	 * @param ran search will be performed with this range only
	 * @param max only subtrees which are no farther than this value are searched*/
	double GetNearest(const wxPoint& p, KDTreeEntry &r, const Range& ran, const double& max);
	/**Visits all points withing given range*/
	void VisitInRange(const Range &ran, Visitor &visitor);
	~KDTree();
};

/**KDTree visitior calculating stats of visited nodes*/
class StatsCollector : public KDTree::Visitor {
	/**@see XYGraph*/
	XYGraph *m_graph;

	/** values of X */
	std::vector<double> xvals;

	/** values of X */
	std::vector<double> yvals;
	
	double ComputeCorrelation(double xavg, double yavg, std::vector<double> &xvals, std::vector<double> &yvals, double &x_standard_deviation, double &y_standard_deviation);

public:
	StatsCollector(XYGraph *graph);
	/**return number of visited nodes*/
	int GetDataCount();
	/**Updates xmin/xmax/etc. values of @see m_graph*/
	void UpdateGraphStats();

	/**Visits node*/
	virtual void Visit(const KDTree::KDTreeEntry& entry);

	virtual ~StatsCollector() {}

};
	
/**Widget displaying XY graph*/
class XYGraphWidget : public wxWindow {
	/**Cursor movement direction*/
	enum MoveDirection {
		UP,
		DOWN,
		LEFT,
		RIGHT
	};

	/**@see XYGraphPainter*/
	XYGraphPainter m_painter;
	/**@see XYPointInfo*/
	XYPointInfo *m_point_info;
	/**When user selects a subrectangle for statistical values calculation, this is the currently selected rectangle*/
	wxRect m_current_rect;
	/**@see XYGraph*/
	XYGraph *m_graph;
	/**Bitmap where axes and points are painted*/
	wxBitmap *m_bmp;
	/**DeviceContext used for painting onto the bitmap*/
	wxMemoryDC m_bdc;
	/**@see KDTree*/
	KDTree *m_kdtree;
	/**Timer for cursor movement speed*/
	wxTimer *m_keyboard_timer;
	/**Current size of widget*/
	wxSize m_current_size;

	int m_keyboard_action;		/**< Action to perform repeatedly (with timer generated 
				  events) after widget refreshing. */
	int m_cursor_index;	/**<Index of point that is currently selected*/

	bool m_drawing;		/**<Flag indicates if widget paints itself during PaintEvent*/

	void OnPaint(wxPaintEvent &event);
				/**<Paints graph*/
	void MoveCursorLeft();
				/**<Move cursor left*/
	void MoveCursorRight();
				/**<Move cursor right*/
	void MoveCursorUp();
				/**<Move cursor up*/
	void MoveCursorDown();
				/**<Move cursor down*/
	void UpdateTree();
				/**<(Re)genrates KDTree*/
	bool UpdateStats();
				/**<Recalculates statistical values*/
	void MoveCursor(MoveDirection dir);
				/**<Moves cursor in given direction*/
	void OnSize(wxSizeEvent &event);
				/**Resizef widget*/
	void OnLeftMouseDown(wxMouseEvent &event);
				/**Select nearest point*/
	void OnRightMouseDown(wxMouseEvent &event);
				/**Start selection of rectangle for statistical values calculation*/
	void OnRightMouseUp(wxMouseEvent &event);
				/**End selection of rectangle for statistical values calculation*/
	/**This metod intentionally does nothing. Common wisdow says that this reduces flickering during repaint...*/
	void OnEraseBackground(wxEraseEvent& WXUNUSED(event));
	/**Redraws selection rectangle (if such is currently painted*/
	void OnMouseMove(wxMouseEvent &event);
	/**Draws cursor*/
	void DrawCursor(wxDC *dc);
	/**Sets cursor at point at given index*/
	void SetCursorIndex(int index);
	/**Moves cursor (if arrow key is pressed)*/
	void OnKeyboardTimer(wxTimerEvent &event);
	/**Updates contents of @see XYPointInfo widget*/
	void UpdateInfoWidget();
	/**Repaints @see  m_bitmap*/
	void RepaintBitmap();
	/**Draws axes*/
	void DrawAxes(wxDC *dc);

	void EnableZoomOut(bool enable);

	virtual ~XYGraphWidget();

	public:
	XYGraphWidget(wxWindow *parent, XYPointInfo *point_info);
	/**Starts (or stops) cursor movement*/
	void SetKeyboardAction(ActionKeyboardType type);
	/**Sarts displaying of XY graph*/
	void StartDrawing();
	/**Prints (show print preview)  of xy graph*/
	void Print(bool preview = false);
	/**Draws rectangle onto the graph*/
	void DrawRectangle(wxDC *dc);
	/**Sets XYGraph info that is to be drawn*/
	void SetGraph(XYGraph *info);
	/**Stops drawing of XYGraph*/
	void StopDrawing();
	/**Zooms graph out*/
	void ZoomOut();
	/**screen margins*/
	static const int bottom_margin;
	static const int top_margin;
	static const int left_margin;
	static const int right_margin;
	DECLARE_EVENT_TABLE()
};

/**Widgets displaying currently selectd point info*/
class XYPointInfo : public wxPanel {
	/**Callback for goto graph button*/
	void OnGoToGraph(wxCommandEvent &event);
	/**Precisions of x and y params*/
	int m_px, m_py;
	/**@see DrawInfo for x and y param*/
	DrawInfo *m_dx, *m_dy;
	/**Widgets displaying selected point dates*/
	wxChoice *m_point_dates_choice;
	/**Selected point dates*/
	std::vector<DTime> m_point_dates;
	/**Widgets displaying number of selected points*/
	wxStaticText *m_points_no;
	/**Goto graph button*/
	wxButton *m_goto_button;
	/**Graphs period*/
	PeriodType m_period;
	/**Configuration manager*/
	ConfigManager *m_config_manager;
	/**Frame manager*/
	FrameManager *m_frame_manager;
	/**Prefix*/
	wxString m_default_prefix;
	wxGrid* m_info_grid;
	public:
	XYPointInfo(wxWindow *parent, ConfigManager *config_manager, FrameManager *frame_manager, wxString default_prefix);
	/**Updates @see m_value_textx and @see m_value_texty*/
	void SetPointInfo(XYGraph *graph, int point_index);
	/**Set @see XYGraph info of current graph*/
	void SetGraphInfo(XYGraph *graph);
	DECLARE_EVENT_TABLE()
};
#endif
