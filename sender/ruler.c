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
#include<errno.h>
#include<signal.h>
#include"msgtools.h"
#include"ipctools.h"
#include"frombase.h"
#include"show.h"
#include"ruler.h"

#define min(a,b) ((float)(a)<(float)(b)?(float)(a):(float)(b))

#define MY_MSG (getpid()%0x8000L)	/* identyfikator ¼ród³a komunikatów */

unsigned short Diags=0;			/* poziom diagnostyczny */

short Tzew[288];			/* bufory parametrów wej¶ciowych */
short Twy[144];
short Twyx[144];
short Tpow[144];
short Qwy[288];
short Qwyx[288];
short Gwy[288];
short DP[144];
short DPmin[144];
short DPmax[144];

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

tIn In={{1994, 1, 1, 1, 1, 0, 0}, 	/* date */	
        0,				/* t */
        0,				/* zima */
        0.0,				/* Tzas */
        0.0,				/* Tpow */
        0.0,				/* Tzew */
        0.0,				/* Gwy */
        0.0,				/* Tz3 */
	0.0,				/* Tz6 */
	0.0,				/* Tz12 */
        0.0,				/* Tz24 */
        0.0,				/* Tz27 */
	0.0,				/* Tz30 */
	0.0,				/* Tz36 */
	0.0,				/* Tz48 */
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
	0.0,				/* DP */
	0.0,				/* DPmin */
	0.0,				/* DPmax */
	0.0,				/* Q */
	0.0,				/* Tkwe */
	0.0,				/* Tkwy */
	0.0,				/* PS */
	0.0,				/* PM */
        0.0,				/* Gg */
	0.0,				/* k */
	0.0,				/* P1 */
	0.0,				/* P2 */
	0.0,				/* P3 */
	0.0,				/* P4 */
	0.0,				/* P5 */
	0.0};				/* Tst */

tOut Out={0.0,						/* P1 */
	  0.0,						/* P2 */
	  0.0,						/* P3 */
          0.0,						/* P4 */
          0.0,						/* P5 */
          0.0,						/* P6 */
          0.0,						/* P7 */
          0.0,                                          /* P8 */
          0.0,                                          /* P9 */
          0.0,                                          /* P10 */
          0.0,						/* P11 */
          {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 
           1.0, 1.0, 1.0, 1.0, 1.0},			/* k[] */
          0.0,						/* Tster */
          50000.0,					/* Qco */
          0.0,						/* Qcwu */
          50000.0, 					/* Qc */
          0.0,						/* Txpow */
          0.0,						/* Qter */
          1};						/* fine */

tCiepProfile Prf={"CIEP£OWNIA G£ÓWNA SUWA£KI",
		  10,			/* zms */ 
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
		  0.1,			/* Tpfault */
		  5000.0,		/* dQnor */
		  2000.0,		/* dQreg */
		  0.25,			/* Qstab */
		  0.20,			/* dQstab */
		  0.05,			/* Gstab */
		  2.0,			/* Tstab */
		  0.95,			/* WSh */
		  0.8,			/* PSl */
		  0.9,			/* PSh */
		  140.0,		/* Tsl */
		  150.0,		/* Tsh */
		  67.0,			/* Twel */
		  75.0,			/* Tweh */
		  125.0,		/* Twyl */
 		  140.0,		/* Twyh */
		  10.0,			/* dTsp */
		  2.0,			/* dTx */
		  200.0,		/* Ggmax */
		  0.95,			/* PMh */
		  72,			/* tback */
		  SHW_TXT};		/* show */

tKocProfile KocProfile[MAX_KOC]={{25000.0, 	/* K1Q */
 				    331.0},	/* K1G */
				 {25000.0,	/* K2Q */
				    331.0},	/* K2G */
				 {25000.0,	/* K3Q */
				    331.0},	/* K3G */
				 {25000.0,	/* K4Q */
				    331.0}};	/* K4G */

tKocIn KocIn[MAX_KOC]={{0.0,		/* K1G */
			0.0,		/* K1Twe */
			0.0,		/* K1Tx */
			0.0,		/* K1Twy */
			0.0,		/* K1Qmin */
			0.0,		/* K1Q */
			0.0,		/* K1Qmax */
			0.0,		/* K1dQ3 */
			0.0,		/* K1Tsp */
			0.0,		/* K1fw */
			  1},		/* K1state */
		       {0.0,            /* K2G */
                        0.0,            /* K2Twe */
                        0.0,            /* K2Tx */
                        0.0,            /* K2Twy */
                        0.0,            /* K2Qmin */
                        0.0,            /* K2Q */
                        0.0,            /* K2Qmax */
                        0.0,            /* K2dQ3 */
                        0.0,            /* K2Tsp */
                        0.0,            /* K2fw */		
			  1},		/* K2state */
		       {0.0,            /* K3G */
                        0.0,            /* K3Twe */
                        0.0,            /* K3Tx */
                        0.0,            /* K3Twy */
                        0.0,            /* K3Qmin */
                        0.0,            /* K3Q */
                        0.0,            /* K3Qmax */
                        0.0,            /* K3dQ3 */
                        0.0,            /* K3Tsp */
                        0.0,            /* K3fw */
			  1},		/* K3state */
                       {0.0,            /* K4G */
                        0.0,            /* K4Twe */
                        0.0,            /* K4Tx */
                        0.0,            /* K4Twy */
                        0.0,            /* K4Qmin */
                        0.0,            /* K4Q */
                        0.0,            /* K4Qmax */
                        0.0,            /* K4dQ3 */
                        0.0,            /* K4Tsp */
                        0.0,            /* K4fw */
			  1}};		/* K4state */
	
tKocOut KocOut[MAX_KOC]={{140.0},	/* K1Tx */
			 {140.0},	/* K2Tx */
			 {140.0},	/* K3Tx */
			 {140.0}};	/* K4Tx */

tParData ParData[MAX_PAR]={{3,288,-1,&Tzew[0]},	/* program buforów */
			   {1,144,-1,&Twy[0]},
			   {2,144,-1,&Twyx[0]},
			   {0,144,-1,&Tpow[0]},
			   {4,288,-1,&Qwy[0]},
			   {6,288,-1,&Gwy[0]},
			   {172,288,-1,&Qwyx[0]}, /* parametr wyliczany */ 
			   {33,144,-1,&DP[0]},
			   {185,144,-1,&DPmin[0]},
			   {186,144,-1,&DPmax[0]}
			  };

tAvgData AvgData[MAX_AVG]={{DO_AVG,144,0.1,&Tzew[0],&In.Tz48},	/* srednie */
			   {DO_AVG,72,0.1,&Tzew[72],&In.Tz36},
			   {DO_AVG,36,0.1,&Tzew[108],&In.Tz30},
			   {DO_AVG,18,0.1,&Tzew[126],&In.Tz27},	
			   {DO_AVG,144,0.1,&Tzew[144],&In.Tz24},
			   {DO_AVG,72,0.1,&Tzew[216],&In.Tz12},
			   {DO_AVG,36,0.1,&Tzew[252],&In.Tz6},
			   {DO_AVG,18,0.1,&Tzew[270],&In.Tz3},
			   {DO_AVG,144,0.1,&Twy[0],&In.Twy},
			   {DO_AVG,144,0.1,&Twyx[0],&In.Twyx},
			   {DO_AVG,144,1,&Gwy[0],&In.Gwy48},
			   {DO_AVG,144,1,&Gwy[144],&In.Gwy24},
			   {DO_SUM,144,0.1,&Tzew[18],&In.Sz24},
			   {DO_SUM,144,10.0/6.0,&Qwy[0],&In.E48},
			   {DO_SUM,144,10.0/6.0,&Qwy[144],&In.E24},
			   {DO_SUM,144,10.0/6.0,&Qwyx[0],&In.Ex48},
			   {DO_SUM,144,10.0/6.0,&Qwyx[144],&In.Ex24},
			   {DO_AVG,1,0.001,&DP[143],&In.DP},
			   {DO_SUM,1,0.1,&Tzew[161],&In.Tzew},
			   {DO_SUM,1,0.1,&Twy[143],&In.Tzas},
			   {DO_SUM,1,0.1,&Tpow[143],&In.Tpow},
			   {DO_SUM,1,1,&Gwy[287],&In.Gwy},
			   {DO_AVG,1,0.001,&DPmin[143],&In.DPmin},
			   {DO_AVG,1,0.001,&DPmax[143],&In.DPmax},
			   {DO_AVG,1,10.0,&Qwy[287],&In.Q}};

tSterData SterData[MAX_STER]={{{
				0L+'1',   /* numer linii*256 + kod jednostki */
				{0,0,0L,0}},	/* adres i warto¶æ parametru */
				&Out.Qc,	/* ¼ród³o sterowania */
				0.1,		/* modyfikator */
				MSG_OK}, 	/* stan komunikatu */
			      {{1L*256L+'0',
				{0,0,0L,0}},
				&KocOut[0].Tx,
				10.0,
				MSG_OK},
                              {{2L*256L+'0',
                                {0,0,0L,0}},
                                &KocOut[1].Tx,
                                10.0,
                                MSG_OK}, 
                              {{3L*256L+'0',
                                {0,0,0L,0}},
                                &KocOut[2].Tx,
                                10.0,
                                MSG_OK},
                              {{4L*256L+'0',
                                {0,0,0L,0}},
                                &KocOut[3].Tx,
                                10.0,
                                MSG_OK}};

tMsgData MsgData[MAX_MSG]={{&Prf.Q0, 10.0}, 
			   {&Prf.Q5, 10.0},
			   {&Prf.Q10, 10.0}};

tKocData KocData[MAX_KOC]={{NO_PARAM,		/* Gc   - NC */
			    NO_PARAM,		/* Twe  - NC */
			    NO_PARAM,		/* Tx	- NC */
			    NO_PARAM,		/* Twy  - NC */
			           7,		/* Qmin */
			           4,		/* Qwy */
                                 169,		/* Qmax */
			    NO_PARAM,		/* Tspl - NC */
  			    NO_PARAM,		/* Tspp - NC */
		            NO_PARAM,		/* fwl  - NC */
			    NO_PARAM,		/* fwp  - NC */
				   1,		/* modG - NC */
				 0.1,		/* modT - NC */
				10.0,		/* modQ */
			       0.001},	   	/* modf - NC */
			   {      13,		/* K1G */
				   9,		/* K1Twe */
				  11,		/* K1Tx */
				  10,		/* K1Twy */ 
			         235,		/* K1Qmin */
                                  12,		/* K1Q */
		                 236,		/* K1Qmax */
			          46,		/* K1Tspl */
			          46,		/* K1Tspp */
			    NO_PARAM,		/* K1fwl */
			    NO_PARAM,		/* K1fwp */      
			         0.1,		/* K1modG */
				 0.1,		/* K1modT */ 
				10.0,		/* K1modQ */
		    	       0.001},	   	/* K1modf */	
                           {      18,           /* K2G */
                                  14,           /* K2Twe */
                                  16,           /* K2Tx */
                                  15,           /* K2Twy */
                            NO_PARAM,           /* K2Qmin */
                                  17,           /* K2Q */
                            NO_PARAM,           /* K2Qmax */
                                  54,           /* K2Tspl */
                                  55,           /* K2Tspp */
                            NO_PARAM,           /* K2fwl */
                            NO_PARAM,           /* K2fwp */
                                 0.1,           /* K2modG */
                                 0.1,           /* K2modT */
                                10.0,           /* K2modQ */
                               0.001},          /* K2modf */
                           {      29,           /* K3G */
                                  25,           /* K3Twe */
                                  27,           /* K3Tx */
                                  26,           /* K3Twy */
                                 215,           /* K3Qmin */
                                  28,           /* K3Q */
                                 216,           /* K3Qmax */
                                  67,           /* K3Tspl */
                                  68,           /* K3Tspp */
                            NO_PARAM,           /* K3fwl */
                            NO_PARAM,           /* K3fwp */
                                 0.1,           /* K3modG */
                                 0.1,           /* K3modT */
                                10.0,           /* K3modQ */
                               0.001},          /* K3modf */
                           {      94,           /* K4G */
                                  90,           /* K4Twe */
                                  92,           /* K4Tx */
                                  91,           /* K4Twy */
                                 167,           /* K4Qmin */
                                  93,           /* K4Q */
                                 168,           /* K4Qmax */
                                 102,           /* K4Tspl */
                                 103,           /* K4Tspp */
                            NO_PARAM,           /* K4fwl */
                            NO_PARAM,           /* K4fwp */
                                 0.1,           /* K4modG */
                                 0.1,           /* K4modT */
                                10.0,           /* K4modQ */
                               0.001}};         /* K4modf */

tSingleData SingleData[MAX_SINGLE]={{30,&In.Tkwe,0.1},	/* parametry pojedyn */
				    {5,&In.Tkwy,0.1},
				    {NO_PARAM,&In.PS,0.001},
				    {NO_PARAM,&In.PM,0.001},
				    {8,&In.Gg,1.0},
				    {174,&In.k,0.1},
				    {179,&In.P1,0.1},
				    {178,&In.P2,0.1},
				    {181,&In.P3,0.1},
				    {180,&In.P4,0.1},
				    {182,&In.P5,0.1},
				    {170,&In.Tst,0.1}};

tShmData ShmData[MAX_SHM]={{&Out.Qc, 125, 0.1}};

tShmData KocShm[MAX_KOC][MAX_KOCSHM]={{{&KocOut[0].Tx, 126, 10.0}},
				      {{&KocOut[1].Tx, 127, 10.0}},
			              {{&KocOut[2].Tx, 128, 10.0}},
				      {{&KocOut[3].Tx, 129, 10.0}}};		
#define MAX_UMES 20

void UserMessage(ushort mescode)
 {
/* #if #TIME_STEP(REAL_TIME) */
  static int MesOnScr[MAX_UMES];
  char args[12][129];
  char *argp[12]={NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		  NULL, NULL};
 
  if (MesOnScr[mescode] && kill(MesOnScr[mescode], 0)==-1 && errno==3) 
    MesOnScr[mescode]=0;
  if (!MesOnScr[mescode])
   {
    argp[0]=&args[0][0];
    sprintf(argp[0], "qrydsp");
    argp[1]=&args[1][0];
    sprintf(argp[1], "%d", mescode);
    argp[2]=&args[2][0];
    sprintf(argp[2], "-1");
    argp[3]=&args[3][0];
    sprintf(argp[3], "%02d:%02d", In.date.hour, In.date.min);
    switch(mescode)
     {
      case 0:
      case 1:argp[4]=&args[4][0];
             sprintf(argp[4], "%0.0f", fabs(In.E24-In.Ex24));
	     sprintf(argp[2], "%ld", MsgRulerDes);
             break;
      case 2:
      case 3:argp[4]=&args[4][0];
             sprintf(argp[4], "%0.1f", In.Gg);
             break;
      default:break;
     }
    if (MesOnScr[mescode]=fork())
      return;
    else
      execv("/home/pbrnd/MsgDsp.d/qrydsp", argp);
   }
/* #endif */
 }

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

unsigned char GetPar(ushort addr, float *dest, float modif)
 {
  short sval;
  if (addr==NO_PARAM)
    return(0);
  if ((sval=GetMin10(addr))==SZARP_NO_DATA)
    return(1);
  *dest=(float)sval*modif;
  return(0);
 }

unsigned char GetKocs()		/* Pobierz dane dotycz±ce kot³ów */
 {
  #define MAX_FAULT 6			/* ile razy moze byc brak danych */
  static unsigned char ComFault[MAX_KOC][2];	/* brak ³±czno¶ci */
  unsigned char i;			/* ile razy, czy teraz te¿ */
  float Qks=0.0;
  float fval;
  static float Qbuf[MAX_KOC][2];
  unsigned char cf=0;
  unsigned char res=0;

#if #TIME_STEP(REAL_TIME)
  ipcRdGetInto(SHM_MIN10);	/* Pobierz moce min, akt i max sumy kot³ów */	
#endif  		     /* Je¿eli brak sk³adników u¿yj mocy ciep³owni */
  
  for(i=0;i<Prf.kotlow;i++)	/* pobie¿ Qmin kot³ów wylicz Qmin ciep³owni */
    if (KocIn[i].state!=KS_DEAD)
      if (GetPar(KocData[i+1].Qmin,&KocIn[i].Qmin,KocData[i+1].modQ))
        cf=ComFault[i][1]=1;	/* brak danych z kot³a */
      else
        Qks+=KocIn[i].Qmin;	/* s± dane z kot³a */
  if (!cf)
   {
    In.Qmin=Qks; 		/* u¿yj danych z kot³ów */
    GetPar(KocData[0].Qmin, &In.Qmin, KocData[0].modQ);  /* przybli¿ sieci± */
    Qks=0.0;
   }
  else
   {
    cf=0;
    GetPar(KocData[0].Qmin, &In.Qmin, KocData[0].modQ);	/* przybli¿ sieci± */
   }
   
  for(i=0;i<Prf.kotlow;i++)	/* pobie¿ Q kot³ów wylicz Qks ciep³owni */
    if (KocIn[i].state!=KS_DEAD)
      if (GetPar(KocData[i+1].Q,&KocIn[i].Q,KocData[i+1].modQ))
        cf=ComFault[i][1]=1;      /* brak danych z kot³a */
      else
        Qks+=KocIn[i].Q;       /* s± dane z kot³a */
  if (!cf)
   {
    In.Qks=Qks;                /* u¿yj danych z kot³ów */
    Qks=0.0;
   }
  else
   {
    cf=0;
    GetPar(KocData[0].Q, &In.Qks, KocData[0].modQ);  /* przybli¿ sieci± */
   }

  for(i=0;i<Prf.kotlow;i++)	/* pobie¿ Qmax i if kot³ów wylicz Qmax ciep³ */
    if (KocIn[i].state!=KS_DEAD)
      if (GetPar(KocData[i+1].Qmax,&KocIn[i].Qmax,KocData[i+1].modQ))
        cf=ComFault[i][1]=1;      /* brak danych z kot³a */
      else
       {			/* poprawka Qmax od wysterowania wyci±gu */
        if (GetPar(KocData[i+1].fwl,&KocIn[i].fw,KocData[i+1].modf)||
            GetPar(KocData[i+1].fwp,&fval,KocData[i+1].modf))
          ComFault[i][1]=1;
        else
         {
	  KocIn[i].fw=(KocIn[i].fw+fval)/2.0;
          KocIn[i].fw=KocIn[i].fw<0.01?0.01:KocIn[i].fw;
          KocIn[i].Qmax=min(KocIn[i].Q/pow(KocIn[i].fw,3),KocIn[i].Qmax);
	 } 
        Qks+=KocIn[i].Qmax;       /* s± dane z kot³a */
       }
  if (!cf)
   {
    In.Qmax=Qks;                /* u¿yj danych z kot³ów */
    GetPar(KocData[0].Qmax, &In.Qmax, KocData[0].modQ);   /* przybli¿ sieci± */
    Qks=0.0;
   }
  else
   {
    cf=0;
    GetPar(KocData[0].Qmax, &In.Qmax, KocData[0].modQ);  /* przybli¿ sieci± */
   }

  for(i=0;i<Prf.kotlow;i++)	/* pobie¿ pozosta³e dane o kot³ach */
   if (KocIn[i].state!=KS_DEAD)
     {
      ComFault[i][1]|=GetPar(KocData[i+1].G,&KocIn[i].G,KocData[i+1].modG);
      ComFault[i][1]|=GetPar(KocData[i+1].Tx,&KocIn[i].Tx,KocData[i+1].modT);
      ComFault[i][1]|=GetPar(KocData[i+1].Twy,&KocIn[i].Twy,KocData[i+1].modT);
      ComFault[i][1]|=GetPar(KocData[i+1].Twe,&KocIn[i].Twe,KocData[i+1].modT);
      ComFault[i][1]|=GetPar(KocData[i+1].Tspl,&KocIn[i].Tsp,KocData[i+1].modT);
      ComFault[i][1]|=GetPar(KocData[i+1].Tspp,&fval,KocData[i+1].modT);
      KocIn[i].Tsp=(KocIn[i].Tsp+fval)/2.0;
      if (Qbuf[i][0]==0.0&&Qbuf[i][1]==0.0)
        Qbuf[i][0]=Qbuf[i][1]=KocIn[i].Q;
      KocIn[i].dQ3=Qbuf[i][0]-KocIn[i].Q;
      Qbuf[i][0]=Qbuf[i][1];
      Qbuf[i][1]=KocIn[i].Q;
      if ((ComFault[i][0]+=ComFault[i][1])>MAX_FAULT)
       {
        ComFault[i][0]=MAX_FAULT;			/* b³±d komunikacji */
        KocIn[i].state=KS_COMM;
       }
      else
       {
        if (!ComFault[i][1])
          ComFault[i][0]=0;
        KocIn[i].state=0;				/* stan parcy kot³a */
        if (KocIn[i].G<0.75*KocProfile[i].Gnom||
            KocIn[i].Q<0.05*KocProfile[i].Qmax||KocIn[i].Tsp<90.0)
          KocIn[i].state=KS_OUT;
        if (KocIn[i].G<(1.0-Prf.Gstab)*KocProfile[i].Gnom||
            KocIn[i].G>(1.0+Prf.Gstab)*KocProfile[i].Gnom)
          KocIn[i].state+=KS_GFAULT;
        if (KocIn[i].Q<Prf.Qstab*KocProfile[i].Qmax)
          KocIn[i].state+=KS_QFAULT;
        if (KocIn[i].Twy<KocIn[i].Tx-Prf.Tstab||
            KocIn[i].Twy>KocIn[i].Tx+Prf.Tstab||KocIn[i].Tsp<100.0)
          KocIn[i].state+=KS_TFAULT;
        if (fabs(KocIn[i].dQ3)/KocProfile[i].Qmax>Prf.dQstab)
          if (KocIn[i].dQ3<0)
            KocIn[i].state+=KS_FASTDN;
          else
            KocIn[i].state+=KS_FASTUP;
        if (KocIn[i].state==0)
         {
          res=1;
          KocIn[i].state=KS_READY;
         }
       }
      ComFault[i][1]=0;
     }
#if #TIME_STEP(REAL_TIME)
  ipcRdGetOut(SHM_MIN10);
#endif
  return(res);
 }   

unsigned char GetSingles()
 {
  static unsigned char ComFault[2];
  ushort i;
  ComFault[1]=0;
  for(i=0;i<MAX_SINGLE;i++)
    ComFault[1]|=GetPar(SingleData[i].addr, SingleData[i].dst, 
                        SingleData[i].modif);
  if (!ComFault[1])
   {
    ComFault[0]=0;
    return(1);
   }
  if (++ComFault[0]>MAX_FAULT) 
   {
    ComFault[0]=MAX_FAULT;
    return(0);
   }
  return(1);
 }

void GetPars()
 {
  Out.fine=GetSingles();	/* pobranie pojedynczych parametrów */
  GetAvgs();			/* pobranie ¶rednich */
  Out.fine&=GetKocs();		/* pobranie danych o kot³ach */
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

  res=(float)Pobor_CWU[dt->wday-1][dt->hour*6+dt->min/10]/1000.0*
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
  return(In.Twy-In.Twyx);		/* a rzeczywisto¶æ = nidotrzymanie */
 }

float comp_P4()				/* parametr steruj±cy P4 */
 {					/* zadany przebieg akumulacji */
  float res;
  float P4A, P4B;
  
  res=In.t<48?5.0:10.0*fabs(In.t-96.0)/48.0-5.0;	/* zadany przebieg */
  if (In.Tz24<5.0)				 /* zadana warto¶æ ¶rednia */	
    In.Ea23=Prf.Ea23_0+(Prf.Ea23_5-Prf.Ea23_0)/5.0*In.Tz24; 
  else if (In.Tz24<10.0) 
    In.Ea23=Prf.Ea23_5+(Prf.Ea23_10-Prf.Ea23_5)/5.0*(In.Tz24-5.0);
  else
    In.Ea23=Prf.Ea23_10;
  P4A=(In.Ea23-In.Eax)/(In.Gwy24)/(float)Prf.tback/C_J2W;   
  if (P4A<-5.0)					   
    P4A=-5.0;			/* roznica bezposrednio do oddania w czasie */
  else if (P4A>5.0)		/* Prf.tback */
    P4A=5.0;
  if (In.DP<In.DPmin)
    P4B=-5.0;
  else if (In.DP>In.DPmax)
    P4B=5.0; 			/* ró¿nica przez ci¶nienie dyspoz */
  else
    P4B=-5.0+10.0/(In.DPmax-In.DPmin)*(In.DP-In.DPmin);

  /* !!!!!! UWAGA !!!!!! */
  /* Do celów testowych zawsze u¿ywamy akumulacji obliczanej klocami */
  /* Sterownik zawsze pracuje od ci¶nienia dyspozycyjnego */
  /* !!!!!! KONIEC !!!!! */ 
  if (1||In.Tpow!=0.0)
   { 
    if (0&&fabs((In.Tpow-Out.Txpow)/In.Tpow)<Prf.Tpfault)
      res-=P4A;		/* aproxymacja akumulacji jest rozs±dna */
    else
      res+=P4B;	/* zbyt du¿y b³±d w akumulacji u¿ywamy DP jak w  sterowniku */
   }
  else
    res-=P4A;
  return(res);
 }

float comp_P5()					/* parametr steruj±cy P5 */ 
 {			/* nocne ziêbienie i szczytowe podgrzanie ruroci±gu */
  return(In.t<48?5.0:In.t<84?0.0:In.t<120?-5.0:0.0);
 }

float comp_P6()				/* parametr steruj±cy P6 */
 {				/* bilans energetyczny przedostatniej doby */
  float res;
  res=In.Gwy48>0.0?(In.E48-In.Ex48)/In.Gwy48/48.0:(In.E48-In.Ex48)/48.0;
  if (res<-5.0)
    return(-5.0);
  if (res>5.0)
    return(5.0);
  return(res);
 }

float comp_P7()
 {
  static ushort t=NO_PARAM;
  static unsigned char underQ=0;
  static float Qmax=0.0;
  float res;

  if (underQ&&(fabs(Out.Qc)-In.Q)<=Prf.dQnor)	/* powrót do normalnej pracy */
   {					/* po ra¿±cym niedotrzymaniu */
    t=0;				/* wyliczenie niedotrzymania */	
    Qmax=2.0*(In.Ex24-In.E24)/(float)Prf.tback;  /* za ostatni± dobê */
   }
  underQ=(fabs(Out.Qc)-In.Q>Prf.dQnor);
  if (!underQ&&t<Prf.tback)	/* czy mozna oddawac */
   {
    res=Qmax*((float)Prf.tback-(float)t)/(float)Prf.tback;
    res=In.Gwy>0.0?res/In.Gwy:0.0;
    res=res>4.0?4.0:res;
    t++;
    res=res>0.0?-res:0.0;
    return(res);
   }
  else
    return(0.0);	/* razace niedotrzymanie lub koniec cyklu oddawania */
 }     

float comp_P8()		/* parametr steruj±cy P8 */
 {
  float dTx;		/* bie¿±cy niedobór akumulacji Twy-Twyx */

  dTx=(((float)Twy[143]-(float)Twyx[143])/10.0);	/* 10-cio minutowe */
  dTx=dTx<-10.0?-10.0:dTx>10.0?10.0:dTx;
  return(dTx);
 }

float comp_P9()				/* parametr steruj±cy P9 */
 {
  return(In.Tz6-In.Tz30);               /* 6-godzin bie¿±ce a z poprzedniej */
 }					/* doby */

float comp_P10()                        /* parametr steruj±cy P10 */
 {
  return(In.Tz12-In.Tz36);              /* 12-godzin bie¿±ce a z poprzedniej */
 }					/* doby */

float comp_P11()                        /* parametr steruj±cy P11 */
 {
  return(In.Tz24-In.Tz48);              /* bie¿±ca doba a poprzednia */
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
  if (0)             /* dTz2>20||dTz3>30) */
    Out.k[1]=Out.k[2]=2.0;  /* gwa³towna zmiana Tzew wzmocniæ param pogodowe */
  else
    Out.k[1]=Out.k[2]=1.0;				/* praca normalna */
  SumP=Out.P1*Out.k[1]+Out.P2*Out.k[2]+Out.P3*Out.k[3]+Out.P4*Out.k[4]+
       Out.P5*Out.k[5]+Out.P6*Out.k[6]+Out.P7*Out.k[7];
  if (In.Qks<In.Qmin||In.Qks>In.Qmax)
    Out.k[0]=Prf.kmin;				/* poza zakresem  mocy */
  else if (SumP>0.0)
    comp_k0(Prf.dQpos);	  /* dodatnia poprawka zmniejszamy ³agodnie moc */
  else	 
    Out.k[0]=Prf.kmax;
 /*   comp_k0(Prf.dQneg); */  /* ujemna poprawka ostro zwiêkszamy moc */

 /* U¿ywaj k ze sterownika z ograniczeniem z profilu */

  Out.k[0]=In.k>Prf.kmax?Prf.kmax:In.k<Prf.kmin?Prf.kmin:In.k;	
 }

void MyCO()		/* Wydajnosc sterujaca na potrzeby C.O. */
 {
  float Tster;
  float dT, P4T;
  Out.P1=comp_P1();
  Out.P2=comp_P2();
  Out.P3=comp_P3();
  Out.P4=comp_P4();
  Out.P5=comp_P5();
  Out.P6=comp_P6();
  Out.P7=comp_P7();
  Out.P8=comp_P8();
  Out.P9=comp_P9();
  Out.P10=comp_P10();
  Out.P11=comp_P11();
  comp_k();
 
  Tster=(In.Sz24/6.0+(Out.k[1]*Out.P1+Out.k[2]*Out.P2+Out.k[3]*Out.P3+
	 Out.k[4]*Out.P4+Out.k[5]*Out.P5+Out.k[6]*Out.P6+Out.k[7]*Out.P7+
         Out.k[8]*Out.P8+Out.k[9]*Out.P9+Out.k[10]*Out.P10+Out.k[11]*Out.P11)*
         Out.k[0])/24.0;
  dT=Tster-Out.Tster;
  dT=dT>1.0?1.0:dT<-1.0?-1.0:dT;
  Out.Tster+=dT;
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

void ChangeQ()			/* potencjalna zmiana wydajnosci cieplowni */
 {				/* za pomoca kotlow */
  unsigned char i,extr;
  if (In.Q-Out.Qc>0.0)		/* proba zmniejszenia */
   {
    if (In.PS>Prf.PSh||In.Tkwe<Prf.Tweh)	/* warunki ogolne cieplowni */
     {
      for(i=0;i<Prf.kotlow;i++)		/* nie spelnione */
        KocOut[i].Tx=-KocIn[i].Tx;	/* brak regulacji kotlami */
      return;
     }
    for(i=0,extr=Prf.kotlow;i<Prf.kotlow;i++)	/* szukaj najgorszego kotla */
     {
      if (KocIn[i].state==KS_READY&&KocIn[i].Tsp>Prf.Tsh&&
          KocIn[i].Twy>Prf.Twyl)
       {
        if (extr==Prf.kotlow||KocIn[i].Tsp>KocIn[extr].Tsp)
          extr=i;
        KocOut[i].Tx=1.0;
       }
      else				/* eliminuj kotly nie spelniajace */
        KocOut[i].Tx=-KocIn[i].Tx;	/* warunkow regulacji w dol */
     }
    if (extr==Prf.kotlow)		/* brak regulowalnych kotlow */
      return;
    for(i=0;i<Prf.kotlow;i++)		/* kotly o podobnych Tsp */
      if (KocOut[i].Tx>0.0&&KocIn[extr].Tsp-KocIn[i].Tsp<Prf.dTsp)
        KocOut[i].Tx=KocIn[i].Tx-Prf.dTx;  /* traktuj jak jeden super kociol */
      else
        KocOut[i].Tx=-KocIn[i].Tx;	/* rozbiezne eliminuj */
    Out.Qc=-Out.Qc;			/* sygnalizuj nic nie rob sieci */
   }
  else					/* proba zwiekszania mocy */
   {
    if (In.Tkwe>Prf.Twel)		/* warunki ogolne cieplowni */
     {
      for(i=0;i<Prf.kotlow;i++)		/* niespelnione - kotly nie beda */
        KocOut[i].Tx=-KocIn[i].Tx;	/* regulowane */
      return;
     }
    for(i=0,extr=Prf.kotlow;i<Prf.kotlow;i++)	/* szukamy najlepszego kotla */
     {
      if (KocIn[i].state==KS_READY&&KocIn[i].Tsp<Prf.Tsl&&
          KocIn[i].Twy<Prf.Twyh&&KocIn[i].fw<Prf.WSh)
       {
        if (extr==Prf.kotlow||KocIn[i].Tsp<KocIn[extr].Tsp)
          extr=i;
        KocOut[i].Tx=1.0;
       }
      else
        KocOut[i].Tx=-KocIn[i].Tx;	/* eliminuj kotly niezdolne do */
     }					/* regulacji w gore */
    if (extr==Prf.kotlow)		/* brak kotlow zdatnych do regulacji */
      return;  
    for(i=0;i<Prf.kotlow;i++)		/* tworz super kociol z kotlow */
      if (KocOut[i].Tx>0.0&&KocIn[i].Tsp-KocIn[extr].Tsp<Prf.dTsp)
        KocOut[i].Tx=KocIn[i].Tx+Prf.dTx;		/* o podobnej Tsp */
      else
        KocOut[i].Tx=-KocIn[i].Tx;
    Out.Qc=-Out.Qc;			/* sygnalizuj nic nie rob sieci */
   }
 }

unsigned char CheckPM()		/* sprawdzenie wysterowania PM */
 {			/* jezeli za duze zdjecie z kotlow o ile mozna */
  unsigned char i,extr;
  float dT;
  if (In.PM<Prf.PMh||In.Gg<Prf.Ggmax)			/* PM w normie */
   {
    return(1);				/* najpierw próba ruchu kotlami */
   }
  Out.fine=3;
  for(i=0,dT=0.0;i<Prf.kotlow;i++)	/* przegrzanymi */
   {					  /* olewaj±c warunki na odgaz */
    if (KocIn[i].state==KS_DEAD)        /* eliminuj kotly nieistotne */
      continue;
    if (KocIn[i].state!=KS_READY)       /* eliminuj kotly niegotowe */
      KocOut[i].Tx=-KocIn[i].Tx;
    else if (KocIn[i].Twy>Prf.Twyl&&KocIn[i].Tsp>Prf.Tsh)
     {
      KocOut[i].Tx=KocIn[i].Tx-Prf.dTx;         /* obnizaj na przegrzanych */
      dT-=Prf.dTx;                   		/* licz zmiane energii */
     }
    else
      KocOut[i].Tx=-KocIn[i].Tx;                /* pozostalych nie dotykaj */
   }
  if (dT==0.0) 		/* jezeli nie by³o ruchu obnizaj na dowolnym którym */
   {			/* da siê w ogóle regulowaæ i ma Twy > Twyl - 5.0 */
    for(i=0,extr=Prf.kotlow;i<Prf.kotlow;i++) 
     {
      if (KocIn[i].state==KS_DEAD)        /* eliminuj kotly nieistotne */
        continue;
      if (KocIn[i].state!=KS_READY)       /* eliminuj kotly niegotowe */
        KocOut[i].Tx=-KocIn[i].Tx;
      else if (KocIn[i].Twy>Prf.Twyl-5.0)	/* zaznacz kot³y zdatne */
       {					/* do regulacji */
        if (extr==Prf.kotlow||KocIn[i].Twy>KocIn[extr].Twy)
          extr=i;	
        KocOut[i].Tx=1.0;    		/* wyznacz najcieplejszy */     
       }
      else
        KocOut[i].Tx=-KocIn[i].Tx;                /* pozostalych nie dotykaj */
     }
    if (extr==Prf.kotlow)
     {				/* brak mo¿liwo¶ci regulacji alarmuj */
      UserMessage(2);
      return(0); 
     }
    for(i=0,dT=0.0;i<Prf.kotlow;i++)	/* twórz super kocio³ */
     {
      if (KocOut[i].Tx>0.0&&KocIn[extr].Twy-KocIn[i].Twy<5.0)
       {
        KocOut[i].Tx=KocIn[i].Tx-Prf.dTx;	/* obni¿aj na podobnych */
       }
     }
   }
  Out.Qc=-Out.Qc;                         /* sygnalizuj sieci zmiane kotlami */
  UserMessage(3);			/* koniec PM program obniza kot³y */
  return(0);
 }

void MyCiep()		/* wydajnosc sterujaca cieplowni */ 
 {			/* moze ewentualne modyfikacje od strat */
  unsigned char i;
  static float perQ;
  static float Qmax[6];
  static float Qks[6];
  static float Qciep[6];
  float Qmaxs;
  float Qkss;
  float Qcieps;
  float Qmaxx;

  for(i=0;i<6;i++)
   {
    Qmax[i]=Qmax[i]?Qmax[i]:In.Qmax;
    Qks[i]=Qks[i]?Qks[i]:In.Qks;
    Qciep[i]=Qciep[i]?Qciep[i]:In.Q;
   }
  MyCWU();
  if (In.zima)
   {
    MyCO();
    Out.Qter=Out.Qc=Out.Qco;
   }
  else
    Out.Qter=Out.Qc=0.0;
  Out.Qc+=Out.Qcwu;
  Out.Qter=Out.Qc;
  for(i=0;i<Prf.kotlow;i++)
    KocOut[i].Tx=-KocIn[i].Tx; 
  if (Out.fine&&CheckPM())	/* jest czym regulowaæ i PM nie przekroczone */
   {
    ChangeQ();
    Out.fine=2;
   }
  for(i=0,Qmaxs=0.0;i<5;i++)
   {
    Qmaxs+=Qmax[i];
    Qkss+=Qks[i];
    Qcieps+=Qciep[i];
    Qmax[i]=Qmax[i+1];
    Qks[i]=Qks[i+1];
    Qciep[i]=Qciep[i+1];
   }
  Qmaxs=(Qmaxs+Qmax[5])/6.0;
  Qkss=(Qkss+Qks[5])/6.0;
  Qcieps=(Qcieps+Qciep[5])/6.0;
  Qmax[5]=0.0;
  Qks[5]=0.0;
  Qciep[5]=0.0;
  Qmaxx=Qmaxs>Qkss?Qmaxs:Qkss;
  Qmaxx=Qmaxx>Qcieps?Qmaxx:Qcieps;
  if (Out.Qter>0.97*Qmaxx)
   {
    Out.Qter=Qmaxx;
    perQ=200;
    Out.fine=6;
   }
  else if (perQ-->193)
   {
    Out.Qter=(float)perQ/200.0*Qmaxx;
    Out.fine=7;
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
  Eout=0.0;
  while(wyciek>0.0)			/* wyci±gamy tyle klocy ile trzeba */
   {					/* by zabraæ okre¶lon± objêto¶æ */
    if (wyciek<rura->out->V)		/* co wiêksze ubytek czy kloc */
     {
      rura->out->V-=wyciek;		/* koniec ubytku wystarczy zmniejszyæ */
      Eout+=C_J2W*wyciek*rura->out->T;	/* ostatni kloc */
      wyciek=0.0;
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
  if (kloc->V>0.0)			/* rury */
    kloc->T=Eout/kloc->V/C_J2W;		/* ustalamy temperaturê zabranego */
  else					/* kloca */
    kloc->T=kloc->V=0.0;		/* koc bez objêto¶ci nie ma */
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
  Eout=0.0;
  kloc->V=wyciek=rura->Vs-rura->Vc;	/* obliczenie nadmiaru wody w rurze */
  if (wyciek>0.0)			/* zrównanie objêto¶ci gumowej do */
    rura->Vs-=wyciek;			/* objêto¶ci nominalnej rury */ 
  while(wyciek>0.0)			/* trzeba odebraæ nadmiar wody i */
   {					/* obliczyæ jak± energiê niesie */
    if (wyciek<rura->out->V)
     {
      rura->out->V-=wyciek;		/* zmniejszenie objêto¶ci ostatniego */
      Eout+=C_J2W*wyciek*rura->out->T;	/* kloca wystarczy do odebrania */
      wyciek=0.0;				/* wycieku */
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
  if (kloc->V>0.0)			/* energiê */
    kloc->T=Eout/kloc->V/C_J2W;		/* wstawienie kloca V0,T0 w rurê */
  else					/* powoduje wyciek kloca V1,T1 z rury */
    kloc->T=kloc->V=0.0;
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
    if (Kwy.V>0.0)
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
    if (Kwy.V>0.0)
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
  Ewy=W_rure(&Rzas, &Kwe);		/* wk³adamy w rurê bie¿±c± produkcjê */
  Ewy-=comp_Odbior(&In.date, In.Tzew);   /* odbieramy energiê z koñca rury */

  /* !!!!!! UWAGA !!!!!!! */
  /* nie zamra¿aæ rury przyj±æ poziom temperatuty poni¿ej którego nie wolno */
  /* wysysaæ energii z kloca */
  /* !!!!!! KONIEC !!!!!! */
  if (Kwe.V>0.0)
    Kwe.T=Ewy/Kwe.V/C_J2W;	/* nowa temperatura kloca po zabraniu energii */
  Ewy=W_rure(&Rpow, &Kwe);	/* objêto¶æ w ca³ym cyklu ta sama */
  if (fabs(Ewy)>1.0)
   {
    printf("Ruler: Extra energy portion %0.1f  \n", Ewy);
    exit(-1);  
   }
  /* !!!!!! UWAGA !!!!!!! */
  /* je¿eli co¶ wysz³o z rury to dziwne, oznacza to niezachowanie bilansu */
  /* trzeba co najmniej alarmowaæ, albo i¶æ do lasu */
  /* !!!!!! KONIEC !!!!!! */
  
  In.Eaz=Rzas.E;		/* akumulacja po stronie zasilania */
  In.Eap=Rpow.E;		/* akumulacja po stronie powrotu */
  In.Eax=Rpow.E+Rzas.E;		/* no to obliczyli¶my zakumulowan± energiê */
 }

void ReadMsg()
 {
  tMsgSetParam msg;
  long lbuf;

  while(msgrcv(MsgRulerDes, &msg, sizeof(tSetParam), -65535L, IPC_NOWAIT)==
        sizeof(tSetParam))
   {
    printf("Ruler: Received message %ld setting parameter %d to %d\n", msg.type,
	   msg.cont.param, msg.cont.value);
    if (msg.cont.param<MAX_MSG)
      *(MsgData[msg.cont.param].dst)=MsgData[msg.cont.param].modif*
       (float)msg.cont.value;
    else
      printf("Ruler: Address %d exceeds limit of %d parameters to set\n",
	     msg.cont.param, MAX_MSG);
    if (msg.cont.rtype)
     {
      printf("Ruler: Sending reply %ld to message %ld setting parameter %d to %d\n",
              msg.cont.rtype, msg.type, msg.cont.param, msg.cont.value);
      lbuf=msg.type;
      msg.type=msg.cont.rtype;
      msg.cont.rtype=lbuf;
      printf("Ruler: Send result %d with errno %d\n",
              msgsnd(MsgRulerDes, &msg, sizeof(tSetParam), IPC_NOWAIT),
              errno);
     }
   else
     printf("Ruler: No reply required for message %ld setting parameter %d to %d\n",
	    msg.type, msg.cont.param, msg.cont.value);
   }
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
      SterData[i].msg.cont.rtype=MY_MSG*0x10000+SterData[i].msg.type; 
      SterData[i].msg.cont.retry=1;
      if (!msgsnd(MsgSetDes, &(SterData[i].msg), sizeof(tSetParam), IPC_NOWAIT))
        SterData[i].status=MSG_CONF;
     }
   }
#endif
 }

void ReadReply()
 {
#if defined(FULL_CTRL)
  ushort i,j;
  tMsgSetParam msg;
  
  for(j=0;j<MAX_STER;j++)
   {
    while (msgrcv(MsgRplyDes, &msg, sizeof(tSetParam), 
          SterData[j].msg.cont.rtype, IPC_NOWAIT)==sizeof(tSetParam))
     {
      for(i=0;i<MAX_STER;i++)
       {
        if (SterData[i].status==MSG_CONF&&msg.cont.retry!=0&&
            SterData[i].msg.type==msg.cont.rtype&&
            SterData[i].msg.cont.param==msg.cont.param&&
            SterData[i].msg.cont.value==msg.cont.value)
          SterData[i].status=MSG_OK; 
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
  if (In.E24-In.Ex24>10000.0) /* 4.0*In.Q) */
    UserMessage(1);
  if (In.Ex24-In.E24>10000.0) /* 4.0*In.Q) */ 
    UserMessage(0);
 }

void Step10()
 {
  ushort tosleep;
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
  Qwyx[287]=(short)rint(fabs(Out.Qc)*SterData[0].modif);
  comp_Avgs();
  comp_Accu();
  MyCiep();
  ShowResults();
#if #TIME_STEP(REAL_TIME)
  for(i=0;i<10;i++)
   {
    time(&sectime);
    ReadMsg();
    SendSter();
    for(tosleep=30-sectime%30;(tosleep=sleep(tosleep))!=0;)
      printf("Ruler:Frozing for %d seconds\n",tosleep);
    time(&sectime);
    ReadReply();
    for(tosleep=30-sectime%30;(tosleep=sleep(tosleep))!=0;)
      printf("Ruler:Frozing for %d seconds\n",tosleep);
    sleep(30-sectime%30);
   }
#else
  ConvertToDate(&In.date, In.date, DZIEN, 1, 0);
  ReadMsg();

 /*  SendSter(); 
  for(tosleep=TIME_STEP/2;(tosleep=sleep(tosleep))!=0;)
    printf("Ruler:Frozing for %d seconds\n",tosleep);
  ReadMsg();
  ReadReply(); 
  for(tosleep=TIME_STEP/2;(tosleep=sleep(tosleep))!=0;)
    printf("Ruler:Frozing for %d seconds\n",tosleep); */
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
  while(yy<1991||yy>1995||mo<1||mo>12||dd<1||dd>31||hh<0||hh>23||mi<0||mi>59);
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

void ExitSeq()
 {
  printf("rm Ruler:%d\n", msgctl(MsgRulerDes, IPC_RMID, NULL));
  printf("saving profile ... ");
  if (ftlSaveCProfile("profile.pat", "dupa.prf", &Prf, &In, &Out))
    printf("Ok.\n");
  else
    printf("Failed.\n");;
 }

void Terminate(int sig)
 {
  exit(-1);
 }

void Reconfigure(int sig)
 {
  printf("Reading profile ... ");
  if (ftlInputCProfile("profile.pat", "dupa.prf", &Prf, &In, &Out))
    printf("Ok.\n");
 }

void main(int argc, char *argv[])
 {
  int i, steps;
  struct sigaction Act;
#if defined(FULL_CTRL)
  msgInitialize();
  msgRulerInit();
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
  printf("Reading profile ... ");
  if (ftlInputCProfile("profile.pat", "dupa.prf", &Prf, &In, &Out))
    printf("Ok.\n");
  else
    printf("Failed.\n"); 
  InitDatabase();
  sterInitialize();
  atexit(ExitSeq);
  Act.sa_handler=Terminate;
  Act.sa_flags=SA_RESTART|SA_RESETHAND;
  sigaction(SIGTERM,&Act,NULL);
  sigaction(SIGINT,&Act,NULL);
  Act.sa_handler=Reconfigure;
  Act.sa_flags=SA_RESTART;
  sigaction(SIGUSR1,&Act,NULL);
#if #TIME_STEP(REAL_TIME)
  while(1)
    Step10();
#else
  for(i=0;i<steps;i++)
    Step10();
#endif
 } 
