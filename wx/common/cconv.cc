#include "cconv.h"

namespace SC {

std::basic_string<unsigned char> S2U(const wxString& c) {
	return S2U(std::wstring(c.c_str()));
}

std::string S2A(const wxString& c) {
	return S2A(std::wstring(c.c_str()));
}
}
