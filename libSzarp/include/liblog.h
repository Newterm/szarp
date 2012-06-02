/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 
 * biblioteka libszarp
 * liblog.h
 */

/*
 * Modul obslugi logowania zdarzen dla systemu SZARP
 *
 * 2001.04.13 Pawel Palucha
 */

#ifndef __LIBLOG_H__
#define __LIBLOG_H__

#define loginit(level, logfile) \
     sz_loginit(level, logfile)

#define loginit_cmdline(leve, logfile, argc, argv) \
     sz_loginit_cmdline(leve, logfile, argc, argv) 

#define logdone() \
     sz_logdone()

/* removed because of conflict with math.h log() function */

#if 0
#define log(level, msg_format, args...) \
     sz_log(level, msg_format, ## args)
#endif

#define log_info(info) \
     sz_log_info(info)


#include "config.h"

#ifndef MING32
#include <syslog.h>
#else
//as there is no syslog under msw we fix our default to 0 there
#define LOG_DAEMON 0
#endif
#include <stdarg.h>

enum SZ_LIBLOG_FACILITY {
	SZ_LIBLOG_FACILITY_DAEMON 
#ifndef MING32
	 			 = LOG_DAEMON
#endif
				,
	SZ_LIBLOG_FACILITY_APP 
#ifndef MING32
	 			 = LOG_LOCAL1
#endif
};


typedef void* (sz_log_init_function)(int level, const char* logname, SZ_LIBLOG_FACILITY facility, void *context);

typedef void (sz_vlog_function)(void* data, int level, const char * msg_format, va_list fmt_args);

typedef void (sz_log_close_function)(void* data);

void sz_log_system_init(sz_log_init_function, sz_vlog_function, sz_log_close_function);

/*
 * loginit - inicjacja mechanizmu logowania na poziomie zadanym
 * parametrem 'level' - od 0 (tylko najwazniejsze komunikaty) do 10 
 * (debugowanie). Przy ustawieniu poziomu logowania na 'n' wypisywane
 * sa komunikaty o poziomie mniejszym lub rownym n. Jesli podany poziom
 * nie miescie sie w zakresie [0..10], to ustawiany jest poziom 0.
 *
 * Proponowana konwencja uzywania poziomow logowania:
 *
 * 0 - blad
 * 1 - ostrzezenie
 * 2 - informacja
 * 3 i wiecej - debugowanie
 *
 * Drugi parametr okresla sciezke do pliku z logiem, np. "/var/log/meaner".
 * Je¶li jest pusty lub rowny NULL, komunikaty wypisywane sa na standardowe
 * wyjscie bledow.
 *
 * Funkcja zwraca ustawiony poziom logowania lub -1 w przypadku niemoznosci
 * otwarcia pliku z logiem.
 *
 */

int sz_loginit(int level, const char * logname, SZ_LIBLOG_FACILITY facility = SZ_LIBLOG_FACILITY_DAEMON, void *context = 0);
/*
 * loginit_cmdline - inicjacja mechanizmu logowania, podobnie jak w przypadku
 * funkcji loginit(). Dodatkowymi argumentami jest wskaznik na ilosc parametrow
 * przekazanych do programu i ich tablica (przekazane z funkcji main()
 * programu). Jezeli wsrod tych parametrow jest parametr postaci "--debug=n"
 * lub "-d n", to ustawiany jest poziom logowania 'n'. Dodatkowo zmniejszana
 * jest wartosc argc i modyfikowana tablica argv (parametry dotyczace
 * logowania sa przenoszone na jej koniec). Dzieki temu nie jest konieczna
 * modyfikacja parsowania parametrow przez program uzywajacy tej funkcji -
 * dodatkowe parametry sa niewidoczne dla kodu po wywolaniu tej funkcji.
 *
 * Jezeli parametrow nie ma lub sa nieprawidlowe, funkcja zachowuje sie jakby
 * wywolano loginit(level, logfile).
 */

int sz_loginit_cmdline(int level, const char * logname, int *argc, char *argv[], SZ_LIBLOG_FACILITY facility = SZ_LIBLOG_FACILITY_DAEMON);

/** logdone - zamyka plik z logiem. Jej uzycie jest zalecane (aczkolwiek
 * niekonieczne przed zakonczeniem programu. Zakonczenie logowania jest
 * logowane na poziomie 2.
 */

void sz_logdone(void);

/**
 * log - wypisanie komunikatu o poziomie waznosci 'level'. Pozostale argumenty
 * - jak dla funkcji printf().
 *
 * Komunikat zostanie wypisany jezeli aktualny poziom logowania jest wiekszy
 * lub rowny 'level'.
 *
 * Jezeli nie ustawiono wczesniej zadnego poziomu logowania, to ustawiony jest
 * poziom 0, z wypisywaniem na stderr.
 *
 * Wypisywane komunikaty maja postac:
 *
 * <data> [<pid>] (<level>) <komunikat>
 *
 * Na przyklad po wywolaniu postaci:
 * 
 * 		log(3, "Meaner: otwarto plik %s", filename)
 *
 * w logach moglby pojawic sie komunikat postaci:
 *
 * Apr 13 09:43:12 [567] (3) Meaner: otwarto do zapisu plik leg1lad.dbf
 *
 * Po wywo³aniu log_info(0) wypisywane komunikaty pozbawione s± trzech
 * pierwszych czê¶ci, wypisywany jest tylko komunikat.
 *
 */

void sz_log(int level, const char * msg_format, ...)
  __attribute__ ((format (printf, 2, 3)));

void vsz_log(int level, const char * msg_format, va_list fmt_args);

/** 
 * Ustala, czy maj± byæ wypisywane dodatkowe informacje (data, pid, poziom
 * wa¿no¶ci). Ustalona warto¶æ obowi±zuje do kolejnej zmiany.
 * @param info 1 je¶li maj± byæ wypisywane dodatkowe informacje, 0 je¶li nie
 */
void sz_log_info(int info);

int sz_level_to_syslog_level(int level);
#endif
