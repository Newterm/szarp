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
 * SZAU - Szarp Automatic Updater
 * SZARP

 * S³awomir Chy³ek schylek@praterm.com.pl
 *
 * $Id$
 */

#ifndef __UPDATER_H__
#define __UPDATER_H__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/wxprec.h>
#include <wx/progdlg.h>
#include <wx/cmdline.h>
#include <wx/taskbar.h>
#include <wx/tokenzr.h>
#include <wx/timer.h>
#include <wx/thread.h>
#else
#include <wx/wx.h>
#endif

#include <curl/curl.h>
#include <fstream>

#include "downloader.h"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"
#include "boost/filesystem/operations.hpp"

#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

namespace fs = boost::filesystem;

enum UpdaterStatus {
	NEWEST_VERSION,
	DOWNLOADING,
	READY_TO_INSTALL,
	UPDATE_ERROR
};

class Updater {
	UpdaterStatus m_status;
	Downloader * m_downloader;
	wxCriticalSection m_internal_cs;
	wxCriticalSection m_version_cs;
	void setStatus(UpdaterStatus status);
	static bool isVersionNewer(wxString version);
	bool needToDownloadFile(wxString newChecksum, wxString version);

	std::wstring m_version;
	void SetCurrentVersion(std::wstring version);
	std::wstring GetCurrentVersion();
public:
	Updater(Downloader * downloader);
	UpdaterStatus getStatus(size_t & value);
	void Run();
	void Install();
	bool InstallFileReadyOnStartup();
	std::wstring VersionToInstallerName(std::wstring version);
	void Start();
};

#endif
