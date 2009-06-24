/* 
  SZARP: SCADA software 

*/
/*
 * szbase - szbdbuf.h - data buffer
 * $Id$
 * <pawel@praterm.com.pl>
 */

/*hashing stuff*/

#ifndef __SZBHASH_H__
#define __SZBHASH_H__

typedef unsigned long int ub4;	/* unsigned 4-byte quantities */
typedef unsigned char ub1;	/* unsigned 1-byte quantities */

/** This is used hash function */
ub4
hash(const wchar_t *k, register ub4 length, register ub4 initval);

#endif
