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

#include "downloader.h"

#include "cconv.h"

bool Downloader::singleton(false);

Downloader::Downloader(char * version_url, char * installer_url, char * checksum_url, fs::wpath installer_file_path) :
	m_version_url(strdup(version_url)), m_installer_url(strdup(installer_url)), m_checksum_url(strdup(checksum_url)),
			m_buf(NULL) {
	assert(!singleton);
	singleton = true;
	curl_global_init(CURL_GLOBAL_ALL);
	m_curl_handler = curl_easy_init();
	m_buf_len = 0;
	m_installer_file_path = installer_file_path;
	m_downloading = false;
	m_current_download_percentage = 0;

}

Downloader::~Downloader() {
	curl_global_cleanup();
	free(m_installer_url);
	free(m_version_url);
	if (m_buf)
		free(m_buf);
}

wxString Downloader::getVersion() {
	wxString version = getTextFileAsString(m_version_url);
	version = version.AfterFirst('\"');
	version = version.BeforeLast('\"');
	return version;
}

wxString Downloader::getChecksum() {

	wxString checksum = getTextFileAsString(m_checksum_url);
	wxStringTokenizer tkz(checksum);
	if (tkz.HasMoreTokens())
		return tkz.GetNextToken();
	return wxString();
}

wxString Downloader::getTextFileAsString(const char* url) {
	m_buf_used = 0;

	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_URL, url);

	if (m_error)
		return wxString();
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_WRITEFUNCTION, WriteFunction);
	if (m_error)
		return wxString();
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_WRITEDATA, (void*)this);
	if (m_error)
		return wxString();
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_HTTPGET, 1);
	if (m_error)
		return wxString();
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_FOLLOWLOCATION, 1);
	if (m_error)
		return wxString();
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_MAXREDIRS, (long)5);
	if (m_error)
		return wxString();
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_TIMEOUT, long(10));
	if (m_error)
		return wxString();

	m_error = curl_easy_perform(m_curl_handler);
	if (m_error)
		return wxString();
	long code;

	curl_easy_getinfo(m_curl_handler, CURLINFO_RESPONSE_CODE, &code);

	if ((code / 100) != 2) {
		return wxString();
	}

	if (m_buf_len > ((m_buf_used + 1) * 4)) {
		m_buf_len = m_buf_len / 2;
		m_buf = (char *) realloc(m_buf, m_buf_len);
	}
	m_buf[m_buf_used] = '\0';

	return wxString(SC::A2S(m_buf));

}

bool Downloader::getInstallerFile() {
	m_buf_used = 0;

	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_URL, m_installer_url);

	if (fs::exists(m_installer_file_path)) {
		try {
			fs::remove(m_installer_file_path);
		} catch(fs::filesystem_error err) {
			return false;
		}
	}
	m_downloading = true;

	m_installer_ofstream = new std::ofstream(SC::S2A(m_installer_file_path.string()).c_str(), std::ios::binary | std::ios::out | std::ios::trunc);

	if (m_error)
		return false;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_WRITEFUNCTION, WriteFileFunction);
	if (m_error)
		return false;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_WRITEDATA, (void*)this);
	if (m_error)
		return false;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_HTTPGET, 1);
	if (m_error)
		return false;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_FOLLOWLOCATION, 1);
	if (m_error)
		return false;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_MAXREDIRS, (long)5);
	if (m_error)
		return false;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_TIMEOUT, long(60*60*2));
	if (m_error)
		return false;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_PROGRESSFUNCTION, ProgressFileFunction);
	if (m_error)
		return false;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_PROGRESSDATA, (void*)this);
	if (m_error)
		return false;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_NOPROGRESS, FALSE);
	if (m_error)
		return false;

	m_error = curl_easy_perform(m_curl_handler);

	if (m_error)
		return false;
	long code;

	curl_easy_getinfo(m_curl_handler, CURLINFO_RESPONSE_CODE, &code);

	if ((code / 100) != 2) {
		return false;
	}

	m_downloading = false;

	m_installer_ofstream->close();

	delete m_installer_ofstream;

	return true;
}

size_t Downloader::WriteFunction(void* ptr, size_t size, size_t nmemb, void *object) {
	assert (object);
	Downloader* c = (Downloader *) object;
	return (c->WriteCallback((char *)ptr, size * nmemb));
}

size_t Downloader::WriteCallback(char* data, size_t size) {
	if (m_buf_used + size > m_buf_len) {
		m_buf_len = m_buf_used + size + 1;
		m_buf = (char *)realloc(m_buf, m_buf_len);
		assert (m_buf);
	}
	memcpy(m_buf + m_buf_used, data, size);
	m_buf_used += size;
	return size;
}

size_t Downloader::WriteFileFunction(void* ptr, size_t size, size_t nmemb, void *object) {
	assert (object);
	Downloader* c = (Downloader *) object;
	return (c->WriteFileCallback((char *)ptr, size * nmemb));
}

size_t Downloader::WriteFileCallback(char* data, size_t size) {
	m_installer_ofstream->write(data, size);
	return size;
}

int Downloader::ProgressFileFunction(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
	assert(clientp);
	double percentage = dlnow * 100.0 / dltotal;

	assert(percentage >= 0);
	if(percentage > 100){
		percentage = 100;
	}

	Downloader* c = (Downloader *) clientp;
	c->ProgressFileCallback((int)percentage);

	return 0;
}

void Downloader::ProgressFileCallback(size_t size) {
	wxCriticalSectionLocker locker(m_current_download_percentage_cs);
	assert(size >= 0);
	assert(size <= 100);
	m_current_download_percentage = size;
}

size_t Downloader::getCurrentDownloadPercentage() {
	wxCriticalSectionLocker locker(m_current_download_percentage_cs);
	return m_current_download_percentage;
}

