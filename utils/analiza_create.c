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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

const char offset = 9;
const char CharsTable[] = "±êó¶³¿¼æñ¡ÊÓ¦£¯¬ÆÑaeoslzzcnAEOSLZZCN";

/* Struktura zwraca numer linii nazwe parametru oraz nazwe kotla */
typedef struct
{
 int linenum;
 int comma ;
 char parname[10];
 char boilername[40];
} linedata;

#define TRUE 1
#define FALSE 0

/* tolower + uwzglednienie polskich znakow */
char *pltolower(char *inStr)
{
 int i,j;
 char result;
 char *OutStr;
 OutStr = (char *)malloc(strlen(inStr));
 for (i=0;i<strlen(inStr);i++){
		result = FALSE;
 	for (j=offset;j<2*offset;j++){
		if (CharsTable[j]==inStr[i]){
			OutStr[i] = CharsTable[j-offset];
			result = TRUE;
		}
	}
	if ((inStr[i]>='A')&&(inStr[i]<='Z')){
		OutStr[i] = inStr[i] + 0x20;
	}else{
		if (result == FALSE){
		    OutStr[i] = inStr[i] ;
		}
	}	
 }
 OutStr[i] = '\0';
 return (OutStr);
}

int plstrcasecmp(char *str1, char *str2)
{
 char *buf1;
 char *buf2;
 int result;
 buf1 = pltolower(str1);
 buf2 = pltolower(str2);
 result = strcmp(buf1,buf2);
 free(buf2);
 free(buf1);
 return (result);
}

/*
Zamiani ciag polskich znakow na ciag be polskich znakow
*/
char *pl2nopl(char *inStr)
{
 int i,j;
 char result;
 char *OutStr;
 OutStr = (char *)malloc(strlen(inStr));
 for (i=0;i<strlen(inStr);i++){
		result = FALSE;
 	for (j=0;j<2*offset;j++){
		if (CharsTable[j]==inStr[i]){
			OutStr[i] = CharsTable[j+2*offset];
			result = TRUE;
		}
	}
	if (result == FALSE){
		    OutStr[i] = inStr[i] ;
	}	
 }
 OutStr[i] = '\0';
 return (OutStr);
}

void getlinedata(char *linebuf,int linebuflen, linedata *ilinedata)
{
 char buf[255];
 int i,j;
 i = 0;
 while ((linebuf[i]!=0x20)&&(i<linebuflen)){
	buf[i]=linebuf[i];
	i++;
 }
 buf[i]='\0';
 ilinedata->linenum = atoi(buf);
 i++;
 j=0;

 while ((linebuf[i]!=0x20)&&(i<linebuflen)){
	 buf[j]=linebuf[i];
	 i++;
	 j++;
 }

 buf[j]='\0';
 ilinedata->comma = atoi(buf);
 i++;
 for (j=0;j<sizeof(buf);j++) buf[j] = '\0';
 
 j=0;
 while ((linebuf[i]!=0x20)&&(i<linebuflen)){
	buf[j]=linebuf[i];
	j++;
	i++;
 }
 buf[j]='\0';
 strcpy(ilinedata->parname, buf) ;
 for (j=0;j<sizeof(buf);j++) buf[j] = '\0';
 i++;
 j=0;
 while ((linebuf[i]!=':')&&(i<linebuflen)){
	buf[j]=linebuf[i];
	j++;
	i++;
 }
 buf[j]='\0';
 strcpy(ilinedata->boilername, buf) ;
}

int readline (FILE *f,char *buf, int buflen)
{
 int i;
 int result;
 for (i=0;i<buflen;i++)
 buf[i]='\0';;
 i = 0;
 result = 0;
 while ((i<buflen)&&(result!=0x0A)&&(result!=0x0D)){
 result = fgetc(f);
 if (result==EOF) 
	 return EOF;
 buf[i] = result;
 i++;
 }
 i --;
 switch (result){
 case 0x0A:
  buf[i] = '\0';    
 break;
 
 case 0x0D:
    buf[i] = '\0';
    result = fgetc(f);
    break;
 }
 return 0;
}

int main(int argc, char *argv[]){
 char PTTPath[1024];
 char *CurrentDirectory;
 FILE *f;
 char partable[20][20];
 char partablesize; /* aktualna ilosc parametrow */
 char partableptr; /* aktualny numer parametru */
 int lineptr; /*numer linii*/
 int linestatus; /* czy nie wystapil EOF */
 int Vr_comma ; /* Pozycja przecinka w predkosci rusztu */
 int i;
 int Param1,Param2,Param3; /* Parametry pierwszej linii z PTT.act*/
 char wel_lock; /* blokada parametru wel Jest ich zawsze kilka w pliku */
 char line_buf[256];
 linedata ilinedata;
 char Vr_PartI_Low[10];
 char Vr_PartI_Hi[10];
 char Vr_PartI_Duration[10];
 char Vr_PartII_Low[10];
 char Vr_PartII_Hi[10];
 char Vr_PartII_Duration[10];
 char Vr_PartIII_Low[10];
 char Vr_PartIII_Hi[10];
 char Vr_PartIII_Duration[10];
 //
 int nr_kotla;
 char boiler_name[100]; 
 int a_enable = 0;
 int work_timer_h = 0;
 int Vc = 0;
 int En_Vc = 0;
 int Fp_4m3_i = 0;
 int WRType = 0;
 //
 int Vr_l = 0;
 int Vr_p = 0;
 int Min_Fp_4m3 = 0;
 int Max_Fp_4m3 = 0;
 int Hw_l = 0;
 int Hw_p = 0;
 int Fp_4m3_o = 0 ;
 int AnalyseStatus = 0;


 if (argc!=(3 + 1)){
	printf("\n#analiza_create <nazwa_kot³a> <numer_kot³a> <WRType>\n");
	printf("\nWRType: WR-25 = 0  , WR-10 = 1  , WR-5 = 2\n");
	printf("\nUWAGA! ewentualne spacje w parametrze nazwa_kotla nalezy zastapic znakami underscore (_)\n");
	printf("\nUWAGA! Plik PTT.act musi byc w biezacym katalogu\n");
	exit (0);
 }

/*
 fprintf(stdout,":%s",pl2nopl(pltoupper(argv[1])));
 */
 strcpy(boiler_name,argv[1]);
 /*Zamiana underscorow na spacje*/
 for (i=0;i<strlen(boiler_name);i++){
	 if (boiler_name[i]=='_'){
		 boiler_name[i] = 0x20;
	 }
 }
 nr_kotla=atoi(argv[2]);
 fprintf(stdout,":KOCIOL_%d\n",nr_kotla);
 WRType=atoi(argv[3]);
 CurrentDirectory = getcwd(NULL,1024);
 sprintf(PTTPath,"%s/PTT.act", CurrentDirectory);
 free (CurrentDirectory);
 f=fopen(PTTPath,"rt");
 fscanf(f,"%d %d %d\n",&Param1,&Param2,&Param3);

 /* Przepisanie nazw parametrow */
strcpy(partable[0],"wel"); /* zakodowany stan wej¶æ logicznych - a_enable*/
strcpy(partable[1],"Tanl"); /* czas pracy w analizie - work_timer_h */
strcpy(partable[2],"V"); /*aktualna objêto¶æ wêgla -Vc */
strcpy(partable[3],"Ko"); /*stosunek energia / objêto¶æ -En_Vc */
strcpy(partable[4],"Pws3"); /*Wzglêdna ilo¶æ powietrza dla 1.6m3/h wêgla -Fp_4m3_i */
if (WRType!=0){ /* Gdy kocio³ != WR25 */
	strcpy(partable[4],"Pws3"); /*Wzglêdna ilo¶æ powietrza dla 1.6m3/h wêgla -Fp_4m3_i */
	strcpy(partable[5],"vr"); /*prêdko¶æ rusztu - Vr_l/Vr_p*/
	strcpy(partable[6],"Dpow");  /*dolna granica powietrza w analizie - Min_Fp_4m3*/
	strcpy(partable[7],"Gpow"); /*górna granica powietrza w analizie Max_Fp_4m3 */
	strcpy(partable[8],"h"); /*wysoko¶æ warstwownicy Hw_l/Hw_p*/
	sprintf(partable[9],"Fpa%d",nr_kotla); /*Strumieñ powietrza w analizie Fp_4m3_o*/
	strcpy(partable[10],"anal"); /*rezultaty analizy - AnalyseStatus*/
	partablesize=11;
}else{ /* WR25 */
	strcpy(partable[4],"Pws4"); /*Wzglêdna ilo¶æ powietrza dla 1.6m3/h wêgla -Fp_4m3_i UWAGA TEN PARAMETR JEST PODEJRZANY!!!! */ 
	strcpy(partable[5],"vl"); /* prêdko¶æ lewego rusztu Vr_l */
	strcpy(partable[6],"vp"); /* prêdko¶æ prawego rusztu Vr_p */
	strcpy(partable[7],"Dpow");/*dolna granica powietrza w analizie Min_Fp_4m3*/
	strcpy(partable[8],"Gpow");/*górna granica powietrza w analizie Max_Fp_4m3 */
	strcpy(partable[9],"hl"); /*wysoko¶æ lewej warstwownicy HW_l*/
        strcpy(partable[10],"hp"); /*wysoko¶æ lewej warstwownicy Hw_p*/
	sprintf(partable[11],"Fpa%d",nr_kotla);/*Strumieñ powietrza w analizie Fp_4m3_o*/
	strcpy(partable[12],"anal");/*rezultaty analizy - AnalyseStatus*/
	partablesize=13;
}
partableptr=0;
linestatus = 0;
lineptr = 1;
Vr_comma = 0 ;
wel_lock = FALSE;
while (linestatus!=EOF){
 lineptr++;
 linestatus=readline(f,line_buf,sizeof(line_buf));
 getlinedata(line_buf,sizeof(line_buf),&ilinedata);
 for (i=partableptr;i<partablesize;i++){
	 if (plstrcasecmp(ilinedata.boilername,boiler_name)==0){ /* Wlasciwy kociol */
		if (strcmp(ilinedata.parname, partable[i])==0){ /* Wlasciwy parametr */
			if (WRType!=0){
				switch (i){
				case 0:
					if (wel_lock == FALSE){
						a_enable = lineptr - 2;
						wel_lock = TRUE;
					}
				break;
				case 1:
					work_timer_h = lineptr - 2;
				break;
				case 2:
					Vc = lineptr - 2;
				break;
				case 3:
					En_Vc = lineptr - 2;
				break;
				case 4:
					Fp_4m3_i = lineptr - 2;
				break;
				case 5:
					Vr_l = Vr_p = lineptr - 2;
					Vr_comma = ilinedata.comma ;
				break;
				case 6:
					Min_Fp_4m3 = lineptr - 2;
				break;
				case 7:
					Max_Fp_4m3 = lineptr - 2;
				break;
				case 8:
					Hw_l = Hw_p = lineptr - 2;
				break;
				case 9:
					Fp_4m3_o = lineptr - 2;
				break;
				case 10:
					AnalyseStatus = lineptr - 2;
				break;
				}
			}else{
				switch (i){
				case 0:
					if (wel_lock == FALSE){
						a_enable = lineptr - 2;
						wel_lock = TRUE;
					}
				break;
				case 1:
					work_timer_h = lineptr - 2;
				break;
				case 2:
					Vc = lineptr - 2;
				break;
				case 3:
					En_Vc = lineptr - 2;
				break;
				case 4:
					Fp_4m3_i = lineptr - 2;
				break;
				case 5:
					Vr_l = lineptr - 2;
					Vr_comma = ilinedata.comma ;  /* Dla niebezpieczenstwa dalem tylko predkosc lewego rusztu */
				break;
				case 6:
					Vr_p = lineptr - 2;
				break;
				case 7:
					Min_Fp_4m3 = lineptr - 2;
				break;
				case 8:
					Max_Fp_4m3 = lineptr - 2;
				break;
				case 9:
					Hw_l  = lineptr - 2;
				break;
				case 10:
					Hw_p = lineptr - 2;
				break;
				case 11:
					Fp_4m3_o = lineptr - 2;
				break;
				case 12:
					AnalyseStatus = lineptr - 2;
				break;
				}

			}
		}

	 }
 }
}
	
fclose(f);

 fprintf(stdout,"#zmienne otrzymane od regulatora kot³a i ich adresy w pamiêci dzielonej\n\n");
 fprintf(stdout,"InData.a_enable=%d \t#Zakodowany stan wej¶æ logicznych\n",a_enable);
 fprintf(stdout,"InData.work_timer_h=%d \t #Czas pracy w analizie\n",work_timer_h);
 fprintf(stdout,"InData.Vc=%d \t #Aktualna objêto¶æ wêgla\n",Vc);
 fprintf(stdout,"InData.En_Vc=%d \t #Stosunek energia/objêto¶æ\n",En_Vc);
 fprintf(stdout,"InData.Fp_4m3=%d \t #Wzglêdna ilo¶æ powietrza dla 0,4 m3 wêgla\n",Fp_4m3_i);
 fprintf(stdout,"InData.WRtype=%d \t # typ kotla: WR-25 = 0  , WR-10 = 1  , WR-5 = 2 , WR-2,5\n",WRType);
 fprintf(stdout,"InData.Vr_l=%d \t #Prêdko¶æ lewego rusztu\n",Vr_l);
 fprintf(stdout,"InData.Vr_p=%d \t #Prêdko¶æ prawego rusztu\n",Vr_p);
 fprintf(stdout,"InData.Min_Fp_4m3=%d \t #Dolna granica powietrza w analizie\n",Min_Fp_4m3);
 fprintf(stdout,"InData.Max_Fp_4m3=%d \t #Górna granica powietrza w analizie\n",Max_Fp_4m3);
 fprintf(stdout,"InData.Hw_l=%d \t #Wysoko¶æ lewej warstwownicy\n",Hw_l);
 fprintf(stdout,"InData.Hw_p=%d \t #Wysoko¶æ prawej warstwownicy\n",Hw_p);
 
 fprintf(stdout,"\n#zmienne wysylane do kotla i ich adresy w pamieci dzielonej\n");
 fprintf(stdout,"OutData.Fp_4m3=%d \t #Strumieñ powietrza w analizie\n",Fp_4m3_o);
 fprintf(stdout,"OutData.AnalyseStatus=%d \t #rezultaty analizy\n",AnalyseStatus);

 fprintf(stdout,"\n# %%, granica górna zmian prêdko¶ci rusztu podczas jednego cyklu analizy\n");
 fprintf(stdout,"InData.d_Vr=15.0\n"); 
 fprintf(stdout,"# %%, granica górna zmian wysoko¶ci warstownicy podczas jednego cyklu analizy\n");
 fprintf(stdout,"InData.d_Hw=4.0\n"); 

/* WR-5  */
if (WRType==2){
	switch (Vr_comma){
		case 0:{
			strcpy (Vr_PartI_Low, "3") ;
			strcpy (Vr_PartI_Hi, "5") ;
			strcpy (Vr_PartI_Duration, "10800") ;
			//
			strcpy (Vr_PartII_Low,"4");
			strcpy (Vr_PartII_Hi,"6");
			strcpy (Vr_PartII_Duration,"8040");
			//
			strcpy (Vr_PartIII_Low,"5");
			strcpy (Vr_PartIII_Hi,"7");
			strcpy (Vr_PartIII_Duration,"7200");
		       }
		break;

		case 1:{
			strcpy (Vr_PartI_Low, "3.0") ;
			strcpy (Vr_PartI_Hi, "5.0") ;
			strcpy (Vr_PartI_Duration, "10800") ;
			//
			strcpy (Vr_PartII_Low,"4.0");
			strcpy (Vr_PartII_Hi,"6.0");
			strcpy (Vr_PartII_Duration,"8040");
			//
			strcpy (Vr_PartIII_Low,"5.0");
			strcpy (Vr_PartIII_Hi,"7.0");
			strcpy (Vr_PartIII_Duration,"7200");
		       }
		break;
		case 2:
			{
			strcpy (Vr_PartI_Low, "3.00") ;
			strcpy (Vr_PartI_Hi, "5.00") ;
			strcpy (Vr_PartI_Duration, "10800") ;
			//
			strcpy (Vr_PartII_Low,"4.00");
			strcpy (Vr_PartII_Hi,"6.00");
			strcpy (Vr_PartII_Duration,"8040");
			//
			strcpy (Vr_PartIII_Low,"5.00");
			strcpy (Vr_PartIII_Hi,"7.00");
			strcpy (Vr_PartIII_Duration,"7200");
			}
		break;
		case 3:
			{
			strcpy (Vr_PartI_Low, "3.000") ;
			strcpy (Vr_PartI_Hi, "5.000") ;
			strcpy (Vr_PartI_Duration, "10800") ;
			//
			strcpy (Vr_PartII_Low,"4.000");
			strcpy (Vr_PartII_Hi,"6.000");
			strcpy (Vr_PartII_Duration,"8040");
			//
			strcpy (Vr_PartIII_Low,"5.000");
			strcpy (Vr_PartIII_Hi,"7.000");
			strcpy (Vr_PartIII_Duration,"7200");
			}
		break;

	}
}
/* WR-10 */
if (WRType==1){
	switch (Vr_comma){
		case 0:
			{
			strcpy(Vr_PartI_Low,"3"); 
			strcpy(Vr_PartI_Hi,"5"); 
			strcpy(Vr_PartI_Duration,"10800");

			strcpy(Vr_PartII_Low,"4" );
			strcpy(Vr_PartII_Hi,"6"  );
			strcpy(Vr_PartII_Duration,"8040" );

			strcpy(Vr_PartIII_Low,"5" );
			strcpy(Vr_PartIII_Hi,"20" );
			strcpy(Vr_PartIII_Duration,"7200"); 
			}
		break;
		case 1:{
			strcpy(Vr_PartI_Low,"3.0"); 
			strcpy(Vr_PartI_Hi,"5.0"); 
			strcpy(Vr_PartI_Duration,"10800");

			strcpy(Vr_PartII_Low,"4.0" );
			strcpy(Vr_PartII_Hi,"6.0"  );
			strcpy(Vr_PartII_Duration,"8040" );

			strcpy(Vr_PartIII_Low,"5.0" );
			strcpy(Vr_PartIII_Hi,"20.0" );
			strcpy(Vr_PartIII_Duration,"7200"); 
		       }
		break;

		case 2:
			{
			strcpy(Vr_PartI_Low,"3.00"); 
			strcpy(Vr_PartI_Hi,"5.00"); 
			strcpy(Vr_PartI_Duration,"10800");

			strcpy(Vr_PartII_Low,"4.00" );
			strcpy(Vr_PartII_Hi,"6.00"  );
			strcpy(Vr_PartII_Duration,"8040" );

			strcpy(Vr_PartIII_Low,"5.00" );
			strcpy(Vr_PartIII_Hi,"20.00" );
			strcpy(Vr_PartIII_Duration,"7200"); 
			}
		break;
		case 3:
			{
			strcpy(Vr_PartI_Low,"3.000"); 
			strcpy(Vr_PartI_Hi,"5.000"); 
			strcpy(Vr_PartI_Duration,"10800");

			strcpy(Vr_PartII_Low,"4.000" );
			strcpy(Vr_PartII_Hi,"6.000"  );
			strcpy(Vr_PartII_Duration,"8040" );

			strcpy(Vr_PartIII_Low,"5.000" );
			strcpy(Vr_PartIII_Hi,"20.000" );
			strcpy(Vr_PartIII_Duration,"7200"); 
			}
		break;

	}
}
/* WR-25 */
if (WRType==0){
	switch (Vr_comma){
		case 0:{
			strcpy(Vr_PartI_Low,"3"); 
			strcpy(Vr_PartI_Hi,"5");  
			strcpy(Vr_PartI_Duration,"10800");  

			strcpy(Vr_PartII_Low,"4"); 
			strcpy(Vr_PartII_Hi,"6");  
			strcpy(Vr_PartII_Duration,"8040");

			strcpy(Vr_PartIII_Low,"5");
			strcpy(Vr_PartIII_Hi,"20" );
			strcpy(Vr_PartIII_Duration,"7200" );
		       }
		break;

		case 1:{
			strcpy(Vr_PartI_Low,"3.0"); 
			strcpy(Vr_PartI_Hi,"5.0");  
			strcpy(Vr_PartI_Duration,"10800");  

			strcpy(Vr_PartII_Low,"4.0"); 
			strcpy(Vr_PartII_Hi,"6.0");  
			strcpy(Vr_PartII_Duration,"8040");

			strcpy(Vr_PartIII_Low,"5.0");
			strcpy(Vr_PartIII_Hi,"20.0" );
			strcpy(Vr_PartIII_Duration,"7200" );
		       }
		break;
		case 2:{
			strcpy(Vr_PartI_Low,"3.00"); 
			strcpy(Vr_PartI_Hi,"5.00");  
			strcpy(Vr_PartI_Duration,"10800");  

			strcpy(Vr_PartII_Low,"4.00"); 
			strcpy(Vr_PartII_Hi,"6.00");  
			strcpy(Vr_PartII_Duration,"8040");

			strcpy(Vr_PartIII_Low,"5.00");
			strcpy(Vr_PartIII_Hi,"20.00" );
			strcpy(Vr_PartIII_Duration,"7200" );
		       }
		break;
		case 3:{
			strcpy(Vr_PartI_Low,"3.000"); 
			strcpy(Vr_PartI_Hi,"5.000");  
			strcpy(Vr_PartI_Duration,"10800");  

			strcpy(Vr_PartII_Low,"4.000"); 
			strcpy(Vr_PartII_Hi,"6.000");  
			strcpy(Vr_PartII_Duration,"8040");

			strcpy(Vr_PartIII_Low,"5.000");
			strcpy(Vr_PartIII_Hi,"20.000" );
			strcpy(Vr_PartIII_Duration,"7200" );
		       }
		break;

	}
}
fprintf(stdout,"\n# Przedzial I\n");

fprintf(stdout,"InData.Vr_PartI_Low=%s # m/h, granica dolna przedzialu - predkosc rusztu\n",Vr_PartI_Low);
fprintf(stdout,"InData.Vr_PartI_Hi=%s  # m/h, granica gorna przedzialu - predkosc rusztu\n",Vr_PartI_Hi);
fprintf(stdout,"InData.Vr_PartI_Duration=%s  # czas przeprowadzania analizy dla tego przedzialu w sek.\n",Vr_PartI_Duration);

fprintf(stdout,"\n# Przedzial II\n");

fprintf(stdout,"InData.Vr_PartII_Low=%s # m/h, granica dolna przedzialu - predkosc rusztu\n",Vr_PartII_Low);
fprintf(stdout,"InData.Vr_PartII_Hi=%s  # m/h, granica gorna przedzialu - predkosc rusztu\n",Vr_PartII_Hi);
fprintf(stdout,"InData.Vr_PartII_Duration=%s # czas przeprowadzania analizy dla tego przedzialu w sek.\n",Vr_PartII_Duration);

fprintf(stdout,"\n# Przedzial III\n");

fprintf(stdout," InData.Vr_PartIII_Low=%s # m/h, granica dolna przedzialu - predkosc rusztu\n",Vr_PartIII_Low);
fprintf(stdout," InData.Vr_PartIII_Hi=%s  # m/h, granica gorna przedzialu - predkosc rusztu\n",Vr_PartIII_Hi);
fprintf(stdout," InData.Vr_PartIII_Duration=%s # czas przeprowadzania analizy dla tego przedzialu w sek.\n",Vr_PartIII_Duration);
return 0; 
}
