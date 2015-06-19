#ifndef __SERIALADAPTER_H_
#define __SERIALADAPTER_H_

#include "bserialport.h"
#include "tcpconn.h"

// start of SerialAdapter-specific definitions
#ifndef B921600
	#define	B921600	(B460800 + 1)
#endif

// SerialAdapter commands
#define	CMD_16		16
#define	CMD_17		17
#define	CMD_LINECTRL	18
#define	CMD_19		19
#define	CMD_20		20
#define	CMD_21		21
#define	CMD_22		22
#define	CMD_23		23
#define	CMD_24		24
#define	CMD_33		33
#define	CMD_34		34
#define	CMD_36		36
#define	CMD_37		37
#define	CMD_38		38
#define	CMD_POLLALIVE	39
#define	CMD_ALIVE	40
#define	CMD_43		43
#define	CMD_PORT_INIT	44
#define	CMD_47		47
#define	CMD_SEND_FIFO	48
#define CMD_51		51
#define CMD_52		52

// serial port configuration
#define	SERIAL_B300		0
#define	SERIAL_B600		1
#define	SERIAL_B1200		2
#define	SERIAL_B2400		3
#define	SERIAL_B4800		4
#define	SERIAL_B7200		5
#define	SERIAL_B9600		6
#define	SERIAL_B19200		7
#define	SERIAL_B38400		8
#define	SERIAL_B57600		9
#define	SERIAL_B115200		10
#define	SERIAL_B230400		11
#define	SERIAL_B460800		12
#define	SERIAL_B921600		13
#define	SERIAL_B150		14
#define	SERIAL_B134		15
#define	SERIAL_B110		16
#define	SERIAL_B75		17
#define	SERIAL_B50		18

#define	SERIAL_BITS8		3
#define	SERIAL_BITS7		2
#define	SERIAL_BITS6		1
#define	SERIAL_BITS5		0

#define	SERIAL_STOP1		0
#define	SERIAL_STOP2		4

#define	SERIAL_EVEN		8
#define	SERIAL_ODD		16
#define	SERIAL_MARK		24
#define	SERIAL_SPACE		32
#define	SERIAL_NONE		0
// end of SerialAdapter-specific definitions

/** Exception specific to SerialAdapter class. */
class SerialAdapterException : public SerialPortException { } ;

/** 
 * Base class for SerialAdapter clients.
 */
class SerialAdapter final: public BaseSerialPort, public ConnectionListener {
public:
	static const int DEFAULT_DATA_PORT = 950;
	static const int DEFAULT_CMD_PORT = 966;

	SerialAdapter(struct event_base*,
		bool enable_fifo=true, bool server_CN2500 = false);
	~SerialAdapter() override;

	/** BaseSerialPort interface */
	void Open() override;
	bool Ready() const override;
	void Close() override;
	void SetConfiguration(const SerialPortConfiguration& serial_conf) override;
	void LineControl(bool dtr, bool rts) override;
	void WriteData(const void* data, size_t size) override;

	/** ConnectionListener interface */
	void OpenFinished(const BaseConnection *conn) override;
	void ReadData(const BaseConnection *conn, const std::vector<unsigned char>& data) override;
	void ReadError(const BaseConnection *conn, short int event) override;
	void SetConfigurationFinished(const BaseConnection *conn) override
	{}

	/** Initializes client with given address:port.
	 * @param address IP address to connect to, in dot format
	 * @param port port number to connect to
	 */
	void InitTcp(std::string address, int data_port=DEFAULT_DATA_PORT,
		int cmd_port=DEFAULT_CMD_PORT);

private:
	void ProcessCmdResponse(const std::vector<unsigned char> &cmd_buf);
	void WriteCmd(std::vector<unsigned char> &cmd_buffer);

private:
	typedef enum {UNINITIALIZED, SETCONF1, SETCONF2, SETCONF3, READY} SerialConfState;

	const bool m_enable_fifo;
	const bool m_server_CN2500;

	TcpConnection* m_data_connection;	/**< connection used for data send/recv with serial port */
	TcpConnection* m_cmd_connection;		/**< connection used for communication with SerialAdapter driver */

	SerialConfState m_serial_state;		/**< state of set serial configuration communication */
	bool m_open_pending;
	std::vector<unsigned char> m_last_serial_conf_cmd;
};

#endif // __SERIALADAPTER_H__
