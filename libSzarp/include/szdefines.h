/* 
  libSzarp - SZARP library

*/
/*
 * Defines for libSzarp
 * $Id$
 */

#ifndef __LIBSZARP_DEFINES_H__
#define __LIBSZARP_DEFINES_H__


#define NOPROBES	"           "

#define LEFT   0        /* do przeszukiwania prze FromValueTable i ParticularValue */
#define RIGHT  1        /* i przy MoveCursor */
#define CENTER 2
#define UP     3
#define DOWN   4
#define HOME   5
#define END    6
#define PRIOR  7
#define NEXT   8
#define OPPOSITEDIR(a)  ((a == LEFT) ? RIGHT : LEFT)

#define UNKNOWN_METHOD 0
#define CODEBASE_METHOD 1
#define NETBASE_METHOD 2
#define SZBASE_METHOD 3
	    
#define SZARP_NO_DATA	-32768	/**< no-data marker  */
#define NO_PARAM	0xFFFF	/**< probe is not collected  */

typedef void (*error_proc) (const char *error_str);


#endif // __LIBSZARP_DEFINES_H__

