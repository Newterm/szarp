/* 
  SZARP: SCADA software 

*/
/* 
 * szbase - data buffer
 * $Id$
 * <pawel@praterm.com.pl>
 */

#include "config.h"

#ifdef MING32
#include <windows.h>
#endif
#include <boost/asio.hpp>

class ProberConnection {
	szb_buffer_t* m_buffer;

	boost::asio::io_service m_io_service;
	boost::asio::ip::tcp::resolver m_resolver;
	boost::asio::deadline_timer m_deadline;
	boost::asio::ip::tcp::socket m_socket;

	boost::asio::streambuf m_output_buffer, m_input_buffer;

	std::string m_address;
	std::string m_port;

	bool m_connected;

	time_t m_from;
	time_t m_to;
	int m_direction;
	std::wstring m_path;

	time_t m_search_result;
	time_t m_server_time;
	time_t m_start_time;
	time_t m_end_time;
	int m_values_count;

	enum { SEND_SEARCH, SEND_GET, SEND_BOUNDARY_TIME } m_operation; 

	enum { IDLE, RESOLVING, CONNECTING, PERFORMING_OPERATION } m_state; 

	std::wstring m_error;

	bool m_timeout;

public:
	ProberConnection(szb_buffer_t* buffer, std::string address, std::string port);

	void Connect();

	void TimeoutHandler(const boost::system::error_code& error);

	bool IsError();

	const std::wstring& Error();

	void ClearError();

	void ResetBuffers();

	void HandleResolve(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator &i);

	void SendSearchQuery();

	void SendGetQuery();

	void SendRangeQuery();

	void SendQuery();

	void HandleConnect(const boost::system::error_code& error);

	void HandleWrite(const boost::system::error_code &error);

	std::string ReadLine();

	void HandleGetFirstLine(const boost::system::error_code &error);

	void HandleReadValues();

	void HandleReadValues(const boost::system::error_code &error, size_t bytes_transferred);

	void HandleRangeResponse(const boost::system::error_code &error);

	void HandleSearchResponse(const boost::system::error_code &error);

	time_t Search(time_t from, time_t to, int direction, std::wstring path);

	int Get(time_t from, time_t to, std::wstring path);

	int GetData(short *buffer, int count);

	time_t GetServerTime();

	void PerformOperation();

	bool GetRange(time_t &first_date, time_t& end_date);

	void Go();

	void Disconnect();

	void StartTimer();
	
	void StopTimer();

};
