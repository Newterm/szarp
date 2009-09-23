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
 * parcook
 * parcook.c
 */

#define _IPCTOOLS_H_		/* Nie dolaczaj ipctools.h z szarp.h */
#define _HELP_H_
#define _ICON_ICON_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <libgen.h>
#include <ctype.h>

#ifndef NO_LUA
#include <lua.hpp>
#endif

#include <argp.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <string>
#include <tr1/unordered_map>

#include "szarp.h"
#include "szbase/szbbase.h"
#include "szbase/szbhash.h"

#include "funtable.h"
#include "daemon.h"
#include "liblog.h"
#include "ipcdefines.h"
#include "szarp_config.h"

#include "conversion.h"

using std::tr1::unordered_map;

/***********************************************************************/
/* Arguments handling, see 'info argp' */

const char *argp_program_version = "parcook 2.""$Revision: 6824 $";
const char *argp_program_bug_address = "coders@praterm.com.pl";
static char doc[] = "SZARP parameters daemon.\v\
Config file:\n\
Configuration options are read from file /etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg.\n\
These options are read from global section and are mandatory:\n\
	parcook_cfg	full path to parcook.cfg file\n\
	parcook_path	path to file used to create identifiers for IPC\n\
			resources, usually the same as parcook_cfg; this\n\
			file must exist.\n\
	linex_cfg	path to file used to create IPC identifiers for\n\
			communicating with line daemons, usually set to\n\
			main configuration directory; this file (or directory)\n\
			must exist.\n\
These options are read from section 'parcook' and are optional:\n\
	log		path to log file, default is " PREFIX "/log/parcook\n\
	log_level	log level, from 0 to 10, default is from command line\n\
			or 2.\n\
";

static struct argp_option options[] = {
	{"debug", 'd', "n", 0, "Set initial debug level to n, n is from 0 \
(error only), to 10 (extreme debug), default is 2. This is overwritten by \
settings in config file."},
	{"D<param>", 0, "val", 0,
	 "Set initial value of libpar variable <param> to val."},
	{"no-daemon", 'n', 0, 0,
	 "Do not fork and go into background, useful for debug."},
	{0}
};

struct arguments
{
	int no_daemon;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments *) state->input;

	switch (key) {
		case 'd':
		case 0:
			break;
		case 'n':
			arguments->no_daemon = 1;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

/***********************************************************************/

#define SUP_COLD 0		/* cold startup - bez odczytu Masakra.dat */
#define SUP_WARM 1		/* warm startup - odczyt Masakra.dat */
#define SUP_RECONF 2		/* rekonfiguracja - odczyt Masakra.dat,
				   przekodowanie */
#define OPTIONS_LIMIT 255 		/* Parametrs limit */

int ProbeDes, MinuteDes, Min10Des, HourDes, SemDes, AlertDes;
int MsgSetDes, MsgRplyDes;

short *Probe;			/* ostatnia probka */

short *Minute;			/* srednia ostatnia minuta */

short *Min10;			/* srednia ostatnie 10 minut */

short *Hour;			/* srednia ostatnia godzina */

unsigned char *Alert;		/* tablica przekroczen */


short first_time;		/* pierwszy przebieg petli glownej programu */

short last_min;			/* ostania minuta w ktorej liczona byla
				   srednia */

short min;			/* biezaca minuta */

short min10;			/* biezaca 10-o minutowka */

short last_min10;		/* ostatnie 10 minut w ktorej liczona byla
				   srednia */

typedef unsigned char tCnts[3];

typedef int tSums[3];

typedef short tProbes[6];

typedef short tMinutes[10];

typedef short tMin10s[6];

tProbes *Probes;		/* bufor probek tworzacych ostatnia minute */

tMinutes *Minutes;		/* bufor ostatnich 10 minut */

tMin10s *Min10s;		/* bufor ostatniej godziny */

tCnts *Cnts;			/* liczniki usrednien */

tSums *Sums;			/* sumy usrednien */

struct sembuf Sem[4];

static char* parcookpat = NULL;	/**< sciezka do parcook.cfg */

#define INFO_LEN	128

struct phLineInfo
{
	unsigned char LineNum;	/* numer linii */
	ushort ParBase;		/* adres bazowy parametrow */
	ushort ParTotal;	/* liczba parametrow */
	std::wstring daemon;	/* plik wykonawczy daemona */
	std::wstring device;	/* nazwa kontrolowanego urzadzenia */
	std::wstring options;	/* dodatkowe opcje do programu */
	int Linedmn;		/* PID demona linii */
	int ShmDes;		/* deskryptor wspolnej pamieci */
	short *ValTab;		/* wskaznik na wspolna pamiec */
};

typedef struct phLineInfo tLineInfo;

const char* lua_ipc_table_key = "ipc_table_key";

struct Hash {
	size_t operator() (const std::wstring& str) const {
		return hash(str.c_str(), str.size(), 0);
	}
};

typedef unordered_map<std::wstring, double, Hash> PH;

struct LuaParamInfo {
	TParam *param;
	int index;
};

std::vector<tLineInfo> LinesInfo;

struct phEquatInfo
{
	ushort len;
	std::vector<std::wstring> tab;
};

typedef struct phEquatInfo tEquatInfo;

tEquatInfo Equations;

ushort VTlen;

unsigned int BasePeriod;
	
unsigned int ExtraPars;	/**< number of extra lines (formulas) */

unsigned int NumberOfLines;

time_t sectime;
struct tm *curtime;
struct tm tmbuf;
	
#ifndef NO_LUA
int lua_ipc_value(lua_State *lua) {

	const char* param = luaL_checkstring(lua, 1);

	lua_pushstring(lua, lua_ipc_table_key);
	lua_gettable(lua, LUA_REGISTRYINDEX);
	PH *ph = (PH*)lua_touserdata(lua, -1);

	PH::iterator i = ph->find(SC::U2S((unsigned char*)param));
	if (i == ph->end())
		luaL_error(lua, "Param %s not found in IPC values", param);

	double result = i->second;
	lua_pushnumber(lua, result);

	return 1;

}

bool compile_script(TParam *p) {

	std::ostringstream paramfunction;

	paramfunction					<< 
	"return function ()"				<< std::endl <<
	"	local p = szbase"			<< std::endl <<
	"	local PT_MIN10 = ProbeType.PT_MIN10"	<< std::endl <<
	"	local PT_HOUR = ProbeType.PT_HOUR"	<< std::endl <<
	"	local PT_HOUR8 = ProbeType.PT_HOUR8"	<< std::endl <<
	"	local PT_DAY = ProbeType.PT_DAY"	<< std::endl <<
	"	local PT_WEEK = ProbeType.PT_WEEK"	<< std::endl <<
	"	local PT_MONTH = ProbeType.PT_MONTH"	<< std::endl <<
	"	local PT_CUSTOM = ProbeType.PT_CUSTOM"	<< std::endl <<
	"	local szb_move_time = szb_move_time"	<< std::endl <<
	"	local i = ipc_value"			<< std::endl <<
	"	local state = {}"			<< std::endl <<
	"	return function (param)"		<< std::endl <<
	"		local v = nil"			<< std::endl <<
	p->GetLuaScript()				<< std::endl <<
	"		return v"			<< std::endl <<
	"	end"					<< std::endl <<
	"end"						<< std::endl;

	std::string str = paramfunction.str();

	const char* content = str.c_str();

	lua_State* lua = Lua::GetInterpreter();

	int ret = luaL_loadbuffer(lua, content, strlen(content), (const char*)SC::S2U(p->GetName()).c_str());
	if (ret != 0) {
		sz_log(0, "Error compiling param %ls: %s\n", p->GetName().c_str(), lua_tostring(lua, -1));
		return false;
	}

	ret = lua_pcall(lua, 0, 1, 0);
	if (ret != 0) {
		sz_log(0, "Error compiling param %ls: %s\n", p->GetName().c_str(), lua_tostring(lua, -1));
		return false;
	}

	ret = lua_pcall(lua, 0, 1, 0);
	if (ret != 0) {
		sz_log(0, "Error compiling param %ls: %s\n", p->GetName().c_str(), lua_tostring(lua, -1));
		return false;
	}

	p->SetLuaParamRef(luaL_ref(lua, LUA_REGISTRYINDEX));

	return true;

}


bool compile_scripts(TSzarpConfig *sc, std::vector<LuaParamInfo*>& param_info) {

	for (TParam* p = sc->GetFirstDefined(); p; p = p->GetNext(false)) 
		if (p->GetLuaScript()) {
			bool ret = compile_script(p);
			if (ret == false)
				return false;
			LuaParamInfo *pi = new LuaParamInfo;
			pi->param = p;
			pi->index = p->GetIpcInd();
			param_info.push_back(pi);
		}

	return true;
}


void register_lua_functions(lua_State *lua) {
	const struct luaL_reg ParseScriptLibFun[] = {
		{ "ipc_value", lua_ipc_value},
		{ NULL, NULL }
	};

	const luaL_Reg *lib = ParseScriptLibFun;

	for (; lib->func; lib++) 
		lua_register(lua, lib->name, lib->func);

}
#endif

/** Removes IPC resources. Registered with 'atexit()' as on exit cleanup
 * function (first level). */
void Rmipc()
{
	unsigned char ii;
	int i;

	errno = 0;

	for (ii = 0; ii < NumberOfLines; ii++) {
		kill(LinesInfo[ii].Linedmn, SIGKILL);
		i = shmctl(LinesInfo[ii].ShmDes, IPC_RMID, NULL);
		sz_log((i < 0 ? 1 : 10),
		    "parcook: removing shared memory segment for line deamon %d, shmctl() returned %d, errno %d",
		    ii, i, errno);
	}

	i = shmctl(ProbeDes, IPC_RMID, NULL);
	sz_log((i < 0 ? 1 : 10),
	    "parcook: removing 'probes' shared memory segment, shmctl() returned %d errno %d",
	    i, errno);

	i = shmctl(MinuteDes, IPC_RMID, NULL);
	sz_log((i < 0 ? 1 : 10),
	    "parcook: removing 'min' shared memory segment, shmctl() returned %d errno %d",
	    i, errno);

	i = shmctl(Min10Des, IPC_RMID, NULL);
	sz_log((i < 0 ? 1 : 10),
	    "parcook: removing 'min10' shared memory segment, shmctl() returned %d errno %d",
	    i, errno);

	i = shmctl(HourDes, IPC_RMID, NULL);
	sz_log((i < 0 ? 1 : 10),
	    "parcook: removing 'hour' shared memory segment, shmctl() returned %d errno %d",
	    i, errno);

	i = shmctl(AlertDes, IPC_RMID, NULL);
	sz_log((i < 0 ? 1 : 10),
	    "parcook: removing 'alert' shared memory segment, shmctl() returned %d errno %d",
	    i, errno);

	i = semctl(SemDes, IPC_RMID, 0);
	sz_log((i < 0 ? 1 : 10),
	    "parcook: removing semaphores, semctl() returned %d errno %d",
	    i, errno);

	i = msgctl(MsgSetDes, IPC_RMID, NULL);
	sz_log((i < 0 ? 1 : 10),
	    "parcook: removing 'set' message queue, msgctl() returned %d errno %d",
	    i, errno);

	i = msgctl(MsgRplyDes, IPC_RMID, NULL);
	sz_log((i < 0 ? 1 : 10),
	    "parcook: removing 'reply' message queue, msgctl() returned %d errno %d",
	    i, errno);

}

/** Releases semaphores. Registered with atexit(), second level. */
void CleanUp()
{
	ushort i;

	for (i = 0; i < 12; i++)
		semctl(SemDes, i, SETVAL, 0);
}


float ChooseFun(float funid, float *parlst)
{
	ushort fid;

	fid = (ushort) funid;
	if (fid >= MAX_FID)
		return (0.0);
	return ((*(FunTable[fid])) (parlst));
}

unsigned char CalculNoData;

short shmGetValue(ushort adr, unsigned char shmdes)
{
	short val;

	switch (shmdes) {
		case 0:
			val = Probe[adr];
			break;
		case 1:
			Sem[0].sem_num = SEM_MINUTE + 1;
			Sem[0].sem_op = 1;
			Sem[1].sem_num = SEM_MINUTE;
			Sem[1].sem_op = 0;
			semop(SemDes, Sem, 2);
			if ((Minute =
			     (short *) shmat(MinuteDes, (void *) 0,
					     0)) == (void *) -1) {
				sz_log(0, "parcook: cannot attach 'min' memory segment, \
errno %d, exiting", errno);
				exit(1);
			}
			val = Minute[adr];
			Sem[0].sem_num = SEM_MINUTE + 1;
			Sem[0].sem_op = -1;
			semop(SemDes, Sem, 1);
			shmdt((void *) Minute);
			break;
		case 2:
			Sem[0].sem_num = SEM_MIN10 + 1;
			Sem[0].sem_op = 1;
			Sem[1].sem_num = SEM_MIN10;
			Sem[1].sem_op = 0;
			semop(SemDes, Sem, 2);
			if ((Min10 =
			     (short *) shmat(Min10Des, (void *) 0,
					     0)) == (void *) -1) {
				sz_log(0, "parcook: cannot attach 'min10' memory segment, \
errno %d, exiting", errno);
				exit(1);
			}
			val = Min10[adr];
			Sem[0].sem_num = SEM_MIN10 + 1;
			Sem[0].sem_op = -1;
			semop(SemDes, Sem, 1);
			shmdt((void *) Min10);
			break;
		case 3:
			Sem[0].sem_num = SEM_HOUR + 1;
			Sem[0].sem_op = 1;
			Sem[1].sem_num = SEM_HOUR;
			Sem[1].sem_op = 0;
			semop(SemDes, Sem, 2);
			if ((Hour =
			     (short *) shmat(HourDes, (void *) 0,
					     0)) == (void *) -1) {
				sz_log(0, "parcook: cannot attach 'min10' memory segment, \
errno %d, exiting", errno);
				exit(1);
			}
			val = Hour[adr];
			Sem[0].sem_num = SEM_HOUR + 1;
			Sem[0].sem_op = -1;
			semop(SemDes, Sem, 1);
			shmdt((void *) Hour);
			break;
		default:
			val = SZARP_NO_DATA;
			break;
	}
	return (val);
}



void Calcul(ushort n)
{
	const wchar_t *chptr;
	ushort adr;
	unsigned char shmdes;
#define SS	30
	float stack[SS+1];
	char nodata[SS+1];
	float tmp;
	short sp = 0;
	short parcnt;
	short val;
	int NullFormula = 0;

	CalculNoData = 0;

	chptr = Equations.tab[n].c_str();
	do {
		if (sp >= SS) {
			sz_log(0, "parcook: stack overflow after %td chars when calculating formula '%ls'",
					chptr - Equations.tab[n].c_str(), Equations.tab[n].c_str());
			CalculNoData = 1;
			return;
		}
		if (iswdigit(*chptr)) {
			tmp = wcstof(chptr, NULL);
			adr = (ushort) rint(floor(tmp));
			shmdes = (unsigned char) rint(10.0 * fmod(tmp, 1.0));
			chptr = wcschr(chptr, L' ');
			if (adr >= VTlen)
				return;
			if ((val = shmGetValue(adr, shmdes)) != SZARP_NO_DATA) {
				nodata[sp] = 0;
			} else {
				nodata[sp] = 1;
			}
			stack[sp++] = (float) val;	
		} else {
			switch (*chptr) {
				case L'&':
					if (sp < 2)	/* swap */
						return;
					tmp = stack[sp - 1];
					stack[sp - 1] = stack[sp - 2];
					stack[sp - 2] = tmp;
					break;
				case L'!':
					stack[sp] = stack[sp - 1];	/* duplicate 
									 */
					sp++;
					break;
				case L'$':
					if (sp-- < 2)	/* function call */
						return;
					parcnt = (short) rint(stack[sp - 1]);
					if (sp < parcnt + 1)
						return;
					stack[sp - parcnt - 1] =
						ChooseFun(stack[sp],
							  &stack[sp - parcnt -
								 1]);
					sp -= parcnt;
					break;
				case L'#':
					nodata[sp] = 0;
					stack[sp++] = wcstof(++chptr, NULL);
					chptr = wcschr(chptr, L' ');
					break;
				case L'i':
					if (*(++chptr) == L'f') {	
				/* warunek <par1> <par2> <cond> if 
				 * jesli <cond> != 0 zostawia <par1> 
				 * w innym wypadku zostawia <par2> */
						if (sp-- < 3)	
							return;
						if (nodata[sp]) {
							CalculNoData = 1;
							break;
						}
						if (stack[sp] == 0)
							stack[sp - 2] =
								stack[sp - 1];
						sp--;
					}
					break;
				case L'n':
					chptr += 3;
					NullFormula = 1;
					break;
				case L'+':
					if (sp-- < 2)
						return;
					if (nodata[sp] || nodata[sp-1]) {
						CalculNoData = 1;
						break;
					}
					stack[sp - 1] += stack[sp];
					break;
				case L'-':
					if (sp-- < 2)
						return;
					if (nodata[sp] || nodata[sp-1]) {
						CalculNoData = 1;
						break;
					}
					stack[sp - 1] -= stack[sp];
					break;
				case L'*':
					if (sp-- < 2)
						return;
					if (nodata[sp] || nodata[sp-1]) {
						CalculNoData = 1;
						break;
					}
					stack[sp - 1] *= stack[sp];
					break;
				case L'/':
					if (sp-- < 2)
						return;
					if (nodata[sp] || nodata[sp-1]) {
						CalculNoData = 1;
						break;
					}
					if (stack[sp] != 0.0)
						stack[sp - 1] /= stack[sp];
					else {
						stack[sp - 1] = 1;
						CalculNoData = 1;
					}
					break;
				case L'>':
					if (sp-- < 2)	/* wieksze */
						return;
					if (nodata[sp] || nodata[sp-1]) {
						CalculNoData = 1;
						break;
					}
					if (stack[sp - 1] > stack[sp])
						stack[sp - 1] = 1;
					else
						stack[sp - 1] = 0;
					break;
				case L'<':
					if (sp-- < 2)	/* mniejsze */
						return;
					if (nodata[sp] || nodata[sp-1]) {
						CalculNoData = 1;
						break;
					}
					if (stack[sp - 1] < stack[sp])
						stack[sp - 1] = 1;
					else
						stack[sp - 1] = 0;
					break;
				case L'~':
					if (sp-- < 2)	/* rowne */
						return;
					if (nodata[sp] || nodata[sp-1]) {
						CalculNoData = 1;
						break;
					}
					if (stack[sp - 1] == stack[sp])
						stack[sp - 1] = 1;
					else
						stack[sp - 1] = 0;
					break;
				case L'N' :	/* no-data */
					if (sp-- < 2) 
						return;
					if (nodata[sp - 1]) {
						stack[sp - 1] = stack[sp];
						nodata[sp - 1] = nodata[sp];
					}
					break;
				case L'm' :    /* no-data if stack[sp -2] < stack [sp - 1], otherwise
			       			stack[sp-2]	*/
					if (sp -- < 2) return;
					if (nodata[sp] || nodata[sp-1]) {
						CalculNoData = 1;
						break;
					}
					if (stack[sp-1] < stack[sp]) {
						nodata[sp-1] = 1;
					}
					break;
				case L'=':
					if (sp-- < 2)
						return;
					if ((ushort) rint(stack[sp]) >= VTlen)
						return;
					adr = (ushort) rint(stack[sp]);
					if (CalculNoData)
						Probe[adr] = SZARP_NO_DATA;
					else if (NullFormula)
						return;	/* don't touch - code 
							 */
					else if (nodata[sp-1]) {
						Probe[adr] = SZARP_NO_DATA;
					} else 
						Probe[adr] =
							(short)
							rint(stack[sp - 1]);
					return;
				case L' ':
					break;
				default:
					sz_log(1, "Uknown character '%lc' in formula '%ls' for parameter %d",
							*chptr, Equations.tab[n].c_str(), n);
			}
		}
	} while (*(++chptr) != 0);
#undef SS
}

void ClearShm(int shmdes, int number)
{
	short *tab;
	int j;

	tab = (short *) shmat(shmdes, (void *) 0, 0);
	if (tab == (void *) -1) {
		sz_log(0, "parcook: cannot attach memory segment %0x for cleaning,\
errno %d, exiting", shmdes, errno);
		exit(1);
	}
	for (j = 0; j < number; j++)
		tab[j] = SZARP_NO_DATA;
	shmdt((void *) tab);
}

/************************************************************************/
/* Signal handling */

RETSIGTYPE Terminate(int sig)
{
	sz_log(0, "parcook: signal %d cought, cleaning up and exiting", sig);
	exit(1);
}

RETSIGTYPE CriticalHandler(int sig)
{
	sz_log(0, "parcook: signal %d cought, exiting, report to authors", sig);
	/* restore default action, which is to abort */
	signal(sig, SIG_DFL);
	raise(sig);
}

int InitSignals()
{
	int ret;
	struct sigaction sa;
	sigset_t block_mask;

	sigfillset(&block_mask);
	sigdelset(&block_mask, SIGKILL);
	sigdelset(&block_mask, SIGSTOP);

	/* set no-cleanup handlers for critical signals */
	sa.sa_handler = CriticalHandler;
	sa.sa_mask = block_mask;
	sa.sa_flags = 0;
	ret = sigaction(SIGFPE, &sa, NULL);
	assert(ret == 0);
	ret = sigaction(SIGQUIT, &sa, NULL);
	assert(ret == 0);
	ret = sigaction(SIGILL, &sa, NULL);
	assert(ret == 0);
	ret = sigaction(SIGSEGV, &sa, NULL);
	assert(ret == 0);
	ret = sigaction(SIGBUS, &sa, NULL);
	assert(ret == 0);

	/* cleanup handlers for termination signals */
	sa.sa_handler = Terminate;
	sa.sa_flags = SA_RESTART;
	ret = sigaction(SIGTERM, &sa, NULL);
	assert(ret == 0);
	ret = sigaction(SIGINT, &sa, NULL);
	assert(ret == 0);
	ret = sigaction(SIGHUP, &sa, NULL);
	assert(ret == 0);
	return 0;
}

/************************************************************************/

/** create semaphores and message queues */
void CreateSemMsg(void)
{	
	key_t key;
	key = ftok(parcookpat, SEM_PARCOOK);
	if (key == -1) {
		sz_log(0, "parcook: ftok('%s', %d) returned error, errno %d, exiting",
				parcookpat, SEM_PARCOOK, errno);
		exit(1);
	}
	SemDes = semget(key, SEM_LINE + NumberOfLines * 2, IPC_CREAT | 00666);
	if (SemDes == -1) {
		sz_log(0,
		    "parcook: cannot get semaphore descriptor, errno %d, exiting",
		    errno);
		exit(1);
	} else {
		sz_log(10, "parcook: semget(%x, %d) successfull", key, 
				SEM_LINE + NumberOfLines * 2);
	}
	if ((MsgSetDes =
	     msgget(ftok(parcookpat, MSG_SET), IPC_CREAT | 00666)) < 0) {
		sz_log(0,
		    "parcook: cannot get 'set' message queue descriptor, errno %d, exiting",
		    errno);
		exit(1);
	}
	if ((MsgRplyDes =
	     msgget(ftok(parcookpat, MSG_RPLY), IPC_CREAT | 00666)) < 0) {
		sz_log(0,
		    "parcook: cannot get 'reply' message queue descriptor, errno %d, exiting",
		    errno);
		exit(1);
	}
}

/**< create memory segments for probes values */
void CreateProbesSegments(char* parcookpat)
{
	key_t key;
	key = ftok(parcookpat, SHM_PROBE);
	if (key == -1) {
		sz_log(0, "parcook: ftok(%s, SHM_PROBE) error, errno %d",
				parcookpat, errno);
		exit(1);
	}
	if ((ProbeDes =
	     shmget(key, VTlen * sizeof(short), IPC_CREAT | 00666))  == -1) {
		sz_log(0,
		    "parcook: cannot get shared memory descriptor for 'probe' segment, errno %d, exiting",
		    errno);
		exit(1);
	}
	key = ftok(parcookpat, SHM_MINUTE);
	if (key == -1) {
		sz_log(0, "parcook: ftok(%s, SHM_MINUTE) error, errno %d",
				parcookpat, errno);
		exit(1);
	}
	if ((MinuteDes =
	     shmget(key, VTlen * sizeof(short), IPC_CREAT | 00666))  == -1) {
		sz_log(0,
		    "parcook: cannot get shared memory descriptor for 'min' segment, errno %d, exiting",
		    errno);
		exit(1);
	}
	key = ftok(parcookpat, SHM_MIN10);
	if (key == -1) {
		sz_log(0, "parcook: ftok(%s, SHM_MIN10) error, errno %d",
				parcookpat, errno);
		exit(1);
	}
	if ((Min10Des =
	     shmget(key, VTlen * sizeof(short), IPC_CREAT | 00666))  == -1) {
		sz_log(0,
		    "parcook: cannot get shared memory descriptor for 'min10' segment, errno %d, exiting",
		    errno);
		exit(1);
	}
	key = ftok(parcookpat, SHM_HOUR);
	if (key == -1) {
		sz_log(0, "parcook: ftok(%s, SHM_HOUR) error, errno %d",
				parcookpat, errno);
		exit(1);
	}
	if ((HourDes =
	     shmget(key, VTlen * sizeof(short), IPC_CREAT | 00666))  == -1) {
		sz_log(0,
		    "parcook: cannot get shared memory descriptor for 'hour' segment, errno %d, exiting",
		    errno);
		exit(1);
	}
	key = ftok(parcookpat, SHM_ALERT);
	if (key == -1) {
		sz_log(0, "parcook: ftok(%s, SHM_ALERT) error, errno %d",
				parcookpat, errno);
		exit(1);
	}
	if ((AlertDes =
	     shmget(key, VTlen * sizeof(unsigned char), 
	     		IPC_CREAT | 00666))  == -1) {
		sz_log(0,
		    "parcook: cannot get shared memory descriptor for 'alert' segment, errno %d, exiting",
		    errno);
		exit(1);
	}
}

/** allocate memory for probes */
void AllocProbesMemory(void)
{
	int i, ii;

	if ((Probes = (tProbes *) calloc(VTlen, sizeof(tProbes))) == NULL) {
		sz_log(0, "parcook: calloc error for Probes[], exiting");
		exit(1);
	}
	if ((Minutes = (tMinutes *) calloc(VTlen, sizeof(tMinutes))) == NULL) {
		sz_log(0, "parcook: calloc error for Minutes[], exiting");
		exit(1);
	}
	if ((Min10s = (tMin10s *) calloc(VTlen, sizeof(tMin10s))) == NULL) {
		sz_log(0, "parcook: calloc error for Min10s[], exiting");
		exit(1);
	}
	if ((Cnts = (tCnts *) calloc(VTlen, sizeof(tCnts))) == NULL) {
		sz_log(0, "parcook: calloc error for Cnts[], exiting");
		exit(1);
	}
	if ((Sums = (tSums *) calloc(VTlen, sizeof(tSums))) == NULL) {
		sz_log(0, "parcook: calloc error for Sums[], exiting");
		exit(1);
	}

	/* set memory to NO_DATA */
	for (ii = 0; ii < VTlen; ii++) {
		for (i = 0; i < 3; i++) {
			Cnts[ii][i] = (unsigned char) 0;
			Sums[ii][i] = (int) 0;
		}
		for (i = 0; i < 6; i++) {
			Probes[ii][i] = (short) SZARP_NO_DATA;
		}
		for (i = 0; i < 10; i++){
			Minutes[ii][i] = (short) SZARP_NO_DATA;
		}
		for (i = 0; i < 6; i++) {
			Min10s[ii][i] = (short) SZARP_NO_DATA;
		}
	}
}

/** start daemon for line i */
void LanchDaemon(int i, char* linedmnpat)
{
	int pid, s;
	struct stat sstat;
	key_t key;

	sz_log(10, "parcook: starting daemon index: %d LineNum: %d ParTotal: %d Daemon: %ls Device: %ls Options: %ls\n",
			i, LinesInfo[i].LineNum, LinesInfo[i].ParTotal, LinesInfo[i].daemon.c_str(), LinesInfo[i].device.c_str(), LinesInfo[i].options.c_str());

	/* set number of first param */
	LinesInfo[i].ParBase = VTlen;
	/* increase global params count */
	VTlen += LinesInfo[i].ParTotal;
	
	key = ftok(linedmnpat, LinesInfo[i].LineNum);
	if (key == -1) {
		sz_log(0, "parcook: ftok('%s', %d) returned error for line %d, errno %d, exiting",
				linedmnpat, LinesInfo[i].LineNum, i + 1, errno);
		exit(1);
	}
	sz_log(10, "parcook: creating segment: key %08x\n", key);

	/* create segment for communicating with daemon */
	if ((LinesInfo[i].ShmDes = shmget(key,
					LinesInfo[i].ParTotal * sizeof(short),
					IPC_CREAT | 00666)) == -1) {
		sz_log(0, "parcook: cannot get memory segment descriptor for line %d, errno %d, exiting",
				i + 1, errno);
		exit(1);
	}
	
	/* clear segment memory */
	ClearShm(LinesInfo[i].ShmDes, LinesInfo[i].ParTotal);

	/* fork to run line daemon */
	if ((pid = fork()) > 0)
		/* parent */
		LinesInfo[i].Linedmn = pid;
	else if (pid < 0) {
		/* parent, error */
		sz_log(0, "parcook: cannot fork for daemon %d, exiting", i+1);
		exit(1);
	} else {
		/* child */
		/* print line number */
		/* check for daemon executable file */
		s = stat(SC::S2A(LinesInfo[i].daemon).c_str(), &sstat);
		if ((s != 0) || ((sstat.st_mode & S_IFREG) == 0)) {
			sz_log(0, "parcook: cannot stat regular file '%ls' (daemon for line %d), exiting",
					LinesInfo[i].daemon.c_str(), i+1);
			exit(1);
		}

		std::wstringstream cmd;
		/* prepare options */
		cmd << LinesInfo[i].daemon << L" " << (i + 1) << L" " << LinesInfo[i].device; 
		if (!LinesInfo[i].options.empty())
			cmd << L" " << LinesInfo[i].options;

		/* run daemon */
		sz_log(10, "Executing '%ls'", cmd.str().c_str());
		execl("/bin/sh", "/bin/sh", "-c", SC::S2A(cmd.str()).c_str(), NULL);
		/* shouldn't get here */
		sz_log(0, "parcook: could not execute '%ls' (daemon for line %d), errno %d",
				LinesInfo[i].daemon.c_str(), i+1, errno);
		exit(1);
	} /* fork */
}

void ParseFormulas(TSzarpConfig *ipk)
{
	unsigned int dmin = std::numeric_limits<unsigned int>::max();

	TParam* defined = ipk->GetFirstDefined();
	if (defined) 
		dmin = defined->GetIpcInd();

	sz_log(10, "Parsing formulas");
	Equations.len = 0;
	for (TParam * p = ipk->GetFirstParam(); p; p = ipk->GetNextParam(p)) {
		std::wstring formula = p->GetParcookFormula();
		if (!formula.empty()) {
			std::wstring::size_type idx = formula.rfind(L'#');
			if (idx != std::wstring::npos)
				formula = formula.substr(0, idx);
			Equations.tab.push_back(formula);
			if (p->GetIpcInd() >= dmin)
				VTlen++;
			Equations.len++;
		}
	}
	sz_log(10, "Found %d formulas", Equations.len);
}

/* parse config file */
void ParseCfg(TSzarpConfig *ipk, char *linedmnpat)
{
	unsigned int i;

	VTlen = 0;

	atexit(Rmipc);

	BasePeriod = ipk->GetReadFreq();
	NumberOfLines = ipk->GetDevicesCount();
	sz_log(10, "parcook: number of lines: %d, base period: %d",
			NumberOfLines, BasePeriod);

	LinesInfo.resize(ipk->GetDevicesCount());

	/* create semaphores and message queues */
	CreateSemMsg();

	/* start deamons */
	i = 0;
	for (TDevice *d = ipk->GetFirstDevice(); d != NULL; 
			d = ipk->GetNextDevice(d)) {
		LinesInfo[i].LineNum = d->GetNum()+1;

		LinesInfo[i].ParTotal = d->GetParamsCount();
		LinesInfo[i].daemon =  d->GetDaemon();

		LinesInfo[i].device = d->GetPath();

		if (d->GetSpeed() >= 0) {
			std::wstringstream wss; 
			wss << d->GetSpeed() << L" " << d->GetOptions();
			LinesInfo[i].options = wss.str();
		} else {
			std::wstringstream wss; 
			wss << d->GetOptions();
			LinesInfo[i].options = wss.str();
		}
		LanchDaemon(i, linedmnpat);
		i++;
	} /* for each line daemon */

	/* parse formulas */
	ParseFormulas(ipk);
}

#ifndef NO_LUA
bool execute_script(TParam *p, double &result) {
	lua_State *lua = Lua::GetInterpreter();

	lua_rawgeti(lua, LUA_REGISTRYINDEX, p->GetLuaParamReference());

	Lua::fixed.push(true);
	int ret = lua_pcall(lua, 0, 1, 0);
	Lua::fixed.pop();
	if (ret != 0) {
		sz_log(0, "Param(%s) execution error: %s", SC::S2A(p->GetName()).c_str(), lua_tostring(lua, -1));
		return false;
	}

	if (lua_isnil(lua, -1))
		result = nan("");
	else 
		result = lua_tonumber(lua, -1);

	lua_pop(lua, -1);

	return true;
}

void execute_scripts(std::vector<LuaParamInfo*>& param_info) {

	Szbase::GetObject()->NextQuery();

	for (std::vector<LuaParamInfo*>::iterator i = param_info.begin();
			i != param_info.end();
			++i) {
		TParam *p = (*i)->param;

		assert(p->GetLuaParamReference() != LUA_NOREF);

		double result;
		bool ret = execute_script(p, result);

		short val = SZARP_NO_DATA;
		if ((ret == true) && !isnan(result)) {
			int prec = p->GetPrec();
			if (prec < 5) for (int i = prec; i > 0; i--, result*= 10);

			if (std::numeric_limits<short>::max() >= val
				&& val <= std::numeric_limits<short>::min())
				val = (short) result;
			else
				sz_log(1, "Param %ls, value overflow %f, setting no data", p->GetName().c_str(), result);

			sz_log(4, "Setting param %ls, val %hd", p->GetName().c_str(), val); 
		}
		Probe[(*i)->index] = val;
	}

}

void calculate_lua_params(TSzarpConfig *ipk, PH& pv, std::vector<LuaParamInfo*>& param_info) {

	TParam* p = ipk->GetFirstParam();
	for (int i = 0; i < VTlen; ++i) {
		if (p == NULL) {
			sz_log(0, "IPK error, number of params mismatch, exiting!");
			assert(false);
		}

		double val = nan("");
		if (Probe[i] != SZARP_NO_DATA) {
			val = Probe[i];
			if (p->GetPrec() < 5) for (int prec = p->GetPrec(); prec > 0; prec--)
				val /= 10;
		}
		
		pv[p->GetName()] = val;
			
		p = p->GetNext(true);
	}

	execute_scripts(param_info);

	pv.clear();
}
#endif

void MainLoop(TSzarpConfig *ipk, PH& ipc_param_values, std::vector<LuaParamInfo*>& param_info) 
{
	int abuf;	/* time index in probes tables */
	int ii;
	ushort addr;

	sectime = time(NULL);
	curtime = localtime(&sectime);
	
	abuf = curtime->tm_sec / 10;
	
	sz_log(10, "1");
	/* Probes semaphore down */
	Sem[0].sem_num = SEM_PROBE + 1;
	Sem[0].sem_op = 1;
	Sem[1].sem_num = SEM_PROBE;
	Sem[1].sem_op = 0;
	semop(SemDes, Sem, 2);
	/* Attach probes */
	if ((Probe =
	     (short *) shmat(ProbeDes, (void *) 0,
			     0)) == (void *) -1) {
		sz_log(0,
		    "parcook: cannot attach 'probe' segment, errno %d, exiting",
		    errno);
		exit(1);
	}
	sz_log(10, "2");
	for (unsigned i = 0; i < NumberOfLines; i++) {

		/* Line semaphore down */
		Sem[0].sem_num = SEM_LINE + 2 * i + 1;
		Sem[0].sem_op = 0;
		Sem[1].sem_num = SEM_LINE + 2 * i;
		Sem[1].sem_op = 1;
		semop(SemDes, Sem, 2);
		/* Attach line daemon segment */
		if ((LinesInfo[i].ValTab =
		     (short *) shmat(LinesInfo[i].ShmDes, (void *) 0,
				     0)) == (void *) -1) {
			sz_log(0,
			    "parcook: cannot attach segment for line %d, errno %d, exiting",
			    i + 1, errno);
			exit(1);
		}
		/* copy values from line to probes segment */
		for (ii = 0; ii < LinesInfo[i].ParTotal; ii++) {
			addr = LinesInfo[i].ParBase + ii;

			Probe[addr] = LinesInfo[i].ValTab[ii];
		}
		/* line semaphore up */
		Sem[0].sem_num = SEM_LINE + 2 * i;
		Sem[0].sem_op = -1;
		semop(SemDes, Sem, 1);
		/* detach line segment */
		shmdt((void *) LinesInfo[i].ValTab);
	} /* for each line daemon */
	/* process formulas, Calcul modifies only Probes[] table */
	sz_log(10, "3");
	for (ii = 0; ii < Equations.len; ii++) {
		Calcul(ii);
	}

#ifndef NO_LUA
	sz_log(10, "4");
	/** calculate lua params*/
	calculate_lua_params(ipk, ipc_param_values, param_info);
#endif

	/* NOW update probes history */
	sz_log(10, "4");
	for (ii = 0; ii < VTlen; ii++) {
		/* remove previous value */
		if (Probes[ii][abuf] != SZARP_NO_DATA) {
			Sums[ii][0] -= (int) Probes[ii][abuf];
			Cnts[ii][0]--;
		}
		/* copy current */
		Probes[ii][abuf] = Probe[ii];
		if (Probes[ii][abuf] != SZARP_NO_DATA) {
			Sums[ii][0] += (int) Probes[ii][abuf];
			Cnts[ii][0]++;
		}
	}
	/* release probes semaphore */
	Sem[0].sem_num = SEM_PROBE + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	/* detach probes segment */
	shmdt((void *) Probe);

	sz_log(10, "5");
	/* update min segment */
	abuf = curtime->tm_min % 10;
	Sem[0].sem_num = SEM_MINUTE + 1;
	Sem[0].sem_op = 1;
	Sem[1].sem_num = SEM_MINUTE;
	Sem[1].sem_op = 0;
	semop(SemDes, Sem, 2);
	if ((Minute =
	     (short *) shmat(MinuteDes, (void *) 0,
			     0)) == (void *) -1) {
		sz_log(0,
		    "parcook: cannot attach 'min' segment, errno %d, exiting",
		    errno);
		exit(1);
	}
	for (ii = 0; ii < VTlen; ii++) {
		Minute[ii] =
			Cnts[ii][0] ? (short) (Sums[ii][0] /
					       (int) Cnts[ii][0]) :
			SZARP_NO_DATA;
		if (Minutes[ii][abuf] != SZARP_NO_DATA) {
			Sums[ii][1] -= Minutes[ii][abuf];
			Cnts[ii][1]--;
		}
		Minutes[ii][abuf] = Minute[ii];
		if (Minutes[ii][abuf] != SZARP_NO_DATA) {
			Sums[ii][1] += Minute[ii];
			Cnts[ii][1]++;
		}
	}
	Sem[0].sem_num = SEM_MINUTE + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Minute);

	/* update min10 segment */
	sz_log(10, "6");
	abuf = curtime->tm_min / 10;
	min = curtime->tm_min;
	if (last_min != min || first_time) {
		Sem[0].sem_num = SEM_MIN10 + 1;
		Sem[0].sem_op = 1;
		Sem[1].sem_num = SEM_MIN10;
		Sem[1].sem_op = 0;
		semop(SemDes, Sem, 2);
		if ((Min10 =
		     (short *) shmat(Min10Des, (void *) 0,
				     0)) == (void *) -1) {
			sz_log(0,
			    "parcook: cannot attach 'min10' segment, errno %d, exiting",
			    errno);
			exit(1);
		}
		for (ii = 0; ii < VTlen; ii++) {
			Min10[ii] =
				Cnts[ii][1] ? (short) (Sums[ii][1] /
						       (int)
						       Cnts[ii][1]) :
				SZARP_NO_DATA;
			if (Min10s[ii][abuf] != SZARP_NO_DATA) {
				Sums[ii][2] -= Min10s[ii][abuf];
				Cnts[ii][2]--;
			}
			Min10s[ii][abuf] = Min10[ii];
			if (Min10s[ii][abuf] != SZARP_NO_DATA) {
				Sums[ii][2] += Min10[ii];
				Cnts[ii][2]++;
			}
		}
		Sem[0].sem_num = SEM_MIN10 + 1;
		Sem[0].sem_op = -1;
		semop(SemDes, Sem, 1);
		shmdt((void *) Min10);
	}
	last_min = min;
	sz_log(10, "7");

	/* update hour segment */
	sz_log(10, "8");
	abuf = curtime->tm_hour;
	min10 = curtime->tm_min / 10;
	if (last_min10 != min10 || first_time) {
		Sem[0].sem_num = SEM_HOUR + 1;
		Sem[0].sem_op = 1;
		Sem[1].sem_num = SEM_HOUR;
		Sem[1].sem_op = 0;
		semop(SemDes, Sem, 2);
		if ((Hour =
		     (short *) shmat(HourDes, (void *) 0,
				     0)) == (void *) -1) {
			sz_log(0,
			    "parcook: cannot attach 'hour' segment, errno %d, exiting",
			    errno);
			exit(1);
		}
		for (ii = 0; ii < VTlen; ii++) {
			Hour[ii] =
				Cnts[ii][2] ? (short) (Sums[ii][2] /
						       (int)
						       Cnts[ii][2]) :
				SZARP_NO_DATA;
		}
		Sem[0].sem_num = SEM_HOUR + 1;
		Sem[0].sem_op = -1;
		semop(SemDes, Sem, 1);
		shmdt((void *) Hour);
	}
	last_min10 = min10;
	sz_log(10, "7");

	first_time = 0;

	/* sleep until next */
	sectime = time(NULL);
	curtime = localtime(&sectime);
	sz_log(10, "7.5");
	sleep((int) BasePeriod - curtime->tm_sec % (int) BasePeriod);

	sz_log(10, "8");
}

int main(int argc, char *argv[])
{

	int i;
	char *logfile;
	int log_level;

	struct arguments arguments;
	char* linedmnpat;	/**< sciezka do linex.cfg */
	char* config_prefix;

	InitSignals();

	setbuf(stdout, 0);

	/* set initial logging, process liblog and libpar command line
	 * params */
	log_level = loginit_cmdline(2, NULL, &argc, argv);
	/* Check for other copies of program. */
	if ((i = check_for_other (argc, argv))) {
		sz_log(0, "parcook: another copy of program is running, pid %d, exiting", i);
		return 1;
	}

	libpar_read_cmdline(&argc, argv);

	/* parse params */
	arguments.no_daemon = 0;
	argp_parse(&argp, argc, argv, 0, 0, &arguments);

	/* read params from szarp.cfg file */
	libpar_init_with_filename("/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg", 1);

	/* init logging */
	logfile = libpar_getpar("parcook", "log_level", 0);
	if (logfile) {
		log_level = atoi(logfile);
		free(logfile);
	}
	logfile = libpar_getpar("parcook", "log", 0);
	if (logfile == NULL)
		logfile = strdup(PREFIX "/logs/parcook.log");
	if (loginit(log_level, logfile) < 0) {
		sz_log(0, "parcook: cannot open log file '%s', exiting",
		    logfile);
		return 1;
	}
	sz_log(0, "parcook: started");

	/* read some other params */
	parcookpat = libpar_getpar("", "parcook_path", 1);
	linedmnpat = libpar_getpar("", "linex_cfg", 1);
	config_prefix = libpar_getpar("parscriptd", "config_prefix", 1);
	/* end szarp.cfg processing */
	libpar_done();
	

	IPKContainer::Init(SC::A2S(PREFIX), SC::A2S(PREFIX), L"pl", new NullMutex());	
	Szbase::Init(SC::A2S(PREFIX));

	IPKContainer* ic = IPKContainer::GetObject();
	TSzarpConfig* ipk = ic->GetConfig(SC::A2S(config_prefix));
	if (ipk == NULL) {
		sz_log(0, "Unable to load IPK for prefix %s", config_prefix);
		return 1;
	}

	/* go into background */
	if (arguments.no_daemon == 0)
		go_daemon();

	/* load configuration, start daemons */
	ParseCfg(ipk, linedmnpat);

	free(linedmnpat);

	/* create segments */
	sz_log(10, "parcook: VTlen: %u, creating probes segments", VTlen);
	CreateProbesSegments(parcookpat);

	/* clear segments */
	sz_log(10, "Clearing segments");
	ClearShm(ProbeDes, VTlen);
	ClearShm(MinuteDes, VTlen);
	ClearShm(Min10Des, VTlen);

	/* alloc memory for probes */
	sz_log(10, "Allocating memory for probes");
	AllocProbesMemory();

	/* register second cleanup handler */
	atexit(CleanUp);

	/* set semaphores */
	for (i = 0; i < 4; i++)
		Sem[i].sem_flg = 0;
	
	/* get current time */
	sectime = time(NULL);
	curtime = localtime(&sectime);

	sleep(60 - curtime->tm_sec);

	last_min = -1;		/* ostania minuta w ktorej liczona byla
				   srednia */
	last_min10 = -1;	/* ostatnie 10 minut w ktorej liczona byla
				   srednia */
	first_time = 1;

	PH param_values;
	std::vector<LuaParamInfo*> pi;

#ifndef NO_LUA
	lua_State* lua = Lua::GetInterpreter();
	register_lua_functions(lua);


	lua_pushstring(lua, lua_ipc_table_key);
	lua_pushlightuserdata(lua, &param_values);
	lua_settable(lua, LUA_REGISTRYINDEX);

	if (compile_scripts(ipk, pi) == false)
		return 1;

#endif
	sz_log(10, "Going main loop");

	
	/* start processing */
	while (1) 
		MainLoop(ipk, param_values, pi);
	/* not reached */	
	return 0;
}
