#include "config.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>

#include "conversion.h"
#include "szbdefines.h"
#include "proberconnection.h"

ProberConnection::ProberConnection(std::string address, std::string port) : m_io_service(), m_resolver(m_io_service), m_deadline(m_io_service), m_socket(m_io_service), m_address(address), m_port(port), m_connected(false) {
}

void ProberConnection::Connect() {
	boost::asio::ip::tcp::resolver::query query(m_address, m_port);
	m_resolver.async_resolve(query, boost::bind(&ProberConnection::HandleResolve, this, _1, _2));
	m_deadline.expires_from_now(boost::posix_time::seconds(1));
	m_deadline.async_wait(boost::bind(&ProberConnection::TimeoutHandler, this, _1));
}

void ProberConnection::TimeoutHandler(const boost::system::error_code& error) {
	std::cerr << "Timeout" << std::endl;
	//m_resolver.cancel();
}

std::string ProberConnection::Error() {
	return m_error;
}

void ProberConnection::ResetBuffers() {
	m_input_buffer.consume(m_input_buffer.size());
	m_output_buffer.consume(m_output_buffer.size());
}

void ProberConnection::HandleResolve(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator &i) {
	if (!error) {
		std::cerr << "Got it!" << std::endl;
		std::cerr << i->endpoint().address() << std::endl;
		m_socket.async_connect(*i, boost::bind(&ProberConnection::HandleConnect, this, _1));
	} else {
		throw error;
	}
}

void ProberConnection::SendSearchQuery() {
	ResetBuffers();

	std::ostream ostream(&m_output_buffer);
	ostream << "SEARCH "
		<< m_from << " "
		<< m_to << " "
		<< m_direction << " "
		<< SC::S2A(m_path) << "\r\n";

	boost::asio::async_write(m_socket, m_output_buffer, 
		boost::bind(&ProberConnection::HandleWrite, this, _1));

	boost::asio::async_read_until(m_socket, m_input_buffer, "\r\n",
			boost::bind(&ProberConnection::HandleSearchResponse, this, _1));
}

void ProberConnection::SendGetQuery() {
	ResetBuffers();

	std::ostream ostream(&m_output_buffer);
	ostream << "GET "
		<< m_from << " "
		<< m_to << " "
		<< SC::S2A(m_path) << "\r\n";

	boost::asio::async_write(m_socket, m_output_buffer, 
		boost::bind(&ProberConnection::HandleWrite, this, _1));

	boost::asio::async_read_until(m_socket, m_input_buffer, "\r\n",
			boost::bind(&ProberConnection::HandleTimeLine, this, _1));
}

void ProberConnection::HandleConnect(const boost::system::error_code& error) {
	if (!error) {
		std::cerr << "Connected" << std::endl;

		m_connected = true;

		switch (m_operation) {
			case SEND_SEARCH:
				SendSearchQuery();
				break;
			case SEND_GET:
				SendGetQuery();
				break;
		}

	} else {
		throw error;
	}
}

void ProberConnection::HandleWrite(const boost::system::error_code &error) {
	if (!error) {
		std::cerr << "Write was successfull" << std::endl;
	} else {
		throw error;
	}
}

std::string ProberConnection::ReadLine() {
	std::string line;
	std::istream istream(&m_input_buffer);
	std::getline(istream, line);
	return line;
}

void ProberConnection::HandleTimeLine(const boost::system::error_code &error) {
	if (!error) {
		std::string line = ReadLine();
		if (line.substr(0, sizeof("ERROR") - 1) != "ERROR") {
			std::istringstream(line) >> m_server_time;
			boost::asio::async_read_until(m_socket, m_input_buffer, "\r\n",
				boost::bind(&ProberConnection::HandleSearchResponse, this, _1));
		} else {
			m_error = line.substr(sizeof("ERROR"));
			throw message_error();
		}
	} else {
		throw error;
	}
}

void ProberConnection::HandleSizeLine(const boost::system::error_code &error) {
	if (!error) {
		std::string line = ReadLine();
		size_t length;
		std::istringstream(line) >> length;
		std::cerr << "Reading " << length << " bytes" << std::endl;
		m_values_count = length / 2;
		if (m_input_buffer.size() == length)
			HandleReadValues();
		else
			boost::asio::async_read(m_socket, m_input_buffer.prepare(length - m_input_buffer.size()), 
				boost::bind(&ProberConnection::HandleReadValues, this, _1));
	} else {
		throw error;
	}
}

void ProberConnection::HandleReadValues() {
	return;
}

void ProberConnection::HandleReadValues(const boost::system::error_code &error) {
	HandleReadValues();
}

void ProberConnection::HandleSearchResponse(const boost::system::error_code &error) {
	if (!error) {
		std::string line;
		time_t response;
		std::istream is(&m_input_buffer);
		std::getline(is, line);
		std::istringstream(line) >> response;
		std::cerr << "Found time:" << response << std::endl;
		m_search_result = response;
	} else {
		throw error;
	}
}

time_t ProberConnection::Search(time_t from, time_t to, int direction, std::wstring path) {
	m_operation = SEND_SEARCH;
	m_from = from;
	m_to = to;
	m_direction = direction;

	try {
		if (m_connected)
			SendSearchQuery();
		else
			Connect();
		Go();
	} catch (const boost::system::error_code& error) {
		m_error = error.message();
		m_search_result = -1;
	} catch (const message_error& error) {
		m_search_result = -1;
	}

	return m_search_result;
}



int ProberConnection::Get(time_t from, time_t to, std::wstring path) {
	m_operation = SEND_GET;
	m_from = from;
	m_to = to;
	m_path = path;

	try {
		if (m_connected)
			SendSearchQuery();
		else
			Connect();
		Go();
	} catch (const boost::system::error_code& error) {
		m_error = error.message();
		return -1;
	} catch (const message_error& error) {
		return -1;
	}
	return m_values_count;
}

int ProberConnection::GetData(short *buffer, int count) {
	count = std::min(count, m_values_count);
	assert(m_values_count > 0);
	std::istream istream(&m_input_buffer);
	istream.read((char*)&buffer, 2 * count);
	m_values_count -= count;
	return count;
}

void ProberConnection::Go() {
	m_io_service.run();
}

