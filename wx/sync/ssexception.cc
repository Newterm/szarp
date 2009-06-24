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
#include "ssexception.h"

#include <string.h>
#ifdef MINGw32
#include <windows.h>
#endif

Exception::Exception(ssstring cause) : m_cause(cause) {
}

ssstring Exception::What() {
	return m_cause;
}

Exception::~Exception() {
}

AppException::AppException(ssstring cause) : Exception(cause) {
}

#ifdef MINGW32
IOException::IOException(DWORD error) : Exception(_T("")) {
	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	m_cause = (wchar_t*) lpMsgBuf;

	LocalFree(lpMsgBuf);

}
#else
IOException::IOException(int error) : Exception(csconv(strerror(error))) {
}
#endif

IOException::IOException(ssstring s) : Exception(s) {
}

SSLException::SSLException(int error) : Exception(csconv(ERR_error_string(error, NULL))) {
}

ConnectionShutdown::ConnectionShutdown() : Exception(_("Connection shut down")) {
}

TerminationRequest::TerminationRequest() : Exception(_("TerminationRequest")) {
}

