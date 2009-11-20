#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <event.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "ttyproxy.h"

#define IAC 255
#define WILL 251
#define WONT 252
#define DO 253
#define DONT 254
#define SE 240
#define SB 250

#define SET_BAUDRATE            1
#define SET_DATASIZE            2
#define SET_PARITY              3
#define SET_STOPSIZE            4
#define SET_CONTROL             5
#define NOTIFY_LINESTATE        6
#define NOTIFY_MODEMSTATE       7
#define FLOWCONTROL_SUSPEND     8
#define FLOWCONTROL_RESUME      9
#define SET_LINESTATE_MASK     10
#define SET_MODEMSTATE_MASK    11
#define PURGE_DATA             12

#define FIRST_SETTING		1
#define LAST_SETTING		4

#define COM_PORT_OPTION		44

#define R_MASK_CD	128
#define R_MASK_RI	64
#define R_MASK_DSR	32
#define R_MASK_CTS	16

#define F_BAUDRATE 1
#define F_DATASIZE (1 << 1)
#define F_PARITY (1 << 2)
#define F_STOPSIZE (1 << 3)

#define MAX (a,b) ((a) > (b) ? (a) : (b))

typedef struct {

	int d_fd;
	int r_fd;

	struct bufferevent* l_endp;
	struct bufferevent* r_endp;

	enum { R_DATA,
		R_IAC, 
		R_WILL, 
		R_WONT, 
		R_DO, 
		R_DONT, 
		R_SB, 
		R_SB_CPO, 
		R_SB_S,
		R_SE } r_state;

	enum { R_S_BR, 
		R_S_DS, 
		R_S_SP, 
		R_S_SS, 
		R_S_SC, 
		R_S_NLS, 
		R_S_NMS, 
		R_S_FCS,
		R_S_FCR,
		R_S_LSM,
		R_S_MSM } r_sstate;

	char cbuf[4];
	int cbuf_fill;

	enum  { L_START, L_DATA, L_COMMAND } l_state;
	unsigned char l_data_to_read;

#define FEATURE_SUPPORTED 1
#define FEATURE_NOT_SUPPORTED 0
#define FEATURE_SUPPORT_NOT_KNOWN -1

	char r_supported_settings[LAST_SETTING + 1];

	unsigned int r_settings[LAST_SETTING + 1];

	enum {
		R_CONNECTING,
		R_NEGOTIATION,
		R_RETRIEVING_SETTINGS,
		R_SETTING_LINE_MASK,
		R_TRANSMISSION
	} g_state;
	
	const char *server_address;
	const char *server_port;
	const char *device_path;

	int r_c_wait;
	int r_suspend;

	struct event_base* ev_base;

} state_t;

void fatal(const char* reason);

void usage(const char* prog_name);

void start_negotiation_phase(state_t *s);

void remote_local_cb(struct bufferevent *bufev, void *arg);

void remote_error_cb(struct bufferevent *bufev, short what, void *arg);

void local_read_cb(struct bufferevent *bufev, void *arg);

void local_error_cb(struct bufferevent *bufev, short what, void *arg);

void remote_read_cb(struct bufferevent *bufev, void *arg);

void enter_set_modem_mask_phase(state_t* s);

void remote_write_cb(struct bufferevent *bufev, void *arg);

int set_nonblock(int fd);

void send_neg_command(state_t *s, unsigned char c, unsigned char t);

void send_setting_command(state_t *s, unsigned char c, unsigned char* t, size_t size);

int start_connection(state_t* s);

int open_device(state_t *s);

void close_device(state_t *s);
	
void set_rsubstate(state_t *s, unsigned char c);

void r_suspend(state_t *s);

void r_resume(state_t *s);

void handle_sb_command(state_t *s, unsigned char c);

void ack_feature_received(state_t *s, unsigned char c, int supported);

void handle_set_termios(state_t *s);

void handle_set_flags(state_t* s, unsigned char c);

void start_settings_retrieval_phase(state_t* s);

void fetch_next_setting(state_t *s);

void enter_transmission_phase(state_t *s);

speed_t rfc2217_2_termios_speed(int speed);

tcflag_t rfc2217_2_termios_data_size(int dsize);

tcflag_t rfc2217_2_termios_stop_size(int ssize);

tcflag_t rfc2217_2_termios_parity(int parity);

int termios_2_rfc2217_speed(speed_t speed);

unsigned char termios_2_rfc2217_data_size(tcflag_t cflags);

int termios_2_rfc2217_stop_size(tcflag_t dsize);

int termios_2_rfc2217_parity(tcflag_t cflags);

void modem_state_mask_recevied(state_t* s, unsigned char c);

void schedule_reconnect(state_t *s);

void reconnect_cb(int fd, short event, void *arg);

void usage(const char* prog_name) {
	fprintf(stderr, "usage: %s path_to_tty_proxy_device converter_address converter_port");
	return;
}

void fatal(const char* reason) {
	fprintf(stderr, "%s", reason);
	exit(1);
}

int termios_2_rfc2217_speed(speed_t speed) {
	switch (speed) {
		default:
		case B0:
			return 0;
		case B50:
			return 50;
		case B75:		
			return 75;
		case B110:
			return 110;
		case B134:
			return 134;
		case B150:
			return 150;
		case B200:
			return 200;
		case B300:
			return 300;
		case B600:
			return 600;
		case B1200:
			return 1200;
		case B1800:
			return 1800;
		case B2400:
			return 2400;
		case B4800:
			return 4800;
		case B9600:
			return 9600;
		case B19200:
			return 19200;
		case B38400:
			return 38400;
		case B57600:
			return 57600;
		case B115200:
			return 115200;
		case B230400:
			return 230400;
	}
}

int rfc2177_2_termios_speed(int speed) {
	switch (speed) {
		default:
		case 0:
			return B0;
		case 50:
			return B50;
		case 75:		
			return B75;
		case 110:
			return B110;
		case 134:
			return B134;
		case 150:
			return B150;
		case 200:
			return B200;
		case 300:
			return B300;
		case 600:
			return B600;
		case 1200:
			return B1200;
		case 1800:
			return B1800;
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		case 230400:
			return B230400;
	}
}

unsigned char termios_2_rfc2217_data_size(tcflag_t cflags) {
	switch (cflags & CSIZE) {
		default:
		case CS8:
			return 8;
		case CS7:
			return 7;
		case CS6:
			return 6;
		case CS5:
			return 5;
	}
}

tcflag_t rfc2217_2_termios_data_size(int dsize) {
	switch (dsize) {
		default:
		case 8:
			return CS8;
		case 7:
			return CS7;
		case 6:
			return CS6;
		case 5:
			return CS5;
	}
}

int termios_2_rfc2217_parity(tcflag_t cflags) {
	if ((cflags & PARENB) == 0)
		return 1;

	if (cflags & PARODD)
		return 2;
	else
		return 3;
}

tcflag_t rfc2217_2_termios_parity(int parity) {
	switch (parity) {
		default:
			return 0;
		case 3:
			return PARENB;
		case 2:
			return PARENB | PARODD;
	}
}

int termios_2_rfc2217_stop_size(tcflag_t cflags) {
	return (cflags & CSTOPB) ? 1 : 2;
}

tcflag_t rfc2217_2_termios_stop_size(int stopb) {
	return (stopb == 2) ? CSTOPB : 0;
}


void start_negotiation_phase(state_t *s) {
	s->g_state = R_NEGOTIATION;

	bufferevent_enable(s->r_endp, EV_READ);
}

void local_read_cb(struct bufferevent *bufev, void *arg) {
	state_t* s = arg;

	unsigned char buffer[1024];
	size_t r;
	size_t i = 0, j = 0;


	while (r = bufferevent_read(bufev, buffer, sizeof(buffer))) for (i = 0; i < r; i++) {
		unsigned char c = buffer[i];

		switch (s->l_state) {
			case L_START:
				if (c == 0)
					s->l_state = L_COMMAND;
				else {
					s->l_state = L_DATA;
					s->l_data_to_read = c;
				}
				break;
			case L_DATA:
				buffer[j++] = buffer[i];
				if (--s->l_data_to_read == 0)
					s->l_state = L_START;
				break;
			case L_COMMAND:
				if (c == 0) {
					handle_set_termios(s);
					s->l_state = L_START;
				} else {
					handle_set_flags(s, c);
					s->l_state = L_START;
				}
				break;
		}

		if (j > 0) {
			bufferevent_write(s->r_endp, buffer, j);
			j = 0;
		}

	}

}

void reconnect_cb(int fd, short event, void *arg) {
	state_t *s = arg;

	if (start_connection(s)) {
		fprintf(stderr, "Remote connection failure, scheduling reconnect.");
		schedule_reconnect(s);
	}
}

void schedule_reconnect(state_t *s) {
	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	event_base_once(s->ev_base, -1, EV_TIMEOUT, reconnect_cb, s, &tv);
}

void remote_error_cb(struct bufferevent *bufev, short what, void *arg) {
	state_t *s = arg;
	if (s->g_state == R_CONNECTING || start_connection(s))  {
		fprintf(stderr, "Remote connection failure, scheduling reconnect.");
		schedule_reconnect(s);
	}
	close_device(s);
}

void local_error_cb(struct bufferevent *bufev, short what, void *arg) {
	fatal("Error while communicating with local endpoint");
}

void remote_read_cb(struct bufferevent *bufev, void *arg) {
	state_t* s = arg;

	unsigned char buffer[1024];
	size_t r;
	size_t i = 0, j = 0;


	while (r = bufferevent_read(bufev, buffer, sizeof(buffer))) for (i = 0; i < r; i++) {
		unsigned char c = buffer[i];
		switch (s->r_state) {
			case R_DATA:
				if (c == IAC)
					s->r_state = IAC;
				else
					buffer[j++] = buffer[i];
				break;
			case R_IAC:
				switch (c) {
					case WILL:
						s->r_state = R_WILL;
						break;
					case WONT:
						s->r_state = R_WONT;
						break;
					case DO:
						s->r_state = R_DO;
						break;
					case DONT:
						s->r_state = R_DONT;
						break;
					case SB:
						s->r_state = R_SB;
						break;
					default:
					case IAC:
						s->r_state = R_DATA;
						buffer[j++] = IAC;
						break;
				}
				break;
			case R_WILL:
				fatal("We are client and we shouldn't recive WILL message!!!");
				break;
			case R_WONT:
				fatal("We are client and we shouldn't recive WON'T message!!!");
				break;
			case R_DO:
				ack_feature_received(s, c, 1);
				break;
			case R_DONT:
				ack_feature_received(s, c, 0);
				break;
			case R_SB:
				if (c == COM_PORT_OPTION)
					s->r_state = R_SB_CPO;
				else
					s->r_state = R_SE;
				break;
			case R_SB_CPO:
				s->r_state = R_SB_S;
				set_rsubstate(s, c);
				break;
			case R_SB_S:
				handle_sb_command(s, c);
				break;
			case R_SE:
				if (c == SE)
					s->r_state = R_DATA;
				break;
		}

		if (j > 0) {
			bufferevent_write(s->l_endp, buffer, j);
			j = 0;
		}
	}
}

void remote_write_cb(struct bufferevent *bufev, void *arg) {
	state_t* s = arg;

	if (s->r_c_wait) {
		s->r_c_wait = 0;
		start_negotiation_phase(s);
	}
}

int set_nonblock(int fd) {

	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) {
		perror("Unable to get socket flags");
		return -1;
	}

	flags |= O_NONBLOCK;
	flags = fcntl(fd, F_SETFL, &flags);
	if (flags < 0) {
		perror("Unable to set socket flags");
		return -1;
	}

	return 0;
}

void send_neg_command(state_t *s, unsigned char c, unsigned char t) {
	unsigned char CIAC = IAC;
	bufferevent_write(s->r_endp, &CIAC, 1);
	bufferevent_write(s->r_endp, &c, 1);
	bufferevent_write(s->r_endp, &t, 1);
}

void send_setting_command(state_t *s, unsigned char c, unsigned char* t, size_t size) {
	unsigned char CIAC = IAC;
	unsigned char CSB = SB;
	unsigned char CCPO = COM_PORT_OPTION;
	unsigned char CSE = SE;

	bufferevent_write(s->r_endp, &CIAC, 1);
	bufferevent_write(s->r_endp, &CSB, 1);
	bufferevent_write(s->r_endp, &CCPO, 1);
	bufferevent_write(s->r_endp, &c, 1);
	bufferevent_write(s->r_endp, t, size);
	bufferevent_write(s->r_endp, &CIAC, 1);
	bufferevent_write(s->r_endp, &CSE, 1);
}

int start_connection(state_t* s) {
	int i;
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));

	if (inet_pton(AF_INET, s->server_address, &sin.sin_addr) != 1) {
		perror("Invalid address");
		return 1;
	}

	sin.sin_port = htons(atoi(s->server_port));
	if (sin.sin_port == 0) {
		fprintf(stderr, "Invalid port\n");
		return 1;
	}

	if (s->r_fd >= 0) {
		close(s->r_fd);
		s->r_fd = -1;
	}

	if (s->r_endp) {
		bufferevent_free(s->r_endp);
		s->l_endp = NULL;
	}

	s->r_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (s->r_fd < 0) {
		perror("Socket creation failed");
		return 1;
	}

	if (set_nonblock(s->r_fd)) {
		close(s->r_fd);
		s->r_fd = -1;
		return 1;
	}

	if (connect(s->r_fd, (struct sockaddr*)&sin, sizeof(sin)) == -1) {
		if (errno != EWOULDBLOCK) {
			perror("Failed to connect");
			close(s->r_fd);
			s->r_fd = -1;
			return 1;
		}
		s->r_c_wait = 1;
	} else
		s->r_c_wait = 0;

	s->r_endp = bufferevent_new(s->r_fd, remote_read_cb, remote_write_cb, remote_error_cb, s);
	bufferevent_base_set(s->ev_base, s->r_endp);

	s->g_state = R_CONNECTING;

	s->r_suspend = 0;
	for (i = FIRST_SETTING; i < LAST_SETTING; i++) {
		s->r_supported_settings[i] = FEATURE_SUPPORT_NOT_KNOWN;
		s->r_settings[i] = 0;
	}

	if (!s->r_c_wait)
		start_negotiation_phase(s);		

	return 0;
}

int open_device(state_t *s) {
	close_device(s);

	s->d_fd = open(s->device_path, O_RDWR);

	if (s->d_fd < 0) {
		perror("Failed to open device");
		return 1;
	}

	if (set_nonblock(s->d_fd)) {
		close(s->d_fd);
		return 1;
	}

	s->l_endp = bufferevent_new(s->d_fd, local_read_cb, NULL, local_error_cb, s);
	bufferevent_base_set(s->ev_base, s->l_endp);
	s->l_state = L_START;

	bufferevent_enable(s->r_endp, EV_READ);

	return 0;
}

void close_device(state_t *s) {
	if (s->d_fd >= 0) {
		close(s->d_fd);
		s->d_fd = -1;
	}

	if (s->l_endp) {
		bufferevent_free(s->l_endp);
		s->l_endp = NULL;
	}

}

void set_rsubstate(state_t *s, unsigned char c) {
	switch (c - 100) {
		case SET_BAUDRATE:
			s->r_sstate = R_S_BR;
			s->cbuf_fill = 0;
			break;
		case SET_DATASIZE:
			s->r_sstate = R_S_DS;
			break;
		case SET_PARITY:
			s->r_sstate = R_S_SP;
			break;
		case SET_STOPSIZE:
			s->r_sstate = R_S_SS;
			break;
		case SET_CONTROL:
			s->r_sstate = R_S_SC;
			break;
		case NOTIFY_LINESTATE:
			s->r_sstate = R_S_NLS;
			break;
		case NOTIFY_MODEMSTATE:
			s->r_sstate = R_S_NMS;
			break;
		case FLOWCONTROL_SUSPEND:
			s->r_sstate = R_S_FCS;
			break;
		case FLOWCONTROL_RESUME:
			s->r_sstate = R_S_FCR;
			break;
		case SET_LINESTATE_MASK:
			s->r_sstate = R_S_LSM;
			break;
		case SET_MODEMSTATE_MASK:
			s->r_sstate = R_S_MSM;
			break;
		case PURGE_DATA:
			s->r_sstate = R_SE;
			break;
	}
}

void r_suspend(state_t *s) {
	if (!s->r_suspend) {
		s->r_suspend = 0;
		bufferevent_disable(s->l_endp, EV_READ);
	}
}

void r_resume(state_t *s) {
	if (s->r_suspend) {
		s->r_suspend = 0;
		bufferevent_enable(s->l_endp, EV_READ);
	}
}

void modem_state_mask_recevied(state_t* s, unsigned char c) {
	unsigned int flags = 0;

	if (c & 128)	
		flags |= TIOCM_CD;
	if (c & 64)
		flags |= TIOCM_RNG;
	if (c & 32)
		flags |= TIOCM_DSR;
	if (c & 16)
		flags |= TIOCM_CTS;

	if (s->d_fd >= 0) {
		if (ioctl(s->d_fd, TIOCMSET, &flags))
			fatal("Failed to set modem flags");
	}
}

void handle_sb_command(state_t *s, unsigned char c) {

	switch (s->r_sstate) {
		case R_S_BR:
			s->cbuf[s->cbuf_fill++] = c;

			if (s->cbuf_fill == 4) {
				unsigned speed = (s->cbuf[0] << 24)
					| (s->cbuf[1] << 16)
					| (s->cbuf[2] << 8)
					| (s->cbuf[3]);
				s->r_settings[SET_BAUDRATE] = c;
				if (s->g_state == R_RETRIEVING_SETTINGS)
					fetch_next_setting(s);	
				fprintf(stderr, "Got speed: %u\n", speed);
				s->r_state = R_SE;
			}
			break;
		case R_S_DS:
			if (c == 0)
				fprintf(stderr, "Request for data size, igonred!\n");
			else {
				s->r_settings[SET_DATASIZE] = c;
				if (s->g_state == R_RETRIEVING_SETTINGS)
					fetch_next_setting(s);	
			}
			s->r_state = R_SE;
			break;
		case R_S_SP: 
			switch (c) {
				case 0:
					fprintf(stderr, "Request for parity, ignored!\n");
					break;
				case 1 ... 5:
					s->r_settings[SET_PARITY] = c;
					if (s->g_state == R_RETRIEVING_SETTINGS)
						fetch_next_setting(s);
					break;
				default:
					fprintf(stderr, "Unknown parity\n");
					break;
			}
			s->r_state = R_SE;
			break;
		case R_S_SS:
			switch (c) {
				case 0:
					fprintf(stderr, "Request for stop size, ignored!\n");
					break;
				case 1 ... 3:
					s->r_settings[SET_STOPSIZE] = c;
					if (s->g_state == R_RETRIEVING_SETTINGS)
						fetch_next_setting(s);
					break;
				default:
					fprintf(stderr, "Unknown stop size\n");
					break;
			}
			s->r_state = R_SE;
			break;
		case R_S_SC:
			switch (c) {
				case 0:
					fprintf(stderr, "Request com port flow settings, ignored!\n");
					break;
				case 1:
					fprintf(stderr, "Use no flow control\n");
					break;
				case 2:
					fprintf(stderr, "Use XON/XOFF flow control\n");
					break;
				case 3:
					fprintf(stderr, "Use hardware flow control\n");
					break;
				case 13:
					fprintf(stderr, "Request com port flow settings\n");
					break;
				case 14:
					fprintf(stderr, "Use no flow control\n");
					break;
				case 15:
					fprintf(stderr, "Use XON/OFF flow control\n");
					break;
				case 16:
					fprintf(stderr, "Use hardware flow control\n");
					break;
				default:
					fprintf(stderr, "Unknown/completly unimplemented set control option\n");
					break;
			}
			s->r_state = R_SE;
			break;
		case R_S_NLS:
			fprintf(stderr, "Notify line state mask, ignored!\n");
			s->r_state = R_SE;
			break;
		case R_S_NMS:
			modem_state_mask_recevied(s, c);
			fprintf(stderr, "Notify line state mask, ignored!\n");
			s->r_state = R_SE;
			break;
		case R_S_FCS:
			r_suspend(s);
			s->r_state = R_SE;
			break;
		case R_S_FCR:
			r_resume(s);
			s->r_state = R_SE;
			break;
		case R_S_LSM:
			fprintf(stderr, "Line state mask - confirmation, ignored!\n");
			s->r_state = R_SE;
			break;
		case R_S_MSM:
			if (s->r_state == R_SETTING_LINE_MASK)
				enter_transmission_phase(s);
			s->r_state = R_SE;
			break;
	}
}

void ack_feature_received(state_t *s, unsigned char c, int supported) {
	int i;

	if (s->g_state != R_RETRIEVING_SETTINGS)
		return;

	if (c < FIRST_SETTING || c > LAST_SETTING)
		fatal("Unexpected feature acknowledgement, exiting!");

	s->r_supported_settings[c] = supported;	

	for (i = FIRST_SETTING; i < LAST_SETTING; i++) 
		if (s->r_supported_settings[i] == FEATURE_SUPPORT_NOT_KNOWN)
			break;

	if (i < LAST_SETTING)
		send_neg_command(s, WILL, i);
	else
		enter_set_modem_mask_phase(s);

}

void start_settings_retrieval_phase(state_t* s) {
	s->g_state = R_RETRIEVING_SETTINGS;
	fetch_next_setting(s);
}

void fetch_next_setting(state_t *s) {
	int i;
	for (i = FIRST_SETTING; i < LAST_SETTING; i++) 
		if (s->r_supported_settings[i] == FEATURE_SUPPORTED
				&& s->r_settings[i] == -1)
			break;

	if (i == LAST_SETTING) {
		enter_set_modem_mask_phase(s);
		return;
	}
 
	if (i == SET_BAUDRATE) {
		unsigned char c[4] = { 0, 0, 0, 0 };
		send_setting_command(s, SET_BAUDRATE, c, 4);
	} else {
		unsigned char c = 0;
		send_setting_command(s, i, &c, 1);
	}
}

void enter_set_modem_mask_phase(state_t* s) {
	s->g_state = R_SETTING_LINE_MASK;
	unsigned char c = R_MASK_CD | R_MASK_CTS | R_MASK_RI | R_MASK_DSR;
	send_setting_command(s, SET_MODEMSTATE_MASK, &c, 1);
}

void enter_transmission_phase(state_t* s) {
	s->g_state = R_TRANSMISSION;

	if (open_device(s))
		fatal("Failed to open device, exiting!");


}

void handle_set_flags(state_t* s, unsigned char c) {
	unsigned char command;

	if (c & SET_DTR) {
		command = 8; 	/*set DTR singal on*/
		send_setting_command(s, SET_CONTROL, &command, 1);
	}

	if (c & CLEAR_DTR) {
		command = 9; 	/*set DTR singal off*/
		send_setting_command(s, SET_CONTROL, &command, 1);
	}

	if (c & SET_RTS) {
		command = 11; 	/*set RTS singal off*/
		send_setting_command(s, SET_CONTROL, &command, 1);
	}

	if (c & CLEAR_RTS) {
		command = 12; 	/*set RTS singal off*/
		send_setting_command(s, SET_CONTROL, &command, 1);
	}

}

void handle_set_termios(state_t *s) {
	struct termios ts;
	int speed;
	unsigned char dsize;
	unsigned char parity;
	unsigned char stopb;

	if (tcgetattr(s->d_fd, &ts)) 
		return;

	speed = termios_2_rfc2217_speed(cfgetospeed(&ts));

	if (s->r_supported_settings[SET_BAUDRATE] == FEATURE_SUPPORTED
			&& s->r_settings[SET_BAUDRATE] != speed) {
		unsigned char c[4];
		c[0] = speed >> 24;
		c[1] = (speed >> 16) & 0xff;
		c[2] = (speed >> 8) & 0xff;
		c[3] = speed & 0xff;

		send_setting_command(s, SET_BAUDRATE, c, 4);
	}

	dsize = termios_2_rfc2217_data_size(ts.c_cflag);

	if (s->r_supported_settings[SET_DATASIZE] == FEATURE_SUPPORTED
			&& s->r_settings[SET_DATASIZE] != dsize) 
		send_setting_command(s, SET_DATASIZE, &dsize, 1);

	parity = termios_2_rfc2217_parity(ts.c_cflag);

	if (s->r_supported_settings[SET_PARITY] == FEATURE_SUPPORTED
			&& s->r_settings[SET_PARITY] != parity)
		send_setting_command(s, SET_PARITY, &parity, 1);

	stopb = termios_2_rfc2217_stop_size(ts.c_cflag);
	if (s->r_supported_settings[SET_STOPSIZE] == FEATURE_SUPPORTED
			&& s->r_settings[SET_STOPSIZE] != parity)
		send_setting_command(s, SET_PARITY, &stopb, 1);


}

int main(int argc, char *argv[]) {
	int dfd, sfd;

	if (argc != 4) {
		usage(argv[0]);
		return;
	}

	state_t state;
	memset(&state, 0, sizeof(state));

	state.device_path = argv[1];
	state.server_address = argv[2];
	state.server_port = argv[3];

	state.r_fd = state.d_fd = -1;

	state.ev_base = event_init();

	if (start_connection(&state))
		return 1;

	event_base_loop(state.ev_base, 0);
}
