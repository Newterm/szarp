/* 
  SZARP: SCADA software 

*/
/*
 * IPK
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * General 'insertion sort' (N^2) algorithm implementation.
 *
 * $Id$
 */

#ifndef __INSORT_H__
#define __INSORT_H__

#ifdef __cplusplus
	extern "C" {
#endif

/** Comparision function type (used for sorting). */
typedef int (compare_fun_t)(int, int, void *);
/** Swapping function type (used for sorting). */
typedef void (swap_fun_t)(int, int, void *);

/**
 * Insertion sort. Sorts data structure in place using insertion sort algorithm 
 * (N^2). Elements are compared and swapped using functions given as arguments.
 * Sorting is stable.
 * @param cmp_f pointer to comparision function. It takes two arguments
 * (indexes elements to compare) and returns -1 if first is less then second, 
 * 0 if * elements are equal and +1 otherwise. Third argument is aditional data
 * passed from insort function.
 * @param swap_f pointer to function used to swap elements order. It takes two
 * arguments - indexes of element to swap and aditional data from insort.
 * @param size number of elements to sort (array length usually)
 * @param data aditional data which is passed to comparision and swap functions
 * without modifications (as last argument).
 */
void insort(compare_fun_t* cmp_f, swap_fun_t* swap_f, int size, void *data);

#ifdef __cplusplus
	}
#endif

#endif

