/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * General 'insertion sort' (N^2) algorithm implementation.
 *
 * $Id$
 */

#include <insort.h>

void insort(compare_fun_t* cmp_f, swap_fun_t* swap_f, int size, void *data)
{
        int i, j;
        for (i = 1; i < size; i++) {
                j = i - 1;
                while ((j >= 0) && (cmp_f(j, j + 1, data) > 0)) {
                        swap_f(j, j + 1, data);
                        j--;
                }
        }
}

