/* $Id$
 *
 * kontroler3 program
 * SZARP

 * ecto@praterm.com.pl
 */

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


#include "szapp.h"

/**
 * Main app class
 */
class kontrApp: public szApp {
  wxLocale locale;
  wxString server;
protected:
  virtual bool OnCmdLineError(wxCmdLineParser &parser);
  virtual bool OnCmdLineHelp(wxCmdLineParser &parser);
  virtual bool OnCmdLineParsed(wxCmdLineParser &parser);
  virtual void OnInitCmdLine(wxCmdLineParser &parser);
  virtual bool OnInit();
};

DECLARE_APP(kontrApp)

