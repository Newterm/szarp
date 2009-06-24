/* 
  SZARP: SCADA software 

*/
#include "szbhash.h"

#define mix(a,b,c) \
{ \
	a -= b; a -= c; a ^= (c>>13); \
	b -= c; b -= a; b ^= (a<<8); \
	c -= a; c -= b; c ^= (b>>13); \
        a -= b; a -= c; a ^= (c>>12);  \
	b -= c; b -= a; b ^= (a<<16); \
	c -= a; c -= b; c ^= (b>>5); \
	a -= b; a -= c; a ^= (c>>3);  \
	b -= c; b -= a; b ^= (a<<10); \
	c -= a; c -= b; c ^= (b>>15); \
}

/** This is used hash function */
ub4
hash(register const wchar_t *k, register ub4 length, register ub4 initval)
    //  register ub1 *k;		/* the key */
    //	register ub4 length;	/* the length of the key */
    //	register ub4 initval;	/* the previous hash, or an arbitrary value */
{
    register ub4 a, b, c, len;

    /* Set up the internal state */
    len = length;
    a = b = 0x9e3779b9;		/* the golden ratio; an arbitrary value */
    c = initval;		/* the previous hash value */

	/*---------------------------------------- handle most
	 * 			* of the key */
    while (len >= 12) {
	a += (k[0] + ((ub4) k[1] << 8) + ((ub4) k[2] << 16) + ((ub4) k[3] << 24));
	b += (k[4] + ((ub4) k[5] << 8) + ((ub4) k[6] << 16) + ((ub4) k[7] << 24));
	c += (k[8] + ((ub4) k[9] << 8) + ((ub4) k[10] << 16) + ((ub4) k[11] << 24));
	mix(a, b, c);
	k += 12;
	len -= 12;
    }

	/*------------------------------------- handle the
	 * 			   * last 11 bytes */
    c += length;
    switch (len) {		/* all the case statements fall through */
    case 11:
	c += ((ub4) k[10] << 24);
    case 10:
	c += ((ub4) k[9] << 16);
    case 9:
	c += ((ub4) k[8] << 8);
    case 8:
	b += ((ub4) k[7] << 24);
    case 7:
	b += ((ub4) k[6] << 16);
    case 6:
	b += ((ub4) k[5] << 8);
    case 5:
	b += k[4];
    case 4:
	a += ((ub4) k[3] << 24);
    case 3:
	a += ((ub4) k[2] << 16);
    case 2:
	a += ((ub4) k[1] << 8);
    case 1:
	a += k[0];
    }
    mix(a, b, c);
    return c;
}

