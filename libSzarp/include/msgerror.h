/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * msgerror.h
 */


#ifndef _MSGERROR_H_
#define _MSGERROR_H_

#include <sys/types.h>

void ErrMessage(ushort ernum, const char *par);
void WarnMessage(ushort ernum, const char *par);

#endif

