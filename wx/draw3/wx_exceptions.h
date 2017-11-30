#ifndef __WX__EXCEPTIONS__
#define __WX__EXCEPTIONS__

#include "exception.h"
#include <wx/wx.h>

class DrawBaseException: public SzException {
public:
	using SzException::SzException;
	DrawBaseException(const wxString& wxStr): SzException(std::string( wxStr.mb_str() )) {}
};

#endif
