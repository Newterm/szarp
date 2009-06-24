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
/*
 * $Id$
 *
 * Daemon Uni-Telway.
 *
 * Autor: Krzysztof Gałązka <kg@praterm.pl>
 * 
 * Pracuje jako urządzenie 'slave'.
 * Odczytuje dane typu 'WORD' z adresów 0-8191.
 *
 * Konfiguracja:
 *  <device
 *	    daemon="/opt/szarp/bin/unitedmn"
 *	    path="/dev/ttyA11"
 *	    speed="9600"
 *	    unitelway:address="6"
 *  >
 *	<unit id="1" type="1" subtype="1" bufsize="1"
 *		unitelway:network="0x00"
 *		unitelway:station="0xFE"
 *		unitelway:gate="0x00"
 *		unitelway:module="0x00"
 *		unitelway:channel="0x00"
 *	>
 *		<param ... unitelway:address="1000">
 *		</param>
 *	</unit>
 *  </device>
 *
 *
 *  Atrybut <device unitelway:address=""> jest wymagany. Określa adres daemona na szynie UniTE
 *  
 *  Atrybut <device unitelway:address=""> jest wymagany. Określa adres rejestru, z którego nastąpi odczyt.
 *  
 *  Atrybuty unitelway:* w elemencie unit określają adres urządzenia, które będzie odpytywane.
 *  Są opcjonalne. Domyślne wartości zgodnie z powyższym przykładem.
 *
 *
 */

#define _BSD_SOURCE 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <event.h>

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxml/tree.h>

#include <vector>

#include <conversion.h>

using namespace std;

#include "xmlutils.h"

#define _IPCTOOLS_H_
#define _HELP_H_
#define _ICON_ICON_
#include "szarp.h"
#include "msgtypes.h"

#include "ipchandler.h"
#include "liblog.h"

#define MAX_FRAME 512

#define UNITELWAY_WORD_MAX_NUMBER 8191

const char * unite_address_parts[]= { "network", "station", "gate", "module", "channel" };

#define UNITELWAY_ADDRESS_NETWORK 0
#define UNITELWAY_ADDRESS_STATION 1
#define UNITELWAY_ADDRESS_GATE 2
#define UNITELWAY_ADDRESS_MODULE 3
#define UNITELWAY_ADDRESS_CHANNEL 4

struct timeval c_tv;
struct timezone c_tz;
struct timeval debug_tv;
struct timezone debug_tz;
    
void callback_sniff(int fd, short event, void * arg);
void callback_data_on_port(int fd, short event, void * arg);
void callback_request_timeout(int fd, short event, void * arg);
void callback_main_loop(int fd, short event, void * arg);

long
mdiff()
{
    gettimeofday(&c_tv, &c_tz);
    long diff = (c_tv.tv_sec * 1000 + c_tv.tv_usec / 1000) - (debug_tv.tv_sec * 1000 + debug_tv.tv_usec / 1000);

    debug_tv = c_tv;

    return  diff;
}

void 
hexdump(FILE * f, size_t lenght, const uint8_t * buf)
{
    size_t it = 0;

    for (; it < lenght; it++) {
	fprintf(f, "%.2hhx ", buf[it]);
    }
}

/**
 * Unitelway Unit.
 * Implement single Unitelway device.
 */
class UnitelwayUnit
{
    public:
	char m_id;

	UnitelwayUnit() {};

	bool Configure(DaemonConfig * cfg, TUnit * unit, xmlNodePtr unit_node, short * read_base, short * send_base);

	void WriteParam(unsigned int num, short val);

	/* TODO */
	uint8_t * GetAddress() { return m_unit_address; };

	int * m_read_addrs;
	unsigned int m_read_count;

	int * m_send_addrs;
	unsigned int m_send_count;

    private:
	bool getAddressPart(int part, xmlNodePtr node, uint8_t def);
	bool parseUnitAddress(xmlNodePtr node);
	bool parseParams(DaemonConfig * cfg, TUnit * unit, xmlNodePtr unit_node);

	bool m_debug;
	bool m_single;

	short * m_read_base;
	short * m_send_base;
	
	uint8_t m_unit_address[5]; // TODO
};

bool
UnitelwayUnit::parseParams(DaemonConfig * cfg, TUnit * unit, xmlNodePtr unit_node)
{
    unsigned int reads_found = 0;
    unsigned int sends_found = 0;

    char * str;
    char * tmp;
    int addr;

    sz_log(5, "DEBUG: UnitelwayUnit parsing params config");

    for (xmlNodePtr node = unit_node->children; node; node = node->next) {
	if (node->ns == NULL)
	    continue;
	
	if (!xmlStrEqual(node->ns->href, SC::S2U(IPK_NAMESPACE_STRING).c_str()))
	    continue;
	
	if (!xmlStrEqual(node->name, BAD_CAST "param") && !xmlStrEqual(node->name, BAD_CAST "send"))
	    continue;

	addr = -1;
	
	str = (char *) xmlGetNsProp(node, BAD_CAST("address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
	if (NULL == str) {
	    sz_log(0, "ERROR: attribute unitelway:address not found (line %ld)", xmlGetLineNo(node));
	    return 1;
	}
	
	addr = strtol(str, &tmp, 0);
	if (tmp[0] != 0) {
	    sz_log(0,
		    "ERROR: parsing unitelway:word_number attribute value ('%s') - line %ld",
		    str, xmlGetLineNo(node));
	    free(str);
	    return 1;
	}
	free(str);
	
	if (addr > UNITELWAY_WORD_MAX_NUMBER) {
	    sz_log(0, "unitelway:word_number attribute value to big (line %ld)", xmlGetLineNo(node));
	    return 1;
	}

	if (xmlStrEqual(node->name, BAD_CAST "param")) {
	    sz_log(5, "DEBUG: UnitelwayUnit: adding read - word address: %d", addr);
	    m_read_addrs[reads_found] = addr;
	    reads_found++;
	}
	else {
	    sz_log(5, "DEBUG: UnitelwayUnit: adding send - word address: %d", addr);
	    m_send_addrs[sends_found] = addr;
	    sends_found++;
	}


    }

    if (sends_found > 0) {
	sz_log(0, "Sending params not supported (yet)");
    }

    assert(reads_found == m_read_count);
    assert(sends_found == m_send_count);

    return true;
}

bool
UnitelwayUnit::getAddressPart(int part, xmlNodePtr node, uint8_t def)
{
    char * str = (char *) xmlGetNsProp(node, BAD_CAST(unite_address_parts[part]), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
    if (NULL == str) {
	sz_log(2, "INFO: attribute unitelway:%s not found assuming %x (line %ld)",
		unite_address_parts[part], def, xmlGetLineNo(node));
	return true;
    }
    
    long int addr = -1;
    char * tmp = NULL;
    
    addr = strtol(str, &tmp, 0);
    if (tmp[0] != 0) {
	sz_log(0,
		"ERROR: parsing unitelway:%s attribute value ('%s') - line %ld",
		unite_address_parts[part], str, xmlGetLineNo(node));
	xmlFree(str);
	return false;
    }

    xmlFree(str);
    m_unit_address[part] = addr & 0xFF;
    return true;
}

bool
UnitelwayUnit::parseUnitAddress(xmlNodePtr node)
{
    if (!getAddressPart(UNITELWAY_ADDRESS_NETWORK, node, 0x00))
	return false;

    if (!getAddressPart(UNITELWAY_ADDRESS_STATION, node, 0xfe))
	return false;

    if (!getAddressPart(UNITELWAY_ADDRESS_GATE, node, 0x00))
	return false;

    if (!getAddressPart(UNITELWAY_ADDRESS_MODULE, node, 0x00))
	return false;

    if (!getAddressPart(UNITELWAY_ADDRESS_CHANNEL, node, 0x00))
	return false;

    return true;
}

bool
UnitelwayUnit::Configure(DaemonConfig * cfg, TUnit * u, xmlNodePtr node, short * read_base, short * send_base)
{
    m_single = cfg->GetSingle();
    m_id = u->GetId();

    m_read_base = read_base;
    m_read_count = u->GetParamsCount();
    assert(m_read_count >= 0);

    if (m_read_count > 0) {
	m_read_addrs = new int[m_read_count];
	assert(m_read_addrs != NULL);
    } else {
	sz_log(1, "WARRING: no params to read");
	m_read_addrs = NULL;
    }

    m_send_base = send_base;
    m_send_count = u->GetSendParamsCount();

    if (m_send_count > 0) {
	m_send_addrs = new int[m_send_count];
	assert(m_send_addrs != NULL);
    } else {
	m_send_addrs = NULL;
    }

    if (!parseUnitAddress(node)) {
	sz_log(0, "ERROR: UnitelwayUnit::Configure - parseUnitAddress failed");
	return false;
    }

    return parseParams(cfg, u, node);
}

void
UnitelwayUnit::WriteParam(unsigned int num, short val)
{
    if (m_single || num >= m_read_count)
	return;

    m_read_base[num] = val;
}

/**
 * Unitelway line.
 * Implements UniTE Slave protocol.
 */
class UnitelwayLine
{
    public:
	UnitelwayLine() : do_request(false), m_state(WAITING), m_address(0), m_init(true), m_ignore_timeout(false), m_read_try(0), m_current_unit(NULL), m_current_unit_num(0), m_current_param(0) {};
	~UnitelwayLine();
	
	int ParseConfig(DaemonConfig * cfg, IPCHandler * ipc);
	
	struct event * m_evport;
	struct event * m_evloop;
	struct event * m_evreq;
	struct timeval * m_looptv;
	struct timeval * m_porttv;
	struct timeval * m_reqtv;

	bool do_request;

	enum { WAITING, FRAME, RESPONSE } m_state;
	
	void PerformCycle();
	
	int DoEventRequestTimeout();
	int DoEventNewData();
	int DoSniff();
	
	int Go();
	int Sniff();

    private:
	int m_fd;
	bool m_single;
	bool m_debug;

	IPCHandler *m_ipc;

	DaemonConfig *m_cfg;

	std::vector<UnitelwayUnit *> m_units;

	uint8_t m_address;

	bool m_init;
	bool m_ignore_timeout; //< got response, ignore timeout event

	int m_send_delay;

	int m_read_try;

	UnitelwayUnit * m_current_unit;
	unsigned int m_current_unit_num;
	unsigned int m_current_param;

	static const uint8_t request_eot; //< resp EOT
	static const uint8_t request_ack; //< resp ACK
	static const uint8_t request_word[]; //< frame for word read

	uint8_t m_buffer[MAX_FRAME];
	uint8_t frame_buffer[MAX_FRAME];
	uint8_t send_buffer[MAX_FRAME];

	/** Open serial port
	 * @return file descriptor on success, -1 on error
	 */
	int openPort(); 
	/** Prepare event structures for event loop */
	int prepareEvents();
	
	/** Prepare event structures for sniffing */
	int prepareSniffEvents();

	/** Move loop to next param */
	void nextParam();
	/** Move loop to next read try */
	void nextTry();

	uint8_t checkSum(const uint8_t * buffer, size_t lenght);

	/** Read bytes from serial port
	 * @param buf buffer for data
	 * @param len count of bytes to read
	 * @return 
	 */
	int readBytes(uint8_t * buf, size_t len);

	int sendRequest();
	int writeRequest(size_t lenght);

	int writeACK();
	int writeEOT();
	
	int readFrame(uint8_t * buffer, size_t lenght);
	
	int processFrame(uint8_t * buffer, size_t lenght);
	
	int stateWaiting(uint8_t * buffer, size_t lenght);
	int stateFrame(uint8_t * buffer, size_t lenght);
	int stateResponse(uint8_t * buffer, size_t lenght);
	int sniff(uint8_t * buffer, size_t lenght);

	void setTimeout();
	void removeTimeout();
};

UnitelwayLine::~UnitelwayLine()
{
    delete m_evloop;
    delete m_looptv;
    /*
    delete m_evport;
    free(m_evloop);
    free(m_looptv);
    */
    free(m_evport);
    free(m_porttv);
}

const uint8_t UnitelwayLine::request_eot = 4;
const uint8_t UnitelwayLine::request_ack = 6;
const uint8_t UnitelwayLine::request_word[] = { 0x10, 0x02, 0x06, 10, 0x20, 0, 0xfe, 0, 0, 0, 0x04, 0x07, 232, 3 };

void
UnitelwayLine::nextParam()
{
    m_read_try = 0;
    m_current_param++;

    if (m_current_param >= m_current_unit->m_read_count) {
	m_current_param = 0;
	m_current_unit_num++;

	if (m_current_unit_num >= m_units.size()) {
	    m_current_unit_num = 0;
	    do_request = 0;
	    sz_log(2, "INFO: Done full cycle");
	    return;
	}

	m_current_unit = m_units[m_current_unit_num];
    }
}

void
UnitelwayLine::nextTry()
{
    m_read_try++;
    if (m_read_try >= 2)
	nextParam();
}


uint8_t
UnitelwayLine::checkSum(const uint8_t * buffer, size_t lenght)
{
    unsigned int sum = 0;

    for (size_t i = 0; i < lenght; i++) {
	sum += buffer[i];
    }

    return (sum & 0x0ff);
}

int
UnitelwayLine::readBytes(uint8_t * buf, size_t len)
{
    int ret;
    size_t rd = 0;

    fd_set fds;
    struct timeval tv;

    do {
	ret = read(m_fd, buf + rd, len - rd);
	if (ret < 0) {
	    sz_log(0, "ERROR: readBytes: read");
	    if (m_debug)
		perror("ERROR: readBytes");
	    return -1;
	}

	rd += ret;
	if (rd == len)
	    return 0;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(m_fd, &fds);

	ret = select(m_fd + 1, &fds, NULL, NULL, &tv);
	if (ret < 0) {
	    perror("read_bytes: select");
	    return -1;
	}

    } while (rd < len);

    return 0;
}

int
UnitelwayLine::processFrame(uint8_t * buffer, size_t lenght)
{
    if (buffer[2] != m_address)
	return 0; //not for me, ignore

    short val = SZARP_NO_DATA;
    int sum = checkSum(buffer, lenght - 1);
    
    int fsum = buffer[lenght - 1];
    
    if (m_debug) {
	fprintf(stderr, "FRAME: ");
	hexdump(stderr, lenght, buffer);
	fprintf(stderr, "\n");
    }
    
    if (sum != fsum) {
	sz_log(1, "WARING: processFrame: ignoring, checksum incorrect: [%d != %d]\n", sum, fsum);

	if (m_debug)
	    fprintf(stderr, "WARING: processFrame: ignoring, checksum incorrect: [%d != %d]\n", sum, fsum);

	m_state = WAITING;
	m_read_try++;
	removeTimeout();
	
	return -1;
    }

    sz_log(10, "DEBUG: processFrame: message for me (state: %d, type: %d)\n", m_state, buffer[10]);
    writeACK();

    if (m_state != RESPONSE) {
	sz_log(1, "WARING: processFrame: message without request");
	return -1;
    }

    if (buffer[10] != 0x34) {
	sz_log(1, "WARING: processFrame: unknown message type\n");
	return -1;
    }
    
    removeTimeout();
    m_state = WAITING;

    val = (unsigned short) buffer[11] | ((unsigned short) buffer[12] << 8);
    
    sz_log(3, "DEBUG: processFrame: unit: %d, word %d, value %d [%x %x]",
	    m_current_unit->m_id,
	    m_current_unit->m_read_addrs[m_current_param],
	    val, buffer[11], buffer[12]);

    if (m_debug)
	fprintf(stderr, "DEBUG: processFrame: unit: %d, word %d, value %d",
	    m_current_unit->m_id,
	    m_current_unit->m_read_addrs[m_current_param],
	    val);



    m_current_unit->WriteParam(m_current_param, val);

    nextParam();

    return 0;
}

int
UnitelwayLine::readFrame(uint8_t * buffer, size_t lenght)
{
    if (lenght < 4) {
	readBytes(buffer + lenght, 4 - lenght); // TODO error handling
	lenght = 4;
    }
    
    size_t frame_lenght = buffer[3] + 5; //5 for header and checksum

    if (lenght < frame_lenght) {
	readBytes(buffer + lenght, frame_lenght - lenght); // TODO error handling
	lenght = frame_lenght;
    }

    processFrame(buffer, frame_lenght); 

    return lenght;
}

int
UnitelwayLine::writeEOT()
{
    usleep(500);

    return write(m_fd, &(request_eot), 1);
}

int
UnitelwayLine::writeACK()
{
    usleep(500);

    return write(m_fd, &(request_ack), 1);
}

int
UnitelwayLine::writeRequest(size_t lenght)
{
    int ret;
    size_t i = 0;

    for (; i < lenght; i++) {
	ret = write(m_fd, frame_buffer + i, 1);
	if (ret < 0) {
	    sz_log(0, "ERROR: write_frame: write");
	    if (m_debug)
		perror("ERROR: writeRequest: write");

	    return -1;
	}
	usleep(50);
    }

    return i + 1;
}

int
UnitelwayLine::sendRequest()
{
    unsigned int i, j;

    memset(send_buffer, 0, MAX_FRAME);
    memset(frame_buffer, 0, MAX_FRAME);

    memcpy(send_buffer, request_word, sizeof(request_word));

    send_buffer[2] = m_address;

    int addr = m_current_unit->m_read_addrs[m_current_param];

    send_buffer[12] = addr & 0xFF;
    send_buffer[13] = (addr >> 8) & 0xFF;

    frame_buffer[0] = send_buffer[0];
    for (i = 1, j = 1; i < sizeof(request_word); i++) {
	frame_buffer[j++] = send_buffer[i];
	if (send_buffer[i] == 0x10) {
	    frame_buffer[j++] = 0x10;
	}
    }

    uint8_t cs = checkSum(frame_buffer, j + 1);
    frame_buffer[j] = cs;

    sz_log(5, "DEBUG: GETTING UNIT %d, WORD %d (try: %d)",
	    m_current_unit_num,
	    m_current_unit->m_read_addrs[m_current_param],
	    m_read_try);

    if (m_debug) {
	fprintf(stderr, "DEBUG: GETTING UNIT %d, WORD %d (try: %d)\n",
	    m_current_unit_num,
	    m_current_unit->m_read_addrs[m_current_param],
	    m_read_try);
	
	    fprintf(stderr, "\tsend_request j:%d, sum:%d, ", j, cs);
	    hexdump(stderr, j + 1, frame_buffer);
	    fprintf(stderr, "\n");
    }

    return writeRequest(j + 1);
}

void
UnitelwayLine::setTimeout()
{
    timerclear(m_reqtv);
    m_reqtv->tv_sec = 0;
    m_reqtv->tv_usec = 600 * 1000;

    event_add(m_evreq, m_reqtv);
    m_ignore_timeout = false;
}

void
UnitelwayLine::removeTimeout()
{
    m_ignore_timeout = true;
    event_del(m_evreq);
}

/** State machine - waiting for pool */
int
UnitelwayLine::stateWaiting(uint8_t * buffer, size_t lenght)
{
    if (lenght == 0) {
	event_add(m_evport, NULL);
	return 0;
    }

    size_t len;

    switch (buffer[0]) {
	case 0x10:
	    if (lenght < 3) {
		readBytes(buffer + lenght, 3 - lenght);
		lenght = 3;
	    }
	    
	    switch (buffer[1]) {
		case 0x05: // pool
		    sz_log(10, "%ld state_waiting pool: %d", mdiff(), buffer[2]);
		    if (buffer[2] == m_address) { // pool for me
			if (!m_init && do_request) {
			    sz_log(10, "%ld state_waiting: sending request", mdiff());
			    if (sendRequest() > 0) {
				m_state = FRAME;
				
				timerclear(m_porttv);
				m_porttv->tv_usec = 40000;

			        event_add(m_evport, m_porttv);
				return 0;
			    }
			}
			else {
			    sz_log(10, "%ld state_waiting: writing eot", mdiff());
			    if (m_init) {
				m_init = 0;
			    }
			    writeEOT();
			}
		    }
		    break;
		    
		case 0x02: 
		    len = readFrame(buffer, lenght);
		    if (len < lenght)
			return stateWaiting(buffer + len, lenght - len);
		    break;
		    
		default: //error
		    sz_log(1, "state_waiting error: unrecognized frame type");
		    break;
	    }
	    break;

	case 0x04: //EOT
	    sz_log(10, "%ld state_waiting: got EOT", mdiff());
	    if (lenght > 1) {
		return stateWaiting(buffer + 1, lenght - 1);
	    }
	    break;

	case 0x06: // ACK
	    sz_log(10, "%ld state_waiting got ACK\n", mdiff());
	    if (lenght > 1) {
		return stateWaiting(buffer + 1, lenght - 1);
	    }
	    break;

	default:
	    sz_log(1, "state_waiting: unrecognized frame");
	    break;
    }

    event_add(m_evport, NULL);

    return 0;
}

/** State machine - request sent, wait for confirmation */
int
UnitelwayLine::stateFrame(uint8_t * buffer, size_t lenght)
{
    if (lenght > 0 && buffer[0] == 0x06) {
	sz_log(10, "DEBUG: stateFrame: got ACK");

	if (m_debug)
	    fprintf(stderr, "state_frame: got ACK\n");
	
	m_state = RESPONSE;
	setTimeout();

	if (lenght > 1)
	    return stateResponse(buffer + 1, lenght - 1);

	event_add(m_evport, NULL);
	return 0;
    }

    m_state = WAITING;

    sz_log(0, "ERROR: stateFrame: no ACK in 40 ms");

    if (m_debug)
	fprintf(stderr, "ERROR: stateFrame: no ACK in 40ms\n");
 
    nextTry();

    return stateWaiting(buffer + 1, lenght - 1);
}

/** State machine - request accepted, wait for response */
int
UnitelwayLine::stateResponse(uint8_t * buffer, size_t lenght)
{
    switch (buffer[0]) {
	case 0x04: //got EOT
	case 0x06: //got ACK
	    sz_log(10, "DEBUG: %ld stateResponse: got EOT or ACK (%x) (not for me)", mdiff(), buffer[0]);
	    if (lenght > 1)
		return stateResponse(buffer + 1, lenght - 1);
	    break;

	case 0x10:
	    if (lenght < 3) {
		readBytes(buffer + lenght, 3 - lenght);
		lenght = 3;
	    }
	    switch (buffer[1]) {
		case 0x05: // pool
		    sz_log(10, "DEBUG: stateResponse pool: %d\n", buffer[2]);
		    if (buffer[2] == m_address)  // pool for me
			writeEOT();
		    if (lenght > 3)
			return stateResponse(buffer + 3, lenght - 3);
		    break;
		    
		case 0x02:
		    sz_log(10, "DEBUG: stateResponse: got frame");
		    readFrame(buffer, lenght);
		    break;
		    
		default: //error
		    sz_log(0, "ERROR: stateResponse: unrecognized frame type");
		    break;
	    }
	    break;

	default:
	    sz_log(0, "ERROR: stateResponse: unrecognized message");
	    break;
    }

    event_add(m_evport, NULL);

    return 0;
}

int
UnitelwayLine::openPort()
{
    m_fd = open(m_cfg->GetDevicePath(), O_RDWR | O_NDELAY | O_NONBLOCK);
    if (m_fd < 0) {
	perror("opening port");
	return -1;
    }

    struct termios rsconf;

    int ret = tcgetattr(m_fd, &rsconf);
    if (ret < 0) {
	perror("tcgetattr");
	close(m_fd);
	return -1;
    }

/*    rsconf.c_cflag = B9600 | CS8 | PARODD | CLOCAL | CREAD; */
    rsconf.c_cflag = B9600 | CS8 | PARENB | PARODD | CREAD | CLOCAL | CRTSCTS;
    rsconf.c_cflag &= ~CSTOPB; 

    rsconf.c_iflag = rsconf.c_oflag = rsconf.c_lflag = 0;
    rsconf.c_iflag &= ~IXOFF;
    rsconf.c_iflag &= ~IXON;
    rsconf.c_iflag |= INPCK;

    rsconf.c_cc[VMIN] = 0;
    rsconf.c_cc[VTIME] = 0;
    rsconf.c_cc[VEOF] = 0x1a;
    rsconf.c_cc[VSTART] = 0x11;
    rsconf.c_cc[VSTOP] = 0x13;

    ret = tcsetattr(m_fd, TCSANOW, &rsconf);
    if (ret < 0) {
	perror("tcsetattr");
	close(m_fd);
	return -1;
    }
    
    unsigned int serial_config = 0;
 
    // Configure the serial port
    ioctl(m_fd, TIOCMGET, &serial_config);

    serial_config &= ~TIOCM_DTR;
    serial_config |= TIOCM_RTS;

    ret = ioctl(m_fd, TIOCMSET, &serial_config);
    if (ret < 0) {
	perror("tcsetattr");
	close(m_fd);
	return -1;
    }

    return m_fd;
}

int
UnitelwayLine::prepareEvents()
{
    m_evloop = new struct event;
    m_looptv = new struct timeval;

    evtimer_set(m_evloop, callback_main_loop, (void *) this);
    timerclear(m_looptv);
    m_looptv->tv_sec = 10;

    event_add(m_evloop, m_looptv);


    m_evreq = new struct event;
    m_reqtv = new struct timeval;

    evtimer_set(m_evreq, callback_request_timeout, (void *) this);


    m_evport = new struct event;
    m_porttv = new struct timeval;

    event_set(m_evport, m_fd, EV_READ, callback_data_on_port, (void *) this);
    timerclear(m_porttv);
    m_porttv->tv_sec = 10;

    /* Add it to the active events, without a timeout */
    event_add(m_evport, NULL);

    return 0;
}

/*
 * No response for request in 1 sec
 */
int
UnitelwayLine::DoEventRequestTimeout()
{
    if (m_ignore_timeout) 
	return 0;

    sz_log(2, "INFO: DoEventRequestTimeout");

    if (m_state == RESPONSE) {
	printf("No response in given time\n");
	m_state = WAITING;

	nextTry();
    }

    return 0;
}

/** Callback - new data received */
int
UnitelwayLine::DoEventNewData()
{
    size_t chars;


    int rd = ioctl(m_fd, FIONREAD, &chars);
    if (rd < 0) {
	sz_log(0, "ERROR: DoEventNewData: cannot get chars count");
	event_add(m_evport, NULL);
	return -1;
    }

    rd = read(m_fd, m_buffer, chars);
    if (rd < 0) {
	sz_log(0, "ERROR: EventData: read error");
	event_add(m_evport, NULL);
	return -1;
    }

    sz_log(5, "DEBUG DoEventNewData: read %d", rd);

    if (rd == 0) {
	sz_log(1, "EventData: no data received\n");
	event_add(m_evport, NULL);
	return 0;
    }
/*
    fprintf(stderr, "CB time %ld, chars %d, read: ", t, chars);
    hexdump(stderr, rd, buffer);
    printf(stderr, "\n");
    */

    switch (m_state) {
	case WAITING:
	    return stateWaiting(m_buffer, rd);
	case FRAME:
	    return stateFrame(m_buffer, rd);
	case RESPONSE:
	    return stateResponse(m_buffer, rd);
	default:
	    sz_log(0, "FATAL: UnitelwayLine in unknown state (%d)", m_state);
	    event_loopexit(NULL);
	    return -1;
    }

    return 0;
}

/** Perform read cycle */
void
UnitelwayLine::PerformCycle()
{
    sz_log(3, "DEBUG: PerformCycle");
    if (do_request) { // TODO cleanup after last cycle
	    sz_log(1, "WARING: unfinished cycle: state: %d, unit: %d, param: %d, try: %d",
		    m_state, m_current_unit_num, m_current_param, m_read_try);
	// TODO cleanup after last cycle
	
	m_state = WAITING;
	removeTimeout();
    }

    if (!m_single)
	m_ipc->GoParcook(); //Copy current values to parcook
    
    m_current_unit = m_units[0];
    m_current_unit_num = 0;
    m_current_param = 0;

    do_request = true;
}

int
UnitelwayLine::ParseConfig(DaemonConfig * cfg, IPCHandler * ipc)
{
    short * read_base = NULL;
    short * send_base = NULL;

    m_single = cfg->GetSingle();

    if (!m_single) {
	read_base = ipc->m_read;
	send_base = ipc->m_send;
    }

    xmlNodePtr device_node = cfg->GetXMLDevice();

    xmlXPathContextPtr xp_ctx = xmlXPathNewContext(cfg->GetXMLDoc());
    xp_ctx->node = device_node;
    
    int ret = xmlXPathRegisterNs(xp_ctx, BAD_CAST "ipk", SC::S2U(IPK_NAMESPACE_STRING).c_str());
    assert (ret == 0);
    
    TUnit *unit = cfg->GetDevice()->GetFirstRadio()->GetFirstUnit();

    xmlNodePtr node;
    UnitelwayUnit *ute;
    size_t unit_num = 0;
    char xpath[256];

    char * str;
    char * tmp;
    int addr;
    
    str = (char *) xmlGetNsProp(device_node, BAD_CAST("address"), BAD_CAST(IPKEXTRA_NAMESPACE_STRING));
    if (NULL == str) {
	sz_log(0, "ERROR: attribute unitelway:address not found (line %ld)", xmlGetLineNo(device_node));
	return 1;
    }
    
    addr = strtol(str, &tmp, 0);
    if (tmp[0] != 0) {
	sz_log(0,
		"ERROR: parsing unitelway:address attribute value ('%s') - line %ld",
		    str, xmlGetLineNo(device_node));
	free(str);
	return 1;
    }
    free(str);

    m_address = addr;

    for (; unit; unit = unit->GetNext()) {
	memset(xpath, 0, 256);
	sprintf(xpath, ".//ipk:unit[position()=%zu]", unit_num + 1);
	node = uxmlXPathGetNode(BAD_CAST xpath, xp_ctx);

	assert(node);
	
	ute = new UnitelwayUnit();
	
	if (!ute->Configure(cfg, unit, node, read_base, send_base)) {
	    sz_log(0, "ERROR: cannot configure unite %d", unit_num);
	    delete ute;
	    return false;
	}
	
	m_units.push_back(ute);
	read_base += unit->GetParamsCount();
	send_base += unit->GetSendParamsCount();
	
	unit_num++;
    }
    
    m_debug = m_single || cfg->GetDiagno();
    
    m_fd = -1;
    m_cfg = cfg;
    m_ipc = ipc;
    
    return 1;
}

/** Start event loop */
int
UnitelwayLine::Go()
{

    if (openPort() < 0) {
	return 1;
    }

    event_init();

    prepareEvents();

    event_dispatch();

    return 0;
}


int
UnitelwayLine::prepareSniffEvents()
{
    m_evport = new struct event;
    m_porttv = new struct timeval;

    event_set(m_evport, m_fd, EV_READ, callback_sniff, (void *) this);
    timerclear(m_porttv);
    m_porttv->tv_sec = 10;

    /* Add it to the active events, without a timeout */
    event_add(m_evport, NULL);

    return 0;
}

/** Interpret recived data. */
int
UnitelwayLine::sniff(uint8_t * buffer, size_t lenght)
{
    if (lenght == 0) {
	event_add(m_evport, NULL);
	return 0;
    }

    size_t len;

    switch (buffer[0]) {
	case 0x10:
	    if (lenght < 3) {
		readBytes(buffer + lenght, 3 - lenght);
		lenght = 3;
	    }
	    
	    switch (buffer[1]) {
		case 0x05: // pool
		    fprintf(stderr, "%ld POOL: %d", mdiff(), buffer[2]);
		    if (lenght > 3) {
			sniff(buffer + 3, lenght - 3);
		    }
		    break;
		    
		case 0x02: 
		    len = readFrame(buffer, lenght);
		    if (len < lenght)
			return sniff(buffer + len, lenght - len);
		    break;
		    
		default: //error
		    fprintf(stderr, "SNIFF: unrecognized frame type\n");
		    break;
	    }
	    break;

	case 0x04: //EOT
	    fprintf(stderr, "%ld SNIFF: got EOT", mdiff());
	    if (lenght > 1) {
		return sniff(buffer + 1, lenght - 1);
	    }
	    break;

	case 0x06: // ACK
	    fprintf(stderr, "%ld SNIFF: got ACK\n", mdiff());
	    if (lenght > 1) {
		return sniff(buffer + 1, lenght - 1);
	    }
	    break;

	default:
	    fprintf(stderr, "SNIFF: unrecognized frame\n");
	    break;
    }

    event_add(m_evport, NULL);

    return 0;
}


/** Do sniff cycle.
 * Read data and show interpretation.
 */
int
UnitelwayLine::DoSniff()
{
    size_t chars;

    int rd = ioctl(m_fd, FIONREAD, &chars);
    if (rd < 0) {
	fprintf(stderr, "ERROR: DoSniff: cannot get chars count\n");
	event_add(m_evport, NULL);
	return -1;
    }

    rd = read(m_fd, m_buffer, chars);
    if (rd < 0) {
	fprintf(stderr, "ERROR: EventData: read error\n");
	event_add(m_evport, NULL);
	return -1;
    }

    fprintf(stderr, "SNIFF: read %d\n", rd);

    if (rd == 0) {
	event_add(m_evport, NULL);
	return 0;
    }

    hexdump(stderr, rd, m_buffer);

    sniff(m_buffer, rd);

    return 0;
}

/**
 * Start sniffing.
 */
int
UnitelwayLine::Sniff()
{
    if (openPort() < 0) {
	return 1;
    }

    event_init();

    prepareSniffEvents();

    event_dispatch();

    return 0;
}

/** Event callback for sniffing */
void
callback_sniff(int fd, short event, void * arg)
{
    class UnitelwayLine * u = (class UnitelwayLine *) arg;

    u->DoSniff();
}

/** Event callback - new data on port */
void
callback_data_on_port(int fd, short event, void * arg)
{
    class UnitelwayLine * u = (class UnitelwayLine *) arg;

    u->DoEventNewData();
}

/** Event callback - timeout waiting for responce on last request */
void
callback_request_timeout(int fd, short event, void * arg)
{
    class UnitelwayLine * u = (class UnitelwayLine *) arg;
    u->DoEventRequestTimeout();
}

/** Event callback - main loop, start read cycle */
void
callback_main_loop(int fd, short event, void * arg)
{
    class UnitelwayLine * u = (class UnitelwayLine *) arg;
    u->PerformCycle();
    event_add(u->m_evloop, u->m_looptv);
}

int
main (int argc, char * argv[])
{
    DaemonConfig   *cfg;
    IPCHandler     *ipc;
    
    cfg = new DaemonConfig("unitedmn");
    
    if (cfg->Load(&argc, argv))
	return 1;
    
    ipc = new IPCHandler(cfg);
    if (!cfg->GetSingle()) {
	if (ipc->Init()) {
	    sz_log(0, "Initialization of IPC failed");
	    return 1;
	}
    }

    UnitelwayLine * unite = new UnitelwayLine();
    
    if (unite->ParseConfig(cfg, ipc) == false) {
	delete unite;
	return 1;
    }

    if (cfg->GetSniff()) {
	unite->Sniff();
    }
    else {
	unite->Go();
    }

    return 0;
}

