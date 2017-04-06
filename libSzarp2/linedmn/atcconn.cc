#include "atcconn.h"

AtcConnection::AtcConnection(struct event_base* base)
	:TcpConnection(base), m_event_base(base), m_action_queued(NO_ACTION),
	m_open_finished_pending(true), m_control_port(DEFAULT_CONTROL_PORT)
{
	m_http_client = new AtcHttpClient(base);
	m_http_client->AddListener(this);
}

AtcConnection::~AtcConnection()
{
	m_http_client->ClearListeners();
	delete m_http_client;
}

void AtcConnection::InitTcp(std::string address,
	int data_port, int control_port)
{
	TcpConnection::InitTcp(address, data_port);
	m_control_port = control_port;
}

void AtcConnection::AtcError(const AtcHttpClient* client)
{
	// use "unrecoverable error encountered"
	TcpConnection::Error(NULL, BEV_EVENT_ERROR);
}

void AtcConnection::Open()
{
	TcpConnection::Open();
	m_http_client->Connect(m_address, m_control_port);
	m_action_queued = RESET;
	m_http_client->GetAuthCookie();
}

void AtcConnection::OpenFinished()
{
	if (m_action_queued == RESET) {
		m_open_finished_pending = true;
	} else {
		TcpConnection::OpenFinished();
		m_open_finished_pending = false;
	}
}

void AtcConnection::Close()
{
	TcpConnection::Close();
	m_http_client->CloseConnection();
	m_open_finished_pending = false;
}

void AtcConnection::SetConfiguration(const SerialPortConfiguration& serial_conf)
{
	m_requested_serial_conf = serial_conf;
	m_action_queued = SET_CONF;
	m_http_client->GetAuthCookie();
}

void AtcConnection::GetAuthCookieFinished(const AtcHttpClient* client)
{
	switch (m_action_queued) {
		case SET_CONF:
			m_http_client->SetConfiguration(m_requested_serial_conf);
			break;
		case RESET:
			m_http_client->ResetDevice();
			break;
		case NO_ACTION:
			break;
	}
	m_action_queued = NO_ACTION;
}

void AtcConnection::SetConfigurationFinished(const AtcHttpClient* client)
{
	TcpConnection::SetConfigurationFinished();
}

void AtcConnection::ResetDeviceFinished(const AtcHttpClient* client)
{
	if (m_open_finished_pending) {
		TcpConnection::OpenFinished();
	}
}
