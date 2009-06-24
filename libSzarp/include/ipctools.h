/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * ipctools.h
 */


/*
 * Modified by Codematic 15.01.1998
 */

#ifndef _IPCTOOLS_H_
#define _IPCTOOLS_H_

extern "C" {
#include <sys/types.h> /* For ushort - Codematic */
}

#include "ipcdefines.h"


extern short *Probe;		/* wspólna pamiêæ próbki */	

extern short *Minute;		/* wspólna pamiêæ minuta */

extern short *Min10;		/* wspólna pamiêæ 10minut */

extern short *Hour;		/* wspólna pamiêæ godzina */

extern short *Day;		/* wspólna pamiêæ doba */

extern unsigned char *Alert;    /* wspólna pamiêæ alarmy przekroczeñ */

struct phPTTEntry		/* opis pojedynczego parametru */
 {
  ushort addr;			/* adres w dzielonych tablicach */
  unsigned char dot;		/* pozycja kropki */
  char sym[SHORT_NAME_LEN];	/* nazwa skrótowa */
  char full[FULL_NAME_LEN];	/* pe³na nazwa */
  char alt[ALT_NAME_LEN];	/* nazwa alternatywna dla Przegladajacego*/
 };

typedef struct phPTTEntry tPTTEntry, *pPTTEntry;

struct phPTTInfo
 {
  ushort rev;			/* revision code */
  ushort len;			/* ilo¶æ wszystkich parametrów */
  ushort blen;			/* ilo¶æ parametrów dla bazy */
  short  dlen;			/* ilo¶æ parametrów definiowanych (0 to brak) */ 
  tPTTEntry tab[2];		/* faktyczna d³ugo¶æ len elementów */	
 };

typedef struct phPTTInfo tPTTInfo;
typedef struct phPTTInfo * pPTTInfo;

extern pPTTInfo PTT;		/* wspólna pamiêæ PTT */

extern ushort VTlen, ExtraPars;

// from (void) to (int) - moified by Vooyeck

/* inicjalizacja segmentów dzielonych parametrów */
extern int ipcInitialize();

/** wola przez podana liczbe sekund @see ipcInitialize ( co jedna sekunde )
 * wychodzi gdy zostanie przekorczony czas lub funkcja ipcInitialize zworci
 * cos innego niz -1
 * @param attempts ilosc prob
 * @return warosc ostatniego wywolania @see ipcInitialize
 */
int ipcInitializewithSleep(int attempts);

#if 0
int ipcPTTGet(char *programname);	/* inicjalizacja segmentu dzielonego PTT */
#endif
//int ipcPTTGetLocal(char *programname);	/* inicjalizacja prywatnej kopii PTT */
int ipcRdGetInto(unsigned char shmdes);	/* wej¶cie dla czytaczy */
int ipcRdGetOut(unsigned char shmdes);		/* wyj¶cie dla czytaczy */
int ipcWrGetInto(unsigned char shmdes); 	/* wej¶cie pisarzy */
int ipcWrGetOut(unsigned char shmdes);		/* wyj¶cie pisarzy */

#endif

