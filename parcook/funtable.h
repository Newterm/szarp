/* $Id$ */

/*
 * SZARP 2.0
 * parcook
 * funtable.h
 */

#define MAX_FID 8

#ifdef __cplusplus
	extern "C" {
#endif

extern float (*(FunTable[MAX_FID]))(float*);

#ifdef __cplusplus
	}
#endif

