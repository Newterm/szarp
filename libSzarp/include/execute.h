/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * Program przegladajacy - draw
 * execute.h
 */

#ifndef EXECUTE_H
#define EXECUTE_H

extern int execute(char *command);

/* Returns output of given command, null on error. */
extern char *execute_subst(char *command);

/**
 * Checks if other copy of currently executed program (with same program name
 * and same arguments) is already run.
 * @param argc main() functions argc param, read only
 * @param argv main() functions argv param, read only
 * @return 0 if this is the only copy, -1 if error occured while scanning proc
 * directory, otherwise returns pid of program's another copy (first found).
 */
int check_for_other (int argc, char *argv[]);
	
#endif
