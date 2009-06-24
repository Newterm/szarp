/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * msgtools.c
 */


/* msgtools.c: Zmienne i procedury dostepu do kolejek sterujacych */

/*
 * Modified by Codematic 10.11.1998
 * dodano obsluge libpar()
 */

#include <config.h>
#ifndef MINGW32

#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "msgerror.h"
#include "libpar.h"
#include "msgtools.h"

int MsgSetDes = -1;

int MsgRplyDes = -1;

int MsgRulerDes = -1;

static char parcookpat[129];

void msgInitialize()
{
    char *c;

    libpar_getkey("", "parcook_path", &c);
    if (!c) ErrMessage(7, "parcook_path");
    strncpy(parcookpat, c, sizeof(parcookpat));    
    libpar_getkey("", "parcook_cfg", &c);
    if (!c) ErrMessage(7, "parcook_cfg");

    if ((MsgSetDes = msgget(ftok(parcookpat, MSG_SET), 00666)) < 0)
	ErrMessage(6, "parcook set");
    if ((MsgRplyDes = msgget(ftok(parcookpat, MSG_RPLY), 00666)) < 0)
	ErrMessage(6, "parcook rply");
}

void msgInitTry()
{
    char *c;

    libpar_getkey("", "parcook_path", &c);
    if (!c) WarnMessage(7, "parcook_path");
    strncpy(parcookpat, c, sizeof(parcookpat));    
    libpar_getkey("", "parcook_cfg", &c);
    if (!c) WarnMessage(7, "parcook_cfg");
        
    if ((MsgSetDes = msgget(ftok(parcookpat, MSG_SET), 00666)) < 0)
	WarnMessage(6, "parcook set");
    if ((MsgRplyDes = msgget(ftok(parcookpat, MSG_RPLY), 00666)) < 0)
	WarnMessage(6, "parcook rply");
    if ((MsgRulerDes = msgget(ftok(parcookpat, MSG_RULER), 00666)) < 0)
	WarnMessage(6, "ruler");
}

void msgRulerInit()
{
    char *c;

    libpar_getkey("", "parcook_path", &c);
    if (!c) ErrMessage(7, "parcook_path");
    strncpy(parcookpat, c, sizeof(parcookpat));    
    libpar_getkey("", "parcook_cfg", &c);
    if (!c) ErrMessage(7, "parcook_cfg");
        
    if ((MsgSetDes = msgget(ftok(parcookpat, MSG_SET), 00666)) < 0)
	ErrMessage(6, "parcook set");
    if ((MsgRplyDes = msgget(ftok(parcookpat, MSG_RPLY), 00666)) < 0)
	ErrMessage(6, "parcook rply");
    if ((MsgRulerDes = msgget(ftok(parcookpat, MSG_RULER), IPC_CREAT | 00666)) < 0)
	ErrMessage(6, "ruler");
}

#endif /* MINGW32 */
