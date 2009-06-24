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
 * funtable.c
 */


#include<math.h>

#define MAX_FID 8

extern unsigned char CalculNoData;

float Fixo(float *parlst)
 {
  short x,y,z;
  float Fi[3][3][33]={
   {{1.00, 0.97, 0.95, 0.92, 0.89, 0.87, 0.84, 0.82, 0.79, 0.76, 0.74, 0.71,
     0.68, 0.66, 0.63, 0.61, 0.58, 0.55, 0.53, 0.50, 0.47, 0.45, 0.42, 0.39,
     0.37, 0.34, 0.32, 0.29, 0.26, 0.24, 0.21, 0.18, 0.16},
    {1.04, 1.01, 0.99, 0.96, 0.93, 0.90, 0.88, 0.85, 0.82, 0.79, 0.77, 0.74,
     0.71, 0.68, 0.65, 0.63, 0.60, 0.57, 0.55, 0.52, 0.49, 0.47, 0.44, 0.41,
     0.38, 0.36, 0.33, 0.30, 0.27, 0.25, 0.22, 0.19, 0.16},
    {1.07, 1.04, 1.01, 0.99, 0.96, 0.93, 0.90, 0.87, 0.84, 0.82, 0.79, 0.76,
     0.73, 0.70, 0.68, 0.65, 0.62, 0.59, 0.56, 0.53, 0.51, 0.48, 0.45, 0.42,
     0.39, 0.37, 0.34, 0.31, 0.28, 0.25, 0.23, 0.20, 0.17}},
   {{0.99, 0.96, 0.93, 0.91, 0.88, 0.85, 0.82, 0.80, 0.77, 0.74, 0.71, 0.69,
     0.66, 0.63, 0.60, 0.58, 0.55, 0.52, 0.49, 0.47, 0.44, 0.41, 0.38, 0.35,
     0.33, 0.30, 0.27, 0.24, 0.22, 0.19, 0.16, 0.13, 0.11},
    {1.03, 1.00, 0.97, 0.94, 0.91, 0.89, 0.86, 0.83, 0.80, 0.77, 0.74, 0.71,
     0.68, 0.66, 0.63, 0.60, 0.57, 0.54, 0.51, 0.48, 0.45, 0.43, 0.40, 0.37,
     0.34, 0.31, 0.28, 0.25, 0.22, 0.20, 0.17, 0.14, 0.11},
    {1.06, 1.03, 1.00, 0.97, 0.94, 0.91, 0.88, 0.85, 0.82, 0.79, 0.76, 0.73,
     0.70, 0.67, 0.65, 0.62, 0.59, 0.56, 0.53, 0.50, 0.47, 0.44, 0.41, 0.38,
     0.35, 0.32, 0.29, 0.26, 0.23, 0.20, 0.17, 0.14, 0.11}},
   {{0.98, 0.95, 0.92, 0.89, 0.86, 0.84, 0.81, 0.78, 0.75, 0.72, 0.69, 0.66,
     0.63, 0.60, 0.57, 0.55, 0.52, 0.49, 0.46, 0.43, 0.40, 0.37, 0.34, 0.31,
     0.29, 0.26, 0.23, 0.20, 0.17, 0.14, 0.11, 0.06, 0.05},
    {1.02, 0.99, 0.98, 0.98, 0.90, 0.87, 0.84, 0.81, 0.78, 0.75, 0.72, 0.69,
     0.66, 0.63, 0.60, 0.57, 0.54, 0.51, 0.48, 0.43, 0.42, 0.39, 0.36, 0.33,
     0.30, 0.27, 0.24, 0.21, 0.18, 0.15, 0.12, 0.09, 0.06},
    {1.05, 1.02, 0.99, 0.96, 0.92, 0.89, 0.86, 0.83, 0.80, 0.79, 0.74, 0.71,
     0.68, 0.65, 0.61, 0.58, 0.55, 0.51, 0.49, 0.44, 0.43, 0.42, 0.37, 0.34,
     0.31, 0.27, 0.24, 0.21, 0.18, 0.15, 0.12, 0.09, 0.05}}};
  x=(short)rint(parlst[0]);	/* Pogoda */
  y=(short)rint(parlst[1]);	/* Wiatr */
  z=(short)rint(parlst[2]);	/* Tzewn */
  if (x<0)
    x=0;
  if (x>2)
    x=2;
  if (y<3)
    y=0;
  else if (y<8)
    y=1;
  else
    y=2;
  if (z<-20)
    z=-20;
  if (z>12)
    z=12;
  z+=20;
  return(Fi[x][y][z]);
 }

float TabRegZas(float *parlst)
 {
  short x;
  float Tz[81]={65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.0, 65.7, 
	      66.4, 67.7, 68.9, 70.2, 71.4, 72.7, 73.9, 75.1, 76.3, 77.6,
	      78.8, 80.0, 81.2, 82.4, 83.6, 84.8, 86.0, 87.2, 88.4, 89.6,
	      90.7, 91.9, 93.0, 94.2, 95.3, 96.5, 97.6, 98.8, 99.9, 101.1,
	      102.2, 103.3, 104.4, 105.6, 106.7, 107.8, 108.9, 110.0, 111.1,
	      112.3, 113.4, 114.5, 115.5, 116.6, 117.7, 118.8, 119.9, 121.0,
	      122.1, 123.2, 124.3, 125.4, 126.5, 127.6, 128.7, 129.9, 131.0, 
	      132.1, 133.1, 134.2, 135.3, 136.4, 137.5, 138.6, 139.7, 140.8,
	      141.9, 143.0, 144.0, 145.1, 146.2};
  x=(short)rint(parlst[0]*100.0);
  if (x<20)
    x=20;
  if (x>100)
    x=100;
  x-=20;
  return(Tz[x]);
 }  

float TabRegPow(float *parlst)
 {
  short x;
  float Tp[81]={49.3, 48.6, 47.9, 47.2, 46.4, 45.7, 45.0, 44.3, 43.5, 43.1,
	      42.6, 42.9, 43.2, 44.2, 45.2, 45.8, 46.3, 46.9, 47.4, 47.9,
	      48.4, 49.0, 49.5, 50.0, 50.4, 51.0, 51.5, 52.0, 52.5, 53.0,
	      53.4, 53.9, 54.4, 54.9, 55.4, 55.9, 56.3, 56.8, 57.3, 57.7,
	      58.1, 58.6, 59.1, 59.6, 60.0, 60.5, 60.9, 61.4, 61.8, 62.3,
	      62.7, 63.1, 63.5, 64.0, 64.4, 64.8, 65.2, 65.7, 66.1, 66.5,
	      66.9, 67.4, 67.8, 68.2, 68.6, 69.1, 69.5, 69.9, 70.3, 70.7,
	      71.1, 71.6, 72.0, 72.4, 72.7, 73.2, 73.6, 74.0, 74.4, 74.8,
	      75.1};
  x=(short)rint(parlst[0]*100.0);
  if (x<20)
    x=20;
  if (x>100)
    x=100;
  x-=20;
  return(Tp[x]);
 } 

float Positive(float *parlst)
 {
  if (parlst[0]>0.0)
    return(parlst[0]);
  else 
    return(0.0);
 }

float SumExisting(float *parlst)
 {
  float sum=0;
  int i;
  unsigned char valid=0;
  for(i=1;i<=rint(parlst[0]);i++)
   if (parlst[i]>-1e20)
    {
     sum+=parlst[i];
     valid+=1;
    }
  if (valid)
    CalculNoData=0;
  else
    CalculNoData=1; 
  return(sum);
 }

float CntExisting(float *parlst)
 {
  float sum=0;
  int i;
  for(i=1;i<=rint(parlst[0]);i++)
   if (parlst[i]>-1e20)
    sum+=1;
  CalculNoData=0;
  return(sum);
 }

float WaterTankSuwa(float *parlst)
 {
  int ind;
  float res;
  float H2Vtab[22]={ 0.0,  0.8,  2.4,  4.4,  6.6,  9.0, 11.6, 14.4, 17.2, 
                    20.0, 23.0, 25.8, 28.8, 31.6, 34.4, 37.0, 39.4, 41.6, 
                    43.6, 45.2, 46.0, 46.0};  
  if (parlst[0]<0.0)
    parlst[0]=0.0;
  if (parlst[0]>100.0)
    parlst[0]=100.0;
  ind=rint(floor(parlst[0]/5.0));
  res=(H2Vtab[ind+1]-H2Vtab[ind])/5.0*(parlst[0]-(float)ind*5.0)+H2Vtab[ind];
  return(res*10);
 } 

float BitOfLog(float *parlst)
 {
  int val;
  int sh;
  int res;
  val=rint(parlst[0]);
  sh=rint(parlst[1]);
  res=((val&(1<<sh))!=0);
/*  printf("BitOfLog(%d,%d)=%d\n", val, sh, res);  */
  return((float)res);
 }

float (*FunTable[MAX_FID])(float*)={Fixo, TabRegZas, TabRegPow, Positive,
				    SumExisting, CntExisting, WaterTankSuwa,
				    BitOfLog};


