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

bool Updater::needToDownloadFile(wxString newChecksum) {

	if (newChecksum.IsEmpty()) {
		return true;
	}

	fs::wpath file = m_downloader->getInstallerFilePath();

	if (!fs::exists(file)) {
		return true;
	}

	unsigned char digest[EVP_MAX_MD_SIZE];
	char *buff = new char[MD5_READ_BLOCK_SIZE];

	std::ifstream installer_file_stream(SC::S2A(file.string()).c_str() , std::ifstream::in | std::ifstream::binary);

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

	if(isVersionNewer(version)) {
		bool ret = needToDownloadFile(m_downloader->getChecksum());
		if(ret){
			return false;
		} else {
			setStatus(READY_TO_INSTALL);
			return true;
		}
	}
	
	return false;
}

void Updater::Run() {

	do {
		wxString version = m_downloader->getVersion();

		bool is_newer_available = isVersionNewer(version);

		if (is_newer_available) {
			if (needToDownloadFile(m_downloader->getChecksum())) {
				setStatus(DOWNLOADING);
				bool result = m_downloader->getInstallerFile();
				if (result || !needToDownloadFile(m_downloader->getChecksum())) {
					setStatus(READY_TO_INSTALL);
				} else {
					setStatus(UPDATE_ERROR);
				}
			} else {
				setStatus(READY_TO_INSTALL);
			}
		}

		boost::xtime xt;
		boost::xtime_get(&xt, boost::TIME_UTC);
		xt.sec = xt.sec + 300;
		boost::thread::sleep(xt);

	} while (1);

}
