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
/* $Id$ */

#define _ICON_ICON_
#define _HELP_H_

#include <config.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sstream>
#include <argp.h>

#include "szarp.h"
#include "ipctools.h"
#include "szarp_config.h"
#include "conversion.h"

#include "daemon.h"
#include "liblog.h"
#include "sender.h"

#include "szdefines.h"

#define MY_ID 200		/* sender identifier */

/*
 * Constants representig control message states:
 * 1. ready to send
 * 2. waiting for confirmation
 * 3. already confirmed
 * 4. without report
 * 5. do not send
 */

#define MSG_SEND    0
#define MSG_CONF    1
#define MSG_OK      2
#define MSG_NOREP   3
#define MSG_NOSEND  4

/*
 * Constants representig kind of average:
 * 1. probe
 * 2. 1 minute
 * 3. 10 minutes
 * 4. hour
 */

#define SEN_PROBE	0
#define SEN_MINUTE	1
#define SEN_MIN10	2
#define SEN_HOUR    3
#define SEN_MAX		SEN_HOUR
#if 0
#define SEN_DAY     4
#endif
#define SEN_CONST   10

typedef struct _SterData {	/* data structure with control parameters */
	tMsgSetParam msg;	/* control message */
	unsigned short srcaddr;	/* address of the shared memory  */
	unsigned char status;	/* transmission status */
	unsigned char avgkind;	/* kind of average */
	unsigned char sendnodata;	/* 0 - do not send of the SZARP_NO_DATA value, 1 - send of the SZARP_NO_DATA value */
} tSterData, *pSterData;

unsigned int NumberOfPars;	/* number of parameters to send */
unsigned int BasePeriod;	/* sending frequency */
pSterData SterInfo;		/* array with parameters to send */

/* Arguments handling, see 'info argp'
*/
const char *argp_program_version = "sender" " $Revision: 6711 $";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "SZARP sender application daemon.\v\
Config file szarp.cfg:\n\
These parameters are read from section 'sender' and are optional:\n\
	log		path to log file, default is " PREFIX "/log/sender.log\n\
	log_level	log level, from 0 to 10, default is from command line\n\
			(-Ddebug=needed_log_level parameter) or 2.\n\
";

static struct argp_option options[] = {
	{"D<param>", 0, "val", 0,
	 "Set initial value of libpar variable <param> to val."},
	{"no-daemon", 'n', 0, 0,
	 "Do not fork and go into background, useful for debug."},
	{0}
};

struct arguments {
    int no_daemon;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *)state->input;
	switch (key) {
    case 'n':
        arguments->no_daemon = 1;
        break;
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = {
	options, parse_opt, 0, doc
};

/*
 * Procedure executed when signal is handled
 */
void Terminate(int sig)
{
	sz_log(1, "Exiting on signal %d", sig);
	exit(-1);
}

/*
 * Configuration reads from param.xml file.
 */
void ReadCfgFileFromParamXML(TSzarpConfig * ipk)
{
	int n;
	TSendParam *sp = 0;
	TParam *p = 0;
	BasePeriod = ipk->GetSendFreq();
	NumberOfPars = 0;

	for (TDevice * d = ipk->GetFirstDevice(); d; d = ipk->GetNextDevice(d))
		for (TRadio * r = d->GetFirstRadio(); r; r = d->GetNextRadio(r))
			for (TUnit * u = r->GetFirstUnit(); u;
			     u = r->GetNextUnit(u))
				for (sp = u->GetFirstSendParam(); sp;
				     sp = u->GetNextSendParam(sp))
					if (sp->IsConfigured())
						NumberOfPars += 1;

	if (NumberOfPars <= 0) {
		sz_log(1, "Cannot find parameters to send");
		return;
	}

	if ((SterInfo =
	     (pSterData) calloc(NumberOfPars, sizeof(tSterData))) == NULL) {
		sz_log(1, "Cannot allocate memory for SterInfo structure");
		exit(-2);
	}

	int k = 0;
	for (TDevice * d = ipk->GetFirstDevice(); d; d = ipk->GetNextDevice(d))
		for (TRadio * r = d->GetFirstRadio(); r; r = d->GetNextRadio(r))
			for (TUnit * u = r->GetFirstUnit(); u; u = r->GetNextUnit(u))
				for (n = 0, sp = u->GetFirstSendParam(); sp; sp = u->GetNextSendParam(sp)) {
					if (!sp->IsConfigured()) {
						sz_log(1, "Send parameter not configured");
						continue;
					}
					if (!sp->GetParamName().empty()) {
						p = ipk->getParamByName(sp->GetParamName());
						if (p == NULL) {
							sz_log(1,
							       "Cannot find parameter to send (%s)",
							       SC::S2A(sp->GetParamName()).c_str());
							return;
						}

						SterInfo[k].srcaddr = p->GetIpcInd();
						if (sp->GetProbeType() > SEN_MAX) {
							sz_log(1,
							       "Bad avg kind= %d of file setting NO_PARAM",
							       sp->GetProbeType());
							SterInfo[k].srcaddr = NO_PARAM;
						}
					       	else {
							SterInfo[k].avgkind = (unsigned char)sp->GetProbeType();
						}
						sz_log(10, "Adding send param num %d - sending '%s'",
								k,
								SC::S2A(p->GetName()).c_str()
								);
					} else {
						SterInfo[k].srcaddr = 0;
						SterInfo[k].msg.cont.value = sp->GetValue();
						SterInfo[k].avgkind = SEN_CONST;
						sz_log(10, "Adding send param num %d - const value %d",
								k,
								SterInfo[k].msg.cont.value);
					}

					SterInfo[k].msg.type = u->GetSenderMsgType();
					SterInfo[k].msg.cont.param = (unsigned short)n;
					SterInfo[k].msg.cont.retry = (unsigned char)sp->GetRepeatRate();
					SterInfo[k].msg.cont.rtype = (long)MY_ID * 256L * 256L + (long)k;

					if (sp->GetSendNoData() == 0)
						SterInfo[k].sendnodata = 0;
					else
						SterInfo[k].sendnodata = 1;

					k++;
					n++;
				}
}

/*
 * Shows configuration data read form params.xml file.
 * Used only for diagnostics.
 */
void ShowCfgTable(void)
{
	char buf[255];

	for (unsigned int i = 0; i < NumberOfPars; i++) {
		snprintf(buf, sizeof(buf),
			 "Par %.03d src=%6d dst=%6ld,%2d retry=%d rtype=%ld", i,
			 SterInfo[i].srcaddr, SterInfo[i].msg.type,
			 SterInfo[i].msg.cont.param, SterInfo[i].msg.cont.retry,
			 SterInfo[i].msg.cont.rtype);

		switch (SterInfo[i].avgkind) {
		case SEN_PROBE:
			strncat(buf, " Probe ", 255 - strlen(buf));
			break;

		case SEN_MINUTE:
			strncat(buf, " Minute", 255 - strlen(buf));
			break;

		case SEN_MIN10:
			strncat(buf, " Min10 ", 255 - strlen(buf));
			break;

		case SEN_HOUR:
			strncat(buf, " Hour  ", 255 - strlen(buf));
			break;
#if 0

		case SEN_DAY:
			strncat(buf, " Day   ", 255 - strlen(buf));
			break;
#endif

		case SEN_CONST:
			strncat(buf, " Const ", 255 - strlen(buf));
			break;

		default:
			strncat(buf, " Unknown Avg", 255 - strlen(buf));
			break;
		}

		if (SterInfo[i].avgkind == SEN_CONST)
			strncat(buf, " SENT", 255 - strlen(buf));
		else if (SterInfo[i].sendnodata == 1)
			strncat(buf, " SENT", 255 - strlen(buf));
		else
			strncat(buf, " skipped", 255 - strlen(buf));

		sz_log(3, buf);
	}
}

/*
 * Gets values of the parameters from shared memory.
 */
void GetSharedData(void)
{
	unsigned int i = 0;
	for (; i < NumberOfPars; i++) {
		if (SterInfo[i].srcaddr != NO_PARAM)
			switch (SterInfo[i].avgkind) {
			case SEN_PROBE:
				ipcRdGetInto(SHM_PROBE);
				SterInfo[i].msg.cont.value =
				    Probe[SterInfo[i].srcaddr];
				ipcRdGetOut(SHM_PROBE);
				break;

			case SEN_MINUTE:
				ipcRdGetInto(SHM_MINUTE);
				SterInfo[i].msg.cont.value =
				    Minute[SterInfo[i].srcaddr];
				ipcRdGetOut(SHM_MINUTE);
				break;

			case SEN_MIN10:
				ipcRdGetInto(SHM_MIN10);
				SterInfo[i].msg.cont.value =
				    Min10[SterInfo[i].srcaddr];
				ipcRdGetOut(SHM_MIN10);
				break;
			case SEN_HOUR:
				ipcRdGetInto(SHM_HOUR);
				SterInfo[i].msg.cont.value =
				    Hour[SterInfo[i].srcaddr];
				ipcRdGetOut(SHM_HOUR);
				break;
#if 0
			case SEN_DAY:
				ipcRdGetInto(SHM_DAY);
				SterInfo[i].msg.cont.value =
				    Day[SterInfo[i].srcaddr];
				ipcRdGetOut(SHM_DAY);
				break;
#endif
			case SEN_CONST:
				break;

			default:
				SterInfo[i].msg.cont.value = SZARP_NO_DATA;
				break;
			}
	}

	for (i = 0; i < NumberOfPars; i++)
		if ((SterInfo[i].srcaddr != NO_PARAM)
		    && (SterInfo[i].msg.cont.value != SZARP_NO_DATA))
			SterInfo[i].status = MSG_SEND;
		else if (SterInfo[i].avgkind == SEN_CONST)
			SterInfo[i].status = MSG_SEND;
		else
			SterInfo[i].status =
			    (SterInfo[i].sendnodata) ? MSG_SEND : MSG_NOSEND;
}

/*
 * Sends messages to deamons.
 */
int SendSter(int pass, int log_level)
{
	int i;
	struct msqid_ds msgdata;
	tMsgSetParam msg;
	char allsent = 1;

	for (i = 0; i < (int)NumberOfPars; i++) {
		if (SterInfo[i].status == MSG_SEND) {
			if (!msgsnd(MsgSetDes, &(SterInfo[i].msg), sizeof(tSetParam), IPC_NOWAIT)) {
				SterInfo[i].status = MSG_CONF;
			}
		       	else {
				sz_log(1, "SendSter: failed msgsnd(MsgSetDes, msg[type=%ld], size=%d, IPC_NOWAIT), errno=%d (%s)for parameter %d", SterInfo[i].msg.type, sizeof(tSetParam), errno, strerror(errno), i);
				allsent = 0;
			}
		}
	}

	/* diagnostics - queues states */
	if (log_level >= 3) {
		if (!msgctl(MsgSetDes, IPC_STAT, &msgdata)) {
			sz_log(3,
			       "Queue (Set) : msg num=%d, msg size=%d  max size=%d pass=%d",
			       (int)msgdata.msg_qnum, (int)msgdata.msg_cbytes,
			       (int)msgdata.msg_qbytes, pass);
		} else {
			sz_log(3, "Status not get errno=%d", errno);
		}
	}

	/*
	 * if not all messages has been sent (Set queue is probably full)
	 * some, the oldest messages will be removed from Set queue
	 */
	if (allsent == 0)
		for (i = 0; i < (int)NumberOfPars; i++)
			msgrcv(MsgSetDes, &msg, sizeof(tSetParam), 0,
			       IPC_NOWAIT);

	/*
	 * diagnostics - queues states
	 */
	if (log_level >= 3) {
		if (!msgctl(MsgSetDes, IPC_STAT, &msgdata)) {
			sz_log(3,
			       "Queue (Set) : msg num=%d, msg size=%d  max size=%d pass=%d",
			       (int)msgdata.msg_qnum, (int)msgdata.msg_cbytes,
			       (int)msgdata.msg_qbytes, pass);
		} else {
			sz_log(5, "Status not get errno=%d", errno);
		}
	}

	return allsent;
}

/*
 * Receives replies from deamons.
 */
void ReadReply(void)
{
	tMsgSetParam msg;

	for (unsigned int i = 0; i < NumberOfPars; i++) {
		while (msgrcv(MsgRplyDes, &msg, sizeof(tSetParam),
			      SterInfo[i].msg.cont.rtype,
			      IPC_NOWAIT) == sizeof(tSetParam)) {
			sz_log(5,
			       "received: msg=%ld par=%d val=%d rtype=%ld, retry=%d",
			       msg.type, msg.cont.param, msg.cont.value,
			       msg.cont.rtype, msg.cont.retry);

			if (SterInfo[i].status == MSG_CONF) {
				for (unsigned int j = 0; j < NumberOfPars; j++) {
					if (SterInfo[j].msg.cont.param ==
					    msg.cont.param
					    && SterInfo[j].msg.cont.value ==
					    msg.cont.value
					    && SterInfo[j].msg.cont.rtype ==
					    msg.type) {
						if (msg.cont.retry == 0)
							SterInfo[j].status =
							    MSG_NOREP;
						else
							SterInfo[j].status =
							    MSG_OK;
					}
				}
			} else {
				sz_log(2, "Status=%d", SterInfo[i].status);
			}
		}		/* end_while (msgrcv)        */
	}			/* end_for (i<NumberOfPars)  */

	/* all others messages are removed from the Reply queue */
	while (msgrcv(MsgRplyDes, &msg, sizeof(tSetParam), 0, IPC_NOWAIT) ==
	       sizeof(tSetParam))
		sz_log(2,
		       "Deleting unknown msg=%ld par=%d val=%d rtype=%ld, retry=%d",
		       msg.type, msg.cont.param, msg.cont.value, msg.cont.rtype,
		       msg.cont.retry);
}

/*
 * Shows status of the send messages.
 * Used only for diagnostics.
 */
void ReportStatistics(void)
{
	for (unsigned int i = 0; i < NumberOfPars; i++)
		switch (SterInfo[i].status) {
		case MSG_SEND:
			sz_log(3,
			       "Message %05ld with param: %d value: %7d rtype: %ld not sent",
			       SterInfo[i].msg.type, SterInfo[i].msg.cont.param,
			       SterInfo[i].msg.cont.value,
			       SterInfo[i].msg.cont.rtype);
			break;
		case MSG_CONF:
			sz_log(3,
			       "Message %05ld with param: %d value: %7d rtype: %ld not confirmed",
			       SterInfo[i].msg.type, SterInfo[i].msg.cont.param,
			       SterInfo[i].msg.cont.value,
			       SterInfo[i].msg.cont.rtype);
			break;
		case MSG_OK:
			sz_log(3,
			       "Message %05ld with param: %d value: %7d rtype: %ld status OK",
			       SterInfo[i].msg.type, SterInfo[i].msg.cont.param,
			       SterInfo[i].msg.cont.value,
			       SterInfo[i].msg.cont.rtype);
			break;
		case MSG_NOREP:
			sz_log(3,
			       "Message %05ld with param: %d value: %7d rtype: %ld rejected",
			       SterInfo[i].msg.type, SterInfo[i].msg.cont.param,
			       SterInfo[i].msg.cont.value,
			       SterInfo[i].msg.cont.rtype);
			break;
		case MSG_NOSEND:
			sz_log(3,
			       "Message %05ld with param: %d value: %7d rtype: %ld bad value, skipped",
			       SterInfo[i].msg.type, SterInfo[i].msg.cont.param,
			       SterInfo[i].msg.cont.value,
			       SterInfo[i].msg.cont.rtype);
			break;
		default:
			break;
		}
}

/*
 * Main function.
 */
int main(int argc, char *argv[])
{
	struct sigaction Act;
	sigset_t block_mask;
	char *log_value;
	int log_level;
	struct arguments arguments;

	sigfillset(&block_mask);
	sigdelset(&block_mask, SIGKILL);
	sigdelset(&block_mask, SIGSTOP);

/*for(int i=0; i<argc; i++)
    printf("%d %s\n",i, argv[i]);
*/

	libpar_read_cmdline(&argc, argv);

	Act.sa_handler = Terminate;
	Act.sa_mask = block_mask;
	Act.sa_flags = SA_RESTART | SA_RESETHAND;
	sigaction(SIGTERM, &Act, NULL);
	sigaction(SIGINT, &Act, NULL);

	log_level = loginit_cmdline(2, NULL, &argc, argv);
	/* Check for other copies of program. */
	int pid;
	if ((pid = check_for_other (argc, argv))) {
		sz_log(0, "sender: another copy of program is running, pid %d, exiting", pid);
		return 1;
	}
	arguments.no_daemon = 0;
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	libpar_init_with_filename("/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg",
				  1);
	log_value = libpar_getpar("sender", "log_level", 0);
	if (log_value) {
		log_level = atoi(log_value);
		free(log_value);
	} else {
		log_level = 2;
	}
	log_value = libpar_getpar("sender", "log", 0);
	if (log_value == NULL)
		log_value = strdup("sender");
	if (sz_loginit(log_level, log_value) < 0) {
		sz_log(0, "sender: cannot open log file '%s', exiting",
		    log_value);
		return 1;
	}
	sz_log(0, "sender: started");
	free(log_value);

	char *config_prefix = libpar_getpar("sender", "config_prefix", 1);
	IPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"pl");
	IPKContainer *ic = IPKContainer::GetObject();
	TSzarpConfig *ipk = ic->GetConfig(SC::L2S(config_prefix));
	if (ipk == NULL) {
		sz_log(0, "Unable to load IPK for prefix %s", config_prefix);
		return 1;
	}
	free(config_prefix);
	ipcInitializewithSleep(5);
	msgInitialize();

	ReadCfgFileFromParamXML(ipk);
	delete ipk;
	libpar_done();

    if (arguments.no_daemon == 0)
        go_daemon();

	if (log_level >= 3) {
		ShowCfgTable();
	}

	/*
	 * Main loop.
	 * 1. reading data from shared memory
	 * 2. sending messages to deamons
	 * 3. waiting
	 * 4. receiving replies from deamons
	 * 5. reporting statistics
	 */
	for (;;) {
		GetSharedData();
		if (SendSter(1, log_level) == 0)	/* if not all data has been sent */
			SendSter(2, log_level);	/* second chance */

		sleep(BasePeriod);

		ReadReply();
		if (log_level >= 3) {
			ReportStatistics();
		}
	}

	return 0;
}
