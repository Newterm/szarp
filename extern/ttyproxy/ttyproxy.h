#define TERMIOS_CHANGE 	0
#define SET_DTR 	1
#define CLEAR_DTR 	(1 << 1)
#define SET_RTS		(1 << 2) 
#define CLEAR_RTS	(1 << 4) 

#define TTYPROXY_MAGIC 'T'
#define TTYPROXY_IOCSREADY _IO(TTYPROXY_MAGIC, 2)
