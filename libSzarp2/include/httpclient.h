#ifndef __HTTPCLIENT_H__
#define __HTTPCLIENT_H__

#include "serialportconf.h"
#include <event2/http.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/keyvalq_struct.h>
#include <map>
#include <vector>
#include <string>

std::string get_header_value(struct evhttp_request *req, const std::string& key);

std::string get_atc_auth_cookie(struct evhttp_request *req);

typedef void(*http_request_cb)(struct evhttp_request *, void *);

class HttpClient
{
public:
	HttpClient(struct event_base *base);
	virtual ~HttpClient();
	void Connect(const std::string ip, const int port=80);
	void CloseConnection();
	void SendRequest(http_request_cb callback, std::string uri,
		const std::map<std::string, std::string>& headers=std::map<std::string, std::string>(),
		int timeout_s=10);

protected:
	struct event_base* m_event_base;
	struct evhttp_connection* m_connection;
};


// forward declaration for AtcHttpClientListener
class AtcHttpClient;

class AtcHttpClientListener
{
public:
	virtual void GetAuthCookieFinished(const AtcHttpClient* client) = 0;
	virtual void SetConfigurationFinished(const AtcHttpClient* client) = 0;
	virtual void ResetDeviceFinished(const AtcHttpClient* client) = 0;
};


class AtcHttpClient: public HttpClient
{
public:
	AtcHttpClient(struct event_base *base)
	:HttpClient(base), m_auth_cookie(""), m_close_on_reset(false)
	{}

	/** Get auth cookie via cgi script using default credentials. */
	void GetAuthCookie();

	/** Sets configuration to ATC UART via cgi script. Auth cookie must be obtained first */
	void SetConfiguration(const SerialPortConfiguration& conf);

	/** Resets device via cgi script. Auth cookie must be obtained first */
	void ResetDevice(bool close_on_reset=false);

	/** subscribe listener for notifications */
	void AddListener(AtcHttpClientListener* listener)
	{
		m_listeners.push_back(listener);
	}
	/** Removes all listeners */
	void ClearListeners()
	{
		m_listeners.clear();
	}
private:
	static void GetAuthCookieFinishedCallback(struct evhttp_request* request, void* obj)
	{
		reinterpret_cast<AtcHttpClient*>(obj)->GetAuthCookieFinished(request);
	}
	void GetAuthCookieFinished(struct evhttp_request* request);

	static void SetConfigurationFinishedCallback(struct evhttp_request* request, void* obj)
	{
		reinterpret_cast<AtcHttpClient*>(obj)->SetConfigurationFinished(request);
	}
	void SetConfigurationFinished(struct evhttp_request* request);

	static void ResetDeviceFinishedCallback(struct evhttp_request* request, void* obj)
	{
		reinterpret_cast<AtcHttpClient*>(obj)->ResetDeviceFinished(request);
	}
	void ResetDeviceFinished(struct evhttp_request* request);

private:
	std::string m_auth_cookie;
	std::vector<AtcHttpClientListener*> m_listeners;
	bool m_close_on_reset;
};

#endif // __HTTPCLIENT_H__
