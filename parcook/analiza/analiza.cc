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
/** Implementacja kodu funkcji wykorzystywanych w programie ANALIZA */

#define WORK

#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
    
#include <string>

#include "liblog.h"
#include "ipctools.h"
#include "libpar.h"
#include "libpar_int.h"

#include "szarp_config.h"
#include "analiza.h"
#include "conversion.h"

#ifdef WORK

/* UWAGA */
//#include <sys/types.h>
//#include <fcntl.h>

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <daemon.h>


#endif
#ifdef DIAGNO

long res ;

#endif
void save_log(const char *fmt,...);

int GetIpcInd(TBoiler* boiler, TAnalysis::AnalysisParam param_type) {

	TParam *param = boiler->GetParam(param_type);

	if (!param) {
		sz_log(0, "No param %ls for boiler: %d", TAnalysis::GetNameForParam(param_type).c_str() ,
				boiler->GetBoilerNo());
		throw std::exception();
	}

	return param->GetIpcInd();
}

/*
 * @return val*10^exp
 **/
int scale(int val, int exp) {
	if (exp < 0) {
		exp = -exp;
		for (int i = 0; i < exp; ++i)
			val /= 10;
	}
	else
		for (int i = 0; i < exp; ++i)
			val *= 10;
	return val;
}
			
/*Czyta konfiguracje z IPK, uruchamia procesy zajmujace sie
 * analiza danych z poszczegolnych kotlow
 * @param path sciezka do programu analiza
 * @param kociol nazwa kotla dla ktorego ma byc wykonywana analiza
 * @param ipk referencja do obiektu ipk
 * @return 0 w przypadku bledu, 1 gdy odczyt konfiguracji zakonczyl sie skucesem*/
int ReadCfg(char *path, const char* kociol, TSzarpConfig* ipk)
{
	int params[NUMBER_OF_PARAMS] ;

#ifdef WORK

	if (kociol != NULL) 
	{ /* czytanie tylko jednego kotla*/
		int boiler_no;
		if (sscanf(kociol, "KOCIOL_%d", &boiler_no) != 1) { 
			return 0;
		}

		TBoiler *boiler = NULL;
		for (boiler = ipk->GetFirstBoiler(); boiler; boiler = boiler->GetNext()) {
			if (boiler->GetBoilerNo() == boiler_no)
				break;
		}

		if (boiler == NULL) {
			sz_log(0, "No boiler of number %d in configuration", boiler_no);
			return 0;
		}

		try {
	 		params[0] = GetIpcInd(boiler, TAnalysis::ANALYSIS_ENABLE);
	 		params[1] = GetIpcInd(boiler, TAnalysis::REGULATOR_WORK_TIME);
	 		params[2] = GetIpcInd(boiler, TAnalysis::COAL_VOLUME);
	 		params[3] = GetIpcInd(boiler, TAnalysis::ENERGY_COAL_VOLUME_RATIO);
	 		params[4] = GetIpcInd(boiler, TAnalysis::AIR_STREAM);
	 		params[5] = GetIpcInd(boiler, TAnalysis::LEFT_GRATE_SPEED);
	 		params[6] = GetIpcInd(boiler, TAnalysis::RIGHT_GRATE_SPEED);
	 		params[7] = GetIpcInd(boiler, TAnalysis::MIN_AIR_STREAM);
	 		params[8] = GetIpcInd(boiler, TAnalysis::MAX_AIR_STREAM);
	 		params[9] = GetIpcInd(boiler, TAnalysis::LEFT_COAL_GATE_HEIGHT);
	 		params[10] = GetIpcInd(boiler, TAnalysis::RIGHT_COAL_GATE_HEIGHT);
	 		params[11] = GetIpcInd(boiler, TAnalysis::AIR_STREAM_RESULT);
	 		params[12] = GetIpcInd(boiler, TAnalysis::ANALYSIS_STATUS);
		}
		catch (std::exception) {
			return 0;
		}

		TBoiler::BoilerType boiler_type = boiler->GetBoilerType();
		switch (boiler_type) {
			case TBoiler::WR25:
				params[13] = 0;
				break;
			case TBoiler::WR10:
				params[13] = 1;
				break;
			case TBoiler::WR5:
				params[13] = 2;
				break;
			case TBoiler::WR2_5:
				params[13] = 3;
				break;
			case TBoiler::WR1_25:
				params[13] = 4;
				break;
			default:
				sz_log(0, "Boiler %d has unsupported type", boiler_no);
				return 0;
		}

		TAnalysisInterval *interval = boiler->GetFirstInterval();
		if (interval == NULL) {
			sz_log(0, "Missing first interval in configuration of boiler %d", boiler_no);
			return 0;
		}


		int prec_left = boiler->GetParam(TAnalysis::LEFT_GRATE_SPEED)->GetPrec();
		int prec_right = boiler->GetParam(TAnalysis::RIGHT_GRATE_SPEED)->GetPrec();

		if (prec_right != prec_left) {
			sz_log(0, "Configuration error for boiler: %d parameters %ls and %ls should have the same \
precision for analysis to work correctly", 
				boiler_no,
				TAnalysis::GetNameForParam(TAnalysis::LEFT_GRATE_SPEED).c_str(),
				TAnalysis::GetNameForParam(TAnalysis::RIGHT_GRATE_SPEED).c_str());
			return 0;
		}

		int prec = prec_left;
		if (prec > 4) {
				sz_log(0, "Invalid precision of %ls parameter in boiler %d configuration",
						TAnalysis::GetNameForParam(TAnalysis::LEFT_GRATE_SPEED).c_str(),
						boiler_no);
				return 0;
		}

		int scale_factor;
		/** Prekosc rusztu w ipk wystepuje w mm/h. Zakladamy, ze 
		 * w pamieci dzielonej ten paramter wystepuje w m/h. 
		 * Stad roznica 3 miejsc znaczacych. Nalezy jeszcze 
		 * uwzglednic precycje parametru predkosc rusztu,
		 * odpowiednio skalujac wartosci by posiadaly
		 * taka sama precycje jak paramter w pamieci dzielonej*/
		scale_factor = prec - 3;

		vr_part_params[0] =  scale(interval->GetGrateSpeedLower(), scale_factor);
		vr_part_params[1] =  scale(interval->GetGrateSpeedUpper(), scale_factor);
		vr_part_params[2] =  interval->GetDuration();
		
		
		interval = interval->GetNext();
		if (interval == NULL) {
			sz_log(0, "Missing second interval in configuration of boiler %d", boiler_no);
			return 0;
		}

		vr_part_params[3] = scale(interval->GetGrateSpeedLower(), scale_factor);
		vr_part_params[4] = scale(interval->GetGrateSpeedUpper(), scale_factor);
		vr_part_params[5] = interval->GetDuration();

		interval = interval->GetNext();
		if (interval == NULL) {
			sz_log(0, "Missing third interval in configuration of boiler %d", boiler_no);
			return 0;
		}
		vr_part_params[6] =  scale(interval->GetGrateSpeedLower(), scale_factor);
		vr_part_params[7] =  scale(interval->GetGrateSpeedUpper(), scale_factor);
		vr_part_params[8] =  interval->GetDuration();

		float_params[0] =  (int)boiler->GetGrateSpeed();
		float_params[1] =  (int)boiler->GetCoalGateHeight();
    		
		Init_RapData(params, NUMBER_OF_PARAMS, true);
 		return 1 ;
	}
	/*Stworznie odrebnego procesu dla kazego kotla w konfiguracji*/
	for (TBoiler* boiler = ipk->GetFirstBoiler(); boiler; boiler = boiler->GetNext()) 
		if (fork() == 0)  {
			char* boiler_name;
			asprintf(&boiler_name, "KOCIOL_%d", boiler->GetBoilerNo());
			execl(path, "analiza", boiler_name, NULL);
			sz_log(0, "execl (%s ,%s) failed", path, boiler_name);
		}

	exit(0);

#endif
 
#ifdef DIAGNO
	Init_RapData(params,NUMBER_OF_PARAMS,true);
#endif

}

/*-----------------------------------------------------------------------------------------------*/
/**
 @brief Funkcja zapisujaca tekst do pliku logu																												
 @param text - ³añcuch znakó³ zapisywany do pliku logu*/
/*-----------------------------------------------------------------------------------------------*/

void save_log1(const char *text)
{
	if (AnalysisData.log) 
	{
		open_log(AnalysisData.LogFileName) ;
		/* UWAGA */
		fputs(text, AnalysisData.LogFile);
		//   write(AnalysisData.LogFile, text, strlen(text));
#ifdef WORK
		/* UWAGA */
		//   write(AnalysisData.LogFile,text,strlen(text)) ;
#endif

#ifdef DIAGNO
		printf(text);
#endif
		close_log() ;
	}
}

/*-----------------------------------------------------------------------------------------------*/
/**
  @brief  Funkcja zapisujaca dane ró¿nych typów do logu.																			
  @param fmt - ³añcych znaków , króre oznaczaj± rodzaj ragumentu np. "s" - ³añcuch znaków char , "d" - int  , "sd" - ³añcuch znaków char, int
  @param ... - dowolna ilo¶æ parametrów, które maj± byæ zapisywane do logów                      */
/*-----------------------------------------------------------------------------------------------*/

void save_log(const char *fmt, ...)
{
	va_list ap;
	char *s;
	int d;
	double f;
	char buffer[12] ;
	va_start(ap,fmt);
	while(*fmt) 
	{
        switch(*fmt++) 
		{
			case 's' : 
						{
							s = va_arg(ap,char *) ;
							save_log1(s);
							break;
						}
           case 'd' : 
						{
							d = va_arg(ap,int );
							sprintf(buffer,"%d",d);
							save_log1(buffer);
							break;
						}
		   case 'f' : 
						{
							f = va_arg(ap,double );
							sprintf(buffer,"%4.3f",f);
							save_log1(buffer);
							break;
						}

		}
	}
	va_end(ap);
}

/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/
/**
 @brief Funkcja wyznaczajaca cykl 3-minutowy , zwraca ilosc sekund                  									
 @param clear - warto¶æ 1 - czy¶ci licznik, warto¶æ 0 - zwraca ilo¶c sekund od ostatniego czyszczenia licznika */
/*-----------------------------------------------------------------------------------------------*/

long get_timer(int clear)
{
#ifdef WORK

	long res;
	struct timeval *tv;
	struct timezone *tz;
	time_t *loctime ;

	tv = (struct timeval *)malloc(sizeof(struct timeval)) ;
	tz = (struct timezone *)malloc(sizeof(struct timezone)) ;
	loctime = (time_t *)malloc(sizeof(time_t)) ;

	time(loctime) ;
	tm_loctime = *localtime(loctime) ;

	gettimeofday(tv,tz);

	res = tv->tv_sec  - AnalysisData.prev_tv ;

	if (AnalysisData.prev_tv == 0 || res<0 || clear) 
	{
		AnalysisData.prev_tv  = tv->tv_sec ;
     	res = 0 ;
	}
	free(tv) ;
	free(tz) ;
	free(loctime);

#endif

#ifdef DIAGNO

	if (clear==1) res = 0 ;
	if (clear==0) res++ ;

#endif

	/* printf ("\n Time = %ld ",  res) ;*/
	return res;
}

/*----------------------------------------------------------------------------------------------------------*/
/**
 @brief Funkcja ustawia wszystkie dane odczytywane z raportu na warto¶æ SZARP_NO_DATA															*/
/**Funkca wywo³ywana przed odczytywaniem danych w ka¿dym cyklu analizy       																*/
/*----------------------------------------------------------------------------------------------------------*/

void Set_NO_DATA()
{
	RapData.a_enable = SZARP_NO_DATA ;
	RapData.work_timer_h = SZARP_NO_DATA ;
	RapData.Vc = SZARP_NO_DATA;
	RapData.En_Vc = SZARP_NO_DATA;
	RapData.Fp_4m3 = SZARP_NO_DATA;
	RapData.Vr = SZARP_NO_DATA ;
	RapData.MinHw = SZARP_NO_DATA ;
	RapData.MaxHw = SZARP_NO_DATA ;
	RapData.MinVr = SZARP_NO_DATA ;
	RapData.MaxVr = SZARP_NO_DATA ;
	RapData.set_NO_DATA = YES ;
}

/*----------------------------------------------------------------------------------------------------------*/
/**
  \brief Funkcja umieszcza odczytane dane ze sterownika kot³a do struktury RapData										  	  */
/*----------------------------------------------------------------------------------------------------------*/

void Set_Data()
{
#ifdef WORK
	if (Samples[0]==SZARP_NO_DATA) RapData.a_enable = SZARP_NO_DATA ;
	else
		if ((Samples[0] & 3) == 3) RapData.a_enable = 1 ;
			else RapData.a_enable = 0 ;

	RapData.prev_work_timer_h = RapData.work_timer_h ;
	RapData.work_timer_h = Samples[1];
	RapData.Vc = Samples[2];
	RapData.En_Vc_Probe = Samples[3];
	RapData.En_Vc = HSamples[3];
	RapData.Fp_4m3 = Samples[4];
	RapData.Vr_l = HSamples[5] ;
	RapData.Vr_p = HSamples[6] ;

	RapData.Vr = (RapData.Vr_l + RapData.Vr_p) / 2 ;

	RapData.Min_Fp_4m3 = Samples[7] ;
	RapData.Max_Fp_4m3 = Samples[8] ;

	RapData.Hw_l = HSamples[9];
	RapData.Hw_p = HSamples[10];

	RapData.Hw = (RapData.Hw_l + RapData.Hw_p) / 2 ;

	if (RapData.MinVr==SZARP_NO_DATA)
		RapData.MinVr=RapData.Vr ;
	else
		if (RapData.Vr<RapData.MinVr && RapData.Vr!=SZARP_NO_DATA)
                RapData.MinVr=RapData.Vr ;

	if (RapData.MaxVr==SZARP_NO_DATA)
		RapData.MaxVr=RapData.Vr ;
	else
		if (RapData.Vr>RapData.MaxVr)
                RapData.MaxVr=RapData.Vr ;

	if (RapData.MinHw==SZARP_NO_DATA)
		RapData.MinHw=RapData.Hw ;
	else
		if (RapData.Hw<RapData.MinHw && RapData.Hw!=SZARP_NO_DATA)
                RapData.MinHw=RapData.Hw ;

	if (RapData.MaxHw==SZARP_NO_DATA)
		RapData.MaxHw=RapData.Hw ;
	else
		if (RapData.Hw>RapData.MaxHw)
                RapData.MaxHw=RapData.Hw ;

	RapData.set_NO_DATA = NO ;

#endif
}

/*----------------------------------------------------------------------------------------------------------*/
/**
  \brief Funkcja sprawdza czy przelacznik szafy kotla jest w pozycji nr 4 ( analiza ) i czy aktualnie przeprowadzana jest analiza */
/*----------------------------------------------------------------------------------------------------------*/

int Check_AnslysisON() 
{
	if (RapData.a_enable == NO && AnalysisData.a_cycle != -1) 
		return  1;
	else 
		return  0;
}
/*----------------------------------------------------------------------------------------------------------*/
/**
    \brief Funkcja pobiera dane przys³ane przez regulator kotla , wcze¶niej ni¿ w czasie kolejnego cyklu analizy
    na wypadek  wyst±pienia braku komunikcaji.*/
/**    Mo¿liwa jest rownie¿ próba po¼niejszego odczytu danych
    ni¿ w wyznaczonym kolejnym cyklu , na wypadek wcze¶niejszego braku komunikacji 													
    @param timer - ilo¶æ czasu, która up³ynê³a od ostatniego cyklu analizy
    @return - zezwolenie na kolejny cykl analizy (YES - 1 , NO - 0) */
/*----------------------------------------------------------------------------------------------------------*/

int get_data(int timer)
{
 
	int good_data ;
  
	/* czytamy dane co 10 sekund */
	if (timer % 10 ==0)  
	{
		good_data = read_data() ;
		if (good_data) 
		{
	  
			RapData.good_data = 1;
			RapData.NumberOfNO_DATA = 0 ;
			Set_Data() ;
			if (Check_AnslysisON()) return 1 ;
		}
		else 
		{
			RapData.NumberOfNO_DATA++;
			if (RapData.NumberOfNO_DATA>18) Set_NO_DATA() ;
			RapData.good_data = 0;
		}
	}
	if (AnalysisData.a_cycle==-1 || AnalysisData.confirmed==NO) 
	{

		if (AnalysisData.VrPartition==0) 
		{ /* predkosc rusztu ponizej dopuszczalnego przedzialu*/
   			if (timer>=AnalysisData.Period) return 1;
		}
		else 
			if (timer>=TIMER_1M) return 1;
		return 0;
	}
	if ( timer==AnalysisData.Period ) return 1;
	if ( timer> TIMER_3H) return 1 ;
	return 0;
}


/*-----------------------------------------------------------------------------------------------*/
/**
  \brief Inicjalizacja struktury przechowujacej dane do analizy */
/*-----------------------------------------------------------------------------------------------*/

void Init_AnalysisData()
{
	AnalysisData.a_cycle = -1 ;  /* numer cyklu analizy */
	AnalysisData.a_timer  = 0;
	AnalysisData.prev_tv = 0;
	AnalysisData.a_dir = A_UP ; /* kierunek analizy - rozpoczynanmy od dodania powietrza podmuchowego */
	AnalysisData.log = 1 ;      /* czy zapisywac informacje o przebiegu analizy do pliku */
	AnalysisData.NumberOfAnalyses = 0 ;  /* Ilosc przeprowadzonych analiz */
	AnalysisData.confirmed =  NO ; /* potwierdzenie otrzymania danych od sterownika - tym samym potwierdzenie wykonania cyklu */
	AnalysisData.restore = NO ;
	AnalysisData.global_timer = 0 ;
	AnalysisData.VrPartition = -1 ;
	AnalysisData.VrPartILow = vr_part_params[0] ; /* granica dolna przedzialu - predkosc rusztu */
 	AnalysisData.VrPartIHi = vr_part_params[1]  ; /* granica gorna przedzialu - predkosc rusztu */
 	AnalysisData.VrPartIDuration = vr_part_params[2]  ; /* czas przeprowadzania analizy dla tego przedzialu w sek. */

 	AnalysisData.VrPartIILow = vr_part_params[3] ; /* granica dolna przedzialu - predkosc rusztu */
 	AnalysisData.VrPartIIHi  = vr_part_params[4] ; /* granica gorna przedzialu - predkosc rusztu */
 	AnalysisData.VrPartIIDuration = vr_part_params[5]  ; /* czas przeprowadzania analizy dla tego przedzialu w sek. */

 	AnalysisData.VrPartIIILow = vr_part_params[6] ; /* granica dolna przedzialu - predkosc rusztu */
 	AnalysisData.VrPartIIIHi =  vr_part_params[7] ; /* granica gorna przedzialu - predkosc rusztu */
 	AnalysisData.VrPartIIIDuration = vr_part_params[8] ; /* czas przeprowadzania analizy dla tego przedzialu w sek. */

 	AnalysisData.d_Vr = float_params[0]  ; /* granica górna zmian prêdko¶ci rusztu podczas jednego cyklu analizy */
 	AnalysisData.d_Hw = float_params[1] ; /* granica górna zmian prêdko¶ci warstwownicy podczas jednego cyklu analizy */
 	
 	init_ActSamples() ;
}

/*-----------------------------------------------------------------------------------------------*/
/**
\brief Inicjowanie struktury przechowujacej dane z regulatora kotla 														
\param params[] - tablica adresów odpowiednich parametrów (odczytywane ze sterownika kot³a) z pliku konfiguracyjnego
\param ipk - czy uzywamy ipk
\param counter - liczba parametrów w tablicy params */
/*-----------------------------------------------------------------------------------------------*/

void  Init_RapData(int params[], int counter,bool ipk)
{

	int i;
	RapData.type = 0 ;
	RapData.lines = counter ; /* ilosc danych z regulatora kotla*/

	for (i = 0 ; i < counter-1 ; i++ ) 
	{
		if (ipk) {	// jezeli uzywamy ipk to od razu dostajemy adresy w pamieci dzielonej
			RapData.addr[i] = params[i] ;
			//save_log("sdsds", "ipk: Parametr: ", i, " adres: ", params[i],"\n");
		}
		else 
			if (params[i]<PTTact.len  &&  PTTact.tab[params[i]]!=NO_PARAM) {
				RapData.addr[i] = PTTact.tab[params[i]];
				//save_log("sdsds", "Parametr: ", i, " adres: ", PTTact.tab[params[i]],"\n");
			}
	}
	RapData.WRtype =  params[counter-1];
	RapData.good_data = NO ;  /* Czy poprawna komunikacja z regulatorem kotla*/
	RapData.NumberOfNO_DATA = 0 ;
	RapData.MinHw = SZARP_NO_DATA ;
	RapData.MaxHw = SZARP_NO_DATA ;
	RapData.MinVr = SZARP_NO_DATA ;
	RapData.MaxVr = SZARP_NO_DATA ;
	RapData.work_timer_h = RapData.prev_work_timer_h = 0;


#ifdef DIAGNO

	RapData.a_enable=1;
	RapData.work_timer_h=10;
	RapData.Vc=2000 ;
	RapData.En_Vc=4000;
	AnalysisData.a_Fp_4m3 = AnalysisData.last_Fp_4m3 = 820;
	RapData.WRtype = 1;
	RapData.Vr=400;
	RapData.Min_Fp_4m3 = 700;
	RapData.Max_Fp_4m3 = 800;
#endif
	/*Koniec testu*/
}

/*-----------------------------------------------------------------------------------------------*/
/**
  \brief Czytanie danych z pamieci dzielnej ( dane z kotla ) i podstawianie do odpowiedznich struktur
  \return czy odczyt danych zakoñczy³ siê sukcesem */
/*-----------------------------------------------------------------------------------------------*/

int read_data( )
{

	int good_data ;
#ifdef WORK
	int i ;

	good_data = YES ;

	/* odczytywanie biezacych probek */
	ipcRdGetInto(SHM_PROBE);
	for (i = 0 ; i < NUMBER_OF_PARAMS - NUMBER_OF_OUTPAR - 1 ; i++ ) 
	{
		Samples[i]  = Probe[RapData.addr[i]] ;
		if (Samples[i]==SZARP_NO_DATA) good_data = NO ;
	}
	ipcRdGetOut(SHM_PROBE);

	/*odczytywanie probek godzinowych */
	ipcRdGetInto(SHM_HOUR);
	for (i = 0 ; i < NUMBER_OF_PARAMS - NUMBER_OF_OUTPAR - 1 ; i++ ) 
	{
		HSamples[i]  = Hour[RapData.addr[i]] ;
		if (HSamples[i]==SZARP_NO_DATA) good_data = NO ;
	}
	ipcRdGetOut(SHM_HOUR);
 	
#endif

#ifdef DIAGNO
	good_data = YES ;
	if (RapData.Vc==4000) RapData.Vc=2100 ;
	RapData.Fp_4m3 = AnalysisData.a_Fp_4m3 ;
	RapData.Vr = RapData.Vr % 650 + 20  ;
#endif
	return good_data ;
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Zapis danych do pamiêci dzielonej przeznaczonych dla regulatora kot³a								*/
/*-----------------------------------------------------------------------------------------------*/

void write_data()
{
	SendData.Fp_4m3 = AnalysisData.a_Fp_4m3 ;

	if (AnalysisData.a_cycle==-1 || AnalysisData.a_cycle>=A_STOP )
         SendData.AnalyseStatus = STOPED ;
	else
 		 SendData.AnalyseStatus = WORKING ;
	
	if (SendData.Fp_4m3 == 0 || (AnalysisData.a_cycle==-1 && AnalysisData.restore==NO))  return ;

#ifdef WORK

	ipcWrGetInto(SHM_PROBE);
	Probe[RapData.addr[11]] = SendData.Fp_4m3;
	Probe[RapData.addr[12]] = SendData.AnalyseStatus;
	ipcWrGetOut(SHM_PROBE);

#endif  
	save_log("sdsds","Zapisywanie danych: (nowa wartosc strumienia powietrza dla 4m3 wêgla)a_Fp_4m3=",SendData.Fp_4m3," status analizy=",SendData.AnalyseStatus,"\n");
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkca inicjuj±ca analizê - wywo³ywana na samym pocz±tku  analizy 		                 */
/*-----------------------------------------------------------------------------------------------*/

void a_init()
{
	AnalysisData.a_Fp_4m3 = RapData.Fp_4m3 ;
	AnalysisData.last_Fp_4m3 = AnalysisData.a_Fp_4m3 ;
	AnalysisData.a_dir=A_UP ;
	save_log("sds","Dotychczasowy wspolczynik strumienia powietrza dla 4m3 wegla Fp_4m3=",AnalysisData.last_Fp_4m3,"\n");
	AnalysisData.restore = NO ;

	RapData.MinVr=RapData.MaxVr=RapData.Vr ;

	RapData.MinHw=RapData.MaxHw=RapData.Hw ;

#ifdef DIAGNO
	RapData.Vc=2000 + AnalysisData.NumberOfAnalyses * 100 ;
	RapData.En_Vc=4000 + (AnalysisData.a_Fp_4m3 % 600) * 10 ;
#endif

}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkcja wywo³ywana na koñcu analizy    																							*/
/*-----------------------------------------------------------------------------------------------*/

void a_close()
{
	save_log("s","Zakoñczenie bie¿±cej analizy...\n") ;
  AnalysisData.a_cycle = -1 ;
	AnalysisData.VrPartition = -1 ;
	AnalysisData.global_timer = 0 ;
	AnalysisData.NumberOfAnalyses = 0 ;
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkcja dodaj±ca / odejmuj±ca  ilo¶æ powietrza w ka¿dym cyklu analizy  						
 \param a_cycle - numer cyklu analizy 																													 */
/*-----------------------------------------------------------------------------------------------*/

void add_air(int a_cycle)
{
	switch (a_cycle)
	{
		case A_START  :  
					{
						AnalysisData.a_Fp_4m3 = AnalysisData.a_Fp_4m3 + CHANGE_PERCENTS;
						if (AnalysisData.a_Fp_4m3>MAX_AIR) AnalysisData.a_Fp_4m3=MAX_AIR;
            				save_log("sds","Zwiêkszenie warto¶æ strumienia powietrza o 1 pkt a_Fp_4m3=",AnalysisData.a_Fp_4m3,"\n");
                    }
                    break ;

		case A_UP     :  
					{
						AnalysisData.a_Fp_4m3 = AnalysisData.a_Fp_4m3 - CHANGE_PERCENTS;
						if (AnalysisData.a_Fp_4m3<MIN_AIR) AnalysisData.a_Fp_4m3=MIN_AIR; /* w praktyce niemozlize */
							save_log("sds","Powrot po zwiekszeniu do warto¶æ strumienia powietrza  a_Fp_4m3=",AnalysisData.a_Fp_4m3,"\n");
					}
                    break;

		case A_UP_RET :  
					{
						AnalysisData.a_Fp_4m3 = AnalysisData.a_Fp_4m3 - CHANGE_PERCENTS;
						if (AnalysisData.a_Fp_4m3<MIN_AIR) AnalysisData.a_Fp_4m3=MIN_AIR; /* w praktyce niemozlize */
							save_log("sds","Zmniejszenie warto¶æ strumienia powietrza a_Fp_4m3=",AnalysisData.a_Fp_4m3,"\n");
    				}
    				break;
		case A_DOWN    : 
					{
						AnalysisData.a_Fp_4m3 = AnalysisData.a_Fp_4m3 + CHANGE_PERCENTS;
						if (AnalysisData.a_Fp_4m3<MIN_AIR) AnalysisData.a_Fp_4m3=MIN_AIR; /* w praktyce niemozlize */
							save_log("sds","Powrot po zmniejszeniu do warto¶æ strumienia powietrza  a_Fp_4m3=",AnalysisData.a_Fp_4m3,"\n");
    				}
    				break;

		case A_DOWN_RET: 
					{
						AnalysisData.a_Fp_4m3 = AnalysisData.a_Fp_4m3 + CHANGE_PERCENTS;
						if (AnalysisData.a_Fp_4m3<MIN_AIR) AnalysisData.a_Fp_4m3=MIN_AIR; /* w praktyce niemozlize */
            				save_log("sds","Zwiêkszenie warto¶æ strumienia powietrza o 1 pkt a_Fp_4m3=",AnalysisData.a_Fp_4m3,"\n");
    				}
    				break;

		default: break;
	}
	AnalysisData.confirmed=NO ;
}

/*-----------------------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------------------*/

int SetPeriodWRtype1()
{

	float period=0 ;
	if (AnalysisData.a_cycle==-1) 
	{ /* poczatek analizy */
		if (RapData.Vr >= AnalysisData.VrPartIIIHi ) 
		{
			AnalysisData.Period = TIMER_3H ; /* 3 godz. (wartosc w sekundach)*/
			AnalysisData.VrPartition = 0 ;		
			period = (float)AnalysisData.Period / 3600.0 ;
			save_log("sfs","Predkosc posuwu rusztu powy¿ej dopuszczalnej wartosci.Czekamy: ",period,"h\n");		
			return 0 ;
		}
		if (RapData.Vr >= AnalysisData.VrPartIIILow ) 
		{
		
			AnalysisData.Period = AnalysisData.VrPartIIIDuration ; /* 2 godz. (wartosc w sekundach)*/ 
			AnalysisData.VrPartition = 3 ;
			period = (float)AnalysisData.Period / 3600.0 ;
			save_log("sfs","Ustawienie dlugosci cyklu analizy na ",period,"h\n");		 
		}
		else 
			if (RapData.Vr >= AnalysisData.VrPartIILow ) 
			{

				AnalysisData.Period = AnalysisData.VrPartIIDuration ; /* 2,4 godz. (wartosc w sekundach)*/ 
				AnalysisData.VrPartition = 2 ;
				period = (float)AnalysisData.Period / 3600.0 ;		
				save_log("sfs","Ustawienie dlugosci cyklu analizy na ",period,"h\n");		
			}
			else 
				if (RapData.Vr >= AnalysisData.VrPartILow ) 
				{
					AnalysisData.Period = AnalysisData.VrPartIDuration ; /* 3 godz. (wartosc w sekundach)*/ 
					AnalysisData.VrPartition = 1 ;
					period = (float)AnalysisData.Period / 3600.0 ;
					save_log("sfs","Ustawienie dlugosci cyklu analizy na ",period,"h\n");		
				}
				else 
				{
	         
					AnalysisData.Period = TIMER_3H ; /* 3 godz. (wartosc w sekundach)*/
					AnalysisData.VrPartition = 0 ;		
					period = (float)AnalysisData.Period / 3600.0 ;		
  					save_log("sfs","Predkosc posuwu rusztu ponizej dopuszczalnej wartosci.Czekamy: ",period,"h\n");				
					return 0 ;
				}
				AnalysisData.a_cycle=A_START ;
				save_log("s","Rozpoczêcie analizy... \n");
	}
	else 
	{	            
		if (AnalysisData.VrPartition == 3) 
		{ /* przedzial - Vr > 5*/

			if (RapData.Vr>AnalysisData.VrPartIIIHi) 
			{
				save_log("s","Predkosc posuwu rusztu powyzej dopuszczalnej wartosci.Przerwanie analizy\n");
				AnalysisData.a_cycle = A_STOP;   /* przerywamy analize */
				AnalysisData.a_Fp_4m3 = AnalysisData.last_Fp_4m3 ;
    			AnalysisData.Period = TIMER_3H ; /* 3 godz. (wartosc w sekundach)*/
    			AnalysisData.VrPartition = 0 ;	
				return 1;
			}
			if (RapData.Vr>=AnalysisData.VrPartIIILow) 
			{
		
				AnalysisData.Period = AnalysisData.VrPartIIIDuration ; /* 2 godz. (wartosc w sekundach)*/
				period = (float)AnalysisData.Period / 3600.0 ;
   				save_log("sfs","Ustawienie dlugosci cyklu analizy na ",period,"h\n");		 		
   				return 1;
			}
			if (RapData.Vr >= AnalysisData.VrPartIILow) 
			{
		    
				AnalysisData.Period = AnalysisData.VrPartIIDuration ; /* 2,4 godz. (wartosc w sekundach)*/ 
				AnalysisData.VrPartition = 2 ; 
  				period = (float)AnalysisData.Period / 3600.0 ;		
				save_log("sfs","Zmiana dlugosci cyklu analizy na ",period,"h\n");
				return 1;
			}
			if (RapData.Vr >= AnalysisData.VrPartILow) 
			{
	
				AnalysisData.Period = AnalysisData.VrPartIDuration ; /* 3 godz. (wartosc w sekundach)*/ 
				AnalysisData.VrPartition = 1 ; 
  				period = (float)AnalysisData.Period / 3600.0 ;		
				save_log("sfs","Zmiana dlugosci cyklu analizy na ",period,"h\n");
				return 1;		
			}
			else 
			{
				save_log("s","Predkosc posuwu rusztu ponizej dopuszczalnej wartosci.Przerwanie analizy\n");		
				AnalysisData.a_cycle = A_STOP;   /* przerywamy analize */
				AnalysisData.a_Fp_4m3 = AnalysisData.last_Fp_4m3 ;
				AnalysisData.Period = TIMER_3H ; /* 3 godz. (wartosc w sekundach)*/ 
   				AnalysisData.VrPartition = 0 ;	
				return 1 ;
			}
		}
		else
			if (AnalysisData.VrPartition == 2) 
			{ /* przedzial - Vr = (6,4 ) */
				if  (RapData.Vr >= AnalysisData.VrPartIIHi ) 
				{
					AnalysisData.Period = AnalysisData.VrPartIIIDuration ; /* 2 godz. (wartosc w sekundach)*/ 
					AnalysisData.VrPartition = 3 ;
     				period = (float)AnalysisData.Period / 3600.0 ;		
	 				save_log("sfs","Zmiana dlugosci cyklu analizy na ",period,"h\n");
				}
				else
					if (RapData.Vr < AnalysisData.VrPartIIHi && RapData.Vr >= AnalysisData.VrPartIILow) 
					{
      					AnalysisData.Period = AnalysisData.VrPartIIDuration ; /* 2,4 godz. (wartosc w sekundach)*/
        				period = (float)AnalysisData.Period / 3600.0 ;		
      					save_log("sfs","Zmiana dlugosci cyklu analizy na ",period,"h\n");
    					return 1;
   					}
					else 
						if (RapData.Vr >= AnalysisData.VrPartILow) 
						{
							AnalysisData.Period = AnalysisData.VrPartIDuration ; /* 3 godz. (wartosc w sekundach)*/ 
							AnalysisData.VrPartition = 1 ; 
							period = (float)AnalysisData.Period / 3600.0 ;		
							save_log("sfs","Zmiana dlugosci cyklu analizy na ",period,"h\n");
							return 1;
						} 
						else 
						{
							save_log("s","Predkosc posuwu rusztu ponizej dopuszczalnej wartosci.Przerwanie analizy\n");		
							AnalysisData.a_cycle = A_STOP;   /* przerywamy analize */
							AnalysisData.a_Fp_4m3 = AnalysisData.last_Fp_4m3 ;
  							AnalysisData.Period = TIMER_3H ; /* 3 godz. (wartosc w sekundach)*/
							AnalysisData.VrPartition = 0 ;
							return 1 ;	
						}
			}
			else 
				if (AnalysisData.VrPartition == 1) 
				{ /* przedzial - Vr = (5,3 ) */	
					if (RapData.Vr < AnalysisData.VrPartIIHi  && RapData.Vr >= AnalysisData.VrPartIHi ) 
					{
						AnalysisData.Period = TIMER_2_4H ; /* 2,4 godz. (wartosc w sekundach)*/ 
						AnalysisData.VrPartition = 2 ;      	
      					period = (float)AnalysisData.Period / 3600.0 ;						
						save_log("sfs","Zmiana dlugosci cyklu analizy na ",period,"h\n");
					}
					else 
						if (RapData.Vr >= AnalysisData.VrPartIIHi ) 
						{
							AnalysisData.Period = TIMER_2H ; /* 2 godz. (wartosc w sekundach)*/ 
							AnalysisData.VrPartition = 3 ;
   							save_log("sfs","Zmiana dlugosci cyklu analizy na ",period,"h\n");
						}
						else 
							if (RapData.Vr < AnalysisData.VrPartILow ) 
							{
								save_log("s","Predkosc posuwu rusztu ponizej dopuszczalnej wartosci.Przerwanie analizy\n");		
								AnalysisData.a_cycle = A_STOP;   /* przerywamy analize */
								AnalysisData.a_Fp_4m3 = AnalysisData.last_Fp_4m3 ;
  								AnalysisData.Period = TIMER_3H ; /* 3 godz. (wartosc w sekundach)*/
  								AnalysisData.VrPartition = 0 ;
								return 1 ;
							}
							else 
							{
								AnalysisData.Period = AnalysisData.VrPartIDuration ; /* 3 godz. (wartosc w sekundach)*/
  	  							period = (float)AnalysisData.Period / 3600.0 ;		
   								save_log("sfs","Zmiana dlugosci cyklu analizy na ",period,"h\n");
								return 1;
							}
				}
	}
	return 1;
}

/*-----------------------------------------------------------------------------------------------*/

int SetPeriod() 
{
	return SetPeriodWRtype1() ; 
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkcja decyduj±ca o rozpoczêciu lub przerwaniu ( jesli trwa ) analizy							
   \return - czy kontynuowaæ proces analizy ( YES - 1 , NO - 0 ) */
/*-----------------------------------------------------------------------------------------------*/

int calculate_data()
{
	AnalysisData.restore = NO ;

	if (RapData.good_data==NO) 
	{
		save_log("s","Brak komunikacji ze sterownikiem kotla\n");
		AnalysisData.VrPartition = -1 ;
 		return 0 ;
	}
	/* przerywamy analize, bo kociol zostal przelaczony w tryb pracy automatycznej bez analizy */
	/* i zostalo zmienione powietrze , wiec przerywamy analize bez przywracania powietrza      */
	if (RapData.a_enable == NO && AnalysisData.a_cycle != -1) 
	{
		save_log("s","Przerwanie analizy po otrzymaniu od kotla informacji \"a_enable=NO\" \n");
		AnalysisData.a_cycle = A_STOP;   /* przerywamy analize */
		AnalysisData.a_Fp_4m3 = SZARP_NO_DATA ;
		AnalysisData.restore = YES ;
    AnalysisData.VrPartition = -1 ;
		return  1;
	}
	if (RapData.a_enable == NO) 
	{
		save_log("s","Brak zezwolenia na analize \n");
		AnalysisData.VrPartition = -1 ;
        return 0; /* aktualnie nie jest przeprowadzana zadna analiza*/
	}
	if (AnalysisData.a_cycle != -1 && RapData.good_data==YES && AnalysisData.confirmed == NO) 
	{
		if (RapData.Fp_4m3==AnalysisData.a_Fp_4m3 || RapData.Fp_4m3<=RapData.Min_Fp_4m3 || RapData.Fp_4m3>=RapData.Max_Fp_4m3) 
		{
  			AnalysisData.confirmed = YES ;
			save_log("s","Potwierdzenie otrzymania danych od sterownika.\n");
			SetPeriod() ;
			return 0 ;
		}
	}
	if (RapData.Fp_4m3!=AnalysisData.a_Fp_4m3 && AnalysisData.a_cycle != -1 && RapData.good_data==YES ) 
	{
		if (RapData.Fp_4m3>RapData.Min_Fp_4m3 && RapData.Fp_4m3<RapData.Max_Fp_4m3) 
		{
			save_log("s","Ponowne wys³anie danych do sterownika!!! \n");
			AnalysisData.confirmed = NO ;
			return 0;
		}
	}
	return SetPeriod() ;
}


/*-----------------------------------------------------------------------------------------------*/
/** \brief Inicjalizacja struktury przechowujacej próbki ostatnich cykli analizy								*/
/*-----------------------------------------------------------------------------------------------*/

void init_ActSamples()
{
	ActSamples.Pointer = 0;
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkcja zapisuj±ca odczytane dane z regulatora kot³a do struktury przechowuj±cej próbki
    \param index - indeks w tablicy przechowuj±cej próbki dla biez±cych próbek
    \param vc - wato¶æ objêto¶ci wêgla, zapisywana do tablicy
    \param en_vc - warto¶æ stosunku energia / objêto¶æ zapisywana do tablicy
    \param fp_4m3 - warto¶æ strumienia powietrza zapisywana do tablicy */
/*-----------------------------------------------------------------------------------------------*/

void save_ActSamples(int index,int vc,int en_vc,int fp_4m3)
{	
	ActSamples.Vc[index]     = vc ;
	ActSamples.En_Vc[index]  = en_vc ;
	ActSamples.Fp_4m3[index] = fp_4m3 ;		
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkcja zapisuj±ca odczytane dane z regulatora kot³a do struktury przechowuj±cej próbki
    \param index - indeks w tablicy przechowuj±cej próbki dla biez±cych próbek                   */
/*-----------------------------------------------------------------------------------------------*/

void save_RapData(int index)
{
	save_ActSamples(index, RapData.Vc,  RapData.En_Vc , RapData.Fp_4m3 ) ;
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkcja sprawdzaj±ca czy mo¿na zmieniæ powietrze wg waunku: 													 */
/*-----------------------------------------------------------------------------------------------*/

int CheckTerms()
{
	float d_Vr ;
	float d_Hw ;

	d_Vr = ((float)RapData.MaxVr - (float)RapData.MinVr) / (float)RapData.MinVr *100.0 ;
	d_Hw = ((float)RapData.MaxHw - (float)RapData.MinHw) / (float)RapData.MinHw *100.0 ;
	
	if (d_Vr > AnalysisData.d_Vr) return 0 ;
	if (d_Hw > AnalysisData.d_Hw) return 0 ;
	return 1 ;
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkcja sprawdzaj±ca efekty analizy.																									 */
/** Decyduje o kierunku zmian strumienia powietrza podmuchowego                                  */
/*-----------------------------------------------------------------------------------------------*/

void analyse_effect()
{
	if (AnalysisData.NumberOfAnalyses >= 1 ) 
	{
		if (ActSamples.En_Vc[1]<=ActSamples.En_Vc[0] && ActSamples.En_Vc[1]<=ActSamples.En_Vc[2]) 
		{
			AnalysisData.restore = YES ; /* zapisywanie powietrza */
			save_log("s","Analiza efektow: nie zmieniamy powietrza \n");
			return ;
		}
		if (ActSamples.En_Vc[0]<=ActSamples.En_Vc[1] && ActSamples.En_Vc[0]<=ActSamples.En_Vc[2]) 
		{
			AnalysisData.a_Fp_4m3 = AnalysisData.a_Fp_4m3 - AIR_CHANGE;
			AnalysisData.restore = YES ; /* zapisywanie powietrza */
			save_log("s","Analiza efektow: zmniejszenie powietrza \n");
			return ;
		}
		if (ActSamples.En_Vc[2]<=ActSamples.En_Vc[0] && ActSamples.En_Vc[2]<=ActSamples.En_Vc[1]) 
		{
			AnalysisData.a_Fp_4m3 = AnalysisData.a_Fp_4m3 + AIR_CHANGE;
			AnalysisData.restore = YES ; /* zapisywanie powietrza */
			save_log("s","Analiza efektow: dodanie powietrza \n");
			return ;
		}
		AnalysisData.restore = YES ; /* zapisywanie powietrza */
		save_log("s","Zmiana powietrza po analizie \n");
	}
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief G³ówna funkcja analizy - po ka¿dorazowym jej wywo³aniu, nastêpuje kolejny cykl analizy */
/*-----------------------------------------------------------------------------------------------*/
void analyse()
{
	float period ;
	if (calculate_data()==0) return;

	AnalysisData.global_timer++ ;
	save_log("sds","Numer cyklu analizy: ",AnalysisData.global_timer,"\n");

	if (AnalysisData.a_cycle!=-1 && AnalysisData.a_cycle!=A_START && AnalysisData.a_cycle!=A_STOP)
		if (CheckTerms()==0) 
		{
			AnalysisData.Period = TIMER_1H ; /* nastepne dane odczytujemy za godzine */
	 		period = (float)AnalysisData.Period / 3600.0 ;
			save_log("sfs","Za du¿e zmany warstwownicy lub prêdkosci rusztu.Czekamy: ",period,"h\n");		
			RapData.MinVr=RapData.MaxVr=RapData.Vr ;
			RapData.MinHw=RapData.MaxHw=RapData.Hw ;
			return ;
		}

	switch (AnalysisData.a_cycle) 
	{
		case A_START  : 
						{
							a_init() ;
							add_air(0) ;
						}
						break ;

		case A_UP     :
						{
							save_RapData(0) ;
							add_air(A_UP) ;
						}
						break;
		case A_UP_RET :
						{
							save_RapData(1) ;
							add_air(A_UP_RET) ;
						}
						break;
		case A_DOWN   :
						{
							save_RapData(2) ;
							add_air(A_DOWN) ;
							AnalysisData.NumberOfAnalyses = 1 ;
						}
						break;
		case A_DOWN_RET:
						{
							save_RapData(1) ;
							add_air(A_DOWN_RET) ;
						}
						break;
		case A_STOP    :
						{
							a_close() ;
						}
						return;

		default: return ;
	}
	AnalysisData.a_cycle++  ;
	if (AnalysisData.a_cycle>A_NUMBER_OF_CYCLES) AnalysisData.a_cycle = 1 ;
	analyse_effect()  ;

	RapData.MinVr=RapData.MaxVr=RapData.Vr ;
	RapData.MinHw=RapData.MaxHw=RapData.Hw ;
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkcja otwieraj±ca plik logu do zapisywania																																																																				
    \param filename - nazwa pliku logu */
/*-----------------------------------------------------------------------------------------------*/

void open_log(char *filename)
{
	char name[255];
 
	strcpy(name,filename) ;
	strcat(name,".log");
	/* UWAGA */
	// AnalysisData.LogFile = open(name, O_RDWR);
	AnalysisData.LogFile = fopen(name, "a");

	if (!AnalysisData.LogFile) 
	{
		sz_log(0, "cannot open file %s", name);
		exit(1);
	}
}

/*-----------------------------------------------------------------------------------------------*/
/** \brief Funkcja zamykaj±ca plik logu 																												*/																																																																				
/*-----------------------------------------------------------------------------------------------*/

void close_log()
{
/* UWAGA */
//  close(AnalysisData.LogFile);
	fclose(AnalysisData.LogFile);
}


/**Sprawdza czy istnieje plik z konfiguracja w starym
 * formacie - jezeli tak - kaze go przerobic do ipk.
 * Zawsze konczy sie bledem.
 * @param argc liczba argumentow wywolania programu
 * @param argv lista argumentow wywolania programu
 * @return 1 w przypadku bledu, 0 gdy operacja zakonczyla sie sukcesem*/
int ReadAnalizaCfg(int argc, char* argv[]) {
	char conf_path[50];
	struct stat stat_buf;

	if (stat(conf_path, &stat_buf) == 0) {
		sz_log(0, "Due to frequent errors in analiza.cfg configuration files");
		sz_log(0, "(grate speed bounds parametrs) this format is no longer supported.");
		sz_log(0, "Use analiza2ipk to integrate existing analiza.cfg into IPK.");
		sz_log(0, "Possible errors in these parameters will be fixed.");
	}

	sz_log(0, "Failed to load configuration.");
	return 1;
}

/*-----------------------------------------------------------------------------------------------*/
/**
  \brief G³ówna funkcja programu
  \param argc - liczba parametrów wywo³ania 																										
  \param argv[] - tablica przechowuj±ca parametry wywo³ania
  \return  wynik dzia³ania funkcji																															*/
/*-----------------------------------------------------------------------------------------------*/

int main(int argc,char *argv[])
{
  //struct sigaction Act;
  	TSzarpConfig* ipk = new TSzarpConfig();
	char buf2[250];
	int loglevel = 0;
	char *logfile;

#ifdef WORK

	if (argc!=2 && argc!=1) 
	{
		printf("\nBledna ilosc parametrow wywolania!\n") ;
		exit(1);
	}

	libpar_init();
	loglevel = loginit_cmdline (2, "", &argc, argv);
  	logfile = libpar_getpar(buf2, "log", 0);
	if (logfile == NULL)
		logfile = strdup(PREFIX "/logs/analiza");

	if (argc == 2) {
		AnalysisData.log = 1 ;     
		strcpy(AnalysisData.LogFileName,PREFIX "/logs/") ;
		strcat(AnalysisData.LogFileName,argv[1]);
	}

  	char *ipk_path = libpar_getpar("analiza", "IPK", 0);
	if (ipk_path != NULL) {
		if (ipk->loadXML(SC::A2S(ipk_path))) {
			if (argc == 1) 
				sz_log(0,"Failed to load IPK file\n");
			if (ReadAnalizaCfg(argc, argv))
				return 1;
		} else 
			if (ipk->GetFirstBoiler() == NULL) {
				if (argc == 1) 
					sz_log(0,"Analiza configuration not present in IPK");
				if (ReadAnalizaCfg(argc, argv)) 
					return 1;
			} else 
				if (!ReadCfg(argv[0], (argc == 1) ? NULL : argv[1], ipk)) 
						return 1;
	} else {
		sz_log(0,"IPK is required for analiza to work");
		return 1;
	}
		
	ipcInitializewithSleep(5) ;
	Init_AnalysisData() ;

  //siê demonizujemy

  //Act.sa_handler = Terminate;
  //Act.sa_flags = SA_RESTART | SA_RESETHAND;
  //sigaction(SIGTERM, &Act, NULL);
  //sigaction(SIGINT, &Act, NULL);
  signal(SIGPIPE, SIG_IGN);
  go_daemon();

#endif

#ifdef DIAGNO

	//ReadCfgFile(NULL,"KOCIOL_1",conf_path) ;
	Init_AnalysisData() ;
	strcpy(AnalysisData.LogFileName,"KOCIOL_1");

#endif

 /* Inicjalizacja globalnej struktury przechowuj¹cej historie poprzednich analiz*/

	delete ipk;

	save_log("s","Start programu...\n");

	get_timer(1) ; // zerowanie licznika wyznaczajacego cykl 3 minutowy
 
	libpar_done();

	while (1) 
	{
		AnalysisData.a_timer = get_timer(0) ; // pobranie wartosci licznia wyznaczjacego cykl 3 minutowy
		if (get_data(AnalysisData.a_timer)==1)  
		{
			tm_loctime.tm_year+=1900 ;
			tm_loctime.tm_mon+=1;
      		save_log("sdsdsdsdsds","Data: ",tm_loctime.tm_mday,"/" , tm_loctime.tm_mon ,"/" ,
			tm_loctime.tm_year,", ", tm_loctime.tm_hour , ":" , tm_loctime.tm_min , "\n");

			save_log("sdsdsdsdsdsdsdsdsdsdsdsdsds","Czytanie danych: a_enable=",RapData.a_enable,", work_timer=",RapData.work_timer_h,
            ", Vc=",RapData.Vc, ", En_Vc1h=",RapData.En_Vc,", En_Vc=" ,RapData.En_Vc, ", Fp_4m3=",RapData.Fp_4m3,", Vr=",RapData.Vr,", Min_Fp_4m3=",RapData.Min_Fp_4m3,", Max_Fp_4m3=",RapData.Max_Fp_4m3,
            ", MinVr=",RapData.MinVr,", MaxVr=",RapData.MaxVr, ", MinHw=",RapData.MinHw,", MaxHw=",RapData.MaxHw,"\n");

			analyse() ;

			write_data();
			get_timer(1) ;  // zerowanie licznika wyznaczajacego cykl 3 minutowy
		}
   #ifdef WORK
		sleep(1) ;
   #endif

   #ifdef DIAGNO
		Sleep(1000) ;
   #endif

	}
	return 0;
}

