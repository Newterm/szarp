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


void msgInitialize()
{
	char *parcookpat = libpar_getpar("", "parcook_path", 0);
	if (NULL == parcookpat)
		ErrMessage(7, "parcook_path");

	if ((MsgSetDes = msgget(ftok(parcookpat, MSG_SET), 00666)) < 0)
		ErrMessage(6, "parcook set");

	if ((MsgRplyDes = msgget(ftok(parcookpat, MSG_RPLY), 00666)) < 0)
		ErrMessage(6, "parcook rply");

	free(parcookpat);
}

#endif /* MINGW32 */
