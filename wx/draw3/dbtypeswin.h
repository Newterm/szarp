#ifndef __DBTYPESWIN_H__
#define __DBTYPESWIN_H__

#include <wx/wx.h>
#include <map>

class BaseLabel : public wxWindow {
	wxString base_name;
	wxString base_type;

	const int margin_left = 10;
	const int margin_right = 10;

protected:
	virtual wxSize DoGetBestSize() const override;

public:
	BaseLabel(wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
	void SetBaseNameText(const std::wstring& base_name);
	void SetBaseTypeText(const std::wstring& base_type);
	void OnPaint(wxPaintEvent &event);
	DECLARE_EVENT_TABLE()
};

class DatabaseTypesWindow : public wxDialog {

	DrawPanel *draw_panel;

	std::vector <std::unique_ptr<BaseLabel>> base_labels;

	wxSizer *sizer;

	int overallWidth;
	int overallHeight;

	int tabsCount;

	void OnClose(wxCloseEvent &event);

public:
	DatabaseTypesWindow(DrawPanel* panel, wxWindow *parent);

	/**Displays window and activates object*/
	virtual bool Show(bool show = true) override;

	void MakeLabelsFromMap(std::map<std::wstring, std::wstring> const &tabMap);

	DECLARE_EVENT_TABLE()
};

#endif //__DBTYPESWIN_H__
