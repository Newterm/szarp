/* 
  SZARP: SCADA software 

*/
/*
 * Tokenizer
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Function for splitting string into tokens.
 *
 * $Id$
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include "tokens.h"

/** maximum token's length */
#define BUFSIZE 128
/** maximum number of tokens */
#define MAXTOKS 512

typedef enum parse_states_tags { OUTSIDE, INSIDE, INCOMM } parse_states;

void
tokenize(const char *str, char ***toks, int *tokc)
{
    int             ind[MAXTOKS * 2];
    int             i,
                    s,
                    t;
    parse_states    state = OUTSIDE;
    char            tmp[BUFSIZE];

    /*
     * free memory 
     */
    if (*tokc > 0) {
	for (i = 0; i < *tokc; i++)
	    free((*toks)[i]);
	free(*toks);
	*toks = NULL;
    }
    *tokc = 0;

    if (str == NULL)
	return;

    state = OUTSIDE;
    t = 0;
    s = strlen(str);
    for (i = 0; i < s; i++) {
	switch (state) {
	case OUTSIDE:
	    switch (str[i]) {
	    case ' ':
	    case '\t':
		break;
	    case '"':
		state = INCOMM;
		i++;
		ind[t++] = i;
		if (t >= (MAXTOKS * 2 - 2))
			goto out;
		break;
	    default:
		state = INSIDE;
		ind[t++] = i;
		if (t >= (MAXTOKS * 2 - 2))
			goto out;
		break;
	    }
	    break;
	case INSIDE:
	    switch (str[i]) {
	    case ' ':
	    case '\t':
		state = OUTSIDE;
		ind[t++] = i;
		if (t >= (MAXTOKS * 2 - 2))
			goto out;
		break;
	    default:
		break;
	    }
	    break;
	case INCOMM:
	    switch (str[i]) {
	    case '"':
		state = OUTSIDE;
		ind[t++] = i;
		if (t >= (MAXTOKS * 2 - 2))
			goto out;
		break;
	    default:
		break;
	    }
	    break;
	}
    }
out:
    if (state != OUTSIDE)
	ind[t++] = i;
    if (t <= 0) {
	return;
    }
    *tokc = (t / 2);
    *toks = (char **) malloc(sizeof(char *) * (*tokc + 10));
    for (i = 0; i < t; i += 2) {
	s = ind[i + 1] - ind[i];
	if (s >= BUFSIZE)
	    s = BUFSIZE - 1;
	strncpy(tmp, &(str[ind[i]]), s);
	tmp[s] = 0;
	(*toks)[i / 2] = strdup(tmp);
    }
}

void
tokenize_d(const char *str, char ***toks, int *tokc, const char *delims)
{
    int             ind[MAXTOKS * 2];
    int             i,
                    s,
                    t;
    parse_states    state = OUTSIDE;
    char            tmp[BUFSIZE];

    // free previous content
    if (*tokc > 0) {
	for (i = 0; i < *tokc; i++)
	    free((*toks)[i]);
	free(*toks);
	*toks = NULL;
    }
    *tokc = 0;

    state = OUTSIDE;
    t = 0;
    if (str)
	s = strlen(str);
    else
	s = -1;
    for (i = 0; i < s; i++) {
	switch (state) {
	case OUTSIDE:
	    if (strchr(delims, str[i]))
		break;
	    state = INSIDE;
	    ind[t++] = i;
	    if (t >= (MAXTOKS * 2 - 2))
		    goto out;
	    break;
	case INSIDE:
	    if (strchr(delims, str[i])) {
		state = OUTSIDE;
		ind[t++] = i;
	    	if (t >= (MAXTOKS * 2 - 2))
			goto out;
	    }
	    break;
	default :
	    break;
	}
    }
out:
    if (state != OUTSIDE) {
	ind[t++] = i;
    }
    if (t <= 0) {
	return;
    }
    *tokc = (t / 2);
    *toks = (char **) malloc(sizeof(char *) * (*tokc + 10));
    for (i = 0; i < t; i += 2) {
	s = ind[i + 1] - ind[i];
	if (s >= BUFSIZE)
		s = BUFSIZE - 1;
	strncpy(tmp, &(str[ind[i]]), s);
	tmp[s] = 0;
	(*toks)[i / 2] = strdup(tmp);
    }
}
