/* 
  libSzarp - SZARP library
  

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
 * help.h
 */


/*
 * help.h
 *
 * Modul obslugi pomocy kontekstowej
 * dla projektu Szarp
 *
 * 1998.03.12 Codematic
 */

#ifndef _HELP_H_
#define _HELP_H_

/* #include <config.h> */

typedef struct {
	char *key, *value;
} help_Translation;

typedef struct {
	char *section;
	void *client_data;
} help_MethodData;

typedef struct {
	help_Translation *translations;
	int trans_count;
	char *help_root;
	char *current_method_name;
	char *program_name;
} help_MethodInfo;

static help_MethodInfo methodinfo;

typedef enum T_id_help_method 
 { method_Init, method_Call, method_Done } id_help_method;


/* Inicjalizacja */ 
void help_init(char *programname, 
               help_Translation* trans_table, int count, 
               void *clientdata);

/* 
 * Zakonczenie prawdopodobnie nie bedzie potrzebne,
 * gdyz automatycznie ustawi sie w Terminate()
 */
 
/* Zakonczenie */
void help_done(); 

/* 
 * Funkcja z zarejestrowanymi metodami
 * Sluzy do wywolania okreslonej metody pomocy 
 * przydatne w przypadku wspolpracy programu z okreslona
 * metoda pomocy
 * normalnie nie uzywane
 */

void help_callmethod(int id, 
	help_MethodData *methoddata, help_MethodInfo *minfo);

/* 
 * Wywolanie pomocy kontekstowej
 * clientdata potrzebne dla roznych metod obslugi help
 */

void help_call(char* url, void *clientdata);

/*
 * Uzyskanie informacji o biezacej metodzie pomocy
 * przydatne w przypadku wspolpracy programu z okreslona
 * metoda pomocy
 * normalnie nie uzywane
 */

help_MethodInfo* help_getmethodinfo();

#endif

