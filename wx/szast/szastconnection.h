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

#ifndef __SZASTFRAME_H_
#define __SZASTFRAME_H_

#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/thread.h"
#endif

#include "ppset.h"

class SzastConnection : public wxThread {
	wxEvtHandler *m_receiver;
	xmlDocPtr m_document;
	wxMutex m_mutex;
	wxSemaphore m_semaphore;
	bool m_finish_flag;
public:
	virtual void* Entry();
	SzastConnection(wxEvtHandler *m_receiver);
	void Query(xmlDocPtr ptr);
	void Finish();
};

class NullDaemonStopper : public PPSET::DaemonStopper {
	public:
	virtual void StartDaemon() {};
	virtual bool StopDaemon(xmlDocPtr *err) { return true; }
};

class NullDaemonStoppersFactory : public PPSET::DaemonStoppersFactory {
	public:
	virtual NullDaemonStopper* CreateDaemonStopper(const std::string &path) { return new NullDaemonStopper(); }
};

#endif
