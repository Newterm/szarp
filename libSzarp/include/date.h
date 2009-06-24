/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka frombase
 * date.h
 */

#ifndef DATE_H
#define DATE_H

#include <stdio.h>

typedef struct tagMyDate {
			  int	year;		/* czterocyfrowy */
			  int	month;		/* 1=styczen..12=grudzien */
			  int	mday;		/* dzien miesiaca */
			  int	week;		/* numer tygodnia w roku - od 1 */
			  int	wday;		/* dzien tygodnia = 1=Niedziela..7=Sobota */
			  int	hour;		/* godzina */
			  int	min;		/* minuty */
			 } MyDate;

extern long     GetJulianDayFromDate(MyDate *Date);
extern void	Month2Week(MyDate *Date);
extern void     GetDateFromJulianDay(long day, MyDate *Date);
extern void	ConvertToDate(MyDate *Dest, MyDate Source, int XAxis, int firstparam, int oldparam);
extern long     GetDateDistance(int XAxis, MyDate *FirstDate, MyDate *SecondDate);
extern int	CompareDates(int period, MyDate *FirstDate, MyDate *SecondDate);

/**
 * Wypisz date, wersja krotka.
 * Wypisuje date poprzedzona komunikatem w formacie "msg YYYY.MM.DD\n"
 * \param file - gdzie (stdout, stderr, ...)
 * \param msg - komunikat
 * \param date - data do wypisania
 */
extern void	DebugDateShort(FILE * file, const char * msg, MyDate * date);

#endif
