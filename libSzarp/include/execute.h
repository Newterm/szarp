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

int execute(char *command);

/**
 * Execute specified command and returns it's output
 * (just like the shell `command` substitution). On error returns NULL.
 * Only the first 4 KB of command output are read. Uses strdup to allocate
 * memory.
 * @param command shell command to execute
 * @return return buffer, NULL on error
 */
char *execute_subst(char *command);
/**
 * Execute specified command with execv, return output.
 * @param path path to command to execute
 * @param argv array of arguments, like for execv() function (must be NULL - terminated)
 * @return first 4 KB of command output, NULL on error
 */
char *execute_substv(const char *path, char * const argv[]);

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
