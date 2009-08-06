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
#include "xmlutils.h"

bool Downloader::singleton(false);

Downloader::Downloader(char * rss_url) :
	m_rss_url(strdup(rss_url)) {
	assert(!singleton);
	singleton = true;
	curl_global_init(CURL_GLOBAL_ALL);
	m_curl_handler = curl_easy_init();
	m_buf = NULL;
	m_buf_len = 0;
	m_downloading = false;
	m_current_download_percentage = 0;

}

Downloader::~Downloader() {
	curl_global_cleanup();
	free(m_rss_url);
	if (m_buf)
		free(m_buf);
}

wxString Downloader::getVersion() {
	if (GetRSS())
		return m_version;
	else
		return wxEmptyString;
}

wxString Downloader::getChecksum() {
	return m_md5;
}

xmlDocPtr Downloader::FetchRSS() {
	m_buf_used = 0;

	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_URL, m_rss_url);

	if (m_error)
		return NULL;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_WRITEFUNCTION, WriteFunction);
	if (m_error)
		return NULL;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_WRITEDATA, (void*)this);
	if (m_error)
		return NULL;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_HTTPGET, 1);
	if (m_error)
		return NULL;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_FOLLOWLOCATION, 1);
	if (m_error)
		return NULL;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_MAXREDIRS, (long)5);
	if (m_error)
		return NULL;
	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_TIMEOUT, long(20));
	if (m_error)
		return NULL;

	m_error = curl_easy_perform(m_curl_handler);
	if (m_error)
		return NULL;
	long code;

	curl_easy_getinfo(m_curl_handler, CURLINFO_RESPONSE_CODE, &code);

	if ((code / 100) != 2) {
		return NULL;
	}

	if (m_buf_len > m_buf_used * 4) {
		m_buf_len = m_buf_len / 2;
		m_buf = (char *) realloc(m_buf, m_buf_len);
	}

	return xmlParseMemory(m_buf, m_buf_len);

}

wxString url2version(wxString url) {
	//url in form of: http://sourceforge.net/project/szarp/files/Windows/szarp-3.1.44.exe/download
	return url.BeforeLast(L'/').AfterLast(L'/').AfterFirst(L'-').BeforeLast(L'.');
}

bool parse_rss_item(xmlNodePtr node, wxString &url, wxString &version, wxString &md5, wxDateTime &datetime) {
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(node->doc);
	xp_ctx->node = node;

	int ret;
	ret = xmlXPathRegisterNs(xp_ctx,
			BAD_CAST "media",
			BAD_CAST "http://video.search.yahoo.com/mrss/");
	assert(ret == 0);

	wxString datetime_string;

	wxString* str_array[] = { &md5, &url, &datetime_string };
	const char * xpath_array[] = { "./media:content/media:hash[@algo='md5']", "./link", "./pubDate" };

	for (size_t i = 0; i < sizeof(str_array) / sizeof(str_array[0]); i++) {
		xmlNodePtr _node = uxmlXPathGetNode(BAD_CAST xpath_array[i], xp_ctx);
		if (_node == NULL || _node->xmlChildrenNode == NULL) {
				xmlXPathFreeContext(xp_ctx);
				return false;
		}
		xmlChar* _str = xmlNodeListGetString(_node->doc, _node->xmlChildrenNode, 1);
		*str_array[i] = SC::U2S(_str);
		xmlFree(_str);
	}

	if (datetime.ParseRfc822Date(datetime_string.c_str()) == NULL) {
		xmlXPathFreeContext(xp_ctx);
		return false;
	}

	version = url2version(url);

	return true;

}

bool Downloader::ParseRSS(xmlDocPtr doc) {
	xmlXPathContextPtr xp_ctx = xmlXPathNewContext(doc);

	xmlXPathObjectPtr xpath_obj = xmlXPathEvalExpression(BAD_CAST "/rss/channel/item", xp_ctx);

	bool ok = false;

	if (xpath_obj->nodesetval) {

		wxDateTime time;
		wxString md5;
		wxString version;
		wxString url;

		for (int i = 0; i < xpath_obj->nodesetval->nodeNr; i++) {
			wxDateTime itime;
			if (parse_rss_item(xpath_obj->nodesetval->nodeTab[i], url, version, md5, itime)
					&& (!time.IsValid() || itime > time)) {
				m_url = url;
				m_version = version;
				m_md5 = md5;
				time = itime;

				ok = true;
			}
		}

	}

	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeContext(xp_ctx);

	return ok;

}

bool Downloader::GetRSS() {

	xmlDocPtr doc = FetchRSS();
	if (doc == NULL)
		return false;

	bool ret = ParseRSS(doc);
	xmlFreeDoc(doc);

	return ret;
}

bool Downloader::getInstallerFile(std::wstring installer_file_path) {
	m_buf_used = 0;

	m_error = curl_easy_setopt(m_curl_handler, CURLOPT_URL, (const char*) SC::S2U(m_url).c_str());

	m_downloading = true;

	m_installer_ofstream = new std::ofstream(SC::S2A(installer_file_path).c_str(), std::ios::binary | std::ios::out | std::ios::trunc);

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
	int percentage = dlnow * 100.0 / dltotal;

	if (percentage < 0)
		percentage = 0;
	if (percentage > 100)
		percentage = 100;

	Downloader* c = (Downloader *) clientp;
	c->ProgressFileCallback((size_t)percentage);

	return 0;
}

void Downloader::ProgressFileCallback(size_t size) {
	wxCriticalSectionLocker locker(m_current_download_percentage_cs);
	m_current_download_percentage = size;
}

size_t Downloader::getCurrentDownloadPercentage() {
	wxCriticalSectionLocker locker(m_current_download_percentage_cs);
	return m_current_download_percentage;
}

