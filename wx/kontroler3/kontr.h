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
protected:
  virtual bool OnInit();
};

DECLARE_APP(kontrApp)

