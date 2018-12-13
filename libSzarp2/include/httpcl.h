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

#ifndef __HTTPCL_H__
#define __HTTPCL_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <libxml/tree.h>


/** Abstract base class defining interface for HTTP client library.
 */
class szHTTPClient {
public:
	/** Initializes library. Should be called at the begining of program.
	 * Otherwise it is called on first use of any other class method.
	 * Libxml2 library should be initialized before it.
	 * @return true on success, false on error
	 */
	static bool Init();
	/** Performs library cleanup. Should be called only once, at the end
	 * of program. Mixing calls to Init() and Cleanup() can be unsupported
	 * by some implementations. */
	static void Cleanup();
	/** Creates new library handler. You are encouraged to reuse this
	 * handler as much as possible - it can speed up connecting, reuse
	 * existing HTTP/1.1 connections and so on. */
	szHTTPClient();
	/** Destroys library handler. */
	virtual ~szHTTPClient() = 0;
	/** Performs GET operation. Will block until all data is received.
	 * @param uri unescaped full uri to resource, including protocol
	 * @param ret_len size of server response if different from NULL
	 * @return response buffer copy - you have to free it using free();
	 * NULL on error - @see GetError(); if response is empty string "\000"
	 * is returned. Warning - returned buffer may be not 0-terminated.
	 */
	virtual char* Get(const char *uri, size_t* ret_len, char *userpwd, int timeout = -1) = 0;
	/** Performs GET operation and tries to parse response as XML
	 * document.
	 * @param uri unescaped full uri to resource
	 * @return pointer to newly created parsed XML Document or NULL on
	 * error - @see GetError()
	 */
	virtual xmlDocPtr GetXML(const char* uri, char *userpwd = NULL, int timeout = -1 ) = 0;
	/** Performs PUT operation.
	 * @param uri unescaped full uri to resource
	 * @param buffer data to send
	 * @param len size of buffer in bytes
	 * @param ret_len size of return data if not NULL
	 * @return pointer to copy of return data or NULL on error
	 */
	virtual char* Put(const char *uri, char *buffer, size_t len, char *userpw = NULL, size_t* ret_len = NULL) = 0;
	/** Performs PUT operation sending XML document.
	 * @param uri unescaped full uri to resource
	 * @param doc pointer to document to send
	 * @param ret_len size of response if not NULL
	 * @return pointer to copy of response or NULL on error
	 */
	virtual char* PutXML(const char *uri, xmlDocPtr doc, char *userpw = NULL, size_t* ret_len = NULL) = 0;

	virtual char* PostXML(const char *uri, xmlDocPtr doc, char *userpwd = NULL, size_t* ret_len = NULL) = 0;
	/** Returns error code. You should check for error code only if error
	 * occured - successfull operations doesn't clear previous error code.
	 * 0 means no error, -1 is reserved for XML parsing error; positive
	 * error codes are implementation specific, these can be, for example,
	 * HTTP error codes from server
	 * @return error code
	 */
	virtual int GetError() = 0;
	/** Gets aditional info about last error (for example, location of XML
	 * parser error or HTTP server error string).
	 * @return pointer to static buffer containing error code, you should
	 * copy it if you want to use it later
	 */
	virtual const char* GetErrorStr() = 0;
protected:
	static bool ms_initialized;	/**< flag - is library initialized? */
};


#ifndef NO_CURL

#include <curl/curl.h>

/** Implementation of szHTTPClient class using curl library 'easy' interface.
 */

class szHTTPCurlClient : public szHTTPClient {
public:
	static bool Init();
	static void Cleanup();
	szHTTPCurlClient();
	~szHTTPCurlClient() override;
	int GetError() override;
	const char* GetErrorStr() override;
	/** This is write callback function, called by library on data
	 * received. */
	static size_t WriteFunction(void* ptr, size_t size, size_t nmemb, void *object);
	/** This is read callback function, called when library want to send
	 * data. */
	static size_t ReadFunction(void* ptr, size_t size, size_t nmemb, void *object);
	char* Get(const char *uri, size_t* ret_len, char *userpwd = NULL, int timeout = -1) override;
	xmlDocPtr GetXML(const char* uri, char *userpwd, int timeout = -1) override;
	char* Put(const char *uri, char *buffer, size_t len, char *userpwd = NULL, size_t* ret_len = NULL) override;
	virtual char* Post(const char *uri, char *buff, size_t len, int timeout, char *userpwd, size_t* ret_len);
	char* PostXML(const char *uri, xmlDocPtr doc, char *userpwd = NULL, size_t* ret_len = NULL) override;
	char* PutXML(const char *uri, xmlDocPtr doc, char *userpwd = NULL, size_t* ret_len = NULL) override;
protected:
	/** Actual write callback method, saves data in buffer.
	 * @param data pointer to data to save
	 * @param size number of bytes to save
	 * @return size of data saved (equal to size argument)*/
	size_t WriteCallback(char* data, size_t size);
	/** Actual read callback method, reads data from buffer.
	 * @param data pointer to output buffer 
	 * @param size size of output buffer
	 * @return number of bytes saved to buffer */
	size_t ReadCallback(char* data, size_t size);
	CURL* m_curl;		/**< Handle for CURL library */
	CURL* m_curl_get;		/**< Handle for CURL library for GET method */
	int m_error;		/**< Last error code. */
	char m_err_buf[CURL_ERROR_SIZE];	
				/**< Buffer for error messages. */
	char* m_buf;		/**< Buffer for data received. */
	size_t m_buf_len;	/**< Current size of buffer. */
	size_t m_buf_used;	/**< Current amount of data in buffer. */
	char* m_pbuf;		/**< Buffer for data send by PUT. */
	size_t m_pbuf_len;	/**< Size of PUT buffer. */
	size_t m_pbuf_send;	/**< Size of data already send. */
};


#endif /* NO_CURL */

#endif /* __HTTPCL_H__ */

