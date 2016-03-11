#include "httpclient.h"
#include <event2/http_struct.h>

std::string get_header_value(struct evhttp_request *req, const std::string& key)
{
	struct evkeyvalq* headers = evhttp_request_get_input_headers(req);
	for (auto header = headers->tqh_first; header; header = header->next.tqe_next) {
		if (std::string(header->key) == key) {
			return std::string(header->value);
		}
	}
	return "";
}

std::string get_atc_auth_cookie(struct evhttp_request *req)
{
	const std::string value_string = get_header_value(req, "Set-Cookie");
	return value_string.substr(0, value_string.find(";"));
}

HttpClient::HttpClient(struct event_base *base)
:m_event_base(base), m_connection(NULL)
{}

HttpClient::~HttpClient()
{
	CloseConnection();
}

void HttpClient::Connect(const std::string ip, const int port)
{
	CloseConnection();
	m_connection = evhttp_connection_base_new(m_event_base, NULL, ip.c_str(), port);
}

void HttpClient::CloseConnection()
{
	if (m_connection != NULL) {
		evhttp_connection_free(m_connection);
		m_connection = NULL;
	}
}

bool HttpClient::IsClosed() const
{
	return m_connection == NULL;
}

int HttpClient::SendRequest(http_request_cb callback, std::string uri,
	const std::map<std::string, std::string>& headers, int timeout_s)
{
	if (m_connection == NULL) {
		return - 1;
	}
	struct evhttp_request* request = evhttp_request_new(callback, this);
	for (auto it = headers.begin(); it != headers.end(); ++it) {
		evhttp_add_header(request->output_headers, it->first.c_str(), it->second.c_str());
	}
	const int ret = evhttp_make_request(m_connection, request, EVHTTP_REQ_GET, uri.c_str());
	if (ret != 0) {
		return - 1;
	}
	evhttp_connection_set_timeout(request->evcon, timeout_s);
	return 0;
}

int AtcHttpClient::SendRequest(http_request_cb callback, std::string uri,
	const std::map<std::string, std::string>& headers, int timeout_s)
{
	const int ret = HttpClient::SendRequest(callback, uri, headers, timeout_s);
	if (ret != 0) {
		NotifyError();
	}
	return ret;
}

void AtcHttpClient::GetAuthCookie()
{
	SendRequest(GetAuthCookieFinishedCallback, "/cgi/login.cgi?Username=admin&Password=system");
}

void AtcHttpClient::GetAuthCookieFinished(struct evhttp_request* request)
{
	if (IsClosed()) {
		return;
	}
	if (request == NULL) {
		NotifyError();
		return;
	}
	if (evhttp_request_get_response_code(request) != 200) {
		NotifyError();
		return;
	}
	const std::string cookie_str = get_header_value(request, "Set-Cookie");
	m_auth_cookie = cookie_str.substr(0, cookie_str.find(";"));
	for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		(*it)->GetAuthCookieFinished(this);
	}
}

void AtcHttpClient::NotifyError()
{
	for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		(*it)->AtcError(this);
	}
}

void AtcHttpClient::SetConfiguration(const SerialPortConfiguration& conf)
{
	const std::map<std::string, std::string> headers = {{"Cookie", m_auth_cookie.c_str()}, {"Connection", "keep-alive"}};
	const int char_size = conf.GetCharSizeInt();
	int parity_code = 0;
	switch (conf.parity) {
		case SerialPortConfiguration::NONE:
			parity_code = 0;
			break;
		case SerialPortConfiguration::ODD:
			parity_code = 8;
			break;
		case SerialPortConfiguration::EVEN:
			parity_code = 24;
			break;
		case SerialPortConfiguration::MARK:
			parity_code = 40;
			break;
		case SerialPortConfiguration::SPACE:
			parity_code = 56;
			break;
	}
	int stop_bits_code = 0;
	if (conf.stop_bits > 1) {
		stop_bits_code = 1;
	}
	const std::string uri = "/cgi/uart.cgi?mode=0&set_baudrate=" + std::to_string(conf.speed) +
		"&set_cb=" + std::to_string(char_size) +
		"&set_pt=" + std::to_string(parity_code) +
		"&set_sb=" + std::to_string(stop_bits_code) +
		"&set_hf=0&D1V=00&D2V=FF&D3V=5&Modify=Update";
	SendRequest(SetConfigurationFinishedCallback, uri, headers);
}

void AtcHttpClient::SetConfigurationFinished(struct evhttp_request* request)
{
	if (IsClosed()) {
		return;
	}
	for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		(*it)->SetConfigurationFinished(this);
	}
}

void AtcHttpClient::ResetDevice(bool close_on_reset)
{
	m_close_on_reset = close_on_reset;
	SendRequest(ResetDeviceFinishedCallback, "/cgi/reset.cgi?back=Reset&reset=ture");
}

void AtcHttpClient::ResetDeviceFinished(struct evhttp_request* request)
{
	if (IsClosed()) {
		return;
	}
	if (m_close_on_reset) {
		CloseConnection();
		m_close_on_reset = false;
	}
	for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		(*it)->ResetDeviceFinished(this);
	}
}
