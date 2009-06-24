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
/*
 * szbiew - editor for SzarpBase files

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * $Id$
 *
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <newt.h>

#ifndef NEWT_KEY_ESCAPE
#define NEWT_KEY_ESCAPE	27
#endif

#include <szbase/szbbase.h>

extern "C" {
    int newtGetKey(void);
}

/** arguments processing, see info argp */
#include <argp.h>

const char *argp_program_version = "szbedit 1.0";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "SZARP database viewer.\v\
Use F1 key to display HELP\n\
";

static char args_doc[] = "FILE";

static struct argp_option options[] = {
	{0}
};

struct arguments
{
	char *args[0];
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *)state->input;

	switch (key) {
		case ARGP_KEY_ARG:
			if (state->arg_num >= 1)
				/* Too many arguments. */
				argp_usage (state);
			
			arguments->args[state->arg_num] = arg;
			
			break;
			
		case ARGP_KEY_END:
			if (state->arg_num < 1)
				/* Not enough arguments. */
				argp_usage (state);
			break;									       
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

static const char * append_string = "-------  DODAJ DANE  -------";
short int * buf;
int modified = 0;
short int avg;
long int avgsum;
int avgcnt;


void Modify(void)
{
	if (modified == 0)
		newtDrawRootText(-19, 0, "M");
	modified = 1;
}

void Average(void)
{
	char * str;
	if (avgcnt <= 0) {
		avg = SZB_FILE_NODATA;
		newtDrawRootText(-35, 0, "¦RD: NO_DATA");
		newtRefresh();
		return;
	}
	avg = avgsum / avgcnt;
	asprintf(&str, "¦RD: %7d", avg);
	newtDrawRootText(-35, 0, str);
	newtRefresh();
	free(str);
}

char *PrepareEntry(int index, int year, int month, int data)
{
	char * str;
	char * str1;
	struct tm tm;

	if (year > 0) {
		probe2local(index, year, month, &tm);
		asprintf(&str1, "%04d-%02d-", tm.tm_year + 1900,
				tm.tm_mon + 1);
	} else {
		probe2gmt(index, 2000, 1, &tm);
		str1 = strdup("        ");
	}
	if (data == SZB_FILE_NODATA)
		asprintf(&str, "%s%02d %02d:%02d     NO_DATA",
				str1, tm.tm_mday, tm.tm_hour,
				tm.tm_min);
	else
		asprintf(&str, "%s%02d %02d:%02d     %7d",
				str1, tm.tm_mday, tm.tm_hour,
				tm.tm_min, data);
	free(str1);
	return str;
}

void EnterNumber(newtComponent list, int key, int year, int month)
{
	short int number = 0;
	int data = 0;
	int minus = 1;
	int i;
	char *str;
	
	newtPushHelpLine("  ESC - wyj¶cie |");
	while (1) {
		if (key == '-')
			minus = (-minus);
		else if ((key >= '0') && (key <= '9')) {
			data = 1;
			number = number * 10 + (key - '0');
			if (number < 0)
				number = (-number);
		} else if (key == NEWT_KEY_BKSPC) {
			number = number / 10;
		} else if (key == NEWT_KEY_ESCAPE) {
			newtBell();
			newtPopHelpLine();
			return;
		} else if (key == NEWT_KEY_ENTER) {
			i = (long) newtListboxGetCurrent(list);
			if (!data)
				number = buf[i];
			else {
				Modify();
				if (minus < 0)
					number = (-number);
			}
			str = PrepareEntry(i, year, month, number);
			newtListboxSetEntry(list, i, str);
			free(str);
			if (buf[i] != SZB_FILE_NODATA) {
				avgsum -= buf[i];
				avgcnt--;
			}
			buf[i] = number;
			if (buf[i] != SZB_FILE_NODATA) {
				avgsum += buf[i];
				avgcnt++;
			}
			Average();
			newtPopHelpLine();
			return;
		} else if (key == NEWT_KEY_DELETE) {
			i = (long) newtListboxGetCurrent(list);
			str = PrepareEntry(i, year, month, SZB_FILE_NODATA);
			if (buf[i] != SZB_FILE_NODATA) {
				avgsum -= buf[i];
				avgcnt--;
				buf[i] = SZB_FILE_NODATA;
				Modify();
			}
			newtListboxSetEntry(list, i, str);
			newtPopHelpLine();
			free(str);
			return;
		} else
			newtBell();
		asprintf(&str, "  ESC - wyj¶cie | ENTER - akceptuj | BACKSPACE | warto¶æ: %d",
				(minus > 0) ? number : - number);
		newtPopHelpLine();
		newtPushHelpLine(str);
		newtRefresh();
		free(str);
		key = newtGetKey();
	}
}

/**
 * @param list list component
 * @param index index in file
 * @param data data to display (probe value)
 * @param year year, 0 if not available
 * @param month month
 */
void AddEntry(newtComponent list, int index, short int data, int year,
		int month)
{
	char * str;

	str = PrepareEntry(index, year, month, data);
	newtListboxAddEntry(list, str, (void*)index);
	free(str);
}

/** analises path argument, sets maximum number of probes, write
 * first info line 
 * @param path file path
 * @param width width of the screen
 * @param maxprobes returns maximum number of probes for file 
 * @param year returns year
 * @param month returns month
 */
void CheckPath(char *path, int width, int * maxprobes, int * year,
		int * month)
{
	char *c;
	char *filename;
	char *str;
	int l, i;

	*month = *year = 0;

	c = rindex(path, '/');
	if (c == NULL)
		filename = strdup(path);
	else
		filename = strdup(c + 1);

	c = filename;
	l = strlen(filename);
	i = l - 1;
	while (i > 4) {
		if (isdigit(filename[i]) && isdigit(filename[i-1])) {
			*month = (filename[i-1] - '0') * 10 +
				filename[i] - '0';\
			i -= 2;
			break;
		}
		i--;
	}
	while (i > 2) {
		if (isdigit(filename[i]) && isdigit(filename[i-1]) &&
				isdigit(filename[i-2]) &&
				isdigit(filename[i-3])) {
			*year = (filename[i-3] - '0') * 1000 +
				(filename[i-2] - '0') * 100 +
				(filename[i-1] - '0') * 10 +
				filename[i] - '0';
			break;
		}
		i--;		
	}
	if ((*year <= 0) || (*year > 9999) || (*month <= 0) || (*month > 12))
		*year = *month = 0;
	
	if (l > (width - 41)) {
		filename[width - 41] = 0;
		asprintf(&str, "%s... | ¦RD: NO_DATA |   %c | Y: %04d M: %02d", 
				filename, (*year > 0) ? 'L' : 'U', 
				*year,* month);
	} else
		asprintf(&str, "%s%*s| ¦RD: NO_DATA |   %c | Y: %04d M: %02d",
				filename, width -l - 37, " ", 
				(*year > 0) ? 'L' : 'U', *year, *month);
	if ((*year > 0) && (*month > 0))
		*maxprobes = szb_probecnt(*year, *month);
	else
		*maxprobes = 31 * 240;
	newtDrawRootText(0, 0, str);
	free(str);
	free(filename);
}

static const char * help_message="\
\n\
  UK£AD EKRANU:\n\
      \n\
      Pierwsza linia zawiera kolejno od lewej:\n\
      - Nazwê edytowanego pliku.\n\
      - Napis AVG: i ¶redni± wszystkich danych w pliku.\n\
      - Status, który mo¿e sk³adaæ siê z 2 liter. Litera M oznacza, ¿e \n\
      plik zosta³ zmieniony od czasu ostatniego zapisu. Druga litera \n\
      oznacza sposób wy¶wietlania czasu - je¶li jest to L, to \n\
      wy¶wietlany jest  czas lokalny. Je¿eli nie jest znany miesi±c,\n\
      którego dotyczy plik, wy¶wietlana jest litera U, co oznacza \n\
      podawanie czasu UTC.\n\
      - Rok i miesi±c, którego dotyczy plik, je¿eli nie s± znane,\n\
      wy¶wietlane s± zera.\n\
      \n\
      G³ówn± czê¶æ ekranu zajmuje lista warto¶ci z pliku. Na lewo od\n\
      warto¶ci znajduje siê data i czas próbki. Je¿eli nie jest znany\n\
      miesi±c i rok, nie jest on wy¶wietlany. Je¿eli plik nie jest do\n\
      koñca zape³niony, ostatnia linia zawiera tekst 'DODAJ DANE'.\n\
      \n\
      Ostatnia linia zawiera podpowiedzi dotycz±ce klawiszy.\n\
      \n\
  KLAWISZE:\n\
      \n\
      UP, DOWN, PGUP, PGDOWN - przewijanie listy (zmiana aktualnej\n\
      próbki).\n\
      \n\
      HOME, END - przej¶cie na pocz±tek / koniec pliku.\n\
      \n\
      <cyfry> i MINUS - zmiana warto¶ci próbki. Potwierdzamy\n\
      klawiszem ENTER, ESCAPE anuluje zmianê, DELETE wstawia warto¶æ\n\
      pust±, BACKSPACE kasuje ostatnio wprowadzon± cyfrê, MINUS\n\
      zmienia znak.\n\
      \n\
      DELETE, BACKSPACE - wstawienie warto¶ci pustej lub usuniêcie\n\
      ostatniej danej z pliku.\n\
      \n\
      ENTER - przej¶cie do nastêpnej próbki lub dodanie pustej\n\
      próbki na koñcu pliku.\n\
      \n\
      ESC - wyj¶cie z programu.\n\
      \n\
      F2 - zapis zmian do pliku.\n\
      \n\
      F1 - wy¶wietlenie pomocy.\n\
      \n\
\
";

void PrintHelp(int w, int h)
{
	newtComponent form, text, button;
	
	newtCenteredWindow(w - 4, h - 4, "POMOC");
	text = newtTextbox(1, 1, w-8, h - 10, NEWT_FLAG_SCROLL);
	newtTextboxSetText(text, help_message);
	button = newtButton(w/2 - 4, h-8, "Ok");
	form = newtForm(NULL, NULL, 0);
	newtFormAddComponents(form, text, button, NULL);
	newtRunForm(form);
	newtFormDestroy(form);
	newtPopWindow();
}

void InitTerminal()
{
	struct newtColors colors;

	newtInit();
	newtCls();
	colors = newtDefaultColorPalette;

	//newt is stupid
	//kids, don't do that at home
	colors.actListboxFg = const_cast<char*>("yellow");
	colors.actListboxBg = const_cast<char*>("brown");
	colors.listboxFg = const_cast<char*>("yellow");
	colors.listboxBg = const_cast<char*>("black");
	colors.helpLineBg = const_cast<char*>("lightgray");
	colors.windowBg = const_cast<char*>("black");
	colors.helpLineFg = const_cast<char*>("black");
	colors.textboxFg = const_cast<char*>("yellow");
	colors.textboxBg = const_cast<char*>("black");
	newtSetColors(colors);
}

void PrintError(char *str)
{
	newtComponent form, label, button;
	int l;

	l = strlen(str);
	newtCenteredWindow(l+6, 8, "B£¡D"); 
	label = newtLabel(2, 1, str);
	button = newtButton(l / 2, 3, "Ok");
	form = newtForm(NULL, NULL, 0);
	newtFormAddComponents(form, label, button, NULL);
	newtRunForm(form);
	newtFormDestroy(form);
	newtPopWindow();
}

void SaveFile(char * path, int lines, int full, int avg)
{
	int fd;
	int towrite;

	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC,
			S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		char *str;
		asprintf(&str, "B³±d numer %d podczas otwierania pliku!",
				errno);
		PrintError(str);
		free(str);
		return;
	}
	//if (full) {
	//	buf[lines] = avg;
	//	towrite = lines * sizeof(short int);
	//} else
	// do not write average value
	towrite = (lines - 1) * sizeof(short int);
		
	if (write(fd, buf, towrite) != towrite ) {
		char *str;
		asprintf(&str, "B³±d numer %d podczas zapisu do pliku!",
				errno);
		PrintError(str);
		free(str);
	} else {
		modified = 0;
		newtDrawRootText(-19, 0, " ");
		newtRefresh();
	}
	close(fd);
}

int CheckExit()
{
	newtComponent form, label, b1, b2;
	struct newtExitStruct ex;
	int ret;
	
	if (modified == 0)
		return 1;
	newtCenteredWindow(40, 8, "Wyj¶cie"); 
	label = newtLabel(2, 1, "Wyj¶æ bez zapisywania?");
	b1 = newtButton(10, 3, "Nie");
	b2 = newtButton(20, 3, "Tak");
	form = newtForm(NULL, NULL, 0);
	newtFormAddComponents(form, label, b1, b2, NULL);
	newtFormRun(form, &ex);
	ret = (ex.reason == newtExitStruct::NEWT_EXIT_COMPONENT) && (ex.u.co == b2);
	newtFormDestroy(form);
	newtPopWindow();
	return ret;
	
}

int main(int argc, char* argv[]) {

	int key, fd;
	struct arguments arguments;
	char *path;
	int w, h;
	int i;
	int lines;
	int maxprobes;
	int year, month;
	int full = 0;
	
	newtComponent form, mainlist;

	argp_parse (&argp, argc, argv, 0, 0, &arguments);
	
	path = arguments.args[0];
	
	
	fd = open(path, O_RDWR | O_CREAT, 
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		fprintf(stderr, "Cannot open file '%s', errno %d\n", path,
				errno);
		return 1;
	}
	
	InitTerminal();
	
	newtGetScreenSize(&w, &h);
	assert (w > 40);
	assert (h > 15);
	CheckPath(path, w, &maxprobes, &year, &month);
	
	buf = (short int*) malloc ((maxprobes) * sizeof(short int));
	lines = read(fd, buf, (maxprobes) * sizeof(short int));
	if (lines < 0) {
		fprintf(stderr, "Error reading from file '%s', errno %d", 
				path, errno);
		close(fd);
		return 1;
	}
	close(fd);
	lines /= sizeof(short int);
	avgsum = 0;
	avgcnt = 0;
	for (i = 0; i < lines; i++) {
		if (buf[i] != SZB_FILE_NODATA) {
			avgsum += buf[i];
			avgcnt++;
		}
	}
	if ((avgcnt > 0) && (avg != SZB_FILE_NODATA)) {
		if ((avgsum / avgcnt) != avg) 
			newtDrawRootText(0, -20, "!");
		else
			newtDrawRootText(0, -20, "¦");
	}
	Average();
	for (i = lines; i < maxprobes; i++)
		buf[i] = SZB_FILE_NODATA;
	//if (lines == maxprobes) {
	//	avg = buf[maxprobes];
	//	lines -= sizeof(short int);
	//}
	
	
	newtPushHelpLine("  ESC - wyjd¼ | DEL - usuñ | <numer> - modifykuj | F2 - zapisz | F1 - pomoc ");

	form = newtForm(NULL, NULL, 0);
	mainlist = newtListbox(0, 1, h-2, NEWT_FLAG_SCROLL );
	newtListboxSetWidth(mainlist, w);
	newtFormAddComponents(form, mainlist, NULL);

	for (i = 0; i < lines; i++)
		AddEntry(mainlist, i, buf[i], year, month);
	if (lines < maxprobes) {
		newtListboxAddEntry(mainlist, append_string, (void*)lines);
		lines++;
	}
	newtDrawForm(form);
	newtRefresh();
	
	while (1) {
		if (full == 0) {
			if (lines == (maxprobes - 1)) {
				full = 1;
				newtDrawRootText(-20, 0, "¦");
				newtRefresh();
			}
		} else if (lines < (maxprobes - 1)) {
			full = 0;
			newtDrawRootText(-20, 0, " ");
			newtRefresh();
		}
		key = newtGetKey();
		switch (key) {
			case NEWT_KEY_ESCAPE:
				if (CheckExit()) {
					newtFinished();
					return 0;
				}
				break;
			case NEWT_KEY_F1:
				PrintHelp(w, h);
				break;
			case NEWT_KEY_F2:
				SaveFile(path, lines, full, avg);
				break;
			case NEWT_KEY_DOWN:
				i = (long) newtListboxGetCurrent(mainlist);
				newtListboxSetCurrent(mainlist, i+1);
				newtRefresh();
				break;
			case NEWT_KEY_UP:
				i = (long) newtListboxGetCurrent(mainlist);
				newtListboxSetCurrent(mainlist, i-1);
				newtRefresh();
				break;
			case NEWT_KEY_PGDN:
				i = (long) newtListboxGetCurrent(mainlist);
				newtListboxSetCurrent(mainlist, i+h-2);
				newtRefresh();
				break;
			case NEWT_KEY_PGUP:
				i = (long) newtListboxGetCurrent(mainlist);
				newtListboxSetCurrent(mainlist, i-(h-2));
				newtRefresh();
				break;
			case NEWT_KEY_HOME:
				newtListboxSetCurrent(mainlist, 0);
				newtRefresh();
				break;
			case NEWT_KEY_END:
				newtListboxSetCurrent(mainlist, lines-1);
				newtRefresh();
				break;
			case NEWT_KEY_BKSPC:
			case NEWT_KEY_DELETE:
				i = (long) newtListboxGetCurrent(mainlist);
				if (i < (lines - 1)) {
					EnterNumber(mainlist, NEWT_KEY_DELETE, 
							year, month);
					newtListboxSetCurrent(mainlist, i+1);
				} else if (lines > 1) {
					lines--;
					newtListboxSetCurrent(mainlist,
							lines - 1);
					newtListboxSetEntry(mainlist, lines,
							"");
					newtListboxDeleteEntry(mainlist,
							(void*)lines);
					newtRefresh();
					lines--;
					newtListboxDeleteEntry(mainlist,
							(void*)lines);
					if (buf[lines] != SZB_FILE_NODATA) {
						avgsum -= buf[lines];
						avgcnt--;
						buf[lines] = SZB_FILE_NODATA;
						Average();
					}
					Modify();
					newtRefresh();
					newtListboxAddEntry(mainlist, 
							append_string,
							(void*)lines);
						newtListboxSetCurrent(mainlist,
								lines);
						lines++;
				}
				newtRefresh();
				break;
			default:
				if ((key == '-') || (key == NEWT_KEY_ENTER) ||
						((key >= '0') && (key <= '9'))) {
					EnterNumber(mainlist, key, year, month);
					i = (long) newtListboxGetCurrent(mainlist);
					if ((i == (lines - 1)) && 
							(lines < maxprobes)) {
						newtListboxAddEntry(mainlist, 
							append_string,
							(void*)lines);
						lines++;
						Modify();
					}
					newtListboxSetCurrent(mainlist, i+1);
					newtRefresh();
				}
				newtBell();
				break;
		}
	}
}

