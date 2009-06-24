/* 
  SZARP: SCADA software 

*/
/*
 *  Pawe³ Pa³ucha 2002
 * Include file for sender.cfg parser.
 *
 * $Id$
 */

#ifndef __SENDER_CFG_H__
#define __SENDER_CFG_H__

#ifdef __cplusplus
	extern "C" {
#endif

/**
 * Info about single param to send.
 */
typedef struct phSendParam tSendParam;
struct phSendParam {
	int is_const;	/**< Send constant or param value ? */
	int value;	/**< Constant or param number to send */
	int line;	/**< Communication line number */
	char unit;	/**< Communication unit id */
	int input_par;	/**< Input param number */
	int repeat;	/**< Repeat rate */
	int type;	/**< Type: 0 - probe, 1 - min, 2 - 10min, 
			  3 - hour, 4 - day */
	int send_no_data;
			/**< 1 if SZARP_NO_DATA should be send, 0 otherwise */
};
		
/**
 * Information about params to send.
 */
struct phSendConf {
	int num;	/**< Number of params */
	int freq;	/**< Send frequency (in seconds) */
	tSendParam **p;	/**< Array of pointers of params to send */
};
typedef struct phSendConf tSendConf;

tSendConf *parse_sender_cfg(const char *path);

void free_sender_cfg(tSendConf *s);

int scfglex(void);

#ifdef __cplusplus
	}
#endif

#endif
