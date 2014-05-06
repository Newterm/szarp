/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * msgtools.h
 */

#ifndef _MSGTOOLS_H_
#define _MSGTOOLS_H_

#define MSG_SET 1
#define MSG_RPLY 2
#define MSG_RULER 3

#include"msgtypes.h"

extern int MsgSetDes;
extern int MsgRplyDes;
extern int MsgRulerDes;

void msgInitialize();

#endif

