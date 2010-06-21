#include "config.h"

#include "liblog.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include "szbbase.h"
#include "conversion.h"
#include "szbdefines.h"
#include "szbase/szbbuf.h"
#include "proberconnection.h"

ProberConnection::ProberConnection(szb_buffer_t *buffer, std::string address, std::string port) : m_buffer(buffer), m_io_service(), m_resolver(m_io_service), m_deadline(m_io_service), m_socket(m_io_service), m_address(address), m_port(port), m_connected(false), m_state(IDLE), m_timeout(false), m_last_range_query_id(-1) {
}

void ProberConnection::Connect() {
	boost::asio::ip::tcp::resolver::query query(m_address, m_port);
	m_resolver.async_resolve(query, boost::bind(&ProberConnection::HandleResolve, this, _1, _2));
	m_state = RESOLVING;
}

void ProberConnection::TimeoutHandler(const boost::system::error_code& error) {
	if (error == boost::asio::error::operation_aborted)
		return;
	sz_log(4, "Timeout handler called, %s", error.message().c_str());
	switch (m_state) {
		case IDLE:
			return;
		case RESOLVING:
			m_resolver.cancel();
			break;
		case CONNECTING:
		case PERFORMING_OPERATION:
			m_socket.cancel();
			break;
	}
	m_timeout = true;
}

bool ProberConnection::IsError() {
	return !m_error.empty();
}

void ProberConnection::ClearError() {
	m_error.clear();
	m_timeout = false;
}

const std::wstring& ProberConnection::Error() {
	return m_error;
}

void ProberConnection::ResetBuffers() {
	m_input_buffer.consume(m_input_buffer.size());
	m_output_buffer.consume(m_output_buffer.size());
}

void ProberConnection::HandleResolve(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator &i) {
	if (error == boost::asio::error::operation_aborted)
		return;
	if (error) {
		m_error = SC::A2S(error.message());
		StopTimer();
		return;
	}
	sz_log(10, "Resolved address");
	m_state = CONNECTING;
	m_socket.async_connect(*i, boost::bind(&ProberConnection::HandleConnect, this, _1));
}

void ProberConnection::SendRangeQuery() {
	ResetBuffers();

	std::ostream ostream(&m_output_buffer);
	ostream << "RANGE" << "\r\n";

	sz_log(10, "Sending range query");

	boost::asio::async_write(m_socket, m_output_buffer, 
		boost::bind(&ProberConnection::HandleWrite, this, _1));

	boost::asio::async_read_until(m_socket, m_input_buffer, "\n",
			boost::bind(&ProberConnection::HandleRangeResponse, this, _1));
}

void ProberConnection::SendSearchQuery() {
	ResetBuffers();

	sz_log(10, "Sending search query, from: %ld, to: %ld, path: %ls", m_from, m_to, m_path.c_str());

	std::ostream ostream(&m_output_buffer);
	ostream << "SEARCH "
		<< m_from << " "
		<< m_to << " "
		<< m_direction << " "
		<< SC::S2A(m_path) << "\r\n";

	boost::asio::async_write(m_socket, m_output_buffer, 
		boost::bind(&ProberConnection::HandleWrite, this, _1));

	boost::asio::async_read_until(m_socket, m_input_buffer, "\n",
			boost::bind(&ProberConnection::HandleSearchResponse, this, _1));
}

void ProberConnection::SendGetQuery() {
	ResetBuffers();

	std::ostream ostream(&m_output_buffer);
	ostream << "GET "
		<< m_from << " "
		<< m_to << " "
		<< SC::S2A(m_path) << "\r\n";

	sz_log(10, "Sending get query, from: %ld, to: %ld, path: %ls", m_from, m_to, m_path.c_str());

	boost::asio::async_write(m_socket, m_output_buffer, 
		boost::bind(&ProberConnection::HandleWrite, this, _1));

	boost::asio::async_read_until(m_socket, m_input_buffer, "\n",
			boost::bind(&ProberConnection::HandleGetFirstLine, this, _1));
}

void ProberConnection::SendQuery() {
	m_state = PERFORMING_OPERATION;
	switch (m_operation) {
		case SEND_SEARCH:
			SendSearchQuery();
			break;
		case SEND_GET:
			SendGetQuery();
			break;
		case SEND_BOUNDARY_TIME:
			SendRangeQuery();
			break;
	}
}

void ProberConnection::HandleConnect(const boost::system::error_code& error) {
	if (error == boost::asio::error::operation_aborted)
		return;
	if (error) {
		m_error = SC::A2S(error.message());
		StopTimer();
		return;
	}
	sz_log(10, "connected to server");
	m_connected = true;
	SendQuery();
}

void ProberConnection::HandleWrite(const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted)
		return;
	if (error) {
		m_error = SC::A2S(error.message());
		StopTimer();
		return;
	}
	sz_log(10, "Successfully written data to server");
}

void ProberConnection::HandleRangeResponse(const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted)
		return;
	if (error) {
		m_error = SC::A2S(error.message());
		StopTimer();
		return;
	}
	sz_log(10, "Got response to range query");

	std::string line = ReadLine();
	std::istringstream is(line);
	is >> m_start_time;
	is >> m_end_time;
	StopTimer();
}

std::string ProberConnection::ReadLine() {
	std::string line;
	std::istream istream(&m_input_buffer);
	std::getline(istream, line);
	return line;
}

void ProberConnection::HandleGetFirstLine(const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted)
		return;
	if (error) {
		m_error = SC::A2S(error.message());
		StopTimer();
		return;
	}
	std::string line = ReadLine();
	sz_log(10, "First line of response to get query: %s", line.c_str());
	if (line.substr(0, sizeof("ERROR") - 1) != "ERROR") {
		std::istringstream is(line); 
		size_t length;
		is >> m_server_time >> length;
		sz_log(10, "Reading %zu bytes of data, %zu already in buffer", length, m_input_buffer.size());
		m_values_count = length / 2;
		if (m_input_buffer.size() == length)
			HandleReadValues();
		else
			boost::asio::async_read(m_socket,
				m_input_buffer.prepare(length - m_input_buffer.size()),
				boost::asio::transfer_at_least(length - m_input_buffer.size()),
				boost::bind(&ProberConnection::HandleReadValues, this, _1, _2));
	} else {
		m_error = std::wstring(L"Error performing operation on prober ") + SC::A2S(line.substr(sizeof("ERROR")));
		StopTimer();
	}
}

void ProberConnection::HandleReadValues() {
	StopTimer();
}

void ProberConnection::HandleReadValues(const boost::system::error_code &error, size_t bytes_transferred) {
	if (error == boost::asio::error::operation_aborted)
		return;
	sz_log(10, "Reading remaining %zu bytes", bytes_transferred);
	m_input_buffer.commit(bytes_transferred);
	HandleReadValues();
}

void ProberConnection::HandleSearchResponse(const boost::system::error_code &error) {
	if (error == boost::asio::error::operation_aborted)
		return;
	if (error) {
		m_error = SC::A2S(error.message());
		StopTimer();
		return;
	}
	std::string line;
	time_t response;
	std::istream is(&m_input_buffer);
	std::getline(is, line);
	std::istringstream(line) >> response;
	sz_log(10, "Found time: %d", (int) response);
	m_search_result = response;
	StopTimer();
}

time_t ProberConnection::Search(time_t from, time_t to, int direction, std::wstring path) {
	ClearError();
	m_operation = SEND_SEARCH;
	m_from = from;
	m_to = to;
	m_direction = direction;
	m_path = path;
	PerformOperation();
	if (IsError()) {
		m_buffer->last_err = SZBE_CONN_ERROR;
		m_buffer->last_err_string = m_error;
		m_search_result = -1;
	}
	m_state = IDLE;
	return m_search_result;
}



int ProberConnection::Get(time_t from, time_t to, std::wstring path) {
	ClearError();
	m_operation = SEND_GET;
	m_from = from;
	m_to = to;
	m_path = path;
	PerformOperation();				
	if (IsError()) {
		m_buffer->last_err = SZBE_CONN_ERROR;
		m_buffer->last_err_string = m_error;
		m_values_count = 0;
	}
	m_state = IDLE;
	return m_values_count;
}

int ProberConnection::GetData(short *buffer, int count) {
	count = std::min(count, m_values_count);
	assert(m_values_count > 0);
	std::istream istream(&m_input_buffer);
	istream.read((char*)buffer, 2 * count);
	assert(istream.gcount() == 2 * count);
	m_values_count -= count;
	return count;
}

time_t ProberConnection::GetServerTime() {
	return m_server_time;
}

void ProberConnection::PerformOperation() {
	bool already_connected = m_connected;
	StartTimer();
	if (already_connected)
		SendQuery();
	else
		Connect();
	Go();
	if (IsError() || m_timeout) {
		if (already_connected) {
			if (m_timeout)
				m_timeout = false;	
			ClearError();
			Disconnect();
			Connect();
			Go();
		} else {
			Disconnect();	
		}
	}
	if (m_timeout)
		m_error = L"Connection timeout";
}

bool ProberConnection::GetRange(time_t& start_time, time_t& end_time) {
	if (m_last_range_query_id == m_buffer->szbase->GetQueryId()) {
		start_time = m_start_time;
		end_time = m_end_time;
		return true;
	}
	ClearError();
	m_operation = SEND_BOUNDARY_TIME;
	bool ok = true;
	PerformOperation();
	if (IsError()) {
		m_buffer->last_err = SZBE_CONN_ERROR;
		m_buffer->last_err_string = m_error;
		ok = false;
	}
	m_state = IDLE;
	start_time = m_start_time;
	end_time = m_end_time;	
	if (ok)
		m_last_range_query_id = m_buffer->szbase->GetQueryId();
	return ok;
}

void ProberConnection::Go() {
	m_io_service.run();
	m_io_service.reset();
}

void ProberConnection::StartTimer() {
	m_deadline.expires_from_now(boost::posix_time::seconds(5));
	m_deadline.async_wait(boost::bind(&ProberConnection::TimeoutHandler, this, _1));
}

void ProberConnection::StopTimer() {
	m_deadline.cancel();
}

void ProberConnection::Disconnect() {
	if (m_connected) {
		m_connected = false;
		m_socket.close();
	}
}
