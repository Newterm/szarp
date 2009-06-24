/* 
  libSzarp - SZARP library

*/
/* $Id$ */

/*
 * SZARP 2.0
 * biblioteka frombase
 * names.h
 */

#ifndef NAMES_H
#define NAMES_H

/** typ opisuj©cy okresy czasu, z jakich dost«pne s© dane */
typedef enum tagRADIO_ENUM {ROK, MIESIAC, TYDZIEN, DZIEN, SEZON, NUM_OF_RADIOS} RADIO_ENUM;

/** typ opisuj©cy semafory okre¯laj©ce dost«p do bazy */
typedef enum tagBASESEMAPHORES {READ_SEM, WRITE_SEM, NUM_OF_BASESEM} BASESEMAPHORES;

/** typ opisuj©cy semafory okre¯laj©ce dost«p do streamer'a */
typedef enum tagSTREAMERSEMAPHORES {SAVE_SEM, REMOTE_SEM, NUM_OF_STREAMSEM} STREAMSEMAPHORES;

/** typ opisuj©cy identyfikatory semafor®w zwi©zanych z meaner'em */
typedef enum tagSEMID {BASE_SEM_ID, STREAMER_SEM_ID} SEMID;

/** typ opisuj±cy segmenty pamiêdzi dzielonej zwi±zane z meanerem */
typedef enum tagMEANERSHM { HOUR8SUM_SHM, HOUR8COUNT_SHM, DAYSUM_SHM,
	DAYCOUNT_SHM, WEEKSUM_SHM, WEEKCOUNT_SHM, MONTHSUM_SHM, 
	MONTHCOUNT_SHM } MEANERSHM;

/** maxymalna liczba rekordów w jednym pliku */
#define MAXRECSPERFILE 4000

/** liczba próbek w jednym rekordzie */
#define NUMBEROFPROBES 15

/* postfixy rozr®±niaj©ce pliki wg kryteri®w czasowych */
#define DAYPOSTFIX "d"
#define WEEKPOSTFIX "w"
#define MONTHPOSTFIX "m"
#define SEASONPOSTFIX "s"
#define YEARPOSTFIX "y"
#define EXISTPOSTFIX "x"
#define MAPFILEPOSTFIX "@@@"
#define LADFILEPOSTFIX "lad"
#define WASHPOSTFIX "wash"

#endif
