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
#ifndef _SSEXCEPTION_H_
#define _SSEXCEPTION_H_
#ifdef MINGW32
#include <wx/wx.h>
#include <windows.h>
#endif
#include <openssl/err.h>

#include "ssstring.h"

/**general exception class*/
class Exception {
protected:
	/**cause of an exception*/
	ssstring m_cause;
public:
	Exception(ssstring cause);
	ssstring What(); 
	virtual ~Exception();
};

/**exception thrown if ivalid authoriazation info is provied by client*/
class AppException : public Exception {
public:
	AppException(ssstring cause);
};


/** exeception thrown when unrecoverable I/O error ocurrs*/
class IOException : public Exception {
public:
#ifdef MINGW32
	IOException(DWORD error);
#else
	IOException(int error);
#endif
	IOException(ssstring s);
};

/** exception thrown when unrecoverable error in I/O over network ocurrs*/
class SSLException : public Exception {
public:
	SSLException(int error);
	SSLException(ssstring s);
};

/** exception thrown when connection is unexpectedly shut down*/
class ConnectionShutdown : public Exception {
public:
	ConnectionShutdown();
};

/** exception thrown when user requests termination of sync process*/
class TerminationRequest : public Exception {
public:
	TerminationRequest();
};


#endif
