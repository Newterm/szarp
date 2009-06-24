/* 
  libSzarp - SZARP library

*/
/**
 * Pawe³ Pa³ucha, 6.10.2001
 * SZARP 2.1
 *
 * $Id$
 *
 * getpasswd - wczytywanie has³a bez echa na ekran
 */

#include <config.h>
#ifndef MINGW32

#define _XOPEN_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>

#include <passwd.h>

struct termios saved_attributes;
int restore_saved_attributes = 0;


void reset_input_mode (void)
{
	if (restore_saved_attributes)
		tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
	restore_saved_attributes = 0;
}

void set_input_mode (void)
{
	struct termios tattr;
	
	/* Make sure stdin is a terminal. */
	if (!isatty (STDIN_FILENO))
		return;
	restore_saved_attributes = 1;
	/* Save the terminal
	 * attributes so we can	
	 * restore them later. */
	tcgetattr (STDIN_FILENO, &saved_attributes);
	atexit (reset_input_mode);
       	/* Set the funny terminal modes. */
	tcgetattr (STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}
						
/**
 * Funkcja wczytuje string, nie pojawia siê on na ekranie.
 * Je¿eli znaki s± czytane z terminala, funkcja wy³±cza echo. 
 * @param print_stars czy zamiast znaków wy¶wietlaæ gwiazdki?
 * @return wka¼nik do statycznego bufora (zamazywany przu kolejnym wywo³aniu)
 */
char *getpasswd(int print_stars)
{
	#define BUFSIZE 255
	static char buf[BUFSIZE];

	int i = 0;
	
	set_input_mode ();
	while (i < BUFSIZE-1) {
		read (STDIN_FILENO, &(buf[i]), 1);
		if (buf[i] == 10)          /* [ENTER] */
			break;
		else if (print_stars)
			putchar ('*');
		i++;
	}
	buf[i] = 0;
	putchar(10);
	reset_input_mode();
	return buf;
}


/**
 * Funkcja sprawdz zgodno¶æ has³a.
 * @param passwd sprawdzane has³o
 * @param encr zaszyfrowane has³o
 * @return 0 je¶li has³o jest poprawne, -1 je¶li nie
 */
int ch_passwd(char *passwd, char *encr)
{
	char salt[3];
	char *test;

	salt[0] = encr[0];
	salt[1] = encr[1];
	salt[2] = 0;
	test = crypt(passwd, salt);
	return (strcmp(test, encr) != 0);
}
#endif /* MINGW32 */


