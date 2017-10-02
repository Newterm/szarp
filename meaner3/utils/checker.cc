/* 
  SZARP: SCADA software 
  Copyright (C) 1994-2007  PRATERM S.A.

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
 * $Id$
 * 2007 Praterm S.A.
 * Pawe³ Kolega <pkolega@praterm.com.pl>
 * 
 * Checker [--start-date <data>|-s <data> && --stop-date <data>|-t <data>]|[--period <data>|-p <data>] [--verbose|-v] [--param <data>|-r <data>] [--log|-l] [--apath|-l] [--nostdout|-n]\n");
 * Tool to advanced analysis of technological data\n");
 * Usage:\n");
 * --start-date, -s <data> - begin of period to check given in seconds from 1970\n");
 * --stop-date, -t <data> - end of period to check given in seconds from 1970\n");
 * --period, -p <data> - period from now() to now() - period to check given in seconds from 1970. This option is default (14400s).\n");
 * --verbose, -v - verbose mode.\n");
 * --param, -r <data> - checks only given parameter eg, 'Sieæ:Sterownik:Praca automatyczna'.\n");
 * --log, -l - creating log in /opt/szarp/<perfix>/checker.log\n");
 * --apath, -a <data> - using alternative path to /opt/szarp/<perfix>/checker.log\n");
 * --nostdout, -n - don't output any message in stdout (console) even in verbose mode\n");
 *
 * Configuration in params.xml:
 * <checker:rules>
 * <checker:rule checker:name="rule name 1">
 * <![CDATA[
 * function main()
 * return ...;
 * end
 * ]]>
 * </checker:rule>
 * ....
 * <checker:rule checker:name="rule name n">
 * <![CDATA[
 * function main()
 * return ...;
 * end
 * ]]>
 * </checker:rule>
 * <checker:rules>
 * </params>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef NO_LUA

#include "liblog.h"
#include "libpar.h"

#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "conversion.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

#include <getopt.h>
#include <time.h>

#include <vector>
using namespace std;
using std::vector;

#include "szdefines.h"

#define S_OK 1
#define S_FAIL 0
#define S_NO_DATA -1

#include <lua.hpp>
#define DEFAULT_PERIOD 14400 //4hours
#define DEFAULT_QUANTUM 600 //[s]
#define MAXIMUM_PSIZE  1440 //10 days

#define MAX_PERIOD  3600 * 24 //Max allowed period 24h
time_t StartDate=0;
time_t StopDate=0;

#define SZARP_CFG		"/etc/szarp/szarp.cfg"

#define IPKCHECKER_NAMESPACE_STRING "http://www.praterm.com.pl/SZARP/ipk-checker"

/**Class keeps All rules (code in LUA and name of rules) from params.xml*/
class TRule{
		public:
	/**Constructor
	 */ 
		TRule(){};
		char *RuleName;  /**<Name of rule*/
		char *LUACode;   /**<All LUA code from one rule*/
};

vector<TRule *>Rules; 
/**Class reads params from command line and configuration/rules from params.xml*/
	class ProgramConfig{
		public:
			ProgramConfig();
			/**
			 * Checks unit configuration for extra:speed attribute.
			 * @param argc from main
			 * @param argv from main
			 * @return if error return 0; othervise return 1
			 */
			int ParseCommandLine(int argc, char **argv);
			/**
			 * Returns start date (begin of period) from command line
			 * @return date
			 */
			time_t GetStartDate(){return m_StartDate;};
			/**
			 * Returns if parametr Start date presents in command line
			 * @return return 1 if presents; 0 othervise
			 */
			char IsStartDate(){return f_m_StartDate;};
			/**
			 * Returns stop date (end of period) from command line
			 * @return date
			 */
			time_t GetStopDate(){return m_StopDate;};
			/**
			 * Returns if parametr Start date presents in command line
			 * @return return 1 if presents; 0 othervise
			 */
			char IsStopDate(){return f_m_StopDate;};
			/**
			 * Returns period (between start and stop date) this value is always (from command line or Stop date - Start date)
			 * @return period
			 */
			time_t GetPeriod(){return m_Period;};
			/**
			 * Returns if parametr period presents in command line
			 * @return return 1 if presents; 0 othervise
			 */
			char IsPeriod(){return f_m_Period;};
			/**
			 * Returns if parametr period presents in command line
			 * @return return 1 if presents; 0 othervise
			 */
			char IsVerbose(){return f_m_Verbose;};
			/**
			 * Returns if parametr help presents in command line
			 * @return return 1 if presents; 0 othervise
			 */
			char IsHelp(){return f_m_Help;};
			/**
			 * Returns if parametr param presents in command line
			 * @return return 1 if presents; 0 othervise
			 */
			char IsParam(){return f_m_Param;};
			/**
			 * Returns param name from command line (if presents)
			 * @return param
			 */
			char *GetParam(){if (f_m_Param) return strdup(m_Param); else return NULL;};
			/**
			 * Returns path to logfile from command line (if presents)
			 * @return path
			 */
			char *PutlPath(){return strdup(lpath);};
			/**
			 * Returns if parametr nostdout presents in command line
			 * @return return 1 if presents; 0 othervise
			 */
			char IsNoStdout(){return f_m_NoStdout;};
			/**
			 * Returns if parametr log presents in command line
			 * @return return 1 if presents; 0 othervise
			 */
			char IsLog(){return (f_m_Log | f_m_Apath);};
			/**
			 * Returns path to params.xml
			 * @return path
			 */
			char *PutPath();
			/**
			 * Returns /opt/szarp/<prefix>/szbase directory
			 * @return path
			 */
			char *PutDir();
			/**
			 * loads params.xml
			 * @return 1 - success; othervise error
			 */
			int loadXML();
			/**
			 * parses params.xml
			 * @param doc - document xml
			 * @return 1 - success; othervise error
			 */
			int parseXML(xmlDocPtr doc);
			/**
			 * parses section rules in params.xml
			 * @param doc - document xml
			 * @param node2 - node that's points to checker:rules
			 * @return 1 - success; othervise error
			 */
			int parseXMLRules(xmlDocPtr xml, xmlNodePtr node2);
			char *path; /**<Path to params.xml*/
			char *dir; /**<Path to /opt/szarp/<prefix>/szbase */ 
			char *lpath; /**<Path to log */
			/**
			 * returns date in format YYYY-MM-DD HH:MM
			 * @param data - data in time_t format
			 * @return string in format YYYY-MM-DD HH:MM
			 */
			char *ReturnDate(time_t data); 
			/**
			 * deletes all structures allocated in constructor
			 * it is used eg. by destructor. It may be used as standalone
			 */
			void DeleteRules();		
			virtual ~ProgramConfig(){ if (dir) free (dir); if (path) free(path); if (m_Param) free(m_Param); if (lpath) free(lpath); DeleteRules();};
			private:
			/**
			 * Displays usage ()
			 */
			virtual void Usage();
			/**
			 * Displays error message ()
			 */
			virtual void Message1();
			/**
			 * Displays error message ()
			 */
			virtual void Message2();
			/**
			 * Displays error message ()
			 */
			virtual void Message3();
			/**
			 * Displays error message ()
			 */
			virtual void Message4();
			/**
			 * Displays error message ()
			 */
			virtual void Message5();
			time_t m_StartDate; /**<start date in time_t format*/
			char f_m_StartDate;/**<start date flag (presents)*/ 
			time_t m_StopDate;/**<stop date in time_T format*/
			char f_m_StopDate;/**<stop date flag (presents)*/
			time_t m_Period;/**<period in time_t format*/
			char f_m_Period;/**<period flag (presents)*/
			char f_m_Verbose;/**< verbose flag (presents)*/
			char f_m_Help;/**< help flag (presents)*/
			char *m_Param;/**< param name*/
			char f_m_Param;/**< param flag (presents)*/
			char f_m_Apath;/**< additional path (presents)*/
			char f_m_Log;/**< log (presents)*/ 
			char f_m_NoStdout;/**<*/
			//Auxilary variables
			char m_todelete;/**< flag (lock) 1 - if alocated structores; 0 - if not allocated (needed by DeleteRules)*/

	};
int ProgramConfig::loadXML(){
    xmlDocPtr doc;
    int ret;
    m_todelete = 0;
    xmlLineNumbersDefault(1);
   
   //doc = xmlParseFile(path);
   doc = xmlParseFile(path);
   
    if (doc == NULL) {
sz_log(1, "XML document not wellformed\n");
	return 0;
    }
    ret = parseXML(doc);
    xmlFreeDoc(doc);

    return ret;
}

int ProgramConfig::parseXML(xmlDocPtr doc){
	xmlNodePtr node;
	int status=0;
	for (node = doc->children; node; node = node->next) {
  		if (node->ns==NULL)
			continue;
   		if (strcmp((char *) node->ns->href, (char *) SC::S2U(IPK_NAMESPACE_STRING).c_str()) !=0)
			continue;

		 if (strcmp((char *) node->name, "params") !=0)
			continue;
	
			status = parseXMLRules(doc, node) ; 

}
return status;
}

int ProgramConfig::parseXMLRules(xmlDocPtr doc, xmlNodePtr node2){
       xmlNodePtr node; 
       xmlNodePtr nodeRule;
       int status = 0;
	char *str;
	for (node = node2->children; node; node = node->next) {

		if (node->ns==NULL)
	       		continue;
	       
	       if (strcmp((char *) node->ns->href, IPKCHECKER_NAMESPACE_STRING) !=0)
			continue;

		if (strcmp((char *) node->name, "rules") !=0)
			continue;
		for (nodeRule = node->children; nodeRule; nodeRule = nodeRule->next) {
	        if (nodeRule->ns==NULL)
	       		continue;
			xmlChar* _str = xmlGetNsProp(nodeRule,
				    BAD_CAST("name"),
				    BAD_CAST(IPKCHECKER_NAMESPACE_STRING));


			if (_str==NULL){
				sz_log(0, "rule name not found in IPK!");
				return 0;
			}

			str = strdup(SC::U2A(_str).c_str());
			xmlFree(_str);


			TRule *R = new TRule();

			R->RuleName = strdup(str);
			xmlChar *ziomal =xmlNodeListGetString(doc, nodeRule->children, 1);
			R->LUACode = (char *) xmlStrdup(ziomal);
			xmlFree(ziomal);
			free(str);
			Rules.push_back(R);
			if (!m_todelete) m_todelete=1;
			status=1;		
		}

}
	return status;
}

char *ProgramConfig::PutPath(){
return (strdup(path));
}
char *ProgramConfig::PutDir(){
return (strdup(dir));
}


ProgramConfig::ProgramConfig(){
 m_StartDate=0;
 f_m_StartDate=0;
 m_StopDate=0;
 f_m_StopDate=0;
 m_Period=DEFAULT_PERIOD; 
 f_m_Period=0;
 f_m_Verbose=0;
 f_m_Help=0;
 f_m_Param=0;
 f_m_Apath=0;
 lpath = NULL;
 f_m_Log=0;
 f_m_NoStdout=0;
 m_Param = NULL;
}

void ProgramConfig::Message1 (){
		printf("Options --start-date,-s and --stop-date.-t ,must be in pair\n"); 
}

void ProgramConfig::Message2 (){
		printf("Options --start-date,-s and --stop-date.-t and --period,-p are exclusive\n"); 
}

void ProgramConfig::Message3 (){
		printf(" --start-date,-s <START_EPOCH> and --stop-date,-t <STOP_EPOCH>. START_EPOCH must be less than STOP_EPOCH\n"); 
}

void ProgramConfig::Message4 (){
		printf("Options --log, -l and --apath, a are exclusive\n"); 
}

void ProgramConfig::Message5 (){
		printf("Period between start date and stop date mus be less than 24h\n"); 
}

void ProgramConfig::Usage (){
	printf("Checker [--start-date <data>|-s <data> && --stop-date <data>|-t <data>]|[--period <data>|-p <data>] [--verbose|-v] [--param <data>|-r <data>] [--log|-l] [--apath|-l] [--nostdout|-n]\n");
	printf("Tool to advaned analysis technological data\n");
	printf("Usage:\n");
	printf(" --start-date, -s <data> - begin of period to check given in seconds from 1970\n");
	printf(" --stop-date, -t <data> - end of period to check given in seconds from 1970\n");
	printf(" --period, -p <data> - period from now() to now() - period to check given in seconds from 1970. This option is default (14400s).\n");
	printf(" --verbose, -v - verbose mode.\n");
	printf(" --param, -r <data> - checks only given parameter eg, 'Sieæ:Sterownik:Praca automatyczna'.\n");
	printf(" --log, -l - creating log in /opt/szarp/<perfix>/checker.log\n");
	printf(" --apath, -a <data> - using alternative path to /opt/szarp/<perfix>/checker.log\n");
	printf(" --nostdout, -n - don't output any message in stdout (console) even in verbose mode\n");
}

int ProgramConfig::ParseCommandLine (int argc, char **argv){
	int c;
	int i;
	 loginit_cmdline(2, NULL, &argc, argv);
        log_info(0);
        libpar_read_cmdline(&argc, argv);
        libpar_init();

	path = libpar_getpar("", "IPK", 0);
	if (path == NULL) {
		sz_log(0, "'IPK' not found in szarp.cfg");
		return 0;
	}
	dir = libpar_getpar("", "datadir", 0);
	if (dir == NULL) {
		sz_log(0, "'datadir' not found in szarp.cfg");
		return 0;
	}

	int option_index = 0;


	 static struct option long_options[] = {
                   {"start-date", 1, 0, 's'}, //--start-date -s
                   {"stop-date", 1, 0, 't'},//--stop-date -t
                   {"period", 1, 0, 'p'},//--period -p
                   {"help", 0, 0, 'h'},//--help -h
                   {"param", 1, 0, 'r'},//--param -r
                   {"log", 0, 0, 'l'},//--log
		   {"apath", 1, 0, 'a'},//--apath -a
                   {"nostdout", 0, 0, 'n'},//--stdout -o
		   {"verbose", 0, 0, 'v'},//--verbose -v
		   {0, 0, 0, 0},
         };
	//First some cheats;
	for (i=1;i<argc;i++){	
		if ((argv[i][0]=='-')&&
			(argv[i][1]=='D')&&	
			(argv[i][2]=='p')&&
			(argv[i][3]=='r')&&
			(argv[i][4]=='e')&&
			(argv[i][5]=='f')&&
			(argv[i][6]=='i')&&
			(argv[i][7]=='x')&&
			(argv[i][8]=='=')){
//			for (j=0;j<(int)strlen(argv[i]);j++){
//				argv[i][j]=0x20;	
			//}
		}


	}

	 while ((c = getopt_long (argc, argv, "s:t:p:vha:ln",long_options,&option_index)) != -1){
	 	switch (c){
			case 's':
				f_m_StartDate=1;
				m_StartDate=atoi(optarg);
			break;
			case 't':
				f_m_StopDate=1;
				m_StopDate=atoi(optarg);
			break;
			case 'p':
				f_m_Period=1;
				m_Period=atoi(optarg);
			break;
			case 'h':
				f_m_Help=1;
			break;
			case 'v':
				f_m_Verbose=1;
			break;
			case 'r':
				f_m_Param=1;
				m_Param = strdup(optarg);
			break;
			case 'a':
				f_m_Apath=1;
				lpath = strdup(optarg);
			break;
			case 'l':
				f_m_Log=1;
			break;
			case 'n':
				f_m_NoStdout=1;
			break;
		}
	 }

	 f_m_Verbose &= !f_m_NoStdout; 

	 if (f_m_Help){
	 	Usage();
		return 0;
	 }

	 if ((f_m_StartDate && !f_m_StopDate)|| (!f_m_StartDate && f_m_StopDate)){
	 	Message1();
		return 0;
	 }
	if ((f_m_StartDate && f_m_StopDate && f_m_Period)){
	 	Message2();
		return 0;
	 }

	if ((f_m_StartDate && f_m_StopDate && ((m_StopDate - m_StartDate) < 0) )){
	 	Message3();
		return 0;
	 }

	if (f_m_Log && f_m_Apath){
		Message4();
		return 0;
	}


	if (!f_m_StartDate && !f_m_StopDate){
		f_m_Period=1;
	}

	if (f_m_Period==1){
		 m_StopDate = time (NULL);	
		 m_StartDate = m_StopDate - m_Period; 
	}
	
	if (m_StopDate - m_StartDate > MAX_PERIOD){
		Message5();
		return 0;
	}


	if (f_m_Log){
		char buf[100];
		char *prefix = libpar_getpar("", "config_prefix", 1);
		memset(buf,0,sizeof(buf));
		strcat(buf,PREFIX);
		strcat(buf,"/");
		strcat(buf,prefix);
		free (prefix);
		strcat(buf,"/checker.log");
		lpath = strdup(buf);
	}

	return 1;
}

char *ProgramConfig::ReturnDate(time_t data){
	struct tm *tm; 
	char *buf=(char *)malloc(sizeof(char)*17);
	memset(buf,0,17);
	tm = localtime(&data) ; //YYYY-XX-xx xx:xx
	sprintf(buf,"%04d-%02d-%02d %02d:%02d",\
	(int)tm->tm_year+1900,\
	(int)tm->tm_mon+1,\
	(int)tm->tm_mday,\
	(int)tm->tm_hour,\
	(int)tm->tm_min\
	);
	return buf;
}

void ProgramConfig::DeleteRules(){
	int i;
	if (!m_todelete) return;
	for (i=(int)Rules.size();i>0;i--){
	if (Rules[i-1]->LUACode!=NULL)
		free (Rules[i-1]->LUACode);
	if (Rules[i-1]->RuleName!=NULL)	
		free (Rules[i-1]->RuleName);
	if (Rules[i-1]!=NULL)
		delete Rules[i-1];
	}
	m_todelete=0;
}
/**This class contains base procedures to intialize LUA engine*/
class Clua{
	public:
		/**
		 * default constructor
		 */
		Clua();
		/**
		 * default destructor
		 */
		~Clua();
		/**
		 * loads LUA code from *.lua file to execute (here to debug)
		 * @param FileName - path to file name
		 */
		void RunFromFile( const char *FileName);
		/**
		 * loads LUA code from stream/string
		 * @param Stream - text stream with LUA code to execute
		 */
		 void RunFromStream( const char *Stream);
		lua_State *m_lua ;/**< handle to LUA code */
};

Clua::Clua(){
   m_lua =  lua_open();
   luaL_openlibs(m_lua);
  /*don't use in llua*/
  ////lua_baselibopen(m_lua);
  //  luaopen_table(m_lua);
  //  luaopen_io(m_lua);
  //  luaopen_string(m_lua);
  //  luaopen_math(m_lua);
}

Clua::~Clua(){
    if ( m_lua != NULL )
    {
        lua_close( m_lua );
	m_lua = NULL;
    
    }
}

void Clua::RunFromFile( const char *FileName )
{
    luaL_dofile( m_lua, FileName);
}

void Clua::RunFromStream( const char *FileName )
{
    luaL_dostring( m_lua, FileName);
}

/**This class defines different methods to test data*/
class Cfunction {
/* -1 NO_DATA; 0 - false; 1 - true */
	public:
	 /**
          * Default constructor
          * @param _Probes array of probes to check
          * @param _Hpr - amount of probes
  	  */
 	 Cfunction(int *_Probes, int _Hpr);
	 /**
          * function checks data (in _Probes) wasn't const
	  * @return -1 NO_DATA; 0 - false; 1 - true 
  	  */
	int CheckNoConst();
	 /**
          * function checks data (in _Probes) wasn't const with hysteresis 
	  * @param Hyst - hysteresis (latitude)
	  * @return -1 NO_DATA; 0 - false; 1 - true 
  	  */

	int CheckNoConstHyst(int Hyst);
	 /**
          * function returns value of given probe 
	  * @param WhichProbe - index to array _Probes (Index is from 0). If index exceeds size of _Probes array function return SZARP_NO_DATA
	  * @return value of given probe. 
  	  */
	int GetProbes (int WhichProbe);
	 /**
          * function checks how values are variable 
	  * @param Hyst - hysteresis (latitude)
	  * @return -1 NO_DATA; 0 - false; 1 - true 
	  */
	int CheckNoSaw (int Hyst);
	 /**
          * function checks if bit is set 
	  * @param bit - which bit (0=LSB)
	  * @return -1 NO_DATA; 0 - false; 1 - true 
	  */
	int CheckBit(int bit);
	 /**
          * function checks if values(in average) are less tahn given constant
	  * @param value - value
	  * @return -1 NO_DATA; 0 - false; 1 - true 
	  */
	int CheckLessThan(int value);
	 /**
          * function checks if values(in average) are greater tahn given constant
	  * @param value - value
	  * @return -1 NO_DATA; 0 - false; 1 - true 
	  */
	int CheckGreaterThan(int value);
	 /**
          * function checks if all values are diffrent than NO_DATA    
	  * @param value - value
	  * @return -1 NO_DATA; 0 - false; 1 - true 
	  */
	int CheckParamsNO_DATA();
	/**
          * function returns amount of probes 
	  * @return _Hpr (amount of probes) 
	  */
	int HowProbes();
	 /**
          * function returns amount of probes !=SZARP_NO_DATA
	  * @return amount of probes !=SZARP_NO_DATA 
	  */
	int HowProbesNO_DATA();
	 /**
          * function returns minimal value from given probes
	  * @return min value
	  */
	int MinProbe();
	 /**
          * function returns maximal value from given probes
	  * @return max value
	  */
	int MaxProbe();
	 /**
          * function returns average from all given probes
	  * @return average
	  */
	int AvgProbe();
	~Cfunction() {free (Probes);};
	int *Probes; /**<array of probes*/
	int Hpr; /**<amount of probes (all in given period)*/ 
};

Cfunction::Cfunction(int *_Probes, int _Hpr) {
	int i;
	Probes = (int *)malloc(sizeof(int)*_Hpr);
	for (i=0;i<_Hpr;i++){
		Probes[i] = _Probes[i];	
	}
	Hpr = _Hpr;
}

int Cfunction::CheckNoConst(){
	if (!HowProbesNO_DATA())
		return S_FAIL;
	if ((MaxProbe() - MinProbe())!=0)
		return S_OK;
	return S_FAIL;
}

int Cfunction::CheckNoConstHyst(int Hyst){
	if (!HowProbesNO_DATA())
		return S_FAIL;
	if (abs((MaxProbe() - MinProbe()))>Hyst)
		return S_OK;
	return S_FAIL;
}


int Cfunction::GetProbes(int WhichProbe){
	if (WhichProbe< 0) return SZARP_NO_DATA;
	if (WhichProbe + 1 > Hpr) return SZARP_NO_DATA;
	return (Probes[WhichProbe]);
}

int Cfunction::HowProbes(){
	return Hpr;
}

int Cfunction::HowProbesNO_DATA(){
	int h = 0;
	int i;
	for (i=0;i<Hpr;i++){
		if (Probes[i]!=SZARP_NO_DATA){
			h++;
		}
	}
	return h;
}


int Cfunction::MinProbe(){
	int i,j;
	int value = 0;
	int check = 0;
	int latch=0;
	j=0;
	for (i=0;i<Hpr;i++){
		if(Probes[i]!=SZARP_NO_DATA){
			value = Probes[i];
			check = 1;
			j = i ;
			break;
		}
	}
	
	if (!check){
		return SZARP_NO_DATA;	
	}

	for (i=j;i<Hpr;i++){
		if (!latch){
			latch = 1;
			value = Probes[i] ;	
		} else
		if ((value > Probes[i]) && (Probes[i] != SZARP_NO_DATA)){
			value = Probes[i] ;	
		}
	}
	return value;
}

int Cfunction::MaxProbe(){
	int i,j;
	int value = 0;
	int check = 0;
	int latch = 0;
	j=0;
	for (i=0;i<Hpr;i++){
		if(Probes[i]!=SZARP_NO_DATA){
			value = Probes[i];
			check = 1;
			j = i ;
			break;
		}
	}
	if (!check){
		return SZARP_NO_DATA;	
	}

	for (i=j;i<Hpr;i++){
		if (!latch){
			latch = 1;
			value = Probes[i] ;	
		}else
		if ((value < Probes[i]) && (Probes[i] != SZARP_NO_DATA)){
			value = Probes[i] ;	
		}
	}
	return value;
}

int Cfunction::AvgProbe(){
	int i;
	int avg = 0;
	int avgp = 0;
	for (i=0;i<Hpr;i++){
		if (Probes[i]!=SZARP_NO_DATA){
			avg+=Probes[i];
			avgp++;
		}
	}
	if (avgp > 0){
	avg/=avgp;
	}else{
	avg = SZARP_NO_DATA;
	}
	return avg;
}

int Cfunction::CheckNoSaw (int Hyst){
	int i;
	int result = 0;
	int avg = AvgProbe();
	int Hprnd = HowProbesNO_DATA();
	if (avg==SZARP_NO_DATA) return S_NO_DATA;
	if (Hprnd == 0)	 return S_NO_DATA;
	for (i=0;i<Hpr;i++){
		if ((abs(avg - Probes[i]) > Hyst) && Probes[i]!=SZARP_NO_DATA){
			result++;
		}
	}
	if (result > 0)
		return S_FAIL;
	return S_OK;
}

int Cfunction::CheckBit (int bit){
	int i;
	int result = 0;
	int Hprnd = HowProbesNO_DATA();
	if (Hprnd == 0) return S_NO_DATA;
	for (i=0;i<Hpr;i++){
		if (((Probes[i]>>bit) & 0x1 ) && (Probes[i]!=SZARP_NO_DATA)){
			result++;
		}
	}
	if (result > Hprnd/2)
		return S_OK;
	return S_FAIL;
}

int Cfunction::CheckLessThan (int value){
	int avg = AvgProbe();
	if (avg == SZARP_NO_DATA) return S_NO_DATA;
	if (value < avg) return S_OK;	
	return S_FAIL;
}

int Cfunction::CheckGreaterThan (int value){
	int avg = AvgProbe();
	if (avg == SZARP_NO_DATA) return S_NO_DATA;
	if (value > avg) return S_OK;	
	return S_FAIL;
}

int Cfunction::CheckParamsNO_DATA (){
	if (HowProbesNO_DATA()==0) return S_NO_DATA;
	if (HowProbes()!=HowProbesNO_DATA()) return S_NO_DATA;
	else	
		return S_OK;	
	return S_FAIL;
}

/** Class keeps ipk configuration */
class CipkConf{
	public:
		TSzarpConfig *ipk;
		unsigned char m_ipk_initialised ;
		unsigned char m_path_initialised;
		unsigned char m_dir_initialised;
		char *path;	
		char * dir;
		CipkConf() {m_ipk_initialised = m_path_initialised = m_dir_initialised =  0;};
		int IpkInit() ;		
		~CipkConf(){if (m_dir_initialised) free(dir); if (m_path_initialised) free(path); if (m_ipk_initialised) delete(ipk);};
};

int CipkConf::IpkInit(){
	if (m_ipk_initialised>0)
		return S_FAIL;
	ipk = new TSzarpConfig();
	m_ipk_initialised = 1;
	path = libpar_getpar("", "IPK", 0);

	if (path == NULL) {
		sz_log(0, "'IPK' not found in szarp.cfg");
		return S_NO_DATA;
	}else{
		m_path_initialised = 1;
	}
	
	dir = libpar_getpar("", "datadir", 0);
	if (dir == NULL) {
		sz_log(0, "'datadir' not found in szarp.cfg");
		return S_NO_DATA;
	}else{
		m_dir_initialised = 1;
	}
	ipk->loadXML(SC::L2S(path));
	return S_OK;
};		
CipkConf IpkConf;

/**Class deriver from Clua it defines additional functions in LUA and return status from LUA main() function*/
class CluaRegister : public Clua {
	public:
	 /**
	  * default constructor
	  */
	CluaRegister();
	 /**
	   * This function defines function in LUA and wraps Cfunction::CheckNoConst()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int checknoconst(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::CheckNoConstHyst()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int checknoconsthyst(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::GetProbes()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int getprobes(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::CheckNoSaw()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int checknosaw(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::CheckBit()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int checkbit(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::HowProbes()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int howprobes(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::HowProbesNO_DATA()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int howprobesnd(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::MinProbe()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int minprobe(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::MaxProbe()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int maxprobe(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::AvgProbe()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int avgprobe(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::CheckLessThan()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int checklessthan(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::CheckGreaterThan()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int checkgreaterthan(lua_State *L);
	 /**
	   * This function defines function in LUA and wraps Cfunction::CheckParamsNO_DATA()
	   * @param L - state of LUA (required by LUA library)
	  */
	static int checkparamsnodata(lua_State *L);
	 /**
	   * This function returns status of main() function defined in LUA -1/0/1 and execute it.
	   * @return status of main() function -1/0/1
	  */
	int main_lua();
	 /**
	   * This function returns only status of main() function without executing it.
	   * @return status of main() function -1/0/1
	  */
	int main_result(){return m_result;};
	int m_result; /**< keeps status of the main() function*/
	 /**
	   * This function reads data from SZARP database
	   * @param StartDate - begin of period
	   * @param StopDate end of period
	   * @param PName - name of SZARP param in database
	   * @param PData - array of read data
	   * @param PSize - amount of read probes
	   * @return status -1/0/1
	  */
	static int GetDataFromParam(time_t StartDate, time_t StopDate, char *PName, int *PData, int *PSize);
	 /**
	   * This function reads data from SZARP database
	   * @param StartDate - begin of period
	   * @param StopDate end of period
	   * @param PWildCard for many params (currently only works asterix in the end of the name)
	   * @param PData - array of read data
	   * @param PSize - amount of read probes
	   * @return status -1/0/1
	  */
	static int GetDataFromParams(time_t StartDate, time_t StopDate, char *PWildCard, int *PData, int *PSize);
	/**
	   * Wildcard support
	   * @param str - string 
	   * @param wcard - string with wildcard
	   * @return status (same as string.compare method)
	  */
	static int CheckWildCardString(char * str, char * wcard);
	~CluaRegister(){};
	
};
int CluaRegister::GetDataFromParam(time_t StartDate, time_t StopDate, char *PName, int *PData, int *PSize){	
	time_t ActualDate=0;
	
	TParam *p=NULL;
	p = IpkConf.ipk->getParamByName(SC::L2S(PName, true));


	if (p==NULL){
		sz_log(0,"ERROR: Parametr '%s' is not at all", PName);	
	//	delete ipk;
		return S_NO_DATA;
	}

	szb_buffer_t* buf ;
	SZBASE_TYPE data;

	int i;
	i = 0;
	*PSize = 0;
	ActualDate = StartDate;
	
	IPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"");
	Szbase::Init(SC::L2S(PREFIX), NULL);
	buf = szb_create_buffer(Szbase::GetObject(), SC::L2S(IpkConf.dir), 1, IpkConf.ipk);

	while(ActualDate < StopDate){
		data = szb_get_data(buf, p, ActualDate);
		PData[i] = (int)p->ToIPCValue(data); 
		ActualDate = ActualDate + DEFAULT_QUANTUM;
		i++;	
	}
	//
	//	
	szb_free_buffer(buf);
	Szbase::Destroy();
	IPKContainer::Destroy();
	*PSize = i;	
//	free (dir);
//	free (path);
//	delete ipk;
	return S_OK;
}

int CluaRegister::GetDataFromParams(time_t StartDate, time_t StopDate, char *PWildCard, int *PData, int *PSize){
	TSzarpConfig *ipk = new TSzarpConfig();
	int ParamsCtr = 0;
	char *path = libpar_getpar("", "IPK", 0);
	szb_buffer_t* buf ;
	SZBASE_TYPE data;

	if (path == NULL) {
		sz_log(0, "'IPK' not found in szarp.cfg");
		return S_NO_DATA;
	}
	char *dir = libpar_getpar("", "datadir", 0);
	if (dir == NULL) {
		sz_log(0, "'datadir' not found in szarp.cfg");
		return S_NO_DATA;
	}
	ipk->loadXML(SC::L2S(path));


	for (TParam *ppp = ipk->GetFirstParam();ppp;ppp=ppp->GetNextGlobal()){
		
		if (CheckWildCardString((char *)(SC::S2A(ppp->GetName()).c_str()),(char *)PWildCard)==0){	
			IPKContainer::Init(SC::L2S(PREFIX), SC::L2S(PREFIX), L"");
			Szbase::Init(SC::L2S(PREFIX), NULL);
			buf = szb_create_buffer(Szbase::GetObject(), SC::L2S(dir), 1, ipk);
			data = szb_get_avg(buf, ipk->getParamByName(ppp->GetName()), StartDate, StopDate);
			PData[ParamsCtr] = (int)ipk->getParamByName(ppp->GetName())->ToIPCValue(data); 
			szb_free_buffer(buf);
			Szbase::Destroy();
			IPKContainer::Destroy();
			if (ParamsCtr++>=MAXIMUM_PSIZE) ParamsCtr= MAXIMUM_PSIZE;
		}
	}
	*PSize = ParamsCtr;  
	if (ParamsCtr==0){
		sz_log(0,"ERROR: WildCard '%s' couldn't match any parametr", PWildCard);	
		delete ipk;
		return S_NO_DATA;
	}

	free (dir);
	free (path);
	delete ipk;
	return S_OK;
}

/** Function implements very simple wildcarding wit asterisk at the endo f the string */
/** returns 0 - if string passes wildcard */
int CluaRegister::CheckWildCardString(char * str, char * wcard){
	const char WILDCARD = '*';
	const short NO_IDX = -1;
	string sstr(str);
	string swcard(wcard);
	int i=0;
	short idx = NO_IDX; 
	/* look for asterisk and cutting the string */
	for (i = swcard.length(); (i > 0) && (idx == NO_IDX); i--){
		if (swcard[i-1] == WILDCARD){
			idx = (int)i;
		}	
	}
	sstr.resize(i);
	swcard.resize(i);
	return sstr.compare(swcard);
}

int CluaRegister::main_lua(){
	int narg, nres;
	int m_result = 0;
	narg=0; /* nof parametrs of main() function */
	nres=1; /* nof refurns of main() function */
	lua_getglobal( m_lua, "main" );  /* get function */
	luaL_checkstack( m_lua, 0, "too many arguments");
	
	if (lua_pcall( m_lua, narg, nres, 0) != 0){
		printf("Error %s\n", lua_tostring(m_lua, -1));
		exit (0);
	}

	nres=-nres;
	m_result = (int)lua_tonumber (m_lua, nres);
	if (m_result > 1) m_result = 1;
	if (m_result < -1) m_result = -1;
	return m_result;

}

/* checkconst (param_name) */
int CluaRegister::checknoconst(lua_State *L){
int n = lua_gettop(L);
Cfunction *fcn;
int buffer [MAXIMUM_PSIZE];
int Hpr;
int result=0;
if ( n != 1 ){
  	lua_pushstring(L, "One argument is required for 'checknoconst'");
	lua_error(L);
}
result = GetDataFromParam(StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
if (result == S_NO_DATA) exit (0);
if (result !=S_NO_DATA){
	fcn = new Cfunction((int *)buffer, (int)Hpr);
	result = fcn->CheckNoConst();
	delete fcn;
}

lua_pushnumber(L, result);
return 1;
}

/* checkconst (param_name, Hyst) */
int CluaRegister::checknoconsthyst(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 2 ){
        	lua_pushstring(L, "Two arguments are required for 'checknoconsthyst'");
		lua_error(L);
    	}
	result = GetDataFromParam( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->CheckNoConstHyst((int)lua_tonumber(L,2));
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;

}
/* getprobes (param_name, probe) */
int CluaRegister::getprobes(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 2 ){
        	lua_pushstring(L, "One argument is required for 'getprobes'");
		lua_error(L);
    	}
	
	result = GetDataFromParam( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->GetProbes((int)lua_tonumber(L,2));
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}

/* checknosaw (param_name, Hyst) */
int CluaRegister::checknosaw(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 2 ){
      	lua_pushstring(L, "Two arguments are required for 'checknosaw'");
		lua_error(L);
  	}
	result = GetDataFromParam(StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->CheckNoSaw((int)lua_tonumber(L,2));
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}

/* checkbit (param_name, bit) */
int CluaRegister::checkbit(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=-1;
	if ( n != 2 ){
        	lua_pushstring(L, "One argument is required for 'checkbit'");
		lua_error(L);
		return S_NO_DATA;
	}
	
      result = GetDataFromParam(StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);

	if (result == S_NO_DATA) exit (0);
	if (result !=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->CheckBit((int)lua_tonumber(L,2));
		printf("zygu zygu %d %d\n",buffer[0],result);
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}

/* howprobes (param_name) */
int CluaRegister::howprobes(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 1 ){
        	lua_pushstring(L, "One argument is required for 'howprobes'");
		lua_error(L);
    	}
	result = GetDataFromParam( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);	
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->HowProbes();
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}

/* howprobes (param_name) */
int CluaRegister::howprobesnd(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 1 ){
        	lua_pushstring(L, "One argument is required for 'howprobesnd'");
		lua_error(L);
    	}

	result = GetDataFromParam( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->HowProbesNO_DATA();
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}

/* minprobe (param_name) */
int CluaRegister::minprobe(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 1 ){
        	lua_pushstring(L, "One argument is required for 'minprobe'");
		lua_error(L);
    	}
	result = GetDataFromParam( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->MinProbe();
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}

/* maxprobe (param_name) */
int CluaRegister::maxprobe(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 1 ){
        	lua_pushstring(L, "One argument is required for 'maxprobe'");
		lua_error(L);
    	}
	result = GetDataFromParam( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->MaxProbe();
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}

/* avgprobe (param_name) */
int CluaRegister::avgprobe(lua_State *L){
int n = lua_gettop(L);
Cfunction *fcn;
int buffer [MAXIMUM_PSIZE];
int Hpr;
int result=0;
if ( n != 1 ){
  	lua_pushstring(L, "One argument is required for 'avgprobe'");
	lua_error(L);
}
result = GetDataFromParam( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
if (result == S_NO_DATA) exit (0);	
if (result!=S_NO_DATA){
	fcn = new Cfunction((int *)buffer, (int)Hpr);
	result = fcn->AvgProbe();
	delete fcn;
}
lua_pushnumber(L, result);
return 1;
}

/* checklessthan (param_name,value) */
int CluaRegister::checklessthan(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 2 ){
        	lua_pushstring(L, "Two arguments are required for 'checklessthan'");
		lua_error(L);
    	}
	result = GetDataFromParam( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);	
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->CheckLessThan((int)lua_tonumber( L, 2 ));
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}

/* checkgreaterthan (param_name,value) */
int CluaRegister::checkgreaterthan(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 2 ){
        	lua_pushstring(L, "Two arguments are required for 'checkgreaterthan'");
		lua_error(L);
    	}
	result = GetDataFromParam( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);	
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->CheckGreaterThan((int)lua_tonumber( L, 2 ));
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}

/* checkparamsnodata (param_name(s)) */
int CluaRegister::checkparamsnodata(lua_State *L){
	int n = lua_gettop(L);
	Cfunction *fcn;
	int buffer [MAXIMUM_PSIZE];
	int Hpr;
	int result=0;
	if ( n != 1 ){
        	lua_pushstring(L, "One argument is required for 'checkparamsnodata'");
		lua_error(L);
    	}
	result = GetDataFromParams( StartDate, StopDate, (char *)lua_tostring( L, 1 ), buffer, &Hpr);
	if (result == S_NO_DATA) exit (0);	
	if (result!=S_NO_DATA){
		fcn = new Cfunction((int *)buffer, (int)Hpr);
		result = fcn->CheckParamsNO_DATA();
		delete fcn;
	}
	lua_pushnumber(L, result);
	return 1;
}


CluaRegister::CluaRegister() : Clua(){
	/* register all new functions for LUA code */
	lua_register(m_lua, "checknoconst", checknoconst);
	lua_register(m_lua, "checknoconsthyst", checknoconsthyst);
	lua_register(m_lua, "getprobes", getprobes);
	lua_register(m_lua, "checknosaw", checknosaw);
	lua_register(m_lua, "checkbit", checkbit);
	lua_register(m_lua, "howprobes", howprobes);
	lua_register(m_lua, "howprobesnd", howprobesnd);
	lua_register(m_lua, "minprobe", minprobe);
	lua_register(m_lua, "maxprobe", maxprobe);
	lua_register(m_lua, "avgprobe", avgprobe);
	lua_register(m_lua, "checklessthan", checklessthan);
	lua_register(m_lua, "checkgreaterthan", checkgreaterthan);
	lua_register(m_lua, "checkparamsnodata", checkparamsnodata);
}

/** This class writes log (into checker.log) */
class Clog{
	public:
	/**
	 * default constructor
	 * @param _StartDate - begin of period
	 * @param _StopDate - end of period
	 * @param _pth - path to params.xml 
	 * @param _tostdout - writting messages to stdout
	 * @param _tofile - writting messages to file
	 */
	Clog(time_t _StartDate, time_t _StopDate, char *_pth, char _tostdout, char _tofile);
	FILE *f; /**<log file handle*/
	/**
	  * returns date in format YYYY-MM-DD HH:MM
	  * @param data - data in time_t format
	  * @return string in format YYYY-MM-DD HH:MM
	  */
	char *ReturnDate(time_t data) ;
	/**
	  * function adds to log parametrs
	  * @param ParamName - name of param (rule)
	  * @param Status - status of rule (return of main function)
	  */
	void AddToLog(char *ParamName, int Status);
	~Clog(){if (tofile) fclose(f);};
	private:
	size_t StartDate; /**<Begin of period*/
	size_t StopDate; /**<End of period*/
	char tostdout; /**< allows writing to stdout */
	char tofile; /**<allows writing to file */

};
char *Clog::ReturnDate(time_t data){
	struct tm *tm; 
	char *buf=(char *)malloc(sizeof(char)*17);
	memset(buf,0,17);
	tm = localtime(&data) ; //YYYY-XX-xx xx:xx
	sprintf(buf,"%04d-%02d-%02d %02d:%02d",\
	(int)tm->tm_year+1900,\
	(int)tm->tm_mon+1,\
	(int)tm->tm_mday,\
	(int)tm->tm_hour,\
	(int)tm->tm_min\
	);
	return buf;
}

Clog::Clog(time_t _StartDate, time_t _StopDate, char *_pth, char _tostdout, char _tofile){
	StartDate = _StartDate;
	StopDate = _StopDate;
	tostdout = _tostdout;
	tofile = _tofile;
	if (tofile && _pth!=NULL)
		f = fopen (_pth,"wt");
	char *c1 = ReturnDate(StartDate);
	char *c2 = ReturnDate(StopDate);
	if (tofile)	
		fprintf(f, "__%s - %s__\n", c1, c2);
	if (tostdout)
		printf("__%s - %s__\n", c1, c2);
	free (c2);
	free(c1);

}

void Clog::AddToLog(char *ParamName, int Status){
	const char *stats[3]={"BRAK_DANYCH","BLAD","OK"};
	if (Status >1) Status =1;
	if (Status<-1) Status=-1;
	if (tofile)	
		fprintf(f, "Param: %s | Status:%d (%s) \n",ParamName, Status,stats[Status+1]);
	if (tostdout)	
		printf("Param: %s | Status:%d (%s) \n",ParamName, Status,stats[Status+1]);

}

int main(int argc, char **argv)
{

ProgramConfig pConfig;

char *dir;
char *path;
int i;

if (!pConfig.ParseCommandLine(argc,argv)){
	return 1;
}
path = pConfig.PutPath();
dir = pConfig.PutDir();
StartDate = pConfig.GetStartDate();
StopDate = pConfig.GetStopDate();

if (pConfig.IsVerbose()){
	printf("Starting in Verbose mode:\n");
	printf("Path: %s\n", path);
	printf("Dir: %s\n", dir);
	if (pConfig.IsStartDate()){
		char *tmp =pConfig.ReturnDate(pConfig.GetStartDate()); 
		printf("Start date:%s\n", tmp );
		free (tmp);
	}
	
	if (pConfig.IsStopDate()){
		char *tmp =pConfig.ReturnDate(pConfig.GetStopDate()); 
		printf("Stop date:%s\n", tmp );
		free (tmp);
	}
	if (pConfig.IsPeriod()){
		char *tmp1 =pConfig.ReturnDate(pConfig.GetStartDate()); 
		char *tmp2 =pConfig.ReturnDate(pConfig.GetStopDate()); 
		printf("Period: from %s to %s\n", tmp1, tmp2 );
		free (tmp2);
		free (tmp1);
	}
	if (pConfig.IsParam()){
		char *pr = pConfig.GetParam();
		if (pr){
			printf("Given Param: %s\n", pr);
			free (pr);
		}
	}

}
free (dir);
free (path);


IpkConf.IpkInit();
if (!pConfig.loadXML()){
	sz_log(1, "ERROR: Parse XML failed\n");
	return 1;
}

Clog *checker_log;
char *lpth=NULL;
if (pConfig.IsLog())
lpth =pConfig.PutlPath(); 

checker_log = new Clog(StartDate, StopDate, lpth, (int)!pConfig.IsNoStdout(), (int)pConfig.IsLog());

if (pConfig.IsLog())
	free (lpth);
CluaRegister *MyLuaRegister;
int ml=0;
char *prm ;

if (pConfig.IsParam()){
prm = pConfig.GetParam();
}
else
	prm=NULL;

char lth=0;
for (i=0;i<(int)Rules.size();i++){
	if (!pConfig.IsParam()){
		MyLuaRegister = new CluaRegister(); 
		MyLuaRegister->RunFromStream(Rules[i]->LUACode); 
		ml = MyLuaRegister->main_lua();	
		if (pConfig.IsVerbose()){
			printf("NAME:%s\n", Rules[i]->RuleName);
			printf("CODE:\n%s\n", Rules[i]->LUACode);
			printf("STATUS: %d\n ", ml) ;
		}
		checker_log->AddToLog(Rules[i]->RuleName, ml);
		delete MyLuaRegister;
	}else{
		if (!strcmp(Rules[i]->RuleName,prm))lth=1;
		MyLuaRegister = new CluaRegister(); 
		MyLuaRegister->RunFromStream(Rules[i]->LUACode); 
		ml = MyLuaRegister->main_lua();	
		if (pConfig.IsVerbose()){
			printf("NAME:%s\n", Rules[i]->RuleName);
			printf("CODE:\n%s\n", Rules[i]->LUACode);
			printf("STATUS: %d\n ", ml) ;
		}
		checker_log->AddToLog(Rules[i]->RuleName, ml);
		delete MyLuaRegister;
	}
}

if (pConfig.IsParam())
	free(prm);
delete checker_log;

if (pConfig.IsParam() && !lth)
	sz_log(1, "ERROR: Given parametr %s not found\n",prm);

}

#endif

