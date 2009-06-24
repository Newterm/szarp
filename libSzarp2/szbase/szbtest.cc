/* 
  SZARP: SCADA software 

*/
/*
 * Test program for SzarpBase library.
 * $Id$
 * <pawel@praterm.com.pl>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <szbdefines.h>
#include <szbfile.h>
#include <szbdate.h>
#include <szbbuf.h>
#include <szbdate.h>

#define TESTING(a) printf("testing "a": ");
#define TEST(a) \
	if (!(a)) { \
		printf(" test %d failed\n", test_num); \
		return(1); \
	} else \
		test_num++; 
#define TEST_OK \
	printf(" OK\n") && ((test_num = 1));
		

int main (void)
{
	char* tmp;
	int tmpi, tmpi1, tmpi2;
	int test_num = 1;
	szb_buffer_t *bp;
	
	printf("SzarpBase test suite.\n\
This program should run under valgrind without errors.\n\n");
	
	/* FILE NAMES CREATING */
	TESTING("szb_createfilename()")
	
	/* param name must be at least 1 character long */
	tmp = szb_createfilename(NULL, 1, 1, NULL, 0);
	TEST(tmp == NULL)
	tmp = szb_createfilename("", 1, 1, NULL, 0);
	TEST(tmp == NULL)
	tmp = szb_createfilename("A", 1, 1, NULL, 0);
	TEST(!strcmp("A.000101.szb",tmp))
	free(tmp);
	/* check for incorrect dates */
	tmp = szb_createfilename("A", -1, 1, NULL, 0);
	TEST(tmp == NULL)
	tmp = szb_createfilename("A", 10000, 1, NULL, 0);
	TEST(tmp == NULL)
	tmp = szb_createfilename("A", 1, 0, NULL, 0);
	TEST(tmp == NULL)
	tmp = szb_createfilename("A", 1, 13, NULL, 0);
	TEST(tmp == NULL)
	/* check if converting characters works OK */
	tmp = szb_createfilename("Ala ma kota:psa", 1, 1, NULL, 0);
	TEST(!strcmp("Ala_ma_kota_psa.000101.szb",tmp))
	free(tmp);
	tmp = szb_createfilename("~`!@#$%^&*()-_++[{]}\\|;':\",<.>/?", 11, 11, NULL, 0);
	TEST(!strcmp("________________________________.001111.szb",tmp))
	free(tmp);
	tmp = szb_createfilename("1234567890", 123, 1, NULL, 0);
	TEST(!strcmp("1234567890.012301.szb",tmp))
	free(tmp);
	tmp = szb_createfilename("qwertyuiopasdfghjklzxcvbnm", 1234, 1, NULL, 0);
	TEST(!strcmp("qwertyuiopasdfghjklzxcvbnm.123401.szb",tmp))
	free(tmp);
	tmp = szb_createfilename("QWERTYUIOPASDFGHJKLZXCVBNM", 1234, 1, NULL, 0);
	TEST(!strcmp("QWERTYUIOPASDFGHJKLZXCVBNM.123401.szb",tmp))
	free(tmp);
	tmp = szb_createfilename("±æê³ñó¶¼¿¡ÆÊ£ÑÓ¦¬¯", 1234, 10, NULL, 0);
	TEST(!strcmp("acelnoszzACELNOSZZ.123410.szb",tmp))
	free(tmp);
	/* check for different format strings */
	tmp = szb_createfilename("abcde", 1234, 10, "defghijk", 10);
	TEST(!strcmp("defghijk", tmp));
	free(tmp);
	TEST_OK

	TESTING("szb_probecnt()")
	/* check for incorrect date */
	tmpi = szb_probecnt(SZBASE_MIN_YEAR-1, 1);
	TEST(tmpi == 0);
	tmpi = szb_probecnt(SZBASE_MAX_YEAR+1, 1);
	TEST(tmpi == 0);
	tmpi = szb_probecnt(1, 0);
	TEST(tmpi == 0);
	tmpi = szb_probecnt(1, 13);
	TEST(tmpi == 0);
	tmpi = szb_probecnt(SZBASE_MIN_YEAR, 0);
	TEST(tmpi == 0);
	/* check for different months */
	tmpi = szb_probecnt(2001, 1);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*31));
	tmpi = szb_probecnt(2001, 3);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*31));
	tmpi = szb_probecnt(2001, 5);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*31));
	tmpi = szb_probecnt(2001, 7);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*31));
	tmpi = szb_probecnt(2001, 8);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*31));
	tmpi = szb_probecnt(2001, 10);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*31));
	tmpi = szb_probecnt(2001, 12);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*31));
	tmpi = szb_probecnt(2001, 4);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*30));
	tmpi = szb_probecnt(2001, 6);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*30));
	tmpi = szb_probecnt(2001, 9);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*30));
	tmpi = szb_probecnt(2001, 11);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*30));
	/* check for july */
	tmpi = szb_probecnt(2000, 2);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*29));
	tmpi = szb_probecnt(1996, 2);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*29));
	tmpi = szb_probecnt(2100, 2);
	TEST(tmpi == (SZBASE_PROBES_PER_DAY*28));
	TEST_OK

	TESTING("szb_open()");
	tmpi = szb_open(NULL, 1999, 1, 0, NULL, 0);
	TEST(tmpi == SZBE_INVPARAM);
	tmpi = szb_open("", 1999, 1, 0, NULL, 0);
	TEST(tmpi == SZBE_INVPARAM);
	tmpi = szb_open("A", SZBASE_MIN_YEAR-1, 1, 0, NULL, 0);
	TEST(tmpi == SZBE_INVDATE);
	tmpi = szb_open("A", SZBASE_MAX_YEAR+1, 1, 0, NULL, 0);
	TEST(tmpi == SZBE_INVDATE);
	tmpi = szb_open("A", 1999, 0, 0, NULL, 0);
	TEST(tmpi == SZBE_INVDATE);
	tmpi = szb_open("A", 1999, 13, 0, NULL, 0);
	TEST(tmpi == SZBE_INVDATE);
	tmpi = szb_open("A", 1999, 1, O_RDWR | O_CREAT, NULL, 0);
	TEST(tmpi >= 0);
	tmpi = szb_close(tmpi);
	TEST(tmpi == 0)
	tmpi = szb_open("A", 1999, 1, O_RDWR, NULL, 0);
	TEST(tmpi >= 0)
	tmpi = szb_close(tmpi);
	TEST(tmpi == 0)
	tmp = szb_createfilename("A", 1999, 1, NULL, 0);
	TEST(tmp != NULL)
	tmpi = unlink(tmp);
	TEST(tmpi == 0)
	free(tmp);
	TEST_OK

	TESTING("szb_create_buffer()");
	bp = szb_create_buffer(100);
	TEST(bp != NULL);
	TEST(bp->state == 0);
	TEST(bp->max_blocks == 100);
	TEST(bp->hashsize == 203);
	TEST(bp->hash != NULL);
	TEST(bp->hash[0] == NULL);
	TEST(bp->hash[202] == NULL);
	TEST(bp->blocks_c == 0);
	TEST(bp->usage_first == NULL);
	TEST(bp->usage_last == NULL);
	TEST(bp->formatstring == NULL);
	TEST(bp->extraspace == NULL);
	TEST_OK
	
	TESTING("szb_free_buffer()");
	tmpi = szb_free_buffer(bp);
	TEST(tmpi == SZBE_OK);
	TEST_OK

	TESTING("szb_time2my()");
	tmpi = szb_time2my(0, NULL, NULL);
	TEST(tmpi == 2);
	tmpi = szb_time2my(0, &tmpi1, NULL);
	TEST(tmpi == 3);
	tmpi = szb_time2my(0, &tmpi1, &tmpi2);
	TEST(tmpi == 0);
	TEST(tmpi1 == 1970);
	TEST(tmpi2 == 1);
	TEST_OK
		
	return 0;	
}
