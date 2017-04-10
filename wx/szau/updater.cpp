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

#include "version.h"
#include "cconv.h"

#include <openssl/evp.h>
#include <fstream>

#include "updater.h"
#include "szauapp.h"

#define MD5_READ_BLOCK_SIZE 8192

class Runner {
private:
	Updater * m_updater;
public:
	Runner(Updater * updater) :
		m_updater(updater) {
	}
	void operator()() {
		m_updater->Run();
	}

};

Updater::Updater(Downloader * downloader) {
	m_downloader = downloader;
	setStatus(NEWEST_VERSION);
}

std::wstring Updater::VersionToInstallerName(std::wstring version) {
	return std::wstring(L"szarp-") + version + L".exe";
}

void Updater::Start(){
	boost::thread startthread(Runner(this));
}

UpdaterStatus Updater::getStatus(size_t& value) {
	wxCriticalSectionLocker locker(m_internal_cs);
	if (m_status == DOWNLOADING) {
		value = m_downloader->getCurrentDownloadPercentage();
	}
	return m_status;
}

void Updater::setStatus(UpdaterStatus status) {
	wxCriticalSectionLocker locker(m_internal_cs);
	m_status = status;
}

bool Updater::needToDownloadFile(wxString newChecksum, wxString version) {

	if (newChecksum.IsEmpty()) {
		return true;
	}

	fs::wpath file = wxGetApp().getInstallerLocalPath() / VersionToInstallerName(version.wc_str());

	if (!fs::exists(file)) {
		return true;
	}

	unsigned char digest[EVP_MAX_MD_SIZE];
	char *buff = new char[MD5_READ_BLOCK_SIZE];

	std::ifstream installer_file_stream(SC::S2A(file.wstring()).c_str() , std::ifstream::in | std::ifstream::binary);

	EVP_MD_CTX mdctx;
	const EVP_MD *md;
	unsigned int md_len;

	OpenSSL_add_all_digests();
	md = EVP_get_digestbyname("md5");

	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, md, NULL);

	while (!installer_file_stream.eof()) {
		installer_file_stream.read(buff, MD5_READ_BLOCK_SIZE);
		int len = installer_file_stream.gcount();
		EVP_DigestUpdate(&mdctx, buff, len);
	}

	EVP_DigestFinal_ex(&mdctx, digest, &md_len);
	EVP_MD_CTX_cleanup(&mdctx);

	wxString local_file;

	for (unsigned int i = 0; i < md_len; i++) {
		wxString tmp;
		tmp.Printf(_T("%02x"), digest[i]);
		local_file += tmp;
	}

	delete[] buff;

	return newChecksum != local_file;
}

bool Updater::isVersionNewer(wxString version) {

	wxString currentVersion= wxString(SC::A2S(SZARP_VERSION));

	wxStringTokenizer vtkz(version, _T("."));
	wxStringTokenizer cvtkz(currentVersion, _T("."));

	while (1) {
		wxString a = vtkz.GetNextToken();
		wxString b = cvtkz.GetNextToken();

		if (a.IsEmpty() || b.IsEmpty())
			return true;

		long av, bv;
		if (!a.ToLong(&av))
			return true;
		if (!b.ToLong(&bv))
			return true;

		if (a > b) {
			return true;
		}

		if (a < b) {
			return false;
		}

		if (vtkz.HasMoreTokens() && cvtkz.HasMoreTokens()) {
			continue;
		}

		if (!vtkz.HasMoreTokens() && !cvtkz.HasMoreTokens()) {
			return false;
		}

		if (vtkz.HasMoreTokens() && !cvtkz.HasMoreTokens()) {
			return false;
		}

		if (!vtkz.HasMoreTokens() && cvtkz.HasMoreTokens()) {
			return true;
		}

	}

}

bool Updater::InstallFileReadyOnStartup(){
	wxString version = m_downloader->getVersion();

	if (isVersionNewer(version)) {
		SetCurrentVersion(version.wc_str());

		bool ret = needToDownloadFile(m_downloader->getChecksum(), version);
		if (ret) {
			return false;
		} else {
			setStatus(READY_TO_INSTALL);
			return true;
		}
	}
	
	return false;
}

void Updater::SetCurrentVersion(std::wstring version) {
	wxCriticalSectionLocker locker(m_version_cs);
	m_version = version;
}

std::wstring Updater::GetCurrentVersion() {
	wxCriticalSectionLocker locker(m_version_cs);
	return m_version;
}

void Updater::Install() {
	std::wstring version = GetCurrentVersion();
	if (version.empty())
		return;

	wxString command = wxString((wxGetApp().getInstallerLocalPath() / VersionToInstallerName(version)).wstring());
	wxExecute(command, wxEXEC_ASYNC);
}

void Updater::Run() {

	do {
		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC_);
		xt.sec = xt.sec + 300;
		boost::thread::sleep(xt);

		wxString version = m_downloader->getVersion();

		bool is_newer_available = isVersionNewer(version);

		if (is_newer_available) {

			SetCurrentVersion(version.wc_str());

			if (needToDownloadFile(m_downloader->getChecksum(), version)) {
				setStatus(DOWNLOADING);

				std::wstring installer_local_path = (wxGetApp().getInstallerLocalPath() / VersionToInstallerName(GetCurrentVersion())).wstring();
				bool result = m_downloader->getInstallerFile(installer_local_path);
				if (result || !needToDownloadFile(m_downloader->getChecksum(), version)) {
					setStatus(READY_TO_INSTALL);
				} else {
					setStatus(UPDATE_ERROR);
				}
			} else {
				setStatus(READY_TO_INSTALL);
			}
		}

	} while (1);

}
