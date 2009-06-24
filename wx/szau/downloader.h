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

#ifndef __DOWNLOADER_H__
#define __DOWNLOADER_H__

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

#include "boost/filesystem/path.hpp"
#include "boost/filesystem/convenience.hpp"
#include "boost/filesystem/operations.hpp"

namespace fs = boost::filesystem;

class Downloader {

	CURL* m_curl_handler;
	bool m_curl_inilialized;
	static size_t WriteFunction(void* ptr, size_t size, size_t nmemb, void *object);
	size_t WriteCallback(char* data, size_t size);
	static size_t WriteFileFunction(void* ptr, size_t size, size_t nmemb, void *object);
	size_t WriteFileCallback(char* data, size_t size);
	static int ProgressFileFunction(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	void ProgressFileCallback(size_t size);

	static bool singleton;

	size_t m_buf_used;
	size_t m_buf_len;
	char* m_version_url;
	char* m_installer_url;
	char* m_checksum_url;
	int m_error;
	char* m_buf;
	fs::wpath m_installer_file_path;
	std::ofstream * m_installer_ofstream;
	size_t m_current_download_percentage;
	wxCriticalSection m_current_download_percentage_cs;
	bool m_downloading;

	wxString getTextFileAsString(const char* url);

public:
	Downloader(char * version_url, char * installer_url, char * checksum_url, fs::wpath installer_file_path);
	~Downloader();
	wxString getVersion();
	wxString getChecksum();
	bool getInstallerFile();
	size_t getCurrentDownloadPercentage();
	fs::wpath getInstallerFilePath() {
		return m_installer_file_path;
	}
	;
};

#endif
