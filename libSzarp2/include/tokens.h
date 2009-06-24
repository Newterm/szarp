/* 
  SZARP: SCADA software 

*/
/*
 * libSzarp2
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * 'Tokenize' functions.
 *
 * $Id$
 */

#ifndef __TOKENS_H__
#define __TOKENS_H__

#ifdef __cplusplus
	extern "C" {
#endif


/** Splits strings into tokens separated by spaces or tabs. Handles '"' and
 * '\"'.
 * @param str string to split
 * @param toks pointer to output array of tokens
 * @param tokc pointer to output array size, if grater then 0 on input,
 * memory is freed; you can pass NULL str to just free memory allocated
 * on previous run (without tokenizing new string)*/
void tokenize(const char *str, char ***toks, int *tokc);

/** Splits strings into tokens separated by delims. It is more general then
 * tokenize, but doesn't handle quotations. 
 * @param str string to split
 * @param toks pointer to output array of tokens
 * @param tokc pointer to output array size, if grater then 0 on input,
 * memory is freed; you can pass NULL str to just free memory allocated
 * on previous run (without tokenizing new string)
 * @param delims array of chars treated as delimiters 
 */
void tokenize_d(const char *str, char ***toks, int *tokc, const char *delims );

#ifdef __cplusplus
	}
#endif

#endif

