/* 
  libSzarp - SZARP library

*/
/*
 * $Id$
 *
 * Wewnêtrzny plik nag³owkowy bliblioteki do parsowania plików 
 * konfiguracyjnych (libparnt).
 *
 * SZARP 2.1
 * Pawe³ Pa³ucha 28.04.2001
 *
 */

#ifndef __LIBPAR_INT_H__
#define __LIBPAR_INT_H__

//#define PAR_NAME_SIZE 30
//#define PAR_CONTENT_SIZE 500
//#define SECT_NAME_SIZE 30
#define CFGNAME "szarp.cfg"

/* Dane - listy, zmienne */

struct _Par {
 char *name;
 char *content;
 struct _Par *next;
};
typedef struct _Par Par;

struct _Sections {
 char *name;
 Par *pars;
 struct _Sections *next;
};
typedef struct _Sections Sections;

/* Dwie listy globalne */

extern Par *globals;
extern Sections *sect;

/* Funkcje pomocnicze */

Sections* AddSection(const char *name);
void AddPar(Par **list, const char *name, const char *content);
void DeleteParList(Par **list);
void DeleteSectList();
char *SeekPar(Par *list, const char *name);
Sections *SeekSect(const char *name);

void setvalue(const char *name, const char *val);
char* getvalue(const char *name);

#endif
