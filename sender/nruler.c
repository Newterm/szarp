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

#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/msg.h>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include"msgtools.h"
#include"ipctools.h"
#include"frombase.h"

#define C_J2W 1.16

#define FULL_CTRL 1
#define TIME_STEP 25 
#define CTRL_MODE FULL_CTRL

/* Rozk³ad 24-ro godzinny (144-ro 10-cio minutowy) ma na ostatniej pozycji
   warto¶æ ¶redniego poboru dobowego CWU */

#define AVG_IND 144

/* Liczba pobieranych parametrów wej¶ciowych, wyliczanych ¶rednich,
   generowanych parametrów steruj±cych oraz wysy³anych komunikatów */

#define MAX_PAR 8
#define MAX_AVG 17
#define MAX_P 6
#define MAX_STER 1

#define MAX_KOC 8    	/* program nie obs³u¿y ciep³owni o wiêcej kot³ach */ 

/* Co liczyæ ¶redni± czy sumê */

#define DO_AVG 1
#define DO_SUM 2

/* Stan komunikatu steruj±cego: gotowy do wys³ania, oczekiwanie na 
   potwierdzenie, potwierdzony */

#define MSG_SEND 0
#define MSG_CONF 1
#define MSG_OK 2

/* tryby wyswietlania wynikow: komentarze tekstowe, plik Tab Separated ASCII */
/* zapis do bazy danych, zapis do segmentu dzielonej pamieci */

#define SHW_TXT 0x0001
#define SHW_TSA 0x0002
#define SHW_DBF 0x0004
#define SHW_SHM 0x0008

typedef struct _SterData		/* dane o parametrach sterujacych*/
 {
  tMsgSetParam msg;			/* rekord dla kolejki sterujacej */
  float *src;				/* zrodlo danych */
  float modif;				/* mnoznik */
  unsigned char status;			/* stan transmisji parametru */
 } tSterData;

unsigned short Diags=0;			/* poziom diagnostyczny */

short Tzew[162];			/* bufory parametrów wej¶ciowych */
short Twy[144];
short Twyx[144];
short Tpow[144];
short Qwy[288];
short Qwyx[288];
short Gwy[288];
short DP[144];

short Pobor_CWU[7][145];    /* tygodniowy rozk³ad + ¶redni dobowy pobór CWU */
float Moc_CWU[12];	 /* miesiêczne moce CWU odpowiadaj±ce 100% rozk³adu */	

typedef struct _Kloc		/* Struktury do obliczania akumulacji */
 {
  float		V;              /* objetosc kloca */
  float		T;              /* temperatura kloca */
  struct _Kloc  *next;
 } tKloc, *pKloc;

typedef struct _Rura
 {
  float         Vc;             /* calkowita pojemnosc rury */
  float         Vs;             /* suma objetosci klocow w rurze */
  float         E;              /* aktualna energia zmagazynowana w rurze */
  pKloc         in;             /* wlot rury */
  pKloc         out;            /* wylot rury */
 } tRura, *pRura;

typedef struct _KocData
 {
  ushort Qmin;
  ushort Qk;
  ushort Qmax;
  float modif;
 } tKocData, *pKocData;

typedef struct _ParData		/* dane o parametrach usrednianych */
 {
  ushort ind;			/* index w tablicy PTT */
  ushort len;			/* dlugosc bufora usrednien*/
  short last;			/* index ostatniej danej przed dziura */
  short *buf;			/* bufor usrednien */
 } tParData; 

typedef struct _AvgData		/* dane o srednich */
 {
  unsigned char mode;		/* DO_AVG usrednianie, DO_SUM sumowanie */
  ushort len;			/* okno sredniej */
  float modif;			/* mnoznik wartosci wyliczonej */
  short *src;			/* pierwszy parametr bufora */
  float *dst; 			/* adres przeznaczenia */
 } tAvgData; 

typedef struct _CiepProfile	/* dane o ciep³owni */
 {
  short zms;            /* poczatkowy miesiac sezonu zimowego */
  short zds;            /* poczatkowy dzien sezonu zimowego */
  short zme;            /* koncowy miesiac sezonu zimowego */
  short zde;            /* koncowy dzien sezonu zimowego */
  unsigned char kotlow;	/* ile kotlow ma cieplownia */
  float kmin;		/* minimalny wspó³czynnik parametrów P1..Pn */
  float kmax;           /* maksymalny wspolczynnik parametrow P1..Pn */
  float dQneg;		/* przedzia³ zmian k0 przy ujemnej korekcie */
  float dQpos;		/* przedzia³ zmian k0 przy dodatniej korekcie */
  float Q0;             /* moc cieplowni dla temperatury zewnetrznej 0 */
  float Q5;             /* moc cieplowni dla temperatury zewnetrznej 5 */
  float Q10;		/* moc cieplowni dla temperatury zewnetrznej 10 */
  float Vzas;		/* pojemosc rury zasilajacej T */
  float Vpow;		/* pojemnosc rury powrotnej T */
  float Ea23_l;		/* akumulacja na 23 w lecie */
  float Ea23_0;		/* ¶rednia akumulacja w zimie dla 0 */
  float Ea23_5;		/* ¶rednia akumulacja w zimie dla 5 */
  float Ea23_10;	/* ¶rednia akumulacja w zimie dla 10 */
  float DP0;		/* poziom normalny ci¶nienia dyspozycjnego */
  float DPmin;		/* minimum ci¶nienia dyspozycyjnego dla akumulacji */
  float DPmax;		/* maximum ci¶nienia dyspozycyjnego dla akumulacji */
  float Tpfault;	/* maxymalny procent b³êdu temperatury powrotu */
  ushort show;		/* tryb pokazywania wynikow */
 } tCiepProfile;

typedef struct _In	/* parametry wej¶ciowe */
 {
  MyDate date;		/* aktualna data kroku sterowania*/
  short t;		/* aktualna 10-cio minuta */ 
  unsigned char zima;	/* czy jest sezon zimowy */
  float Tzas;		/* aktualna temperatura zasilania */
  float Tpow;		/* aktualna temperatura powrotu */
  float Tzew;		/* aktualna temperatura zewnetrzna */
  float Gwy;		/* aktualny przeplyw wyjsciowy */
  float Tz3;		/* srednia Tzewn z ostatnich 3-ech godzin */
  float Tz24;		/* srednia Tzewn z ostatnich 24-ech godzin */
  float Tz27;		/* Tzewn z ostatnich 3-ech godzin poprz doby */
  float Twy;		/* srednia Twy z ostatnich 24-ech godzin */
  float Twyx;		/* srednia Twy z tabeli z ostatnich 24-ech godzin */
  float Gwy24;		/* sredni przeplyw z ostatnich 24-ech godzin */
  float Gwy48;		/* sredni przeplyw z poprzednich 24-ech godzin */
  float Sz24;		/* suma temperatur zewnetrznych 24-godzin */
  float E24;		/* energia wyprodukowana w ostatnich 24 godzinach */
  float E48;		/* energia wyprodukowana w poprzednich 24 godzinach */ 
  float Ex24;		/* energia wymagana w ostatnich 24 godzinach */ 
  float Ex48;		/* energia wymagana w poprzednich 24 godzinach */
  float Eaz;		/* energia zakumulowana po stronie zasilania */
  float Eap;		/* energia zakumulowana po stronie powrotu */
  float Eax;		/* energia zakumulowana w danej chwili */
  float Ea23;		/* energia do zakumulowania na godzine 23 */
  float Qks;		/* aktualna moc cieplowni suma kotlow */ 
  float Qmin;		/* minimalna dozwolona suma mocy kotlow */
  float Qmax;		/* maxymalna dozwolona suma mocy kotlow */
  float DP;		/* aktualne ci¶nienie dyspozycyjne */
 } tIn;

typedef struct _Out	/* parametry wyj¶ciowe */
 {
  float P1;		/* Tzew3-Tzew24 */
  float P2;		/* Tzew3-Tzew27 */
  float P3;		/* Twyx24-Twy24 */
  float P4;		/* zadany przebieg akumulacji */
  float P5;		/* obni¿enie nocne, podwy¿ka popo³udniowa */
  float P6;		/* ró¿nica energii za poprzednie 24 goziny */
  float k[MAX_P+1];	/* wspó³czynniki wzmocnienia P1..Pn, k[0] wspólny */
  float Tster;		/* temperatura sterujaca dla Qco */
  float Qco;		/* wyliczone Qco */
  float Qcwu;		/* wyliczone Qcwu */
  float Qc;		/* wyliczone Q cieplowni */
  float Txpow;		/* prognoza powrotu z obliczeñ akumulacji */
 } tOut;

tIn In={{1994, 1, 1, 1, 1, 0, 0}, 	/* date */	
        0,				/* t */
        0,				/* zima */
        0.0,				/* Tzas */
        0.0,				/* Tpow */
        0.0,				/* Tzew */
        0.0,				/* Gwy */
        0.0,				/* Tz3 */
        0.0,				/* Tz24 */
        0.0,				/* Tz27 */
        0.0,				/* Twy */
        0.0,				/* Twyx */
        0.0,				/* Gwy24 */
        0.0,				/* Gwy48 */
        0.0,				/* Sz24 */
        0.0,				/* E24 */
        0.0,				/* E48 */
	0.0,				/* Ex24 */
	0.0,				/* Ex48 */
	0.0,				/* Eaz */
	0.0,				/* Eap */
	0.0,				/* Eax */
	0.0,				/* Ea23 */
        20000.0,			/* Qks */
        5000.0,				/* Qmin */
        70000.0,			/* Qmqx */
	0.0};				/* DP */

tOut Out={0.0,						/* P1 */
	  0.0,						/* P2 */
	  0.0,						/* P3 */
          0.0,						/* P4 */
          0.0,						/* P5 */
          0.0,						/* P6 */
          {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0},		/* k[] */
          0.0,						/* Tster */
          50000.0,					/* Qco */
          0.0,						/* Qcwu */
          50000.0, 					/* Qc */
          0.0};						/* Txpow */

tCiepProfile Prf={10,			/* zms */ 
		  15, 			/* zds */
		  3, 			/* zme */
		  15, 			/* zde */
		  4, 			/* kotlow */
		  1.0,			/* kmin */
 		  5.0, 			/* kmax */
		  0.1,			/* dQneg */
		  0.4,			/* dQpos */
	 	  45000.0,		/* Q0 */
 		  35000.0, 		/* Q5 */
		  25000.0, 		/* Q10 */
   		  2250.0,		/* Vzas */
                  2250.0, 		/* Vpow */
		  271000.0,		/* Ea23_l */
		  328860.0,		/* Ea23_0 */
		  281880.0,		/* Ea23_5 */
		  271000.0,		/* Ea23_10 */
		  0.0,			/* DP0 */
		  0.0,			/* DPmin */
		  0.0,			/* DPmax */
		  0.1,			/* Tpfault */
		  SHW_TXT};		/* show */

tParData ParData[MAX_PAR]={{3,162,-1,&Tzew[0]},	/* program buforów */
			   {1,144,-1,&Twy[0]},
			   {2,144,-1,&Twyx[0]},
			   {0,144,-1,&Tpow[0]},
			   {4,288,-1,&Qwy[0]},
			   {6,288,-1,&Gwy[0]},
			   {4,288,-1,&Qwyx[0]}, /* parametr wyliczany */ 
			   {0,144,-1,&DP[0]},
			  };

tAvgData AvgData[MAX_AVG]={{DO_AVG,18,0.1,&Tzew[0],&In.Tz27},	/* ¶rednie */
			   {DO_AVG,144,0.1,&Tzew[18],&In.Tz24},
			   {DO_AVG,18,0.1,&Tzew[144],&In.Tz3},
			   {DO_AVG,144,0.1,&Twy[0],&In.Twy},
			   {DO_AVG,144,0.1,&Twyx[0],&In.Twyx},
			   {DO_AVG,144,1,&Gwy[0],&In.Gwy48},
			   {DO_AVG,144,1,&Gwy[144],&In.Gwy24},
			   {DO_SUM,144,0.1,&Tzew[18],&In.Sz24},
			   {DO_SUM,144,10.0/6.0,&Qwy[0],&In.E48},
			   {DO_SUM,144,10.0/6.0,&Qwy[144],&In.E24},
			   {DO_SUM,144,10.0/6.0,&Qwyx[0],&In.Ex48},
			   {DO_SUM,144,10.0/6.0,&Qwyx[144],&In.Ex24},
			   {DO_AVG,3,0.001,&DP[141],&In.DP},
			   {DO_SUM,1,0.1,&Tzew[161],&In.Tzew},
			   {DO_SUM,1,0.1,&Twy[143],&In.Tzas},
			   {DO_SUM,1,0.1,&Tpow[143],&In.Tpow},
			   {DO_SUM,1,1,&Gwy[287],&In.Gwy}};

tSterData SterData[MAX_STER]={{{
				0L+'1',   /* numer linii*256 + kod jednostki */
				{0,0}},		/* adres i warto¶æ parametru */
				&Out.Qc,	/* ¼ród³o sterowania */
				0.1,		/* modyfikator */
				MSG_OK}}; 	/* stan komunikatu */

tKocData KocData[MAX_KOC]={{NO_PARAM,4,NO_PARAM,10.0},	   /* moce ciep³owni */
			   {NO_PARAM,12,NO_PARAM,10.0},	   /* moce kot³ow */	
                     	   {NO_PARAM,17,NO_PARAM,10.0},
			   {NO_PARAM,22,NO_PARAM,10.0},
 		           {NO_PARAM,41,NO_PARAM,10.0}};

void FindFAD(ushort addr, MyDate *FAD)		/* Cache FAD */
 {
  #define FAD_ALLOC 3
  typedef struct _FADCache
           {
	    ushort addr;
	    MyDate FAD;
	   } tFADCache, *pFADCache;
  static ushort FADCacheSize=0;
  static ushort FADCacheFree=0;
  static pFADCache FADCache=NULL;
  ushort i;

  if (FADCache==NULL)
   {
    FADCache=calloc(FAD_ALLOC, sizeof(tFADCache));
    FADCacheSize=FAD_ALLOC;
    if (Diags>=9) 
      printf("FADCache initialized, buffer size:%d\n", FADCacheSize);
   }
  else if (FADCacheSize==FADCacheFree)
   {
    FADCacheSize+=FAD_ALLOC;
    FADCache=realloc(FADCache, FADCacheSize*sizeof(tFADCache));
    if (Diags>=9)
      printf("FADCache reallocated, new buffer size:%d\n", FADCacheSize);
   } 
  for (i=0;i<FADCacheFree;i++)
    if (FADCache[i].addr==addr)
     {
      *FAD=FADCache[i].FAD;
      if (Diags>=9)
        printf("FADCache hit: %d.%02d.%02d:%02d:%02d \n", 
	       FAD->year, FAD->month, FAD->mday, FAD->hour, FAD->min);
      return;
     }
  GetFirstAvailableDate(addr/NUMBEROFPROBES+1, addr%NUMBEROFPROBES, FAD);
  FADCache[FADCacheFree].addr=addr;
  FADCache[FADCacheFree++].FAD=*FAD;
  if (Diags>=9)
    printf("FADCache miss, buffer size:%d, occupied:%d\n", FADCacheSize,
	 FADCacheFree);
 }

short dbfGetMin10(ushort addr)		/* Próbka z bazy adres uniwersalny */
 {
  short res;
  int rec, fld;
  MyDate Date, FAD;
  if (addr==NO_PARAM)
    return(SZARP_NO_DATA);
  Date=In.date;
  rec=addr/NUMBEROFPROBES+1;
  fld=addr%NUMBEROFPROBES;
  FindFAD(addr, &FAD); 
/*  GetFirstAvailableDate(rec, fld, &FAD); */
  if (Diags>=8)
    printf("Request record: %d, field: %d from %d.%02d.%02d:%02d:%02d\n",
           rec, fld, Date.year, Date.month, Date.mday, Date.hour, Date.min);
  res=ParticularValue(&Date, FAD, DZIEN, rec, fld, CENTER, NULL);
  if (Diags>=8)
    printf("Got: %d\n", res);
  if (res==SZARP_NO_DATA)
    return(SZARP_NO_DATA);
  return(res);
 } 
  
short GetMin10(ushort addr)	/* Próbka z pamiêci dzielonej adres uniwers */
 {
  if (addr==NO_PARAM)
    return(SZARP_NO_DATA);
#if #TIME_STEP(REAL_TIME)
  return(Min10[PTT.tab[addr].addr]);
#else  
  return(dbfGetMin10(addr));
#endif
 }

void GetAvgs()		/* uzupelnij bufory srednich raz na 10 minut */
 {
  ushort i;
  ushort ii;
  short sval;
  for(i=0;i<MAX_PAR;i++)
    for(ii=0;ii<(ushort)(ParData[i].len-1);ii++)
      ParData[i].buf[ii]=ParData[i].buf[ii+1];
#if #TIME_STEP(REAL_TIME)
  ipcRdGetInto(SHM_MIN10);
#endif
  for(i=0;i<MAX_PAR;i++)
   {
    if ((sval=GetMin10(ParData[i].ind))==SZARP_NO_DATA)
      sval=ParData[i].buf[ParData[i].len-2];
    ParData[i].buf[ParData[i].len-1]=sval;
   }
#if #TIME_STEP(REAL_TIME)
  ipcRdGetOut(SHM_MIN10);
#endif
 }

void GetKocs()		/* Pobierz moce min, akt i max sumy kot³ów */
 {			/* Je¿eli brak sk³adników u¿yj mocy ciep³owni */
  unsigned char i;
  short sval;
  float Qks,Qmin,Qmax;
  Qks=Qmin=Qmax=0.0;
#if #TIME_STEP(REAL_TIME)
  ipcRdGetInto(SHM_MIN10);
#endif  
  for(i=1;i<=Prf.kotlow;i++)			/* najpierw moc aktualna */
   {
    if (KocData[i].Qk==NO_PARAM)		/* kocio³ nieczynny */
      continue;
    if ((sval=GetMin10(KocData[i].Qk))!=SZARP_NO_DATA)
      Qks+=(float)sval*KocData[i].modif;	/* s± dane ze sterownika */
    else
     { 
      if ((sval=GetMin10(KocData[0].Qk))!=SZARP_NO_DATA)
        Qks=(float)sval*KocData[0].modif;	/* moc cieplowni przybliza */
      else
        Qks=In.Qks;				/* brak danych uzyj */ 
      break;					/* poprzedniej warto¶ci */
     }
   }
  for(i=1;i<=Prf.kotlow;i++)			/* moc minimalna */
   {
    if (KocData[i].Qk==NO_PARAM)                /* kocio³ nieczynny */
      continue;					/* decyduje moc akt a nie min */
    if ((sval=GetMin10(KocData[i].Qmin))!=SZARP_NO_DATA)
      Qmin+=(float)sval*KocData[i].modif;	/* s± dane ze sterownika */
    else
     { 
      if ((sval=GetMin10(KocData[0].Qmin))!=SZARP_NO_DATA)
        Qmin=(float)sval*KocData[0].modif;	/* moc cieplowni przybliza */
      else
        Qmin=In.Qmin;				/* brak danych uzyj */
      break;					/* poprzedniej warto¶ci */
     }
   }
  for(i=1;i<=Prf.kotlow;i++)
   {
    if (KocData[i].Qk==NO_PARAM)                /* kocio³ nieczynny */
      continue;                                 /* decyduje moc akt a nie max */
    if ((sval=GetMin10(KocData[i].Qmax))!=SZARP_NO_DATA)
      Qmax+=(float)sval*KocData[i].modif;	/* s± dane ze sterownika */
    else
     { 
      if ((sval=GetMin10(KocData[0].Qmax))!=SZARP_NO_DATA)
        Qmax=(float)sval*KocData[0].modif;	/* moc cieplowni przybliza */
      else
        Qmax=In.Qmax;				/* brak danych uzyj */
      break;					/* poprzedniej warto¶ci */
     }
   }
#if #TIME_STEP(REAL_TIME)
  ipcRdGetOut(SHM_MIN10);
#endif
  In.Qks=Qks;
  In.Qmin=Qmin;
  In.Qmax=Qmax;       
 }   

void GetPars()
 {
  GetAvgs();		/* pobranie parametrów do buforów */
  GetKocs();		/* pobranie danych o kot³ach */
 }

void comp_Avgs()	/* wyliczenie ¶rednich wg programu */
 {
  ushort i;
  ushort ii;
  long sum;
  for(i=0;i<MAX_AVG;i++)
   {
    sum=0;
    for(ii=0;ii<AvgData[i].len;ii++)
      sum+=AvgData[i].src[ii];
    if (AvgData[i].mode==DO_SUM)
      *(AvgData[i].dst)=(float)sum*AvgData[i].modif;
    else
      *(AvgData[i].dst)=(float)sum/(float)AvgData[i].len*AvgData[i].modif;
   }
 }

float TabelaCO_10_5_0(float T)
 {
  return(T>10.0?Prf.Q10:T>5.0?(Prf.Q10-Prf.Q5)/5.0*(T-5.0)+Prf.Q5:
	 (Prf.Q5-Prf.Q0)/5.0*T+Prf.Q0);
 }

float comp_Odbior(MyDate *dt, float Tzew)	/* oblicz odbiór ciep³a */
 {						/* od daty i temp zewn */
  float res;

  res=(float)Pobor_CWU[dt->wday-1][dt->hour*6+dt->min/10]/6000.0*
      Moc_CWU[dt->month-1]/6.0;
  if (!In.zima)					/* latem tylko pobór CWU */
    return(res);
  res+=TabelaCO_10_5_0(Tzew)/6.0;			/* zim± jeszcze CO */
  return(res);
 }
    
float comp_P1()				/* parametr steruj±cy P1 */
 {
  return(In.Tz3-In.Tz24);		/* 3-godziny a 24-godziny */
 }

float comp_P2()				/* parametr steruj±cy P2 */
 {					/* ostatnie 3-godziny a  */
  return(In.Tz3-In.Tz27);		/* 3-godziny z poprzedniej doby */
 }

float comp_P3()				/* parametr steruj±cy P3 */
 {					/* ¦rednia 24-h na sieæ zadana */
  return(In.Twyx-In.Twy);		/* a rzeczywisto¶æ = nidotrzymanie */
 }

float comp_P4()				/* parametr steruj±cy P4 */
 {					/* zadany przebieg akumulacji */
  float res, DP;
  float P4A, P4B;
  
  res=In.t<48?5.0:10.0*fabs(In.t-96.0)/48.0-5.0;	/* zadany przebieg */
  if (In.Tz24<5.0)				 /* zadana warto¶æ ¶rednia */	
    In.Ea23=Prf.Ea23_0+(Prf.Ea23_5-Prf.Ea23_0)/5.0*In.Tz24; 
  else if (In.Tz24<10.0) 
    In.Ea23=Prf.Ea23_5+(Prf.Ea23_10-Prf.Ea23_5)/5.0*(In.Tz24-5.0);
  else
    In.Ea23=Prf.Ea23_10;
  P4A=(In.Ea23-In.Eax)/(Prf.Vzas+Prf.Vpow)/C_J2W;   /* ró¿nica bezpo¶rednio */
  DP=In.DP-Prf.DP0;
  DP=DP<Prf.DPmin?Prf.DPmin:DP;		/* ró¿nica przez ci¶nienie dyspoz */
  DP=DP>Prf.DPmax?Prf.DPmax:DP;
  P4B=-5.0+10.0/(Prf.DPmax-Prf.DPmin)*(In.DP-Prf.DPmin);

  /* !!!!!! UWAGA !!!!!! */
  /* Do celów testowych zawsze u¿ywamy akumulacji obliczanej klocami */
  /* Sterownik zawsze pracuje od ci¶nienia dyspozycyjnego */
  /* !!!!!! KONIEC !!!!! */ 
  
  if (1||fabs((In.Tpow-Out.Txpow)/In.Tpow)<Prf.Tpfault)
    res-=P4A;		/* aproxymacja akumulacji jest rozs±dna */
  else
    res-=P4B;	/* zbyt du¿y b³±d w akumulacji u¿ywamy DP jak w  sterowniku */
  return(res);
 }

float comp_P5()					/* parametr steruj±cy P5 */ 
 {			/* nocne ziêbienie i szczytowe podgrzanie ruroci±gu */
  return(In.t<48?1.0:In.t<72?0.0:In.t<120?-1.0:0.0);
 }

float comp_P6()				/* parametr steruj±cy P6 */
 {				/* bilans energetyczny przedostatniej doby */
  return((In.E48-In.Ex48)/In.Gwy48/48.0);
 }

void comp_k0(float dQ)	/* wspólny wspó³czynnik wzmocnienia parametrów */
 {
  float Q;
  if (In.Qks<In.Qmin+dQ*(In.Qmax-In.Qmin))
    Out.k[0]=(Prf.kmax-Prf.kmin)/(dQ*(In.Qmax-In.Qmin))*(In.Qks-In.Qmin)+
              Prf.kmin;
  else if (In.Qks<In.Qmax-dQ*(In.Qmax-In.Qmin))
    Out.k[0]=Prf.kmax;
  else
    Out.k[0]=-(Prf.kmax-Prf.kmin)/(dQ*(In.Qmax-In.Qmin))*(In.Qks-In.Qmax)+
              Prf.kmin;
 }

void comp_k()		/* wspó³czynniki wzmocnienia parametrów */
 {
  float SumP;
  short dTz2, dTz3;
  
  dTz2=abs(Tzew[150]-Tzew[161]);	/* zmiana temperatury za 2 godz */
  dTz3=abs(Tzew[142]-Tzew[161]);	/* zmiana temperatury za 3 godz */
  if (dTz2>20||dTz3>30)
    Out.k[1]=Out.k[2]=2.0;  /* gwa³towna zmiana Tzew wzmocniæ param pogodowe */
  else
    Out.k[1]=Out.k[2]=1.0;				/* praca normalna */
  SumP=Out.P1*Out.k[1]+Out.P2*Out.k[2]+Out.P3*Out.k[3]+Out.P4*Out.k[4]+
       Out.P5*Out.k[5]+Out.P6*Out.k[6];
  if (In.Qks<In.Qmin||In.Qks>In.Qmax)
    Out.k[0]=Prf.kmin;				/* poza zakresem  mocy */
  else if (SumP>0.0)
    comp_k0(Prf.dQpos);	  /* dodatnia poprawka zmniejszamy ³agodnie moc */
  else	 
    comp_k0(Prf.dQneg);   /* ujemna poprawka ostro zwiêkszamy moc */
 }

void MyCO()		/* Wydajnosc sterujaca na potrzeby C.O. */
 {
  Out.P1=comp_P1();
  Out.P2=comp_P2();
  Out.P3=comp_P3();
  Out.P4=comp_P4();
  Out.P5=comp_P5();
  Out.P6=comp_P6();
  comp_k();
 
  Out.Tster=(In.Sz24+(Out.k[1]*Out.P1+Out.k[2]*Out.P2+Out.k[3]*Out.P3+
	     Out.k[4]*Out.P4+Out.k[5]*Out.P5+Out.k[6]*Out.P6)*
             Out.k[0])/144.0;
  Out.Qco=TabelaCO_10_5_0(Out.Tster);
 }

float diff23_10(short t)	/* ile 10-minut do 23:00 nie mniej niz 6 */
 {
  if (t<22*6)
    return(23.0*6.0-(float)t);
  if (t<23*6)
    return(6.0);
  return(47.0*6.0-(float)t);
 }

void MyCWU()			/* wydajnosc sterujaca na potrzeby CWU */
 {
  Out.Qcwu=(float)Pobor_CWU[In.date.wday-1][AVG_IND]/1000.0*
           Moc_CWU[In.date.month-1];
  if (In.zima)
    return;
  In.Ea23=Prf.Ea23_l;		/* cwilowo sta³a zadaj±ca akumulacjê na 23 */
  Out.Qcwu+=((float)(Pobor_CWU[In.date.wday-1][In.t]-
            Pobor_CWU[In.date.wday-1][AVG_IND]))/1000.0*
	    Moc_CWU[In.date.month-1]-(In.Eax-In.Ea23)/diff23_10(In.t);
 }

void MyCiep()		/* wydajnosc sterujaca cieplowni */ 
 {			/* moze ewentualne modyfikacje od strat */
  MyCWU();
  Out.Qc=Out.Qcwu;
  if (In.zima)
   {
    MyCO();
    Out.Qc+=Out.Qco;
   }
 }

void Nowa_rura(pRura rura, float V, float T)
 {
  pKloc tmp;				/* inicjalizacja rury */
					/* o energii okre¶lonej przez V i T */
  tmp=malloc(sizeof(tKloc));		/* rura stanowi jednokierunkow± */ 
  rura->Vc=V;				/* listê klocy */
  tmp->V=rura->Vc;			/* in wska¼nik na ostatni kloc listy */
  tmp->T=T;				/* out wska¼nik na pierwszy kloc listy*/
  tmp->next=NULL;			/* in wej¶cie do rury */
  rura->Vs=rura->Vc;			/* out wyj¶cie z rury */
  rura->E=C_J2W*V*T;
  rura->in=rura->out=tmp;
 }

float Z_rury(pRura rura, pKloc kloc)	/* wyjêcie z rury kloca o okre¶lonej */
 {					/* objêto¶ci */
  pKloc tmp;				/* ustalenie energii niesionej przez */
  float wyciek;				/* kloc oraz temperatury kloca */
  float Eout;

  wyciek=kloc->V;			/* tyle zabieramy */
  rura->Vs-=kloc->V;			/* Vs gumowa objêto¶æ rury siê kurczy */
  Eout=0;
  while(wyciek>0)			/* wyci±gamy tyle klocy ile trzeba */
   {					/* by zabraæ okre¶lon± objêto¶æ */
    if (wyciek<rura->out->V)		/* co wiêksze ubytek czy kloc */
     {
      rura->out->V-=wyciek;		/* koniec ubytku wystarczy zmniejszyæ */
      Eout+=C_J2W*wyciek*rura->out->T;	/* ostatni kloc */
      wyciek=0;
     }
    else
     {
      wyciek-=rura->out->V;		/* ubytek wiêkszy ni¿ kloc wiêc go */
      Eout+=C_J2W*rura->out->V*rura->out->T;	/* wyrzucamy */
      tmp=rura->out;
      rura->out=rura->out->next;
      free(tmp);
     }
   }
  rura->E-=Eout;			/* zabrali¶my kloce to spad³a energia */
  if (kloc->V>0)			/* rury */
    kloc->T=Eout/kloc->V/C_J2W;		/* ustalamy temperaturê zabranego */
  else					/* kloca */
    kloc->T=kloc->V=0;			/* koc bez objêto¶ci nie ma */
  return(Eout);				/* zwracamy pobran± energiê */
 }

float W_rure(pRura rura, pKloc kloc)	/* w³o¿enie do rury kloca o */ 
 {					/* okre¶lonych T i V */
  pKloc tmp;				/* wylanie nadmiaru wody z rury */
  float Eout;
  float wyciek;

  tmp=malloc(sizeof(tKloc));		/* tworzymy kloc dla rury */
  *tmp=*kloc;
  rura->Vs+=kloc->V;			/* rozci±gamy gumow± objêto¶æ Vs */
  rura->E+=C_J2W*kloc->V*kloc->T;		/* zwiêkszamy energiê rury */
  if (rura->in==NULL)
    rura->in=rura->out=tmp;		/* pierwszy kloc w rurze */
  else
   {
    rura->in->next=tmp;			/* kolejny kloc */
    rura->in=tmp;
   }
  Eout=0;
  kloc->V=wyciek=rura->Vs-rura->Vc;	/* obliczenie nadmiaru wody w rurze */
  if (wyciek>0)				/* zrównanie objêto¶ci gumowej do */
    rura->Vs-=wyciek;			/* objêto¶ci nominalnej rury */ 
  while(wyciek>0)			/* trzeba odebraæ nadmiar wody i */
   {					/* obliczyæ jak± energiê niesie */
    if (wyciek<rura->out->V)
     {
      rura->out->V-=wyciek;		/* zmniejszenie objêto¶ci ostatniego */
      Eout+=C_J2W*wyciek*rura->out->T;	/* kloca wystarczy do odebrania */
      wyciek=0;				/* wycieku */
     }
    else
     {
      wyciek-=rura->out->V;		/* wyrzucenie ostatniego kloca */
      Eout+=C_J2W*rura->out->V*rura->out->T;	/* aby zrównaæ objêto¶ci */
      tmp=rura->out;
      rura->out=rura->out->next;
      free(tmp);
     }
   }
  rura->E-=Eout;			/* zabrano objêto¶æ trzeba te¿ zabraæ */
  if (kloc->V>0)			/* energiê */
    kloc->T=Eout/kloc->V/C_J2W;		/* wstawienie kloca V0,T0 w rurê */
  else					/* powoduje wyciek kloca V1,T1 z rury */
    kloc->T=kloc->V=0;
  return(Eout);				/* energia wyci¶niêta z rury */
 }

void Niszcz_rure(pRura rura)		/* zwolnienie pamiêci u¿ytej przez */
 {					/* rurê */
  pKloc ptr;
  
  while (rura->out)
   {
    ptr=rura->out;
    rura->out=ptr->next;
    free(ptr);
   }
 }

void comp_Accu()
 {
  static tRura Rzas, Rpow;
  static tKloc Kwe, Kwy;
  static unsigned char running=0;
  float Ewe, Ewy, Eodb;
  int i;
  MyDate newdate;

  if (!running)			/* inicjalizacja */
   {
    running=1;
    Kwe.next=Kwy.next=NULL;
    Kwe.T=0.0;
    Kwe.V=0.0;
    Ewe=0.0;
    for (i=143;i>0;i--)		/* wype³niamy rurê zasilaj±c± bior±c tyle */
     {				/* ostatnio wyprodukowanych w ciep³owni klocy */
      Ewe+=C_J2W*(float)Twy[i]*(float)Gwy[144+i]/60.0;	/* ile siê zmie¶ci */
      if ((Kwe.V+=(float)Gwy[144+i]/6.0)>Prf.Vzas)
        break;						/* rura ju¿ pe³na */
     }
    if (Diags>=7)
      printf("wykonano %d krokow, zmagazynowano %0.0f kW\n", 143-i, Ewe);
    Kwe.V-=Prf.Vzas;				/* a tyle siê wyla³o wody */
    Kwe.T=(float)Twy[i]/10.0;			/* o takiej temperaturze */
    Ewy=C_J2W*Kwe.V*Kwe.T;			/* taka energia wysz³a */
    Ewe-=Ewy;				/* w rurze zosta³o tyle energii */
    Kwe.T=Ewe/Prf.Vzas/C_J2W;			/* rura ma tak± temperaturê */
    Nowa_rura(&Rzas, Prf.Vzas, Kwe.T);		/* mo¿na stworzyæ rurê */
    if (Diags>=7)
      printf("po poprawce zmagazynowano %0.0f kW\n", Ewe);

    ConvertToDate(&newdate, In.date, DZIEN, 0, 144-i);		/* data kloca */
    Ewe=Ewy;				/* taka energia wysz³a z rury zasil */
    Kwy.V=(float)Gwy[144+i]/6.0; 	/* objêto¶æ ca³ego dzielonego kloca */
    Ewe-=Kwe.V/Kwy.V*comp_Odbior(&newdate, (float)Tzew[18+i]/10.0);	
    if (Ewe>0&&Kwe.V>0)			/* zmniejsz energiê czê¶ci kloca */
      Kwe.T=Ewe/Kwe.V;			/* o proporcjonaln± czê¶æ poboru */
    else 				/* co¶ zosta³o oblicz temperaturê */
      Kwe.T=Kwe.V=Ewe=0.0;		/* nie to zapomnij o tym klocuszku */

    for (i--;i>0;i--)			/* wype³niamy rurê powrotn± klocami */
     {					/* z ciep³owni pomniejszonymi o pobór */
      ConvertToDate(&newdate, In.date, DZIEN, 0, 144-i);
      Ewe+=C_J2W*(float)Twy[i]*(float)Gwy[144+i]/60.0;
      Ewe-=comp_Odbior(&newdate, (float)Tzew[18+i]/10.0);
      if ((Kwe.V+=(float)Gwy[144+i]/6.0)>Prf.Vzas)
        break;						/* rura ju¿ pe³na */
     }
    if (Diags>=7)
      printf("wykonano %d krokow, zmagazynowano %0.0f kW\n", 143-i, Ewe);
    Kwy.V=(float)Gwy[144+i]/6.0;	/* klocek który siê nie mie¶ci */
    Kwy.T=(float)Twy[i]/10.0;
    Ewy=C_J2W*Kwy.V*Kwy.T-comp_Odbior(&newdate, (float)Tzew[18+i]/10.0);
    Ewe-=(Kwe.V-Prf.Vpow)/Kwy.V*Ewy;		/* zmniejszamy energiê o to */
    Kwe.T=Ewe/Prf.Vpow/C_J2W;				/* co siê wyla³o */
    Nowa_rura(&Rpow, Prf.Vpow, Kwe.T);	/* robimy now± rurê powrotn± */
    if (Diags>=7)
     {    
      printf("wykonano %d krokow, zmagazynowano %0.0f kW\n", 143-i, Ewe);
      Ewe=Rzas.E+Rpow.E;
      printf("po inicjalizacji mamy zakumulowane %0.0f kW\n", Ewe);
     }
   }					/* wszyscy w domu zaczynamy */

  Kwe.V=In.Gwy/6.0;			/* ciep³ownia pobiera kloc z powrotu */
  Ewe=Z_rury(&Rpow,&Kwe);		/* zgodnie z aktualnym przep³ywem */
  Out.Txpow=Kwe.T;			/* temperatura tego kloca powinna */

  /* !!!!!! UWAGA !!!!!!! */
  /* sprawdziæ i je¿eli ¼le przyj±æ */  /* odpowiadaæ aktualnemu powrotowi */
  /* aktualne Twy jako ¶redni± zasilania a Tpow jako ¶redni± powrotu */
  /* !!!!!! KONIEC !!!!!! */

  Kwe.T=In.Tzas;			
  Ewy=W_rure(&Rzas, &Kwy);		/* wk³adamy w rurê bie¿±c± produkcjê */
  Ewy-=comp_Odbior(&In.date, In.Tzew);	/* odbieramy energiê z koñca rury */

  /* !!!!!! UWAGA !!!!!!! */
  /* nie zamra¿aæ rury przyj±æ poziom temperatuty poni¿ej którego nie wolno */
  /* wysysaæ energii z kloca */
  /* !!!!!! KONIEC !!!!!! */

  Kwe.T=Ewy/Kwy.V;		/* nowa temperatura kloca po zabraniu energii */
  Kwe.V=Kwy.V;			/* objêto¶æ w ca³ym cyklu ta sama */
  Ewy==W_rure(&Rpow, &Kwe);

  /* !!!!!! UWAGA !!!!!!! */
  /* je¿eli co¶ wysz³o z rury to dziwne, oznacza to niezachowanie bilansu */
  /* trzeba co najmniej alarmowaæ, albo i¶æ do lasu */
  /* !!!!!! KONIEC !!!!!! */
  
  In.Eaz=Rzas.E;		/* akumulacja po stronie zasilania */
  In.Eap=Rpow.E;		/* akumulacja po stronie powrotu */
  In.Eax=Rpow.E+Rzas.E;		/* no to obliczyli¶my zakumulowan± energiê */
 }

void SendSter()
 {
#if defined(FULL_CTRL)
  ushort i;
  for(i=0;i<MAX_STER;i++)
   {
    if (SterData[i].status==MSG_SEND)
     {
      SterData[i].msg.cont.value=rint(*(SterData[i].src)*SterData[i].modif);
      if (!msgsnd(MsgSetDes, &(SterData[i].msg), sizeof(tSetParam), IPC_NOWAIT))
        SterData[i].status=MSG_CONF;
     }
   }
#endif
 }

void ReadReply()
 {
#if defined(FULL_CTRL)
  ushort i,ii;
  tMsgSetParam msg;
  for(i=0;i<MAX_STER;i++)
   {
    if (SterData[i].status==MSG_CONF)
     {
      if (msgrcv(MsgRplyDes, &msg, sizeof(tSetParam), SterData[i].msg.type,
	  IPC_NOWAIT)==sizeof(tSetParam))
        for(ii=0;ii<MAX_STER;ii++)
         {
          if (SterData[ii].msg.cont.param==msg.cont.param&&
              SterData[ii].msg.cont.value==msg.cont.value)
            SterData[ii].status=MSG_OK; 
         }
     }
   }
#endif
 }

void ReportStatistics()
 {
  ushort i;
  for(i=0;i<MAX_STER;i++)
    switch(SterData[i].status)
     {
      case MSG_SEND:printf("Message %ld with param: %d value: %d not send\n",
  			   SterData[i].msg.type, SterData[i].msg.cont.param,
			   SterData[i].msg.cont.value);
		    break;
      case MSG_CONF:printf("Message %ld with param: %d value: %d not confirmed\n",
                           SterData[i].msg.type, SterData[i].msg.cont.param,
                           SterData[i].msg.cont.value);
                    break;
      default:break;
     }
 }

void ShowTXT()
 {
  char *Week[7]={"Niedziela", "Poniedzia³ek", "Wtorek", "¦roda", "Czwartek", 
 		 "Pi±tek", "Sobota"};
  printf("\n\nProgram steruj±cy, stan dn. %02d.%02d.%d %s godz. %02d:%02d, sezon ",
         In.date.mday, In.date.month, In.date.year, Week[In.date.wday-1],
         In.date.hour, In.date.min);
  if (In.zima)
    printf("grzewczy\n");
  else
    printf("letni\n");
  printf("Krok: %d\n", In.t);
  printf("\nPARAMETRY WEJ¦CIOWE\n");
  printf("Tzew:%3.1f\tTzas:%3.1f\tTpow:%3.1f\tTpwx:%3.1f\tQodb:%0.0f\n",
	 In.Tzew, In.Tzas, In.Tpow, Out.Txpow, 
         6.0*comp_Odbior(&In.date,In.Tzew));
  printf("Tz3:%3.1f\tTz24:%3.1f\tTz27:%3.1f\tSz0_23:%6.1f\n", 
         In.Tz3, In.Tz24, In.Tz27, In.Sz24);
  printf("Twy:%3.1f\t\tTwyx:%3.1f\n", In.Twy, In.Twyx);
  printf("Gwy24:%3.2f\t\tGwy48:%3.2f\n", In.Gwy24, In.Gwy48);
  printf("E24:%0.0f\t\tE48:%0.0f\n", In.E24, In.E48);
  printf("Ex24:%0.0f\t\tEx48:%0.0f\n", In.Ex24, In.Ex48);
  printf("Eax:%0.0f\t\tEa23:%0.0f\n", In.Eax, In.Ea23);
  printf("Qks:%0.0f\t\tQmin:%0.0f\t\tQmax:%0.0f\n", In.Qks, In.Qmin, In.Qmax);
  printf("\nPARAMETRY STERUJ¡CE\n");
  printf("P1:%3.1f\t\tP2:%3.1f\t\tP3:%3.1f\n", Out.P1, Out.P2, Out.P3);
  printf("P4:%3.1f\t\tP5:%3.1f\t\tP6:%3.1f\n", Out.P4, Out.P5, Out.P6);
  printf("k:%2.2f\t\tTst:%3.1f\t\tQco:%0.0f\t\tQcwu:%0.0f\n",
         Out.k[0], Out.Tster, Out.Qco, Out.Qcwu);
  printf("¯±dana moc ciep³owni Qc:%0.0f kW\n", Out.Qc);
 }

void ShowTSA()
 {
 }

void ShowDBF()
 {
 }

void ShowSHM()
 {
 }

void ShowResults()
 {
  if (Prf.show&SHW_TXT)
    ShowTXT();
  if (Prf.show&SHW_TSA)
    ShowTSA();
  if (Prf.show&SHW_DBF)
    ShowDBF();
  if (Prf.show&SHW_SHM)
    ShowSHM();
 }

void Step10()
 {
  ushort i;
  time_t sectime;
  struct tm *lt;
  time(&sectime);
#if defined(FULL_CTRL)
  ReportStatistics();
  for(i=0;i<MAX_STER;i++)
    SterData[i].status=MSG_SEND;
#endif
#if #TIME_STEP(REAL_TIME)
  lt=localtime(&sectime);
  In.date.year=lt->tm_year+1900;
  In.date.month=lt->tm_mon+1;
  In.date.mday=lt->tm_mday;
  In.date.wday=lt->tm_wday+1;
  In.date.hour=lt->tm_hour;
  In.date.min=lt->tm_min;
#endif
  In.t=In.date.hour*6+In.date.min/10;
  if (In.date.month<Prf.zms&&In.date.month>Prf.zme)
    In.zima=0;
  else if (In.date.month==Prf.zms&&In.date.mday<=Prf.zds)
    In.zima=0;
  else if (In.date.month==Prf.zme&&In.date.mday>=Prf.zde)
    In.zima=0;
  else
    In.zima=1;
  Month2Week(&In.date);
  GetPars();
  Qwyx[287]=(short)rint(Out.Qc*SterData[0].modif);
  comp_Avgs();
  comp_Accu();
  MyCiep();
  ShowResults();
#if #TIME_STEP(REAL_TIME)
  for(i=0;i<10;i++)
   {
    SendSter();
    sleep(30-sectime%30);
    time(&sectime);
    ReadReply();
    sleep(30-sectime%30);
   }
#else
  ConvertToDate(&In.date, In.date, DZIEN, 1, 0);
  SendSter();
  sleep(TIME_STEP/2);
  ReadReply();
  sleep(TIME_STEP/2);
#endif
 }

void GetSymStartDate()
 {
  char buf[81];
  int yy, mo, dd, hh, mi;
  do
   {
    printf("Start date(yyyy:mm:dd:hh:mm):");
    fgets(buf, 81, stdin);
    sscanf(buf, "%4d:%2d:%2d:%2d:%2d", &yy, &mo, &dd, &hh, &mi);
   }
  while(yy<1991||yy>1994||mo<1||mo>12||dd<1||dd>31||hh<0||hh>23||mi<0||mi>59);
  In.date.year=yy;
  In.date.month=mo;
  In.date.mday=dd;
  In.date.hour=hh;
  In.date.min=mi;
 }

void GetStartDate()
 {
  time_t sectime;
  struct tm *lt;
  time(&sectime);  
  lt=localtime(&sectime);
#if #TIME_STEP(REAL_TIME) 
  In.date.year=lt->tm_year+1900;
  In.date.month=lt->tm_mon+1;
  In.date.mday=lt->tm_mday;
  In.date.wday=lt->tm_wday+1;
  In.date.hour=lt->tm_hour;
  In.date.min=lt->tm_min;
#else
  GetSymStartDate();
#endif 
  In.t=In.date.hour*6+In.date.min/10;
  if (In.date.month<Prf.zms&&In.date.month>Prf.zme)
    In.zima=0;
  else if (In.date.month==Prf.zms&&In.date.mday<=Prf.zds)
    In.zima=0;
  else if (In.date.month==Prf.zme&&In.date.mday>=Prf.zde)
    In.zima=0;
  else
    In.zima=1;  
 }

unsigned char ReadInCWU(char *fname)
 {
  FILE *CWUfile;
  ushort i,ii;
  CWUfile=fopen(fname, "r");
  for(i=0;i<7;i++)
   {
    for(ii=0;ii<144;ii++) 
     {
      fscanf(CWUfile,"%hu\t",&Pobor_CWU[i][ii]);
      if (Diags>=9)
        printf("Pobor_CWU[%d][%d]=%d\n",i,ii,Pobor_CWU[i][ii]);
     }
    fscanf(CWUfile,"%hu\n",&Pobor_CWU[i][ii]);
    if (Diags>=9)
      printf("Pobor_CWU[%d][%d]=%d\n",i,ii,Pobor_CWU[i][ii]);
   }
  for(i=0;i<11;i++)
   {
    fscanf(CWUfile,"%f\t",&Moc_CWU[i]);
    if (Diags>=7)
      printf("Moc_CWU[%d]=%0.0f\n",i,Moc_CWU[i]);
   }
  fscanf(CWUfile,"%f\n",&Moc_CWU[i]);
  if (Diags>=7)
    printf("Moc_CWU[%d]=%0.0f\n",i,Moc_CWU[i]);
  fclose(CWUfile);
 }

void sterInitialize()
 {
  MyDate tmpDate;
  ushort i, ii, j, ind;
  short yp, yk, dx, res;
  GetStartDate();
  tmpDate=In.date;
  if (Diags>=7)
    printf("Current date: %02d.%02d.%4d time: %02d:%02d\n", tmpDate.mday, 
	   tmpDate.month, tmpDate.year, tmpDate.hour, tmpDate.min);
  ConvertToDate(&In.date, In.date, DZIEN, 0, 288);
  if (Diags>=7)
    printf("Start date: %02d.%02d.%4d time: %02d:%02d\n", In.date.mday,
           In.date.month, In.date.year, In.date.hour, In.date.min);
  for(i=0;i<288;i++)
   {
    for(ii=0;ii<MAX_PAR;ii++)
     {
      if (i>=(ushort)(288-ParData[ii].len))
       {
        ind=i+ParData[ii].len-288;
        res=dbfGetMin10(ParData[ii].ind);
        if (res!=SZARP_NO_DATA)
         {
          if (Diags>=8)
            printf("time:%d index:%d value:%d\n", ind,
                   ii, res);
          ParData[ii].buf[ind]=res;
          for(j=ParData[ii].last+1;j<i;j++)
           {
            if (ParData[ii].last!=288-ParData[ii].len-1)
             {
              yk=ParData[ii].buf[ParData[ii].len+i-288];
	      yp=ParData[ii].buf[ParData[ii].len+ParData[ii].last-288];
	      dx=i-ParData[ii].last;
              ParData[ii].buf[ParData[ii].len+j-288]=
               (short)((long)(yk-yp)*(long)(j-ParData[ii].last)/(long)dx)+yp;
             }
            else
             {
              ParData[ii].buf[ParData[ii].len+j-288]=
               ParData[ii].buf[ParData[ii].len+i-288];
 	     }
            if (Diags>=8)
              printf("Fixing the hole, time: %d index: %d with value: %d\n",
                     ParData[ii].len+j-288, ii,
                     ParData[ii].buf[ParData[ii].len+j-288]);
           }
          ParData[ii].last=i;
         }
        else
         {
          if (Diags>=8)
	    printf("time:%d index:%d contains no data\n", i, ii);
         }
       }
      else
       {
        ParData[ii].last=i;
       } 
     }
    ConvertToDate(&In.date, In.date, DZIEN, 1, 0);
   } 
  ReadInCWU("art.txt"); 
  In.date=tmpDate;
 }

void main(int argc, char *argv[])
 {
  int i, steps;
#if defined(FULL_CTRL)
  msgInitialize();
  printf("msg Initialize: ... done\n");
#endif
#if #TIME_STEP(REAL_TIME)
  ipcInitialize();
  msgInitialize();
  ipcPTTGet();
#else
  if (argc<2 || (steps=atoi(argv[1]))<=0)
   {
    if (steps<0)
     {
      Diags=(unsigned char)abs(steps);
      printf("Diagnostic level: %d(%d)\n", Diags, steps);
     }
    else 
      Diags=1;
    steps=100;
   }
#endif
  InitDatabase();
  sterInitialize();
#if #TIME_STEP(REAL_TIME)
  while(1)
    Step10();
#else
  for(i=0;i<steps;i++)
    Step10();
#endif
 } 
