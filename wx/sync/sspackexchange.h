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
#ifndef _SSPACKEEXCHANGE_H
#define _SSPACKEEXCHANGE_H

#include "config.h"

#include <wx/platform.h>

#ifndef MINGW32
#include <netinet/in.h>
#else
#include <winsock.h>
#include <stdint.h>
#endif

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include <deque>

#include "liblog.h"
#include "ssexception.h"

#define PROTOCOL_VERSION 2

/**Basic unit of information exchange*/
struct Packet {
	/**Indicates that error ocurred during
	 * transmistion of current message.
	 * Teminates message*/
	static const uint8_t ERROR_PACKET;
	/**This type is set for all packets
	 * but last packet is message*/
	static const uint8_t OK_PACKET;
	/**Last packet of current message*/
	static const uint8_t LAST_PACKET;
	/**EOS packet used internally to indicate that all request are sent*/
	static const uint8_t EOS_PACKET;
	/**EOF packet used internally to indicate that reading pipe of the connection is closed*/
	static const uint8_t EOF_PACKET;
	/**Maximum length of packet body*/
	static const uint16_t MAX_LENGTH;
	/**Length of header (size of type and packet_length fields
	 * in packet)*/
	static const size_t HEADER_LENGTH;

	/**Type of a packet*/
	uint8_t m_type;
	/**Length of a packet*/
	uint16_t m_size;
	/**Packet body*/
	uint8_t *m_data;
};

class PacketReader {
public:
	virtual bool WantPacket() { return true; }
	virtual void ReadPacket(Packet *p) = 0;
	virtual ~PacketReader() {}
};

class PacketWriter {
public:
	virtual Packet* GivePacket() = 0;
	virtual ~PacketWriter() {}
};

/**Termination checker, a class that checks for termination
 * request from a user*/
class TermChecker {
public:
	/**Performs actual check, shall throw
	 * TerminationRequest exception if user requests 
	 * to terminate synchronization process*/
	virtual void Check() = 0;
	virtual ~TermChecker() {}
};


/**Performs packets exchange with a peer*/
class PacketExchanger {
	/**Buffer for data sent/received from network*/
	struct IoBuffer {
		/**Buffer - its size shall at least exceed size of packet,
		 * for optimum performance ought to be few times larger*/
		uint8_t m_buf[1 << 16];
		/**Current read/write positon in a buffer*/
		uint32_t m_buf_pos;
	};
	
	PacketReader* m_packet_reader;	
	PacketWriter* m_packet_writer;

	bool m_writer_want_write;

	/**SSL object associated with current connection*/
	SSL* m_ssl;
	/**Connetion socket*/
	int m_fd;
	/**Time that can be spent on selecting for data availability
	 * or write feasibility to a socket*/
	time_t m_select_timeout;
	/** Object that is used to query for termination requests*/
	TermChecker* m_term;
	/** Read/write buffers*/
	IoBuffer m_read_buffer, m_write_buffer;
	/** Incoming packets queue*/
	std::deque<Packet*> m_input_queue;
	/** Outgoing packets queue*/
	std::deque<Packet*> m_output_queue;
	/** Current size of data in input queue*/
	uint32_t m_input_queue_size;
	/** Current size of data in output*/
	uint32_t m_output_queue_size;
	/** Denotes if SSL_read operation "blocked" during most recent read*/
	bool m_blocked_on_read;
	/** Denotes if SSL_write operation "blocked" during most recent read*/
	bool m_write_blocked_on_read;
	/** Denotes if SSL_write operation "blocked" during most recent write*/
	bool m_blocked_on_write;
	/** Denotes if SSL_write operation "blocked" during most recent write*/
	bool m_read_blocked_on_write;
	/** True if peer has closed its reading pipe*/
	bool m_read_closed;
	/** Maximum size of queues*/
	static const uint32_t QUEUE_SIZE_LIMIT;

	/**Removes a packet from output queue
	 * @return packet from a queue's front, NULL if queue is empty*/
	Packet* DequeOutputPacket();

	/**Enqueues  a packet in output queue
	 * @param p packet to enqueue*/
	void EnqueOutputPacket(Packet *p);

	/**Attempts to write as many data as possible to the socket.
	 * Exits if no more output data is available or write operation
	 * would block
	 * @return true if any data was written*/
	bool WritePackets();

	/**Performs select operation on a socket, clears m_write_blocked,
	 * m_read_blocked flags if connection may be written to or read from.
	 * If value of timeout is non-zero and select operation return 0, 
	 * check for termination request is performed
	 * @param timeout timeout passwd to a select function(number of seconds
	 * @return return value of select call*/
	int Select(bool check_read, bool check_write, int timeout);

	/**Attemts read packets from a connection
	 * @return true if some data was read*/
	bool ReadPackets(bool read_one_packet = false);

	/**Checks for termination request, does nothing if m_term is NULL*/
	void CheckTermination(); 

	/**@return true if input queue size does not exceed QUEUE_SIZE_LIMIT*/
	bool MayRead();

	/**passes enqued input packets to @see PacketReader*/
	void PutPacketsToReader(); 

	/**reads packets from @see PacketWriter*/
	void GetPacketsFromWriter();

	/**@return true if output queue is not empty or there are 
	 * some data in output buffer*/
	bool WantWrite();

	bool ReadNeedsUnblock();

	bool WriteNeedsUnblock();

	/**@return true if output queue is not empty or there are 
	 * some data in output buffer*/
	Packet* DequeInputPacket();

	/**Puts packet in input queue*/
	void EnqueInputPacket(Packet *p);

	/**Push the packet at the front of output queue*/
	void PushFrontOutputPacket(Packet *p);

public:
	PacketExchanger(SSL *ssl, time_t select_timeout, TermChecker* term);

	~PacketExchanger();

	/**Push the packet at the front of output queue*/
	void ReadBackPacket(Packet *p);

	/**Waits until data are sent to a peer
	 * @param if set to true returns only if all packets from output 
	 * queue are sent otherwise returns when output queue size does not exceed
	 * QUEUE_SIZE_LIMIT*/  
	void FlushOutput();

	void Drive();

	/**sets @see PacketReader object*/
	void SetReader(PacketReader*);

	/**sets @see PacketWriter object*/
	void SetWriter(PacketWriter*);

	/**@return true if output queue size exceeds QUEUE_SIZE_LIMIT*/
	bool OutputQueueAboveHighWaterMark();

	bool OutputQueueAboveLowWaterMark();

	/**return true if input queue is not empty*/
	bool InputAvaiable();

	void Disconnect();

};

/**Helper class for sending messages (understood as sequence of packets with
 * last packet of type LAST_PACKET)*/
class MessageSender : public PacketWriter {
	/**Current packet*/
	Packet *m_packet;
	/**@see PacketExchanger object used for sending packets*/
	PacketExchanger *m_exchanger;
	/**Creates fresh empty packet*/
	void CreateNewPacket();
	/**Send current packet via PacketExchanger*/
	void SendPacket();

public:
	virtual Packet * GivePacket();

	MessageSender(PacketExchanger* exchanger);
	/**Add a byte to a message*/
	void PutByte(uint8_t value);
	/**Add a squence of bytes to a message*/
	void PutBytes(const void* bytes, size_t size); 
	/**Adds an unsigned int to a message*/
	void PutUInt32(uint32_t value); 
	/**Adds an unsigned short int to a message*/
	void PutUInt16(uint16_t value); 
	/**Adds an string to a message*/
	void PutString(const char *string);

	/**Finishes message
	 * @param flush if set to true PacketExchanger is requested
	 * this method will block until whole message is sent*/
	void FinishMessage(bool flush = true);

	virtual ~MessageSender();
	
};

/**Helper class for receining messages (understood as sequence of packets with
 * last packet of type LAST_PACKET or ERROR_PACKET)*/
class MessageReceiver : public PacketReader {
	/**Current read position in a packet*/
	uint16_t m_cur_packet_pos;
	/**@see PacketExchanger object used for receivng packets*/
	PacketExchanger* m_exchanger;
	/**Current packet from which data is read*/
	Packet *m_packet;
	/**Removes current packet*/
	void FreePacket();
	/**Reads next packet from PacketExchanger*/
	void GetPacket();
public:
	MessageReceiver(PacketExchanger* e);

	virtual ~MessageReceiver();

	virtual void ReadPacket(Packet *p);

	/**reads byte from a message
	 * @return read byte*/
	uint8_t GetByte();
 
	/**reads a sequence of bytes from a message
	 * @param buffer pointer to a buffer where bytes are stored
	 * @param size number of bytes to read*/
	void GetBytes(void *buffer, size_t size);

	/**reads unsigned integer from a message
	 * @return read integer*/
	uint32_t GetUInt32();

	/**reads short unsigned integer from a message
	 * @return read integer*/
	uint16_t GetUInt16();

	/**reads string from a message
	 * @return read NULL terminated string (should be freed by a caller*/
	char *GetString();

};
/**
 * Message used on authorization step in protocol nr2, AUTH2
 * Used by CheckUser in XMLUserDB.
 */
struct Message {
	static const uint16_t NULL_MESSAGE; 	/**<All ok*/
	static const uint16_t INVALID_HWKEY;	/**<invalid hardware key*/
	static const uint16_t OLD_VERSION;	/**<old protocol usd by client*/
	static const uint16_t NEAR_ACCOUNT_EXPIRED; /**<account has almost expired <10 days left*/
	static const uint16_t ACCOUNT_EXPIRED;	/**< account expired*/
	static const uint16_t REDIRECT;		/**<Redirecting, next message have to be string*/
	static const uint16_t AUTH_OK; 		/**<Can be some warning, but synchrnization will start*/
	static const uint16_t AUTH_FAILURE; 	/**<password, or hwkey or account expired*/
	static const uint16_t AUTH_REDIRECT; 	/**<redirectind*/
	static const uint16_t INVALID_AUTH_INFO;/**<bad password*/
};


/**Types of messages sent from client to server (this value occupies first 2 bytes in first packet 
 * of a message)*/
struct MessageType {
	/**Authentication request, contains two strings username and password 
	 * server responds with a message of the following format
	 * N  - number of directories ,if zero this indicates error condition, in that case following uint32_t
	 * gives the number of the error as in @see FetchFileListEx
	 * if N > 0 the number is followed by N strings which name directories available for synchronization
	 * Now it represents protocol version*/
	static const uint16_t AUTH;
	/**
	 * Second version of protocol and next step authorization. 
	 * In this stage only @Message are used.
	 * First client send AUTH2 message, and server response:
	 * AUTH_OK, AUTH_FAILURE, AUTH_REDIRECT @see @Message for more information
	 * Next server sends message for client or NULL_MESSAGE if it's all ok
	 */
	static const uint16_t AUTH2;
	/**
	 * Same as AUTH2. Indicates that the server and the client are unicode ready.
	 */
	static const uint16_t AUTH3;
	/** 
	 * Message begins with a N - index of directory from response to AUTH request that client wishes
	 * to synchronize, after the number, two strings follow analogous to --exclude and --include options in rsync.
	 * Server responds with a message of format:
	 * N - number of selected file, if 0 this may indicate an error which is encoded in following uint32_t number
	 * (@see FetchFileListEx for meaning of this value)
	 * if N > 0 info on selected files is sent in following format:
	 * string: path to first file, uint16_t - file type as in @see TPath, 
	 * 	uint32_t: size of a file, uint16_t: file modification time , uint8_t file mod. time month, uint8_t - file mod time
	 * 	hour, uint8_t file mod time - minute
	 * each sucuessive file info is sent in the same format except for path to a file, which is 
	 * encoded by @see Server::GenerateScript*/ 
	static const uint16_t GET_FILE_LIST;
	/**Request for a complete file, message contains file number to sent as specified in response to GET_FILE_LIST message.
	 * Server responds with a message containg raw data of file*/
	static const uint16_t SEND_COMPLETE_FILE;
	/**Request for a complete file, message contains file number*/
	/* Server responds with a message containg a uint16_t - length of following string which is a path 
	 * to a link destination relative to directory where link itself is located*/
	static const uint16_t SEND_LINK;
	/**Request for a file patch, message contains file number to sent as specified in response to GET_FILE_LIST message
	 * followed by a signature generated by librsync, server responds with a patch generated by librsync*/
	static const uint16_t SEND_FILE_PATCH;
	/** Goodbye message sent by client. Upon reception of this message server stops receing data from a client.
	 * Connection will be closed when responses to all queued requests are sent.*/
	static const uint16_t BYE;

	static const uint16_t BASE_MODIFICATION_TIME;
};

/**Struct used as an exception when for some reason files' list cannot be sent to a client*/
struct FetchFileListEx {
	/**Client provieded invalid authorization information*/
	static const uint32_t INVALID_AUTH_INFO;
	static const uint32_t INVALID_HW_KEY;
	/**Server was unable to compile regexp, it indicates configuration error if sent in response
	 * to AUTH response, client code error if sent is respone to GET_FILE_LIST*/
	static const uint32_t REGEX_COMP_FAILED;
	/**Base dir specified in configuration does to exist - configuration error*/
	static const uint32_t NOT_SUCH_DIR;
	/**No files were selected by see @see ScanDir function - configuration error if sent 
	 * in response to AUTH message*/
	static const uint32_t NO_FILES_SELECTED;

	FetchFileListEx(uint32_t error);
	/**Error type*/
	uint32_t m_error;
};

#define  BASESTAMP_FILENAME "szbase_stamp"


#endif
