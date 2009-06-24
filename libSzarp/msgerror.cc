/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * msgerror.c
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

void WarnMessage(unsigned short errnum, const char *par)
{
    switch (errnum) {
    case 0:
	fprintf(stderr,"Usage: linedmn [1-8]\n");
	break;
    case 1:
	fprintf(stderr,"Terminated due to memory allocation error: %s error: 			  %d\n", par, errno);
	break;
    case 2:
	fprintf(stderr,"Can not open: %s error: %d\n", par, errno);
	break;
    case 3:
	fprintf(stderr,"Can not get shared segment: %s error: %d\n", par, errno);
	break;
    case 4:
	fprintf(stderr,"Can not attach shared segment: %s error: %d\n", par, errno);
	break;
    case 5:
	fprintf(stderr,"Can not get semaphore set: %s error: %d\n", par, errno);
	break;
    case 6:
	fprintf(stderr,"Can not get message queue: %s error: %d\n", par, errno);
	break;
    case 7:
	fprintf(stderr,"Nie znaleziono parametru: %s w pliku konf.\n", par);
	break;
    case 8:
    	fprintf(stderr,"Blad systemu pomocy: %s\n", par);
	break;
	
    default:
	fprintf(stderr,"Unhandled error:%d with message: %s\n", errno, par);
	break;
    }
}

void ErrMessage(unsigned short errnum, const char *par)
{

    WarnMessage(errnum, par);
    fprintf(stderr,"Last system error: \n");
    perror("");
    fprintf(stderr,"\n");
    exit(-1);

}
