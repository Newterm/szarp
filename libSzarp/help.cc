/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka MyInc
 * help.c
 */

/*
 * help.c
 *
 * Modul obslugi pomocy kontekstowej
 * dla projektu Szarp
 *
 * 02.10.1998 Codematic
 */

#include <config.h>
#ifndef NO_MOTIF

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "libpar.h"
#include "help.h"
#include "msgerror.h"
#include "execute.h"



/* Procedury pomocnicze */

char* FindTranslation(char *key, help_MethodInfo *minfo)
{
 int i;
 
 if (!minfo || !(minfo->translations)) return NULL;
 for (i = 0; i < minfo->trans_count; i++) {
 	if (!strcmp(key, (minfo->translations)[i].key ))
 		return (minfo->translations[i]).value;
 }
 return NULL;
}

char* Translate(char *key, help_MethodInfo *minfo)
{
 char *c = NULL;
 
 if (key) c = FindTranslation(key, minfo);
 if (c) return c;
 else return key; 
}

char* newstring(char *str)
{
 char *c;
 
 c = (char*) malloc(strlen(str)+1);
 if (!c) return NULL;
 strcpy(c, str);
 return c;
}

 /* majac $key robi "$helproot/$key. Alokuje pamiec */
char* MakeNameWithPath(char *key, help_MethodInfo *minfo)
{
 char *c;
 int i;
 i = strlen(minfo->help_root)+1+strlen(key)+1; /* root+/+key+/0 */
 c = (char*) malloc(i);
 if (!c) return NULL;
 c[0] = 0;
 strcat(c, minfo->help_root);
 if (c[0]) if (c[strlen(c)-1] != '/') strcat(c, "/");
 strcat(c, key);
 return c;
}

/* Obsluga metod pomocy */

/*
procedury do metod pomocy. Na razie sa cztery:

command) Wczytywanie z szarp.cfg komendy ze znaczkiem %s na url i wywolanie
xmhtml) Tworzenie okienka z XmHtml
dt) Unixware'owe Dt.h
debug) Po prostu na stdout pokazuje nazwe url'a ktory ma byc odpalany

Maybe someday zaawansowane KDE lub GNOME
*/

void Help_CommandMethod(int id,  help_MethodData *methoddata, 
       help_MethodInfo *minfo)
{
 char *c, commandbuf[400];
 static char command[200];

 if (id == method_Init) {
 	libpar_readpar(minfo->program_name, "helpcommand", 
 	 command, sizeof(command), 1);
 };
 if (id == method_Call) {
	c = MakeNameWithPath(methoddata->section, (help_MethodInfo*) minfo);
	sprintf(commandbuf, command, c);
	execute(commandbuf);
	free(c);
 } 	 
}

void Help_DebugMethod(int id,  help_MethodData *methoddata, void *minfo)
{
 char *c;
 printf("Help_DebugMethod called, ");
 switch (id) {
 	case method_Init: printf("id = methodInit\n");break;
 	case method_Call: printf("id = methodCall\n");break;
 	case method_Done: printf("id = methodDone\n");break;
 	default: printf("id = unknown (%d)\n", id);break;
 };
 if (id == method_Call) {
	c = MakeNameWithPath(methoddata->section, (help_MethodInfo*) minfo);
 	printf("Section: %s (url: %s)\n", methoddata->section, c);
	free(c);
 } 	 
}

/* Funkcja z zarejestrowanymi metodami */

void help_callmethod(int id, 
	help_MethodData *methoddata, help_MethodInfo *minfo)
{
 if (!strcmp(minfo->current_method_name, "none")); /* Nic nie rob */
 else if (!strcmp(minfo->current_method_name, "debug"))
 	Help_DebugMethod(id, methoddata, minfo);
 else if (!strcmp(minfo->current_method_name, "command"))
 	Help_CommandMethod(id, methoddata, minfo); 
/* else if (!strcmp(methodname, "xmhtml"))
 	Help_XmHtmlMethod(id, clientdata);
 else if (!strcmp(methodname, "dt"))
 	Help_DtMethod(id, clientdata); */
 else if (id == method_Init)
 	WarnMessage(8, "Nieznana metoda pomocy !");
}

/* Interface */

/* Inicjalizacja */ 
void help_init(char *programname, 
               help_Translation* trans_table, int count, 
               void *clientdata)
{
 char buf[200] = "";
 help_MethodData mdata;

 
	methodinfo.translations = trans_table;
	methodinfo.trans_count = count;
	methodinfo.program_name = newstring(programname);

	libpar_readpar(programname, "helpmethod", buf, sizeof(buf), 1);
	methodinfo.current_method_name =
	 newstring(buf);

	libpar_readpar(programname, "helproot", buf, sizeof(buf), 1);
	methodinfo.help_root = newstring(buf);
		
	mdata.section = NULL;
	mdata.client_data = clientdata;
		
	help_callmethod(method_Init, &mdata, &methodinfo);

}

/* Zakonczenie */
void help_done()
{
 methodinfo.translations = NULL;
 methodinfo.trans_count = 0;
 /* Zaleznie od metody, np. zamyka okno */
 /* Przy zakonczeniu niepotrzebne sa dodatkowe dane */
 help_callmethod(method_Done, NULL , &methodinfo);

 free(methodinfo.current_method_name);
 free(methodinfo.help_root); 
 free(methodinfo.program_name);
}

/* 
 * Wywolanie pomocy kontekstowej
 * clientdata potrzebne dla roznych metod obslugi help
 */

void help_call(char* url, void *clientdata)
{
 help_MethodData mdata;

 	mdata.section = Translate(url, &methodinfo);
 	mdata.client_data = clientdata;
	help_callmethod(method_Call, &mdata, &methodinfo);
}

help_MethodInfo* help_getmethodinfo()
{
 return &methodinfo;
}

#endif /* NO_MOTIF */
