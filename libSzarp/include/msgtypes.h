/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * msgtypes.h
 */


/*
 * Modified by Codematic 15.01.1998
 */

#ifndef _MSGTYPES_H_
#define _MSGTYPES_H_

#include <sys/types.h>

typedef struct _SetParam
{
    ushort param;
    short value;
    long rtype;
    unsigned char retry;
} tSetParam, *pSetParam;

typedef struct _MsgSetParam
{
    long type;
    tSetParam cont;
} tMsgSetParam, *pMsgSetParam;

#endif

