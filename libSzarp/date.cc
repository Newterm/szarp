/* 
  libSzarp - SZARP library
*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka frombase
 * date.c
 */

#include <time.h>
#include "date.h"
#include "names.h"

/** zwraca dzien julianowski z Date */
long GetJulianDayFromDate(MyDate *Date)
{
 double y1  = Date -> year + ((double)(Date -> month - 1) - 1.85) / 12.0;
 int    y2  = y1 / 100;

 return  (long)((long)((long)(367 * y1) - 1.75 * (int)y1 + Date -> mday) - 0.75 * y2) + 1721115;
}

/** na podstawie month i mday ustala week i wday */
void
Month2Week(MyDate *Date)
{
    long j, jp;
    int month, mday;
    
    j = GetJulianDayFromDate(Date);
    month = Date->month;
    mday = Date->mday;
    Date->month = Date->mday = 1;
    jp = GetJulianDayFromDate(Date);
    Date->month = month;
    Date->mday = mday;
    Date->wday = (j + 1) % 7 + 1;
    Date->week = ((jp + 1) % 7 + j - jp) / 7 + 1;
}

/** z dnia day - takiego jak GetJulianDayFromDate - robi date Date */
void
GetDateFromJulianDay(long day, MyDate *Date)
{
    struct tm *tblock;
    
#ifdef __USLC__
    day = (day - 2440588) * 24 * 3600;
#else
    day = (day - 2440588) * 24 * 3600 + 18000;
#endif
    tblock = localtime(&day);
    Date->year = tblock->tm_year + 1900;
    Date->month = tblock->tm_mon + 1;
    Date->mday = tblock->tm_mday;
    Month2Week(Date);
}

/** Ustala date Dest wg daty Source na podstawie XAxis (okresla znaczace
 * pola daty) i roznicy firstparam - oldparam ; dacie Dest odpowiada
 * firstparam a dacie Source - oldparam
 * jesli np XAxis == ROK, to firstparam - oldparam wyraza roznice
 * w miesiacach m-dzy datami Dest a Source
 */
void
ConvertToDate(MyDate *Dest, MyDate Source, int XAxis, int firstparam, int oldparam)
{
    long temp = 0, temp2;

    Dest->year = Source.year;
    Dest->month = Source.month;
    Dest->mday = Source.mday;
    Dest->week = Source.week;
    Dest->wday = Source.wday;
    Dest->hour = Source.hour;
    Dest->min = Source.min;
 
    /* sluzy do przepisywania dat */
    if (firstparam == oldparam)
	return;
    
    switch (XAxis){
	case ROK:
	    Dest->month += firstparam - oldparam;
	    break;
	case SEZON:
	    temp = (firstparam - oldparam) * 7;
	    XAxis = MIESIAC;
	    break;
	case MIESIAC:   /* roznica firstparam - oldparam w dniach */
	    temp = firstparam - oldparam;
	    break;
	case TYDZIEN:  /* podzial co osiem godzin */
	    temp = 0;
	    Dest -> hour += (firstparam - oldparam) * 8;
	    break;
	case DZIEN:
	    temp = Dest->hour * 6 + Dest->min / 10 + firstparam - oldparam;
	    break;
    }

    switch (XAxis) {
	case DZIEN:
	    temp2 = temp;
	    if (temp2 >= (24 * 6)) {
		temp2 = temp % (24 * 6);
		temp /= (24 * 6);
	    }
	    else
		if (temp2 < 0) {
		    temp2 = ((24 * 6) - (-temp) % (24 * 6)) % (24 * 6);
		    temp = (temp - (24 * 6) + 1) / (24 * 6);
		}
		else
		    temp = 0;
	    Dest->hour = temp2 / 6;
	    Dest->min = (temp2 % 6) * 10;
	    /* FALLTHRU */
	case TYDZIEN:
	    if (Dest->hour >= 24) { /* moze byc 0, 8, 16 */
		temp += Dest->hour / 24;
		Dest->hour = Dest->hour % 24;
	    }
	    if (Dest->hour < 0) {
		temp += (Dest->hour - 16) / 24;
		Dest->hour -= (Dest->hour - 16) / 24 * 24;
	    }
	    /* FALLTHRU */
	case MIESIAC:
	    if (temp) {
		temp2 = GetJulianDayFromDate(Dest);
		GetDateFromJulianDay(temp + temp2, Dest);
		break; /* Dest is already correct */
	    }
	case ROK:
	    if (Dest->month > 12) {
		Dest->year += (Dest->month - 1) / 12;
		Dest->month = (Dest->month - 1) % 12 + 1;
	    }
	    if (Dest->month < 1) {
		Dest->year += (Dest->month - 12) / 12;
		Dest->month -= (Dest->month - 12) / 12 * 12;
	    }
    }
}

/** zwraca roznice mdzy dwoma datami wg XAxis */
long
GetDateDistance(int XAxis, MyDate *FirstDate, MyDate *SecondDate)
{
    switch (XAxis) {
	case ROK:
	    return (SecondDate->year - FirstDate->year) * 12 + SecondDate->month - FirstDate->month;
	case SEZON:
	    return (GetJulianDayFromDate(SecondDate) - 2440591) / 7 - (GetJulianDayFromDate(FirstDate) - 2440591) / 7;
	case MIESIAC:
	    return GetJulianDayFromDate(SecondDate) - GetJulianDayFromDate(FirstDate);
	case TYDZIEN:
	    return (GetJulianDayFromDate(SecondDate) - GetJulianDayFromDate(FirstDate)) * 3
		+ SecondDate->hour / 8 - FirstDate->hour / 8;
	case DZIEN:
	    return (GetJulianDayFromDate(SecondDate) - GetJulianDayFromDate(FirstDate)) * 144
		+ (SecondDate->hour - FirstDate->hour) * 6 + SecondDate->min / 10 - FirstDate->min / 10;
    }
    return 0;
}

/** porównuje daty wg period i zwraca:
 * > 0 gdy FirstDate starsza od SecondDate
 * < 0 gdy SecondDate starsza od FirstDate 
 * = 0 gdy FirstDate i SecondDate r®wne (wg period)
 *
 * <b>znacznie szybsze od GetDateDistance !!!</b>
 *
 * UWAGA - zak³ada siê, ¿e data przesz³a przez Month2Week i prawid³owa jest
 * data przy TYDZIEN - godzina 0/8/16 - to jest pobozne zyczenie, na razie
 * pelna kontrola godziny: dwa razy; docelowo to trzeba zmienic
 * najlepiej stosowaæ w pêtlach z krokiem ConvertToDate
 */
int
CompareDates(int period, MyDate *FirstDate, MyDate *SecondDate)
{
    if (FirstDate->year != SecondDate->year)
	return FirstDate->year - SecondDate->year;
    
    if (period == SEZON)
	return FirstDate->week - SecondDate->week;
    
    if ((FirstDate->month != SecondDate->month) || (period == ROK))
	return FirstDate->month - SecondDate->month;
    
    if ((FirstDate->mday != SecondDate->mday) || (period == MIESIAC))
	return FirstDate->mday - SecondDate->mday;
    
    if (period == TYDZIEN)
	return FirstDate->hour / 8 * 8 - SecondDate->hour / 8 * 8;
    
    if (FirstDate->hour != SecondDate->hour)
	return FirstDate->hour - SecondDate->hour;
    
    /* period == DZIEN */
    return FirstDate->min - SecondDate->min;
}

/*
 * Wypisuje date i komunikat na podanym urzadzeniu (stdout, stderr)
 */
void
DebugDateShort(FILE * file, const char * msg, MyDate * date)
{
    fprintf(file, msg);
    fprintf(file, " %4d.%02d.%02d\n", date->year, date->month, date->mday);
}


#ifdef __USLC__
static int		BaseSem = -1;		/* -2 przy braku semafora */
static unsigned char	OpenedSemaphore = 0;	/* =1 gdy podniesiony semafor */
static struct sembuf	Sem[NUM_OF_BASESEM];

static char		Meaner[80];
#endif

