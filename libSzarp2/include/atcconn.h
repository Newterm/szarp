#ifndef __ATCCONN_H_
#define __ATCCONN_H_

#include "tcpconn.h"
#include "httpclient.h"

/** 
 * Serial client communicating through ATC1000 TCP to RS converter.
 * Sets serial port baudrate using http protocol and auth cookie.
 */
class AtcConnection final: public TcpConnection, public AtcHttpClientListener {
public:
	static const int DEFAULT_DATA_PORT = 23;
	static const int DEFAULT_CONTROL_PORT = 80;

	AtcConnection(struct event_base* base);
	~AtcConnection() override;

	void InitTcp(const std::string& address,
		int data_port=DEFAULT_DATA_PORT,
		int control_port=DEFAULT_CONTROL_PORT);

	/* BaseConnection interface */
	void Open() override;
	void Close() override;
	void SetConfiguration(const SerialPortConfiguration& serial_conf) override;

	/* AtcHttpClientListener interface */
	void GetAuthCookieFinished(const AtcHttpClient* client) override;
	void SetConfigurationFinished(const AtcHttpClient* client) override;
	void ResetDeviceFinished(const AtcHttpClient* client) override;
	void AtcError(const AtcHttpClient* client) override;
protected:
	void OpenFinished() override;

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
	int m_control_port;
};

#endif // __ATCCONN_H__
