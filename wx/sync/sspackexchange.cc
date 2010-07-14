/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#include "sspackexchange.h"
#include <libxml/xinclude.h>

const uint16_t MessageType::AUTH = 1;
const uint16_t MessageType::AUTH2 = 2;
const uint16_t MessageType::AUTH3 = 4;
const uint16_t MessageType::GET_FILE_LIST = 3;
const uint16_t MessageType::SEND_COMPLETE_FILE = 5;
const uint16_t MessageType::SEND_LINK = 7;
const uint16_t MessageType::SEND_FILE_PATCH = 9;
const uint16_t MessageType::BYE = 11;
const uint16_t MessageType::BASE_MODIFICATION_TIME = 12;
const uint16_t MessageType::SEND_FILE_REST = 13;

const uint16_t Message::AUTH_OK = 0;
const uint16_t Message::AUTH_FAILURE = 1;
const uint16_t Message::AUTH_REDIRECT = 2;

const uint16_t Message::NULL_MESSAGE = 5;
const uint16_t Message::OLD_VERSION = 6;
const uint16_t Message::NEAR_ACCOUNT_EXPIRED = 7;
const uint16_t Message::ACCOUNT_EXPIRED = 11;
const uint16_t Message::INVALID_AUTH_INFO = 13;
const uint16_t Message::INVALID_HWKEY = 14;

const uint32_t PacketExchanger::QUEUE_SIZE_LIMIT = 8 * (Packet::HEADER_LENGTH + Packet::MAX_LENGTH);

const uint8_t Packet::OK_PACKET = 0;
const uint8_t Packet::LAST_PACKET = 1;
const uint8_t Packet::ERROR_PACKET = 2;
const uint8_t Packet::EOS_PACKET = 3;
const uint8_t Packet::EOF_PACKET = 4;

const uint16_t Packet::MAX_LENGTH = 2 * 4092;

const size_t Packet::HEADER_LENGTH = sizeof(uint8_t) + sizeof(uint16_t);

const uint32_t FetchFileListEx::INVALID_AUTH_INFO = 0;

const uint32_t FetchFileListEx::NOT_SUCH_DIR = 1;
const uint32_t FetchFileListEx::REGEX_COMP_FAILED = 2;
const uint32_t FetchFileListEx::NO_FILES_SELECTED = 3;

const int connection_timeout_seconds = 5 * 60;

Packet* PacketExchanger::DequeInputPacket() { 
	if (m_input_queue.size() == 0)
		return NULL;
	Packet *p = m_input_queue.front();
	m_input_queue.pop_front();
	m_input_queue_size -= p->m_size + Packet::HEADER_LENGTH;
	return p;
}

void PacketExchanger::EnqueOutputPacket(Packet *p) {
	m_output_queue.push_back(p);
	m_output_queue_size += p->m_size + Packet::HEADER_LENGTH;
}

Packet* PacketExchanger::DequeOutputPacket() {
	if (m_output_queue.size() == 0)
		return NULL;
	Packet *p = m_output_queue.front();
	m_output_queue.pop_front();
	m_output_queue_size -= p->m_size + Packet::HEADER_LENGTH;
	return p;
}

bool PacketExchanger::WritePackets() {
	bool result = false;

	if (m_blocked_on_write || m_write_blocked_on_read)
		return result;
	do {
		if (m_write_buffer.m_buf_pos == 0 && m_output_queue.size() == 0) 
			return result;

		do {
			if (m_output_queue.size() == 0)
				break;

			Packet *p = DequeOutputPacket();
			if (sizeof(m_write_buffer.m_buf) - m_write_buffer.m_buf_pos <
				p->m_size + Packet::HEADER_LENGTH) {
				PushFrontOutputPacket(p);	
				break;
			} 
				
			result = true;
			
			uint8_t* buf = m_write_buffer.m_buf + m_write_buffer.m_buf_pos;
			*buf = p->m_type;

			buf += sizeof(uint8_t);
			*((uint16_t*)buf) = htons(p->m_size);

			buf += sizeof(uint16_t);
			memcpy(buf, p->m_data, p->m_size);
		
			m_write_buffer.m_buf_pos += p->m_size + Packet::HEADER_LENGTH;

			free(p->m_data);
			delete p;
		} while (true);

		uint8_t* buf = m_write_buffer.m_buf;
		size_t size = m_write_buffer.m_buf_pos;
		
		int ret = SSL_write(m_ssl, buf, size);

		if (ret <= 0) {
			int error = SSL_get_error(m_ssl, ret);
			switch (error) {
				case SSL_ERROR_WANT_WRITE:
					m_blocked_on_write = true;
					return result;
				case SSL_ERROR_ZERO_RETURN:
					throw ConnectionShutdown();
				case SSL_ERROR_SYSCALL: 
#ifdef MINGW32
					throw IOException(GetLastError());
#else
					throw IOException(errno);
#endif
				case SSL_ERROR_WANT_READ:
					m_write_blocked_on_read = true;
					return result;
				default:
					throw SSLException(ERR_get_error());
			}

		}

		size_t sent = ret;
		size_t remaining = m_write_buffer.m_buf_pos - sent;
		memmove(m_write_buffer.m_buf, m_write_buffer.m_buf + sent, remaining);
		m_write_buffer.m_buf_pos = remaining;


	} while (true);

	//NOT REACHED 
	return false;
}

void PacketExchanger::PushFrontOutputPacket(Packet *p) {
	m_output_queue_size += p->m_size + Packet::HEADER_LENGTH;
	m_output_queue.push_front(p);
}

bool PacketExchanger::ReadNeedsUnblock() {
	return !m_read_closed && ((MayRead() && m_blocked_on_read) || m_write_blocked_on_read);
}

bool PacketExchanger::WriteNeedsUnblock() {
	return (WantWrite() && m_blocked_on_write) || m_read_blocked_on_write;
}

int PacketExchanger::Select(bool check_read, bool check_write, int timeout) {
again:
	fd_set rd_set, wr_set;
	FD_ZERO(&rd_set);
	FD_ZERO(&wr_set);

	if (check_read)
		FD_SET(m_fd, &rd_set);
			
	if (check_write)
		FD_SET(m_fd, &wr_set);



	if (!FD_ISSET(m_fd, &rd_set) && !FD_ISSET(m_fd, &wr_set))
		return 0;

	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = timeout;

#ifndef MINGW32
	int ret = select(m_fd + 1, &rd_set, &wr_set, NULL, &tv);
#else
	fd_set err_set;
	FD_ZERO(&err_set);
	FD_SET(m_fd, &err_set);
	//under windows there must be at least one descriptor
	//set in descriptors' set, otherwise NULL must be passed
	//as argument to select
	fd_set* prd_set = FD_ISSET(m_fd, &rd_set) ? &rd_set : NULL;
	fd_set* pwr_set = FD_ISSET(m_fd, &wr_set) ? &wr_set : NULL;

	int ret = select(m_fd + 1, prd_set, pwr_set, &err_set, &tv);
#endif

	if (ret == -1) {
#ifdef MINGW32
		throw IOException(GetLastError());
#else
		if (errno == EINTR) {
			goto again;
		} else {
			throw IOException(errno);
		}
#endif
	}

	if (ret > 0) {
#ifdef MINGW32
		if (FD_ISSET(m_fd, &err_set))
			throw Exception(_("connection with server terminated"));
#endif
		if (FD_ISSET(m_fd, &rd_set)) {
			if (m_write_blocked_on_read)
				m_write_blocked_on_read = false;
			else
				m_blocked_on_read = false;
		}
		if (FD_ISSET(m_fd, &wr_set)) {
			if (m_read_blocked_on_write)
				m_read_blocked_on_write = false;
			else
				m_blocked_on_write = false;
		}

	} else if (ret == 0 && timeout)
			CheckTermination();

	return ret;

}

bool PacketExchanger::ReadPackets(bool read_one_packet) {
	bool result = false;

	if (m_read_closed)
		return false;

	if (m_blocked_on_read || m_read_blocked_on_write)
		return false;

	do {
		if (!MayRead())
			return result;

		int ret = 0;
		int to_read;

		if (read_one_packet == false) {
			to_read = sizeof(m_read_buffer.m_buf) - m_read_buffer.m_buf_pos;
			assert(to_read > 0);
		} else {
			if (m_read_buffer.m_buf_pos >= Packet::HEADER_LENGTH) {
				uint16_t length = ntohs(*((uint16_t*)(m_read_buffer.m_buf + sizeof(uint8_t))));
				to_read = length + Packet::HEADER_LENGTH - m_read_buffer.m_buf_pos;
				if (to_read < 0)
					to_read = 0;
			} else {
				to_read = Packet::HEADER_LENGTH - m_read_buffer.m_buf_pos;
			}
		}

		if (to_read) {
			ret = SSL_read(m_ssl, m_read_buffer.m_buf + m_read_buffer.m_buf_pos,
					to_read);
				
			if (ret <= 0) {
				int error = SSL_get_error(m_ssl, ret);
				switch (error) {
					case SSL_ERROR_WANT_READ:
						m_blocked_on_read = true;
						return result;
					case SSL_ERROR_WANT_WRITE:
						m_read_blocked_on_write = true;
						return result;
					case SSL_ERROR_SYSCALL: 
#ifdef MINGW32
						if (GetLastError() != 0)
							throw IOException(GetLastError());
#else
						if (errno)
							throw IOException(errno);
#endif
					case SSL_ERROR_ZERO_RETURN: {
						Packet *p = new Packet;
						p->m_type = Packet::EOF_PACKET;
						p->m_size = 0;
						p->m_data = NULL;
						EnqueInputPacket(p);
						return true;
					}
					default:
						throw SSLException(ERR_get_error());
				}
			}
		}

		m_read_buffer.m_buf_pos += ret;

		while (m_read_buffer.m_buf_pos >= Packet::HEADER_LENGTH) {
			uint16_t length = ntohs(*((uint16_t*)(m_read_buffer.m_buf + sizeof(uint8_t))));
			if (length + Packet::HEADER_LENGTH <= m_read_buffer.m_buf_pos) {
				Packet *p = new Packet;
				p->m_type = *m_read_buffer.m_buf;
				p->m_size = length;
				if (p->m_size) {
					p->m_data = (uint8_t*)malloc(p->m_size);
					memcpy(p->m_data, m_read_buffer.m_buf + Packet::HEADER_LENGTH, length);
				} else 
					p->m_data = 0;

				size_t remaining_data_size = m_read_buffer.m_buf_pos - (Packet::HEADER_LENGTH + length);

				memmove(m_read_buffer.m_buf, 
						m_read_buffer.m_buf + Packet::HEADER_LENGTH + length, 
						remaining_data_size);

				m_read_buffer.m_buf_pos = remaining_data_size;

				EnqueInputPacket(p);
				result = true;
				if (read_one_packet)
					return result;
			} else
				break;

		}

	} while (true);

	//NOT REACHED
	return false;

}

void PacketExchanger::CheckTermination() {
	if (m_term) 
		m_term->Check();
}

bool PacketExchanger::MayRead() {
	return m_input_queue_size < QUEUE_SIZE_LIMIT;
}

bool PacketExchanger::WantWrite() {
	return (m_write_buffer.m_buf_pos != 0) || (m_output_queue.size() > 0);
}

void PacketExchanger::EnqueInputPacket(Packet *p) {
	if (p->m_type == Packet::EOF_PACKET)
		m_read_closed = true;
	m_input_queue_size += p->m_size + Packet::HEADER_LENGTH;
	m_input_queue.push_back(p);
}

PacketExchanger::PacketExchanger(SSL *ssl, time_t select_timeout, TermChecker* term) :
		m_packet_reader(NULL), m_packet_writer(NULL), m_writer_want_write(false),
		m_ssl(ssl), m_fd(-1), m_select_timeout(select_timeout),
		m_term(term), m_input_queue_size(0), m_output_queue_size(0),
		m_blocked_on_read(false), m_write_blocked_on_read(false),
		m_blocked_on_write(false), m_read_blocked_on_write(false),
		m_read_closed(false) {

	m_fd = SSL_get_fd(m_ssl);

	m_read_buffer.m_buf_pos = m_write_buffer.m_buf_pos = 0;

}

PacketExchanger::~PacketExchanger() {
	if (m_fd >= 0) {
#ifdef MINGW32
		shutdown(m_fd, SD_BOTH);
		closesocket(m_fd);
#else
		shutdown(m_fd, SHUT_RDWR);
		close(m_fd);
#endif
	}
	SSL_free(m_ssl);
}

void PacketExchanger::ReadBackPacket(Packet *p) {
	assert(p != NULL);
	m_input_queue_size += p->m_size + Packet::HEADER_LENGTH;
	m_input_queue.push_front(p);
}

void PacketExchanger::PutPacketsToReader() {
	while (m_packet_reader 
			&& m_packet_reader->WantPacket() 
			&& m_input_queue.size())
		m_packet_reader->ReadPacket(DequeInputPacket());
}

void PacketExchanger::GetPacketsFromWriter() {
	if (OutputQueueAboveLowWaterMark())
		return;

	while (m_packet_writer && !OutputQueueAboveHighWaterMark()) {
		Packet *p = m_packet_writer->GivePacket();
		if (p != NULL)
			EnqueOutputPacket(p);
		else
			break;
	}
}

void PacketExchanger::Drive() {
	size_t no_transition_cycles = 0;
	while (true) {

		PutPacketsToReader();
		GetPacketsFromWriter();

		bool has_written = WritePackets();
		bool has_read = ReadPackets();

		if (has_read || has_written) {
			no_transition_cycles = 0;
			continue;
		} else if (no_transition_cycles++ * m_select_timeout
				> connection_timeout_seconds) {
			throw AppException(_("Connection timeout!"));
		}

		if (!(m_packet_reader || m_packet_writer))
			break;

		Select(ReadNeedsUnblock(), WriteNeedsUnblock(), m_select_timeout);
	
	}

}

void PacketExchanger::FlushOutput() {
	while (WantWrite()) {
		Select(m_write_blocked_on_read, m_blocked_on_write, m_select_timeout);
		WritePackets();
	}
}

void PacketExchanger::Disconnect() {
	Packet * p = new Packet;

	p->m_data = NULL;
	p->m_size = 0;
	p->m_type = Packet::EOF_PACKET;
	EnqueOutputPacket(p);

	SSL_set_read_ahead(m_ssl, 0);

	while (!m_read_closed || WantWrite()) {
		Select(!m_read_closed, WantWrite(), m_select_timeout);
		ReadPackets(true);
		WritePackets();
	}

	while ((p = DequeInputPacket()) != NULL) {
		if (p->m_size != 0)
			free(p->m_data);
		delete p;
	}

#ifdef MINGW32
	shutdown(m_fd, SD_BOTH);
	closesocket(m_fd);
#else
	shutdown(m_fd, SHUT_RDWR);
	close(m_fd);
#endif
	m_fd = -1;
}


bool PacketExchanger::OutputQueueAboveHighWaterMark() {
	return m_output_queue_size > QUEUE_SIZE_LIMIT;
}

bool PacketExchanger::OutputQueueAboveLowWaterMark() {
	return m_output_queue_size > QUEUE_SIZE_LIMIT / 2;
}

bool PacketExchanger::InputAvaiable() {
	return m_input_queue.size() > 0;
}

void PacketExchanger::SetWriter(PacketWriter *writer) {
	m_writer_want_write = true;
	m_packet_writer = writer;
}

void PacketExchanger::SetReader(PacketReader *reader) {
	m_packet_reader = reader;
}


void MessageSender::CreateNewPacket() {
	m_packet = new Packet();
	m_packet->m_data = NULL;
	m_packet->m_size = 0;
	m_packet->m_type = Packet::OK_PACKET;
}

void MessageSender::SendPacket() {
	assert(m_packet);

	m_exchanger->SetWriter(this);
	m_exchanger->Drive();

	assert(m_packet == NULL);
}

Packet * MessageSender::GivePacket() {
	assert(m_packet);

	Packet * p = m_packet;
	m_packet = NULL;
	m_exchanger->SetWriter(NULL);

	return p;
}


MessageSender::MessageSender(PacketExchanger* exchanger) : m_packet(NULL), m_exchanger(exchanger) 
{
}

void MessageSender::PutByte(uint8_t value) {
	PutBytes(&value, sizeof(value));
}

void MessageSender::PutBytes(const void* bytes, size_t size) {

	size_t w = 0;
	while (w < size) {
		if (m_packet == NULL) 
			CreateNewPacket();

		if (m_packet->m_size == Packet::MAX_LENGTH) {
			SendPacket();
			continue;
		}

		size_t to_add = std::min(size - w , (size_t)Packet::MAX_LENGTH - m_packet->m_size);
		
		m_packet->m_data = (uint8_t*) realloc(m_packet->m_data, m_packet->m_size + to_add);
		memcpy(m_packet->m_data + m_packet->m_size, (uint8_t*)bytes + w, to_add);
		m_packet->m_size += to_add;

		w += to_add;
	}
}

void MessageSender::PutUInt32(uint32_t value) {
	value = htonl(value);
	PutBytes(&value, sizeof(uint32_t));
}

void MessageSender::PutUInt16(uint16_t value) {
	value = htons(value);
	PutBytes(&value, sizeof(uint16_t));
}

void MessageSender::PutString(const char *string) {
	xmlChar *str = xmlCharStrdup(string);
	uint16_t size = strlen((char*)str);

	assert (size < 128 * 128);

	if (size / 128) 
		PutByte((size / 128) | (1 << 7));
	PutByte(size % 128);
	
	PutBytes(str, size);
	xmlFree(str);
}

void MessageSender::FinishMessage(bool flush) {
	assert(m_packet);
	m_packet->m_type = Packet::LAST_PACKET;
	SendPacket();
	if (flush)
		m_exchanger->FlushOutput();
}

MessageSender::~MessageSender() {
	if (m_packet) {
		free(m_packet->m_data);
		delete m_packet;
		sz_log(9, "Packet not empty in MessageSender destructor");
	}
}

void MessageReceiver::FreePacket() {
	if (m_packet) {
		free(m_packet->m_data);
		delete m_packet;
		m_packet = NULL;
	}
}

void MessageReceiver::GetPacket() {
	if (m_packet) {
		if (m_packet->m_type == Packet::LAST_PACKET
			|| m_packet->m_type == Packet::ERROR_PACKET) 
			throw AppException(_("Attempt to read data from message but there is no" 
				" more data in it (protocol mismatch?)"));
		FreePacket();
	}
	m_exchanger->SetReader(this);
	m_exchanger->Drive();
}

void MessageReceiver::ReadPacket(Packet *p) {
	m_cur_packet_pos = 0;
	m_packet = p;
	m_exchanger->SetReader(NULL);
}

MessageReceiver::MessageReceiver(PacketExchanger* e) : m_cur_packet_pos(0), 
						       m_exchanger(e), 
						       m_packet(NULL)
{}

MessageReceiver::~MessageReceiver() {
	FreePacket();
}

uint8_t MessageReceiver::GetByte() {
	uint8_t result;
	GetBytes(&result, sizeof(result));
	return result;
}
 
void MessageReceiver::GetBytes(void *buffer, size_t size) {
	size_t r = 0;
	while (r < size) {
		if (m_packet == NULL || m_packet->m_size == 0)
			GetPacket();

		if (m_packet->m_type == Packet::EOF_PACKET)
			throw ConnectionShutdown(); 

		size_t to_read = std::min(size - r, (size_t) m_packet->m_size);

		memcpy((uint8_t*)buffer + r, 
			(uint8_t*)m_packet->m_data + m_cur_packet_pos,
			to_read);

		m_cur_packet_pos += to_read;
		r += to_read; 
		m_packet->m_size -= to_read;
	}

}

uint32_t MessageReceiver::GetUInt32() {
	uint32_t temp;
	GetBytes(&temp, sizeof(uint32_t));
	return ntohl(temp);
}

uint16_t MessageReceiver::GetUInt16() {
	uint16_t temp;
	GetBytes(&temp, sizeof(uint16_t));
	return ntohs(temp);
}

char *MessageReceiver::GetString() {
	uint16_t size = 0;

	uint8_t t = GetByte();
	if (t & (1 << 7)) {
		size = (t & ~(1 << 7)) * 128;
		t = GetByte();
	}
	size += t;
	//fprintf(stderr, "Getting string of size %zu\n", size);

	char* string = (char*)malloc(size + sizeof(char));
	string[size] = '\0';
	GetBytes(string, size);
	return string;
}

FetchFileListEx::FetchFileListEx(uint32_t error) : m_error(error) {
}
