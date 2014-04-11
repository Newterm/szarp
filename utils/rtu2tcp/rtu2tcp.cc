#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <string>
#include <vector>
#include <map>

int g_debug = 0;

struct PDU {
	unsigned char func_code;
	std::vector<unsigned char> data;
};

class modbus_rtu_stream;
class modbus_tcp_client;

class modbus_proxy
{
    public:
	modbus_proxy(struct event_base * base) : m_event_base(base) {};
	~modbus_proxy();

	int configure(std::string & src_addr, int src_port, std::string & dst_addr, int dst_port);


	void add_map(unsigned char uid, unsigned short src_addr, unsigned short dst_addr);

	void forward(unsigned char uid, unsigned short address, std::vector<short> & vals);

    protected:
	struct event_base * m_event_base;

	modbus_rtu_stream * m_rtu_stream;
	modbus_tcp_client * m_tcp_client;

	std::map<unsigned int, unsigned short> m_reg_map;

	bool m_reorder;
};

class modbus_tcp_client
{

    public:
	modbus_tcp_client(struct event_base * base, modbus_proxy * proxy) : m_event_base(base), m_proxy(proxy) {} ; 

	int configure(std::string & address, int port);

	void write_multiple_registers(unsigned short address, std::vector<short> & vals);

	static void bev_read_cb(struct bufferevent *bev, void * ptr);
	static void bev_write_cb(struct bufferevent *bev, void * ptr);
	static void bev_event_cb(struct bufferevent *bev, short events, void * ptr);

    protected:
	struct event_base * m_event_base;
	modbus_proxy * m_proxy;

	struct bufferevent * m_bev;
	struct evbuffer * m_buffer;
	struct sockaddr_in m_sin;

	short m_trans_id;

	union {
	    char c[2];
	    unsigned short v;
	} m_short_conv;

	void send_pdu(PDU & pdu);

	void on_connected();
	void on_error();
	void on_timeout();
	void on_read();
	void on_write();

};

class modbus_rtu_stream
{

    public:
	modbus_rtu_stream(struct event_base * base, modbus_proxy * proxy) ;

	int configure(std::string & address, int port);

	static void bev_read_cb(struct bufferevent *bev, void * ptr);
	static void bev_write_cb(struct bufferevent *bev, void * ptr);
	static void bev_event_cb(struct bufferevent *bev, short events, void * ptr);


    protected:
	void on_connected();
	void on_error();
	void on_timeout();
	void on_read();
	void on_write();

	int parse_frame();
	void parse_request();
	void parse_response();

	struct event_base * m_event_base;
	modbus_proxy * m_proxy;

	struct bufferevent * m_bev;
	struct evbuffer * m_buffer;

	struct sockaddr_in m_sin;
	struct timeval m_read_timeout;
	struct timeval m_resp_timeout;

	union {
	    char c[2];
	    unsigned short v;
	} m_short_conv;

	unsigned char m_uid;
	unsigned char m_function;

	unsigned short m_addr;
	unsigned short m_count;
};

int modbus_tcp_client::configure(std::string & address, int port)
{
    m_buffer = evbuffer_new();

    memset(&m_sin, 0, sizeof(m_sin));
    m_sin.sin_family = AF_INET;
    m_sin.sin_port = htons(port); 

    if (!inet_aton(address.c_str(), &m_sin.sin_addr)) {
	printf("Address conv error\n");
	return -1;
    }

    m_bev = bufferevent_socket_new(m_event_base, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(m_bev,
	    modbus_tcp_client::bev_read_cb,
	    NULL,
	    modbus_tcp_client::bev_event_cb,
	    this);
    bufferevent_enable(m_bev, EV_READ | EV_WRITE);

    if (bufferevent_socket_connect(m_bev, (struct sockaddr *)&m_sin, sizeof(m_sin)) == -1) {
        /* Error starting connection */
	printf("Error starting connection");
        bufferevent_free(m_bev);
        return -1;
    }

    printf("modbus_tcp_client::configure done\n");

    return 0;
}

void modbus_tcp_client::send_pdu(PDU & pdu)
{
    unsigned short tcp_header[4];
    tcp_header[0] = htons(m_trans_id++);
    tcp_header[1] = 0; //protocol
    tcp_header[2] = htons((unsigned short)(2 + pdu.data.size()));

    m_short_conv.c[0] = 0x01;
    m_short_conv.c[1] = pdu.func_code;

    tcp_header[3] = m_short_conv.v;  // uid & func_code

    if (g_debug) {
	unsigned char * p = (unsigned char *)tcp_header;

	printf("modbus_tcp_client::send_pdu: ");
	for (size_t i = 0; i < sizeof(tcp_header); i++)
	    printf("%02x", p[i]);
	printf(" ");
	for (size_t i = 0; i < pdu.data.size(); i++)
	    printf("%02x", pdu.data[i]);
	printf("\n");
    }

    bufferevent_write(m_bev, static_cast<void *>(tcp_header), sizeof(tcp_header));
    bufferevent_write(m_bev, static_cast<void *>(&(pdu.data[0])), pdu.data.size());
}

void modbus_tcp_client::write_multiple_registers(unsigned short address, std::vector<short> & vals)
{
    if (g_debug)
	printf("modbus_tcp_client::write_multiple_registers: addr: %d, count: %d\n", address, vals.size());

    if (vals.size() == 0) {
	printf("modbus_tcp_client::write_multiple_registers: no values, not doing\n");
	return;
    }

    size_t val_len = vals.size() * 2;

    PDU pdu;
    pdu.func_code = 16;
    pdu.data.reserve(6 + val_len);
    
    m_short_conv.v = address;

    pdu.data.push_back(m_short_conv.c[1]);
    pdu.data.push_back(m_short_conv.c[0]);

    m_short_conv.v = vals.size();

    pdu.data.push_back(m_short_conv.c[1]);
    pdu.data.push_back(m_short_conv.c[0]);

    pdu.data.push_back(val_len);

    for (size_t i = 0; i < vals.size(); i++) {
	m_short_conv.v = vals[i];
	pdu.data.push_back(m_short_conv.c[0]);
	pdu.data.push_back(m_short_conv.c[1]);
    }

    send_pdu(pdu);
}

void modbus_tcp_client::on_connected()
{
    printf("Connected\n");
    m_trans_id = 0;
}

void modbus_tcp_client::on_error()
{
    printf("Error in modbus_tcp_client, exiting\n");
     /* An error occured while connecting. */
    bufferevent_free(m_bev);
    event_base_loopexit(m_event_base, NULL);
}

void modbus_tcp_client::on_read()
{
    unsigned char in_buff[512];
    size_t n = bufferevent_read(m_bev, static_cast<void *>(in_buff), sizeof(in_buff));
    if (g_debug) {
	printf("tcp_client got response: ");
	for (size_t i = 0; i < n; i++) {
	    printf("%02x", in_buff[i]);
	}
	printf("\n");
    }
}

void modbus_tcp_client::on_write()
{
    printf("tcp_client on write\n");
}

void modbus_tcp_client::on_timeout()
{
    return;
}

void modbus_tcp_client::bev_read_cb(struct bufferevent *bev, void * ptr)
{
    modbus_tcp_client * tcp_client = static_cast<modbus_tcp_client *>(ptr);
    tcp_client->on_read();
}

void modbus_tcp_client::bev_write_cb(struct bufferevent *bev, void * ptr)
{
    modbus_tcp_client * tcp_client = static_cast<modbus_tcp_client *>(ptr);
    tcp_client->on_write();
}

void modbus_tcp_client::bev_event_cb(struct bufferevent *bev, short events, void *ptr)
{
    modbus_tcp_client * tcp_client = static_cast<modbus_tcp_client *>(ptr);

    if (events & BEV_EVENT_CONNECTED) {
	tcp_client->on_connected();
	return;
    }
    if (events & BEV_EVENT_ERROR) {
	tcp_client->on_error();
	return;
    } 
    if (events & (BEV_EVENT_TIMEOUT | BEV_EVENT_READING)) {
	tcp_client->on_timeout();
	return;
    }

    printf("Warn: unrecognized event\n");
}

modbus_proxy::~modbus_proxy()
{
    delete m_tcp_client;
    delete m_rtu_stream;
}

int modbus_proxy::configure(std::string & src_addr, int src_port, std::string & dst_addr, int dst_port)
{
    m_reorder = true;

    m_tcp_client = new modbus_tcp_client(m_event_base, this);
    if (m_tcp_client->configure(dst_addr, dst_port) == -1) {
	printf("Cannot configure TCP client\n");
	return 1;
    }

    m_rtu_stream = new modbus_rtu_stream(m_event_base, this);
    if (NULL == m_rtu_stream) {
	printf("Something went terribly wrong\n");
    }
    if (m_rtu_stream->configure(src_addr, src_port) == -1) {
	printf("Cannot configure RTU stream sniffer\n");
	return 1;
    }

    return 0;
}

void modbus_proxy::forward(unsigned char uid, unsigned short address, std::vector<short> & vals)
{
    unsigned int key = (uid << 16) + address;
    std::map<unsigned int, unsigned short>::iterator it = m_reg_map.find(key);
    if (it == m_reg_map.end()) {
	// reg not mapped, ignore
	return;
    }

    if (g_debug)
	printf("Forwarding data uid: %d, addr: %d, dst addr: %d\n", uid, address, it->second);

    if (m_reorder) {
	std::vector<short> tmp;
	tmp.reserve(vals.size());
	for (size_t i = 0; i < vals.size(); i += 2) {
	    tmp.push_back(vals[i + 1]);
	    tmp.push_back(vals[i]);
	}
	m_tcp_client->write_multiple_registers(it->second, tmp);
    }
    else {
	m_tcp_client->write_multiple_registers(it->second, vals);
    }
}


void modbus_proxy::add_map(unsigned char uid, unsigned short src_addr, unsigned short dst_addr)
{
    unsigned int key = (uid << 16) + src_addr;

    m_reg_map[key] = dst_addr;
}

modbus_rtu_stream::modbus_rtu_stream(struct event_base * base, modbus_proxy * proxy) : m_event_base(base), m_proxy(proxy), m_buffer(NULL)
{
    m_buffer = evbuffer_new();
}


int modbus_rtu_stream::configure(std::string & address, int port)
{
    if (g_debug)
	printf("modbus_rtu_stream::configure addr: %s, port: %d\n", address.c_str(), port);

    m_read_timeout.tv_sec = 0;
    m_read_timeout.tv_usec = 3000;

    memset(&m_sin, 0, sizeof(m_sin));
    m_sin.sin_family = AF_INET;
    m_sin.sin_port = htons(port); 

    if (!inet_aton(address.c_str(), &m_sin.sin_addr)) {
	printf("Address conv error\n");
	return -1;
    }

    m_bev = bufferevent_socket_new(m_event_base, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(m_bev,
	    modbus_rtu_stream::bev_read_cb,
	    modbus_rtu_stream::bev_write_cb,
	    modbus_rtu_stream::bev_event_cb,
	    this);
    bufferevent_set_timeouts(m_bev, &m_read_timeout, NULL);
    bufferevent_enable(m_bev, EV_READ);

    if (bufferevent_socket_connect(m_bev, (struct sockaddr *)&m_sin, sizeof(m_sin)) == -1) {
        /* Error starting connection */
	printf("Error starting connection");
        bufferevent_free(m_bev);
        return -1;
    }

    return 0;
}

void modbus_rtu_stream::parse_request()
{
    char frame[8];

    size_t i = evbuffer_remove(m_buffer, frame, 8);
    if (i != 8) {
	// error
	return;
    }

    m_uid = frame[0];

    m_function = frame[1];

    m_short_conv.c[0] = frame[3];
    m_short_conv.c[1] = frame[2];

    m_addr = m_short_conv.v;

    m_short_conv.c[0] = frame[5];
    m_short_conv.c[1] = frame[4];

    m_count = m_short_conv.v;


    if (g_debug)
	printf("\trequest: uid: %d, addr: %d, count: %d\n", m_uid, m_addr, m_count);
}

void modbus_rtu_stream::parse_response()
{
    char frame[9];
    size_t i = evbuffer_remove(m_buffer, frame, 9);
    if (i != 9) {
	// error
	return;
    }

    // TODO crc check

    if (m_uid != frame[0]) {
	// error
	printf("ERROR: response uid differs from last request\n");
	return;
    }

    if (m_count * 2 != (frame[2])) {
	// error
	printf("ERROR: response lenght differs from regs count in request\n");
	return;
    }

    std::vector<short> vals;
    vals.reserve(m_count);
    size_t end = 3 + (m_count * 2) - 1;
    for (size_t i = 3; i < end; i += 2) {
	m_short_conv.c[0] = frame[i];
	m_short_conv.c[1] = frame[i + 1];
	vals.push_back(m_short_conv.v);
    }

    if (g_debug)
	printf("\treqponse: uid: %d, parsed vals: %d\n", frame[0], vals.size());

    m_proxy->forward(m_uid, m_addr, vals);
}

int modbus_rtu_stream::parse_frame()
{
    size_t len = evbuffer_get_length(m_buffer);

    if (len < 3)
	return 1; // not enough data

    char buf[3];

    int i = evbuffer_copyout(m_buffer, buf, 3);
    if (i == -1) {
	// Error
	evbuffer_drain(m_buffer, evbuffer_get_length(m_buffer));
	return 1;
    }

    if (i < 3)
	return 1; // not enough data


    if (buf[1] == 0x03) {
	if (buf[2] == 0x1b) {
	    // request
	    if (len < 8)
		return 1;
	    parse_request();

	}
	else if (buf[2] == 0x04) {
	    // response
	    if (len < 9)
		return 1;
	    parse_response();
	}
    }
    return 0;
}

void modbus_rtu_stream::on_read()
{
    if (evbuffer_get_length(m_buffer) == 0) {
	bufferevent_set_timeouts(m_bev, &m_read_timeout, NULL);
    }

    int ret = bufferevent_read_buffer(m_bev, m_buffer);

    if (ret != 0) {
	//TODO error
	printf("modbus_rtu_stream::on_read bufferevent_read_buffer error\n");
    }
    
    struct timeval ct;
    gettimeofday(&ct, NULL);
   
    if (g_debug)
	printf("[%ld.%ld] New data: read: %d\n", ct.tv_sec, ct.tv_usec, ret);

    bufferevent_enable(m_bev, EV_READ);
}

void modbus_rtu_stream::on_write()
{
    printf("on write (?)\n");
}

void modbus_rtu_stream::on_connected()
{
    printf("Connected\n");
     /* We're connected to 127.0.0.1:8080.   Ordinarily we'd do
	something here, like start reading or writing. */
}

void modbus_rtu_stream::on_error()
{
    printf("Error in modbus_rtu_stream, exiting\n");
     /* An error occured while connecting. */
    bufferevent_free(m_bev);
    event_base_loopexit(m_event_base, NULL);
}

void modbus_rtu_stream::on_timeout()
{
	struct timeval ct;
	if (g_debug) {
	    gettimeofday(&ct, NULL);

	    printf("[%ld.%ld] on_timeout start, in buffer: %d\n",
	       ct.tv_sec, ct.tv_usec, evbuffer_get_length(m_buffer));
	}

	while (evbuffer_get_length(m_buffer) > 0) {
	    if (parse_frame()) {
		if (g_debug)
		    printf("Not enough data, wait for more\n");
		break;
	    }
	}

	if (evbuffer_get_length(m_buffer) == 0) {
	    if (g_debug)
		printf("Empty buffer, wait for first part of data\n");
	    bufferevent_set_timeouts(m_bev, NULL, NULL);
	}

	bufferevent_enable(m_bev, EV_READ);

	if (g_debug) {
	    gettimeofday(&ct, NULL);

	    printf("[%ld.%ld] on_timeout end, in buffer: %d\n",
		   ct.tv_sec, ct.tv_usec, evbuffer_get_length(m_buffer));
	}
}

void modbus_rtu_stream::bev_read_cb(struct bufferevent *bev, void * ptr)
{
    modbus_rtu_stream * rtu_stream = static_cast<modbus_rtu_stream *>(ptr);
    rtu_stream->on_read();
}

void modbus_rtu_stream::bev_write_cb(struct bufferevent *bev, void * ptr)
{
    modbus_rtu_stream * rtu_stream = static_cast<modbus_rtu_stream *>(ptr);
    rtu_stream->on_write();
}

void modbus_rtu_stream::bev_event_cb(struct bufferevent *bev, short events, void *ptr)
{
    modbus_rtu_stream * rtu_stream = static_cast<modbus_rtu_stream *>(ptr);

    if (events & BEV_EVENT_CONNECTED) {
	rtu_stream->on_connected();
	return;
    }
    if (events & BEV_EVENT_ERROR) {
	rtu_stream->on_error();
	return;
    } 
    if (events & (BEV_EVENT_TIMEOUT | BEV_EVENT_READING)) {
	rtu_stream->on_timeout();
	return;
    }

    printf("Warn: unrecognized event\n");
}

void configure_k1(modbus_proxy * proxy)
{
    std::string src_addr;
    std::string dst_addr;

    src_addr = "192.168.127.254";
    int src_port = 4004;

    dst_addr = "127.0.0.1";
    int dst_port = 9007;

    proxy->configure(src_addr, src_port, dst_addr, dst_port);

    proxy->add_map(1, 7006, 2);
    proxy->add_map(1, 7008, 6);
    proxy->add_map(1, 7010, 10);
    proxy->add_map(1, 7012, 14);
    proxy->add_map(2, 7006, 4);
    proxy->add_map(2, 7008, 8);
    proxy->add_map(2, 7010, 12);
    proxy->add_map(2, 7012, 16);
    proxy->add_map(3, 7006, 18);
    proxy->add_map(3, 7008, 20);
    proxy->add_map(4, 7006, 24);
    proxy->add_map(4, 7008, 22);
    proxy->add_map(4, 7010, 28);
    proxy->add_map(4, 7012, 26);
    proxy->add_map(5, 7006, 32);
    proxy->add_map(5, 7008, 30);
}

void configure_k2(modbus_proxy * proxy)
{
    std::string src_addr;
    std::string dst_addr;

    src_addr = "192.168.127.254";
    int src_port = 4005;

    dst_addr = "127.0.0.1";
    int dst_port = 9007;


    proxy->configure(src_addr, src_port, dst_addr, dst_port);

    proxy->add_map(1, 7006, 34);
    proxy->add_map(1, 7008, 38);
    proxy->add_map(1, 7010, 42);
    proxy->add_map(1, 7012, 46);
    proxy->add_map(2, 7006, 36);
    proxy->add_map(2, 7008, 40);
    proxy->add_map(2, 7010, 44);
    proxy->add_map(2, 7012, 48);
    proxy->add_map(3, 7006, 50);
    proxy->add_map(3, 7008, 52);
    proxy->add_map(4, 7006, 56);
    proxy->add_map(4, 7008, 54);
    proxy->add_map(4, 7010, 60);
    proxy->add_map(4, 7012, 58);
    proxy->add_map(5, 7006, 64);
    proxy->add_map(5, 7008, 62);
}

void configure_k4(modbus_proxy * proxy)
{
    std::string src_addr;
    std::string dst_addr;

    dst_addr = "127.0.0.1";
    int dst_port = 9007;

    src_addr = "192.168.127.254";
    int src_port = 4006;

    proxy->configure(src_addr, src_port, dst_addr, dst_port);

    proxy->add_map(1, 7006, 66);
    proxy->add_map(1, 7008, 70);
    proxy->add_map(1, 7010, 74);
    proxy->add_map(1, 7012, 78);
    proxy->add_map(2, 7006, 68);
    proxy->add_map(2, 7008, 72);
    proxy->add_map(2, 7010, 76);
    proxy->add_map(2, 7012, 80);
    proxy->add_map(3, 7006, 82);
    proxy->add_map(3, 7008, 84);
    proxy->add_map(4, 7006, 88);
    proxy->add_map(4, 7008, 86);
    proxy->add_map(4, 7010, 92);
    proxy->add_map(4, 7012, 90);
    proxy->add_map(5, 7006, 96);
    proxy->add_map(5, 7008, 94);
}

void usage(char * basename)
{
    fprintf(stderr, "Usage: %s [-d] -k <nr>\n", basename);
    fprintf(stderr, "\t-d - enable debug messages\n");
    fprintf(stderr, "\t-k <nr> - boiler number\n");
    fprintf(stderr, "\t\t<nr>: possible values: 1, 2, 4\n");
}

int main(int argc, char * argv[])
{
    int opt;
    int boiler = -1;

    while ((opt = getopt(argc, argv, "dk:")) != -1) {
	switch (opt) {
	    case 'd':
		g_debug = 1;
		break;
	    case 'k':
		boiler = atoi(optarg);
		break;
	    default: /* '?' */
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
    }


    struct event_base *base = event_base_new();

    if (NULL == base) {
	printf("No event base!\n");
	return -1;
    }

    modbus_proxy * proxy = new modbus_proxy(base);

    switch(boiler) {
	case 1:
	    configure_k1(proxy);
	    break;
	case 2:
	    configure_k2(proxy);
	    break;
	case 4:
	    configure_k4(proxy);
	    break;
	default:
	    usage(argv[0]);
	    exit(EXIT_FAILURE);
    }

    event_base_dispatch(base);
    printf("Shouldnt go here\n");
    return 0;
}

