/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * libpar.h
 */


/*
 * libpar.h
 *
 * Modul obslugi plikow konfiguracyjnych
 * dla projektu Szarp
 *
 * 1998.03.12 Codematic
 */

/*
 * Szuka plikow konfiguracyjnych w nastepujacej kolejnosci:
 *
 *  ./szarp.cfg
 *  $HOME/szarp.cfg
 *  /etc/szarp/szarp.cfg
 *  /etc/szarp.cfg 
 *
 */

#ifndef _LIBPAR_H_
#define _LIBPAR_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wchar.h>
#include <string>
		
/* Inicjalizacja */

/** Initialization with given config file
 * @param filename config file name
 * @param exit_on_error if non-zero call exit on error
 * @return 0 on success, -1 on error.
 */
int libpar_init_with_filename(const char *filename, int exit_on_error);
void libpar_init();
void libpar_init_from_folder(std::string folder_path);

/**Ponowna inicjalizacja - wczytanie pliku konfiguracyjnego jeszcze raz
 * adresy zwrocone przez libpar_getkey staja sie niewazne*/
void libpar_reinit_with_filename(const char *filename, int exit_on_error);

/**Ponowna inicjalizacja - wczytanie pliku konfiguracyjnego jeszcze raz
 * adresy zwrocone przez libpar_getkey staja sie niewazne*/
void libpar_reinit();

/** Ponowna inicjalizacja po zlej skladni */
void libpar_hard_reset();

/* Zakonczenie */
void libpar_done();

/* Pobranie adresu klucza */
void libpar_getkey(const char *section, const char *par, char **buf);

/* "Inteligentne" wczytanie parametru - obsluga bledow */
void libpar_readpar(const char *section, const char *par, char *buf, 
      int size, int exit_on_error);

/* Jak libpar_readpar, tylko zwraca utworzon± za pomoc± strdup() kopiê
 * parametru. Je¿eli exit_on_error jest równe 0 a parametru nie ma, zwraca
 * NULL.
 */
char* libpar_getpar(const char *section, const char *par, int exit_on_error);

/* Ustawienie zmiennej XENVIRONMENT */
void libpar_setXenvironment(const char *programname);

/* Drukowanie */
void libpar_printfile(char *programname, char *printcmd, char *filename);

/* Wczytanie warto¶ci zmiennych z linii komend. Wczytane warto¶ci
 * s± usuwane z linii komend. Prawid³owe wywo³anie:
 * libpar_read_cmdline(&argc, argv);
 */
void libpar_read_cmdline(int *argc, char *argv[]);

/**
 * Wersja libpar_read_cmdline dla wielobajtowych stringów w argv.
 */
void libpar_read_cmdline_w(int *argc, wchar_t *argv[]);


#endif

