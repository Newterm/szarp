#ifndef __ATCCONN_H_
#define __ATCCONN_H_

#include "tcpconn.h"
#include "httpclient.h"

/** 
 * Serial client communicating through ATC1000 TCP to RS converter.
 * Sets serial port baudrate using http protocol and auth cookie.
 */
class AtcConnection : public TcpConnection, public AtcHttpClientListener {
public:
	AtcConnection(struct event_base* base);
	virtual ~AtcConnection();

	void InitTcp(std::string address)
	{
		TcpConnection::InitTcp(address, 23);
	}
	/* BaseConnection interface */
	virtual void Open();
	virtual void Close();
	virtual void SetConfiguration(const SerialPortConfiguration& serial_conf);

	/* AtcHttpClientListener interface */
	virtual void GetAuthCookieFinished(const AtcHttpClient* client);
	virtual void SetConfigurationFinished(const AtcHttpClient* client);
	virtual void ResetDeviceFinished(const AtcHttpClient* client);
protected:
	virtual void OpenFinished();

protected:
	struct event_base* m_event_base;
	AtcHttpClient* m_http_client;
	SerialPortConfiguration m_requested_serial_conf;

	enum ActionQueued {
		NO_ACTION,
		SET_CONF,
		RESET
	} m_action_queued;

	bool m_open_finished_pending;
};

#endif // __ATCCONN_H__
