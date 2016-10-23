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

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
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
#include <boost/tokenizer.hpp>
#include <tr1/unordered_map>

#include <zmq.hpp>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "szarp.h"
#include "szbase/szbbase.h"
#include "szbase/szbhash.h"

#include "protobuf/paramsvalues.pb.h"

#include "funtable.h"
#include "daemon.h"
#include "liblog.h"
#include "ipcdefines.h"
#include "szarp_config.h"

#include "conversion.h"

using std::tr1::unordered_map;

/***********************************************************************/
/* Arguments handling, see 'info argp' */

const char *argp_program_version = "parcook " VERSION;
const char *argp_program_bug_address = "coders@szarp.org";
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

int ProbeDes, MinuteDes, Min10Des, HourDes, SemDes, AlertDes, ProbeBufDes;
int MsgSetDes, MsgRplyDes;

int ProbeBufSize = 0;

short *Probe;			/* ostatnia probka */

short *ProbeBuf;		/* ostatnia probka */

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

typedef long long tSums[3];

typedef short tProbes[6];

typedef short tMinutes[10];

typedef short tMin10s[6];

tProbes *Probes;		/* bufor probek tworzacych ostatnia minute */

tMinutes *Minutes;		/* bufor ostatnich 10 minut */

tMin10s *Min10s;		/* bufor ostatniej godziny */

tCnts *Cnts;			/* liczniki usrednien */

tSums *Sums;			/* sumy usrednien */

struct tParamInfo {
	TParam * param;
	enum { SINGLE, COMBINED, LSW, MSW } type;
	int param_no;
	int lsw, msw;
	bool send_to_meaner;
};

tParamInfo **ParsInfo;
std::vector<tParamInfo*> CombinedParams;

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
	int ShmDes;		/* deskryptor wspolnej pamieci */
	short *ValTab;		/* wskaznik na wspolna pamiec */
};

typedef struct phLineInfo tLineInfo;

void calculate_average(short *probes, int param_no, int probe_type)
{
	sz_log(7, "Entering calculate average, probe type: %d", probe_type);
	if (ParsInfo[param_no]->type == tParamInfo::SINGLE) {
		if (Cnts[param_no][probe_type])
			probes[param_no] = Sums[param_no][probe_type] / (int) Cnts[param_no][probe_type];
		else
			probes[param_no] = SZARP_NO_DATA;
		return;
	}

	tParamInfo *pi = ParsInfo[param_no];
	if (pi->type != tParamInfo::MSW)
		//we will update both sums when updating msw
		return;

	if (!Cnts[param_no][probe_type]) {
		sz_log(7, "Calculating average for combined both counts equal 0");
		probes[pi->lsw] = SZARP_NO_DATA;
		probes[param_no] = SZARP_NO_DATA;
		return;
	}

	int msw = param_no;
	int lsw = pi->lsw;

	int v = (int)(Sums[msw][probe_type] / Cnts[msw][probe_type]);

	sz_log(7, "Combined sum value %lld, Calculated value %d", Sums[msw][probe_type], v);

	probes[msw] = (unsigned short)(v >> 16);
	probes[lsw] = (unsigned short)(v & 0xFFFF);

	sz_log(7, "Leaving calculate average, msw set to: %hhu, lsw set to: %hhu", probes[msw], probes[lsw]) ;
}

template<class OVT> void update_value(int param_no, int probe_type, short* ivt, OVT ovt, int abuf)
{
	if (ParsInfo[param_no]->type == tParamInfo::SINGLE) {
		if (ovt[param_no][abuf] != SZARP_NO_DATA) {
			Sums[param_no][probe_type] -= (int) ovt[param_no][abuf];
			Cnts[param_no][probe_type]--;
		}

		ovt[param_no][abuf] = ivt[param_no];
		if (ovt[param_no][abuf] != SZARP_NO_DATA) {
			Sums[param_no][probe_type] += (int) ivt[param_no];
			Cnts[param_no][probe_type]++;
		}
		return;
	}
	if (ParsInfo[param_no]->type != tParamInfo::MSW)
		//we will update both sums when updating msw
		return;

	sz_log(7, "Entering update value for combined param %ls, probe type: %d", ParsInfo[param_no]->param->GetName().c_str(), probe_type);
	/*
	We cast everything to unsigned, because we don't really
	care about signs in combined params.
	NOTE: It is possible that casting is unneccessary
	and values would behave as expected anyway, but 
	it is not immidiately obvious (at least for me)
	so I hope I will save a future developer some head scratching
	while hunting for bugs in parcook ;)
	*/

	int lsw = ParsInfo[param_no]->lsw;
	unsigned short * pmsw = (unsigned short *)&ovt[param_no][abuf];
	unsigned short * plsw = (unsigned short *)&ovt[lsw][abuf];

	int prev_val = (int)((*pmsw) << 16) | (*plsw);

	sz_log(8, "probe_type: %d, param_no: %d, msw: %u, lsw: %u, prev_val: %d, sum: %lld",
	       	probe_type, param_no, *pmsw, *plsw, prev_val, Sums[param_no][probe_type]);

	if (ovt[param_no][abuf] != SZARP_NO_DATA) {
		Sums[param_no][probe_type] -= prev_val;
		Cnts[param_no][probe_type]--;
		sz_log(7, "Decreasing sum counts for combined param cause at least one value is data");
	} else {
		sz_log(7, "Not decreasing sum counts for combined param cause both values are no data");
	}

	ovt[param_no][abuf] = ivt[param_no];
	ovt[lsw][abuf] = ivt[lsw];

	int val = (int)(*pmsw << 16) | *plsw;

	if (SZARP_NO_DATA != ovt[param_no][abuf]) {
		Sums[param_no][probe_type] += val;
		Cnts[param_no][probe_type]++;
		sz_log(7, "Increasing vals count: msw %hu, lsw %hu, prev: %u, val: %u, sum: %lld",
		       	*pmsw, *plsw, prev_val, val, Sums[param_no][probe_type]);
	}
	sz_log(7, "Leaving update for combined param");
}

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

ushort DParamsCount;
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
		sz_log(1, "Error compiling param %s: %s\n", SC::S2U(p->GetName()).c_str(), lua_tostring(lua, -1));
		return false;
	}

	ret = lua_pcall(lua, 0, 1, 0);
	if (ret != 0) {
		sz_log(1, "Error compiling param %s: %s\n", SC::S2U(p->GetName()).c_str(), lua_tostring(lua, -1));
		return false;
	}

	ret = lua_pcall(lua, 0, 1, 0);
	if (ret != 0) {
		sz_log(1, "Error compiling param %s: %s\n", SC::S2U(p->GetName()).c_str(), lua_tostring(lua, -1));
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

/** Removes IPC resources. Called on exit. */
void Rmipc()
{
	unsigned char ii;
	int i;

	errno = 0;

	kill(0, SIGTERM);
	for (ii = 0; ii < NumberOfLines; ii++) {
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

	if (ProbeBufSize) {
		i = shmctl(ProbeBufDes, IPC_RMID, NULL);
		sz_log((i < 0 ? 1 : 10),
		    "parcook: removing 'probesbuf' memory segment, shmctl() returned %d errno %d",
		    i, errno);
	}

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

	for (i = 0; i < SEM_LINE; i++)
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
				case L'M' :    /* no-data if stack[sp -2] > stack [sp - 1], otherwise
			       			stack[sp-2]	*/
					if (sp -- < 2) return;
					if (nodata[sp] || nodata[sp-1]) {
						CalculNoData = 1;
						break;
					}
					if (stack[sp-1] > stack[sp]) {
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
	sz_log(2, "parcook: signal %d caught, cleaning up and exiting", sig);
	exit(1);
}

RETSIGTYPE CriticalHandler(int sig)
{
	sz_log(0, "parcook: signal %d caught, exiting, report to authors", sig);
	/* restore default action, which is to abort */
	signal(sig, SIG_DFL);
	raise(sig);
}

RETSIGTYPE ChildDied(int sig)
{
	int stat;
	pid_t pid = waitpid(-1,&stat,WNOHANG);

	if( pid == -1 )
		sz_log(1, "Waiting for process on SIGCHLD: %s", strerror(errno));
	else if( pid == 0 )
		sz_log(0, "No process died after waiting on SIGCHLD: strange");
	else
		sz_log(WIFEXITED(stat) && !WEXITSTATUS(stat) ? 10 : 1,
			"Process %i ended with code %i", pid, WEXITSTATUS(stat));
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

	sa.sa_handler = ChildDied;
	sa.sa_flags = 0;
	ret = sigaction(SIGCHLD, &sa , NULL );
	assert(ret == 0);

	(void)ret;

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
		    "parcook: cannot get semaphore descriptor (%d,%d), errno %d, exiting",
		    key , SEM_LINE + NumberOfLines * 2 , errno);
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
	key = ftok(parcookpat, SHM_PROBES_BUF);
	if (key == -1) {
		sz_log(0, "parcook: ftok(%s, SHM_PROBES_BUF) error, errno %d",
				parcookpat, errno);
		exit(1);
	}
	if (ProbeBufSize)
		if ((ProbeBufDes =
		     shmget(key, (VTlen * ProbeBufSize + SHM_PROBES_BUF_DATA_OFF) * sizeof(short)
			   , IPC_CREAT | 00666))  == -1) {
			sz_log(0,
			    "parcook: cannot get shared memory descriptor for 'probes buf' segment, errno %d, exiting",
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

	if ((ParsInfo = (tParamInfo**) calloc(VTlen, sizeof(tParamInfo*))) == NULL) {
		sz_log(0, "parcook: calloc error for ParsInfo, exiting");
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

bool param_is_sent_to_meaner(TParam* p) {
	TUnit* u = p->GetParentUnit();
	if (!u)
		return true;

	TDevice* d = u->GetDevice();
	if (!d)
		return true;

	return d->isParcookDevice();
}

void configure_pars_infos(TSzarpConfig *ipk) 
{
	int combined_param_no = VTlen;
	for (TParam* p = ipk->GetFirstDrawDefinable(); p; p = p->GetNext(), combined_param_no++) {
		if (p->GetType() != TParam::P_COMBINED)
			continue;

		TParam **p_cache = p->GetFormulaCache();
		unsigned int msw = p_cache[0]->GetIpcInd();
		unsigned int lsw = p_cache[1]->GetIpcInd();

		tParamInfo* msw_pi = (tParamInfo*) malloc(sizeof(tParamInfo));
		msw_pi->param = p_cache[0];
		msw_pi->type = tParamInfo::MSW;
		msw_pi->lsw = lsw;
		msw_pi->msw = msw;
		msw_pi->send_to_meaner = param_is_sent_to_meaner(p_cache[0]);
		ParsInfo[msw] = msw_pi;

		tParamInfo* lsw_pi = (tParamInfo*) malloc(sizeof(tParamInfo));
		lsw_pi->param = p_cache[1];
		lsw_pi->type = tParamInfo::LSW;
		lsw_pi->lsw = lsw;
		lsw_pi->msw = msw;
		lsw_pi->send_to_meaner = param_is_sent_to_meaner(p_cache[1]);
		ParsInfo[lsw] = lsw_pi;

		tParamInfo* combined = (tParamInfo*) malloc(sizeof(tParamInfo));
		combined->param = p;
		combined->type = tParamInfo::COMBINED;
		combined->lsw = lsw;
		combined->msw = msw;
		combined->param_no = combined_param_no;
		combined->send_to_meaner = lsw_pi->send_to_meaner && msw_pi->send_to_meaner;
		
		CombinedParams.push_back(combined);
	}

	for (int i = 0; i < VTlen; i++) {
		if (ParsInfo[i])
			continue;
		tParamInfo* pi = (tParamInfo*) malloc(sizeof(tParamInfo));
		pi->param = ipk->getParamByIPC(i);
		pi->type = tParamInfo::SINGLE;
		pi->send_to_meaner = param_is_sent_to_meaner(pi->param);
		ParsInfo[i] = pi;
	}
}

/**
 * Split options string to tokens and return as array of chars suitable for execv() function.
 * Double quoting and escaping using '\' is preserved.
 * @param path daemon path
 * @param num daemon line number
 * @param device device argument for daemon
 * @param options string with options
 * @return (m)allocated 2-dimensional array of arguments suitable for execv, last element is NULL
 */
char * const * wstring2argvp(std::wstring path, int num, std::wstring device, std::wstring options)
{
	using namespace boost;
	typedef escaped_list_separator<wchar_t, std::char_traits<wchar_t> > wide_sep;
	typedef tokenizer<wide_sep, std::wstring::const_iterator, std::wstring> wide_tokenizer;
	/* ? - I'd like to use only " separator for compatiblity with
	 * libSzarp2 tokenize() function, but it does not work this way... */
	wide_sep esp(L"\\", L" ", L"\"'");
	wide_tokenizer tok(options, esp);
	std::vector<char *> argv_v;
	for (wide_tokenizer::iterator i = tok.begin(); i != tok.end(); i++) {
		argv_v.push_back(strdup(SC::S2A(*i).c_str()));
	}
	char ** ret = (char **) malloc(sizeof(char *) * (argv_v.size() + 4));
	ret[0] = strdup(SC::S2A(path).c_str());
	asprintf(&(ret[1]), "%d", num);
	ret[2] = strdup(SC::S2A(device).c_str());
	int j = 3;
	for (std::vector<char *>::iterator i = argv_v.begin(); i != argv_v.end(); i++, j++) {
		ret[j] = *i;
	}
	ret[j] = NULL;
	return ret;
}


/** start daemon for line i */
void LanchDaemon(int i, char* linedmnpat)
{
	int pid, s;
	struct stat sstat;
	key_t key;

	/* set number of first param */
	LinesInfo[i].ParBase = VTlen;
	/* increase global params count */
	VTlen += LinesInfo[i].ParTotal;
	DParamsCount += LinesInfo[i].ParTotal;
	
	key = ftok(linedmnpat, LinesInfo[i].LineNum);
	if (key == -1) {
		sz_log(0, "parcook: ftok('%s', %d) returned error for line %d, errno %d, exiting",
				linedmnpat, LinesInfo[i].LineNum, i + 1, errno);
		exit(1);
	}
	sz_log(5, "parcook: creating segment: key %08x", key);

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
	if ((pid = fork()) > 0) {
		/* parent, do nothing */
		sz_log(5, "parcook: starting daemon\nIndex: %d\nLineNum: %d\nParTotal: %d\nDaemon: %ls\nDevice: %ls\nOptions: %ls\nPID: %d",
			i, LinesInfo[i].LineNum, LinesInfo[i].ParTotal, LinesInfo[i].daemon.c_str(), LinesInfo[i].device.c_str(), LinesInfo[i].options.c_str(),pid);

		return;
	} else if (pid < 0) {
		/* parent, error */
		sz_log(0, "parcook: cannot fork for daemon %d, exiting (errno %d)", i + 1, errno);
		exit(1);
	} else {
		/* child */
		/* check for daemon executable file */
		s = stat(SC::S2A(LinesInfo[i].daemon).c_str(), &sstat);
		if ((s != 0) || ((sstat.st_mode & S_IFREG) == 0)) {
			sz_log(0, "parcook: cannot stat regular file '%s' (daemon for line %d), exiting",
					SC::S2A(LinesInfo[i].daemon).c_str(), i + 1);
			exit(1);
		}

		execv(SC::S2A(LinesInfo[i].daemon).c_str(), 
				wstring2argvp(
					LinesInfo[i].daemon, i + 1, LinesInfo[i].device, LinesInfo[i].options)
				);
		/* shouldn't get here */
		sz_log(0, "parcook: could not execute '%ls' (daemon for line %d), errno %d (%s)",
				LinesInfo[i].daemon.c_str(), i + 1, errno, strerror(errno));
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
	DParamsCount = 0;

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
		sz_log(1, "Param(%s) execution error: %s", SC::S2A(p->GetName()).c_str(), lua_tostring(lua, -1));
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
		if ((ret == true) && !std::isnan(result)) {
			int prec = p->GetPrec();
			if (prec < 5) for (int i = prec; i > 0; i--, result*= 10);

			if (result > std::numeric_limits<short>::max())  {
				unsigned int ushortmax = (((unsigned int)(std::numeric_limits<short>::max())) + 1) << 1;
				if (result < ushortmax) {
					unsigned short us = result;
					val = *((short*) &us);
				} else {
					sz_log(1, "Param %s, value overflow %f, setting no data", SC::S2U(p->GetName()).c_str(), result);
				}
			} else if (result < std::numeric_limits<short>::min())
				sz_log(1, "Param %s, value underflow %f, setting no data", SC::S2U(p->GetName()).c_str(), result);
			else
				val = (short) result;

			sz_log(4, "Setting param %s, val %hd", SC::S2U(p->GetName()).c_str(), val);
		}
		Probe[(*i)->index] = val;
	}

}

void calculate_lua_params(TSzarpConfig *ipk, PH& pv, std::vector<LuaParamInfo*>& param_info)
{

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

void str_deleter(void* data, void* obj) {
	delete (std::string*)obj;
}

void publish_values(short* Probe, zmq::socket_t& socket) {
	std::string* buffer = new std::string();
	{
		google::protobuf::io::StringOutputStream stream(buffer);
		szarp::ParamsValues param_values;

		time_t now = time(NULL);
		for (int i = 0; i < VTlen; i++) {
			if (!ParsInfo[i]->send_to_meaner)
				continue;
			szarp::ParamValue* param_value = param_values.add_param_values();
			param_value->set_param_no(i);
			param_value->set_time(now);
			int tmp = Probe[i];
			param_value->set_int_value(tmp);
		}

		for (size_t i = 0; i < CombinedParams.size(); i++) {
			szarp::ParamValue* param_value = param_values.add_param_values();
			tParamInfo* pi = CombinedParams[i];	
			param_value->set_param_no(pi->param_no);
			param_value->set_time(now);
			param_value->set_int_value(
				unsigned(Probe[pi->lsw]) << 16
				| unsigned(Probe[pi->msw]));
		}

		param_values.SerializeToZeroCopyStream(&stream);
	}

	zmq::message_t msg((void*)buffer->data(), buffer->size(), str_deleter, buffer);
	socket.send(msg);
}

void update_probes_buf(short *Probe, short *ProbeBuf) {
	if (ProbeBufSize == 0)
		return;

	short* current_pos = &ProbeBuf[SHM_PROBES_BUF_POS_INDEX];
	short* count = &ProbeBuf[SHM_PROBES_BUF_CNT_INDEX];

	*count = std::min(*count + 1, ProbeBufSize);

	for (int i = 0; i < VTlen; i++)
		ProbeBuf[SHM_PROBES_BUF_DATA_OFF + i * ProbeBufSize + *current_pos] = Probe[i];
	
	*current_pos = (*current_pos + 1) % ProbeBufSize;
}

void MainLoop(TSzarpConfig *ipk, PH& ipc_param_values, std::vector<LuaParamInfo*>& param_info, zmq::socket_t& zmq_socket) 
{
	int abuf;	/* time index in probes tables */
	int ii;
	ushort addr;

	sectime = time(NULL);
	curtime = localtime(&sectime);
	
	abuf = curtime->tm_sec / 10;
	
	/* Probes semaphore down */
	Sem[0].sem_num = SEM_PROBE + 1;
	Sem[0].sem_op = 1;
	Sem[1].sem_num = SEM_PROBE;
	Sem[1].sem_op = 0;

	if (ProbeBufSize) {
		Sem[2].sem_num = SEM_PROBES_BUF + 1;
		Sem[2].sem_op = 1;
		Sem[3].sem_num = SEM_PROBES_BUF;
		Sem[3].sem_op = 0;
		semop(SemDes, Sem, 4);

		if ((ProbeBuf =
		     (short *) shmat(ProbeBufDes , (void *) 0,
				     0)) == (void *) -1) {
			sz_log(0,
			    "parcook: cannot attach 'probes buf' segment, errno %d, exiting",
			    errno);
			exit(1);
		}
	} else {
		semop(SemDes, Sem, 2);
	}
	/* Attach probes */
	if ((Probe =
	     (short *) shmat(ProbeDes, (void *) 0,
			     0)) == (void *) -1) {
		sz_log(0,
		    "parcook: cannot attach 'probe' segment, errno %d, exiting",
		    errno);
		exit(1);
	}
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
	for (ii = DParamsCount; ii < VTlen; ii++) {
		Probe[ii] = SZARP_NO_DATA;
	}
	/* process formulas, Calcul modifies only Probes[] table */
	for (ii = 0; ii < Equations.len; ii++) {
		Calcul(ii);
	}

#ifndef NO_LUA
	sz_log(10, "4");
	/** calculate lua params*/
	calculate_lua_params(ipk, ipc_param_values, param_info);
#endif

	/* NOW update probes history */
	for (ii = 0; ii < VTlen; ii++)
		update_value(ii, 0, Probe, Probes, abuf);

	sz_log(10, "publishing new values");
	publish_values(Probe, zmq_socket);

	update_probes_buf(Probe, ProbeBuf);

	/* release probes semaphore */
	Sem[0].sem_num = SEM_PROBE + 1;
	Sem[0].sem_op = -1;
	if (ProbeBufSize) {
		Sem[1].sem_num = SEM_PROBES_BUF + 1;
		Sem[1].sem_op = -1;
		semop(SemDes, Sem, 2);

		/* detach probes buf segment */
		shmdt((void *) ProbeBuf);
	} else {
		semop(SemDes, Sem, 1);
	}
	/* detach probes segment */
	shmdt((void *) Probe);

	/* update min segment */
	abuf = curtime->tm_min % 10;
	Sem[0].sem_num = SEM_MINUTE + 1;
	Sem[0].sem_op = 1;
	Sem[1].sem_num = SEM_MINUTE;
	Sem[1].sem_op = 0;
	semop(SemDes, Sem, 2);
	if ((Minute =
	     (short *) shmat(MinuteDes, (void *) 0, 0)) == (void *) -1) {
		sz_log(0,
		    "parcook: cannot attach 'min' segment, errno %d, exiting",
		    errno);
		exit(1);
	}
	for (ii = 0; ii < VTlen; ii++)
		calculate_average(Minute, ii, 0);
	for (ii = 0; ii < VTlen; ii++)
		update_value(ii, 1, Minute, Minutes, abuf);
	Sem[0].sem_num = SEM_MINUTE + 1;
	Sem[0].sem_op = -1;
	semop(SemDes, Sem, 1);
	shmdt((void *) Minute);

	/* update min10 segment */
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
		for (ii = 0; ii < VTlen; ii++)
			calculate_average(Min10, ii, 1);
		for (ii = 0; ii < VTlen; ii++)
			update_value(ii, 2, Min10, Min10s, abuf);
		Sem[0].sem_num = SEM_MIN10 + 1;
		Sem[0].sem_op = -1;
		semop(SemDes, Sem, 1);
		shmdt((void *) Min10);
	}
	last_min = min;

	/* update hour segment */
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
		for (ii = 0; ii < VTlen; ii++) 
			calculate_average(Hour, ii, 2);
		Sem[0].sem_num = SEM_HOUR + 1;
		Sem[0].sem_op = -1;
		semop(SemDes, Sem, 1);
		shmdt((void *) Hour);
	}
	last_min10 = min10;

	first_time = 0;

	/* sleep until next */
	sectime = time(NULL);
	curtime = localtime(&sectime);
	sleep((int) BasePeriod - curtime->tm_sec % (int) BasePeriod);
}

int main(int argc, char *argv[])
{

	int i;
	char *logfile;
	int log_level;

	struct arguments arguments;
	char* linedmnpat;	/**< path for ftok */
	char* config_prefix;
	char* probes_buffer_size;
	std::string parhub_address;

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
		logfile = strdup("parcook");
	if (sz_loginit(log_level, logfile) < 0) {
		sz_log(0, "parcook: cannot open log file '%s', exiting",
		    logfile);
		return 1;
	}
	sz_log(0, "parcook: started");

	/* read some other params */
	parcookpat = libpar_getpar("", "parcook_path", 1);
	linedmnpat = libpar_getpar("", "linex_cfg", 1);
	config_prefix = libpar_getpar("parscriptd", "config_prefix", 1);
	parhub_address = libpar_getpar("parhub", "sub_conn_addr", 1);

	probes_buffer_size = libpar_getpar("", "probes_buffer_size", 0);
	if (probes_buffer_size) {
		ProbeBufSize = atoi(probes_buffer_size);
		free(probes_buffer_size);
	}

	/* end szarp.cfg processing */
	libpar_done();
	
	IPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"pl");
	Szbase::Init(SC::L2S(PREFIX));

	IPKContainer* ic = IPKContainer::GetObject();
	TSzarpConfig* ipk = ic->GetConfig(SC::L2S(config_prefix));
	if (ipk == NULL) {
		sz_log(0, "Unable to load IPK for prefix %s", config_prefix);
		return 1;
	}
	ipk->PrepareDrawDefinable();

	/* go into background */
	if (arguments.no_daemon == 0) {
		go_daemon();
	}

	/* load configuration, start daemons */
	ParseCfg(ipk, linedmnpat);

	atexit(Rmipc);

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
	configure_pars_infos(ipk);

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

	zmq::context_t zmq_context(1);
	zmq::socket_t socket(zmq_context, ZMQ_PUB);

	uint64_t hwm = 10; //max 100 sec worth of buffered msgs
	socket.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));

	sz_log(7, "ZMQ connect to '%s'", parhub_address.c_str());
	try {
		socket.connect(parhub_address.c_str());
	} catch (const zmq::error_t& exception) {
		sz_log(1, "ZMQ socket connect failed: %d:'%s' on uri: '%s'", exception.num(),
			exception.what(), parhub_address.c_str());
		throw;
	}
	
	/* start processing */
	while (1) 
		MainLoop(ipk, param_values, pi, socket);
	/* not reached */	
	return 0;
}
