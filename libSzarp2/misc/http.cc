/* 
  SZARP: SCADA software 

*/
/*
 * Simple HTTP client library.
 *
 * SZARP

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * $Id$
 */

#include "httpcl.h"

#include <assert.h>
#include <string.h>

bool szHTTPClient::ms_initialized = false;

bool szHTTPClient::Init() 
{
	ms_initialized = true;
	return true;
}

void szHTTPClient::Cleanup()
{
	ms_initialized = false;
}

szHTTPClient::szHTTPClient()
{
	if (!ms_initialized) {
		szHTTPClient::Init();
	}
}

szHTTPClient::~szHTTPClient()
{
}

#ifndef NO_CURL

bool szHTTPCurlClient::Init()
{
	assert (!ms_initialized);
	if (!szHTTPClient::Init())
		return false;;
	curl_global_init(CURL_GLOBAL_ALL);
	return true;
}

void szHTTPCurlClient::Cleanup()
{
	assert (ms_initialized);
	curl_global_cleanup();
	szHTTPClient::Cleanup();
}

szHTTPCurlClient::szHTTPCurlClient() :
	szHTTPClient(),	m_buf(NULL), m_buf_len(0)
{
	if (!ms_initialized) {
		assert (Init());
	}
	m_curl = curl_easy_init();
	assert (m_curl);
	m_curl_get = curl_easy_init();
	assert (m_curl_get);
	m_error = curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_err_buf);
}

szHTTPCurlClient::~szHTTPCurlClient()
{
	if (m_buf) free(m_buf);
	curl_easy_cleanup(m_curl);
}

int szHTTPCurlClient::GetError()
{
	return m_error;
}

const char* szHTTPCurlClient::GetErrorStr()
{
	if (m_error < 0) {
		return m_err_buf;
	} else {
		return curl_easy_strerror((CURLcode)m_error); 
	}
}

char* szHTTPCurlClient::Get(const char *uri, size_t* ret_len, char *userpwd, int timeout)
{
	assert (uri);
	m_buf_used = 0;
	m_error = curl_easy_setopt(m_curl_get, CURLOPT_URL, uri);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl_get, CURLOPT_WRITEFUNCTION, WriteFunction);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl_get, CURLOPT_READFUNCTION, ReadFunction);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl_get, CURLOPT_WRITEDATA, (void*)this);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl_get, CURLOPT_READDATA, (void*)this);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl_get, CURLOPT_HTTPGET, 1);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl_get, CURLOPT_FOLLOWLOCATION, 1);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl_get, CURLOPT_MAXREDIRS, (long)5);
	if (m_error) return NULL;
	curl_easy_setopt(m_curl_get, CURLOPT_FORBID_REUSE, 1);
#ifdef CURL_SSL
	curl_easy_setopt(m_curl_get, CURLOPT_USE_SSL, CURLUSESSL_TRY);
	curl_easy_setopt(m_curl_get, CURLOPT_SSL_VERIFYPEER, long(0));
#endif
	if (timeout != -1) {
		m_error = curl_easy_setopt(m_curl_get, CURLOPT_TIMEOUT, long(timeout));
		if (m_error) return NULL;

	}
#ifdef MINGW32
	else {
		m_error = curl_easy_setopt(m_curl_get, CURLOPT_TIMEOUT, long(10));
		if (m_error) return NULL;
	}
#endif
	if (userpwd) {
		m_error = curl_easy_setopt(m_curl_get, CURLOPT_USERPWD, userpwd);
		if (m_error) return NULL;
	}
	m_error = curl_easy_perform(m_curl_get);
	if (m_error) return NULL;
	long code;
	curl_easy_getinfo(m_curl_get, CURLINFO_RESPONSE_CODE, &code);
	if ((code / 100) != 2) {
		return NULL;
	}
	
	if (ret_len) {
		*ret_len = m_buf_used;
	}
	if (m_buf_len > ((m_buf_used + 1) * 4)) {
		m_buf_len = m_buf_len / 2;
		m_buf = (char *) realloc(m_buf, m_buf_len);
	}
	char *ret = (char *) malloc(m_buf_used);
	assert (ret);
	memcpy(ret, m_buf, m_buf_used);
	return ret;
}

char* szHTTPCurlClient::Post(const char *uri, char *buff, size_t len, int timeout, char *userpwd, size_t* ret_len)
{
	assert (uri);
	m_buf_used = 0;
	m_error = curl_easy_setopt(m_curl, CURLOPT_URL, uri);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteFunction);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, ReadFunction);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)this);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_READDATA, (void*)this);
	if (m_error) return NULL;

	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: text/xml");

	m_error =  curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, buff);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, len);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, (long)5);
	if (m_error) return NULL;
	if (timeout != -1) {
		m_error = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, long(timeout));
		if (m_error) return NULL;

	}
#ifdef MINGW32
	else {
		m_error = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, long(10));
		if (m_error) return NULL;
	}
#endif
	curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1);
#ifdef CURL_SSL
	curl_easy_setopt(m_curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, long(0));
#endif
	if (userpwd) {
		m_error = curl_easy_setopt(m_curl, CURLOPT_USERPWD, userpwd);
		if (m_error) return NULL;
	}
	m_error = curl_easy_perform(m_curl);

	curl_slist_free_all(headers);

	if (m_error) return NULL;
	long code;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &code);
	if ((code / 100) != 2) {
		return NULL;
	}
	
	if (ret_len) {
		*ret_len = m_buf_used;
	}
	if (m_buf_len > ((m_buf_used + 1) * 4)) {
		m_buf_len = m_buf_len / 2;
		m_buf = (char *) realloc(m_buf, m_buf_len);
	}
	char *ret = (char *) malloc(m_buf_used + 1);
	assert (ret);
	memcpy(ret, m_buf, m_buf_used);
	return ret;
}

size_t szHTTPCurlClient::WriteFunction(void* ptr, size_t size, size_t nmemb, 
		void *object)
{
	assert (object);
	szHTTPCurlClient* c = (szHTTPCurlClient *) object;
	return (c->WriteCallback((char *)ptr, size * nmemb));
}

size_t szHTTPCurlClient::WriteCallback(char* data, size_t size)
{
	if (m_buf_used + size > m_buf_len) {
		m_buf_len = m_buf_used + size + 1;
		m_buf = (char *)realloc(m_buf, m_buf_len);
		assert (m_buf);
	}
	memcpy(m_buf + m_buf_used, data, size);
	m_buf_used += size;
	return size;
}

size_t szHTTPCurlClient::ReadFunction(void* ptr, size_t size, size_t nmemb, 
		void *object)
{
	assert (object);
	szHTTPCurlClient* c = (szHTTPCurlClient *) object;
	return (c->ReadCallback((char *)ptr, size * nmemb));
}

size_t szHTTPCurlClient::ReadCallback(char* data, size_t size)
{
	size_t len = m_pbuf_len - m_pbuf_send;
	if (len > size) {
		len = size;
	}
	memcpy(data, m_pbuf + m_pbuf_send, len);
	m_pbuf_send += len;
	printf("SEND: %zd\n", len);
	return len;
}

xmlDocPtr szHTTPCurlClient::GetXML(const char* uri, char *userpwd, int timeout)
{
	size_t l;
	char* buf = Get(uri, &l, userpwd, timeout);
	if (buf == NULL) {
		return NULL;
	}
	xmlDocPtr doc = xmlParseMemory(buf, l);
	if (doc == NULL) {
		m_error = -1;
		xmlErrorPtr err = xmlGetLastError();
		if (err == NULL) {
			snprintf(m_err_buf, CURL_ERROR_SIZE, "Unknown XML parsing error");
		} else {
			snprintf(m_err_buf, CURL_ERROR_SIZE, "%s (line %d col %d)", 
					err->message, err->line, err->int2);
		}
		m_err_buf[CURL_ERROR_SIZE-1] = 0;
	}
	free(buf);
	return doc;
}

char* szHTTPCurlClient::Put(const char *uri, char *buffer, size_t len, char *userpwd, size_t* ret_len)
{
	assert (uri);
	m_buf_used = 0;
	m_pbuf = buffer;
	m_pbuf_len = len;
	m_pbuf_send = 0;
	m_error = curl_easy_setopt(m_curl, CURLOPT_URL, uri);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteFunction);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, ReadFunction);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void*)this);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_READDATA, (void*)this);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_INFILESIZE, (long)len);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);
	if (m_error) return NULL;
	m_error =  curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, (long)5);
	if (m_error) return NULL;
#ifdef MINGW32
	m_error = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, long(10));
	if (m_error) return NULL;
#endif

	curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 1);
#ifdef CURL_SSL
	curl_easy_setopt(m_curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);
	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, long(0));
#endif
	if (userpwd)
		m_error = curl_easy_setopt(m_curl, CURLOPT_USERPWD, userpwd);
	curl_slist *slist=NULL;
	slist = curl_slist_append(slist, "Expect:");
	m_error = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, slist);
	if (m_error) return NULL;
	m_error = curl_easy_perform(m_curl);
	m_error = curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, NULL);
	curl_slist_free_all(slist);
	if (m_error) return NULL;
	long code;
	curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &code);
	if ((code / 100) != 2) {
		return NULL;
	}
	
	if (ret_len) {
		*ret_len = m_buf_used;
	}
	if ((m_buf_len > ((m_buf_used + 1) * 4)) && (m_buf_len > 1024)) {
		m_buf_len = m_buf_len / 2;
		m_buf = (char *) realloc(m_buf, m_buf_len);
	} else if (m_buf == NULL) {
		m_buf_len = 1024;
		m_buf = (char *) realloc(m_buf, m_buf_len);
	}
	char *ret = (char *) malloc(m_buf_used + 1);
	assert (ret);
	memcpy(ret, m_buf, m_buf_used);
	return ret;
}

char* szHTTPCurlClient::PostXML(const char *uri, xmlDocPtr doc, char *userpwd, size_t* ret_len)
{
	assert (uri);
	assert (doc);

	xmlChar* buf;
	int l;
	char *ret;
	
	xmlDocDumpMemory(doc, &buf, &l);
	assert (buf);
	l++;
	ret = Post(uri, (char *)buf, (size_t)l, 0, userpwd, ret_len);
	xmlFree(buf);
	return ret;
}


char* szHTTPCurlClient::PutXML(const char *uri, xmlDocPtr doc, char* userpw, size_t* ret_len)
{
	assert (uri);
	assert (doc);

	xmlChar* buf;
	int l;
	char *ret;
	
	xmlDocDumpMemory(doc, &buf, &l);
	assert (buf);
	l++;
	ret = Put(uri, (char *)buf, (size_t)l, userpw, ret_len);
	xmlFree(buf);
	return ret;
}

#endif /* NO_CURL */
