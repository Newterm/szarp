#!/bin/bash

# winsetup.sh
# Skrypt tworz±cy wersjê instalacyjn± programu pod Windows z wykorzystaniem
# Nullsoft Scriptable Install System.

# Pawe³ Pa³ucha <pawel@praterm.com.pl>
# $Id$

# Sposób u¿ycia:
# winsetup.sh -c [ -d <dest_dir> file ]
# winsetup.sh <name> <executable> [ -d <dest_dir> file ... ] ...
# winsetup.sh -m [ -d <dest_dir> file ... ] ...
 
# Pierwsza postaæ tworzy program instalacyjny dla bibliotek wspó³dzielonych
# SZARP'a, druga - dla konkretnego programu, trzecia dla grupy programow. Argumenty:
# name - nazwa programu, nie powinna zawieraæ spacji ani polskich znaków
# executable - ¶cie¿ka do g³ównego pliku wykonywalnego, umieszczony on bêdzie
# w podkatalogu 'bin'; je¿eli ¶cie¿ka jest pusta, to utworzony zostanie 
# instalator dla wspólnych bibliotek SZARP, zawieraj±cy biblioteki DLL itp.
# dest_dir - podkatalog w którym umieszczane bêd± kolejne pliki
# file - ¶cie¿ka do pliku umieszczanego w instalce
# 
# Przyk³ad:
# winsetup.sh Raporter3 raporter3.exe -d resources @srcdir@/resources/dtd/params-list.rng
# Powy¿sze wywo³anie utworzy instalkê zawieraj±c± nastepuj±ce pliki i
# katalogi:
# C:\Program Files\SZARP
#	\bin
#		\raporter3.exe
#		\*.dll - wymagane biblioteki DLL
#	\resources
#		\params-list.rng




# Kasuje katalog tymczasowy
function RmTmp() {
	if [ -d "$TEMPDIR" ] ; then
		rm -rf "$TEMPDIR";
	fi
}

# Wypisuje informacjê o b³êdzie i koñczy program.
function Error() {
	_STR=$1
	echo "winsetup error: $_STR"
	#RmTmp
	exit 1
}

# Zmienne globalne
TEMPDIR=`mktemp -t -d winsetup.XXXXXX` || Error "Cannot create temporary directory";
CURDIR=`dirname "$0"`
STRIP="i586-mingw32msvc-strip"
MAKENSIS="makensis"
GUNZIP="gunzip"
GREP="grep"
SED="sed"
ICONV="iconv"
SORT="sort"
PROGS="draw3.exe scc.exe ssc.exe wxhelp.exe raporter3.exe ekstraktor3.exe isledit.exe szau.exe szast.exe"
COMMON=
MULTI=

# FUNKCJE DO ROZWINIÊCIA
function EvalFunct()
{
	#KILL_ALL_PROCESSES
	for i in $PROGS
	do
		echo -e "StrCpy \$PROCESS \"$i\" \n Call un.Kill\n" >> "`GetTmp`/KillProcUninstall.nsh"
		echo -e "StrCpy \$PROCESS \"$i\" \n Call Kill\n" >> "`GetTmp`/KillProcUpgrade.nsh"
	done
}	

# Wypisuje informacje o sposobie u¿ycia i koñczy program
function Usage() {
	if [ -n "$1" ] ; then
		echo -e "winsetup.sh error: $1";
	fi
	echo -e "\n\
Creates Windows installation package for program.\n\
\n\
Usage: \n\
winsetup.sh -c [ -d <dir> <file> ...] ...\n\
winsetup.sh <name> <executable> [ -d <dir> <file> ... ] ...\n\
\t-c\t\tcreate setup for common SZARP libraries\n\
\tname\t\tname of program (only ASCII letters and digits, no spaces)\n\
\texecutable\tpath to main executable file\n\
\tdir\t\tdirectory when to place following files\n\
\tfile\t\tfile to add to install package\n\
";
	RmTmp
	exit 2
}

# Zwraca ¶cie¿kê do katalogu tymczasowego, w razie potrzeby go tworzy
function GetTmp() {
	if [ ! -d "$TEMPDIR" ] ; then
		Error "Temporary dir '$TEMPDIR' does not exist!";
	fi
	echo $TEMPDIR
}

# Dodaje plik do listy plików do instalacji/usuniêcia. Plik powinien byæ
# podany ze ¶cie¿k± od g³ównego katalogu instalacyjnego.
function AddFile() {
	local _FILE=$1
	echo "Adding file: $_FILE"
	
	local access1="AccessControl::GrantOnFile \""
	local access2="\" \"\$USER\" \"GenericRead + GenericWrite\""

	# plik w wersji dla Windows
	local _NAME=`echo $_FILE | tr '/' '\\\\'`
	# sprawd¼ co z katalogiem 
	local _DIR=`dirname $_FILE`
	local _DIRSTR="SetOutPath \$INSTDIR/$_DIR"
	echo "$access1\$INSTDIR/$_DIR$access2" >> "`GetTmp`/FileAccessList.nsh"
	_DIRSTR=`echo $_DIRSTR | tr '/' '\\\\'` 
	grep "$_DIR"  "`GetTmp`/dirlist" &> /dev/null || {
		# dodaj katalog do listy
		echo "$_DIRSTR" >> "`GetTmp`/FileList.nsh"
		echo $_DIR >> "`GetTmp`/dirlist"
	}
	# dodaj kopiowanie/usuwanie pliku
	echo "$access1$_FILE$access2" >> "`GetTmp`/FileAccessList.nsh"
	echo "FILE \"$_FILE\"" >> "`GetTmp`/FileList.nsh"
	echo "Delete \$INSTDIR\\$_NAME" >> "`GetTmp`/DeleteList.nsh"
}

# Generuje listê katalogów do usuniêcia podczas deinstalacji
function ListRmdir() {
	local _DIRLIST=`GetTmp`/dirlist
	local _N=`wc -l $_DIRLIST | cut -d ' ' -f 1`
	local _LIST=
	while [ $_N -gt 0 ] ; do
		local _I=`head -n $_N $_DIRLIST | tail -n 1`

		while echo $_I | grep '/' &> /dev/null; do
			_LIST="${_LIST}RmDir \$INSTDIR/${_I}\n"
			_I=`dirname $_I`
		done
		_LIST="RmDir \$INSTDIR/$_I\n$_LIST"
		
		_N=$(($_N-1))
	done
	echo -en $_LIST | $SORT -r | tr '/' '\\\\' >> "`GetTmp`/DeleteList.nsh"
}

# Sprawdza czy obecny jest program o nazwie podanej jako argument
function CheckProgram() {
	local _PROG=$1
	which "$_PROG" &> /dev/null || \
		Error "Program $_PROG does not exist";
}

# Kopiuje biblioteki DLL do podanego katalogu
function CopyDlls()
{
	local _DIR=$1
	mkdir -p "$_DIR"

	MINGW="/usr/share/doc/mingw32-runtime/mingwm10.dll.gz"
	DDLS="\
	/usr/local/i586-mingw32msvc/lib/wxbase28u_*.dll \
	/usr/local/i586-mingw32msvc/lib/wxmsw28u_*.dll \
	/usr/local/i586-mingw32msvc/lib/*eay32*.dll \
	/usr/local/i586-mingw32msvc/lib/*ssl*.dll \
	/usr/local/i586-mingw32msvc/*/zlib*.dll \
	/usr/local/i586-mingw32msvc/bin/*iconv*.dll \
	/usr/local/i586-mingw32msvc/bin/libxml2*.dll \
	/usr/local/i586-mingw32msvc/lib/librsync*.dll \
	/usr/local/i586-mingw32msvc/bin/libcares*.dll \
	/usr/local/i586-mingw32msvc/bin/libexpat*.dll \
	/usr/local/i586-mingw32msvc/*/lua*.dll \
	$MINGW"
	
	cat >> "`GetTmp`/FileList.nsh" <<EOF
SetOutPath \$INSTDIR\bin
EOF

	for i in $DDLS ; do
		if [ ! -f $i ] ; then
			Error "Can't find DLL $i"
		fi
		AddFile "bin/`basename "$i" .gz`"
		cp $i "$_DIR/bin"
	done
	
	$GUNZIP "$_DIR"/bin/*.gz
}

# Przygotowuje z szablonu ostateczn± wersjê skryptu dla programu makensis.
# Parametry - nazwa programu i pliku wykonywalnego.
function PrepareScript() {
	local _NAME="$1"
	local _EXEC="$2"
	cp "$CURDIR"/*.nsh "`GetTmp`"
	#cp "$CURDIR"/user.ini "`GetTmp`"
	
	cat "$CURDIR"/script.nsi \
		| $SED -e "s/__NAME__/$_NAME/g" \
		| $SED -e "s/__EXEC__/$_EXEC/g" \
		| $ICONV -f latin2 -t cp1250 \
		> "`GetTmp`/script.nsi"
	cat "$CURDIR"/require.ini \
		| $ICONV -f latin2 -t cp1250 \
		> "`GetTmp`/require.ini"
	cat "$CURDIR"/../../gpl-2.0-pl.txt \
		| $SED -e "s/__NAME__/$_NAME/g" \
		| $ICONV -f latin2 -t cp1250 \
		> "`GetTmp`/license.pl"
	cat "$CURDIR"/../../gpl-2.0.txt \
		| $SED -e "s/__NAME__/$_NAME/g" \
		> "`GetTmp`/license.en"
	PROGS=$_EXEC
	EvalFunct
}

function PrepareMultiScript() {
	local _NAME="$1"
	local _DIR=$2
	$STRIP "$_DIR/bin/draw3.exe"
	$STRIP "$_DIR/bin/ssc.exe"
	$STRIP "$_DIR/bin/scc.exe"
	$STRIP "$_DIR/bin/szau.exe"
	$STRIP "$_DIR/bin/wxhelp.exe"
	$STRIP "$_DIR/bin/ekstrator3.exe"
	$STRIP "$_DIR/bin/raporter3.exe"
	$STRIP "$_DIR/bin/isledit.exe"
	$STRIP "$_DIR/bin/szast.exe"

	cp "$CURDIR"/*.nsh "`GetTmp`"
	#cp "$CURDIR"/user.ini "`GetTmp`"
	
	cat "$CURDIR"/script.nsi \
		| $SED -e "s/__NAME__/$_NAME/g" \
		| $SED -e "s/;!define SzarpPrograms/!define SzarpPrograms/g" \
		| $ICONV -f latin2 -t cp1250 \
		> "`GetTmp`/script.nsi"
	cat "$CURDIR"/require.ini \
		| $ICONV -f latin2 -t cp1250 \
		> "`GetTmp`/require.ini"
	cat "$CURDIR"/../../gpl-2.0-pl.txt \
		| $SED -e "s/__NAME__/$_NAME/g" \
		| $ICONV -f latin2 -t cp1250 \
		> "`GetTmp`/license.pl"
	cat "$CURDIR"/../../gpl-2.0.txt \
		| $SED -e "s/__NAME__/$_NAME/g" \
		> "`GetTmp`/license.en"
	EvalFunct
}

# Przygotowuje z szablonu ostateczn± wersjê skryptu dla programu makensis.
# Parametry - nazwa programu i pliku wykonywalnego.
function PrepareCommonScript() {
	local _DIR=$2
	$STRIP "$_DIR/bin/draw3.exe"
	$STRIP "$_DIR/bin/ssc.exe"
	$STRIP "$_DIR/bin/scc.exe"
	$STRIP "$_DIR/bin/szau.exe"
	$STRIP "$_DIR/bin/wxhelp.exe"
	$STRIP "$_DIR/bin/ekstrator3.exe"
	$STRIP "$_DIR/bin/raporter3.exe"
	$STRIP "$_DIR/bin/isledit.exe"
	$STRIP "$_DIR/bin/szast.exe"

	cat "$CURDIR"/common.nsi \
		| $ICONV -f latin2 -t cp1250 \
		> "`GetTmp`/script.nsi"
	cat "$CURDIR"/upgrade.ini \
		| $ICONV -f latin2 -t cp1250 \
		> "`GetTmp`/upgrade.ini"
	cp "$CURDIR"/*.nsh "`GetTmp`"
	#cp "$CURDIR"/user.ini "`GetTmp`"
	cat "$CURDIR"/../../gpl-2.0-pl.txt \
		| $SED -e "s/__NAME__//g" \
		| $ICONV -f latin2 -t cp1250 \
		> "`GetTmp`/license.pl"
	cat "$CURDIR"/../../gpl-2.0.txt \
		| $SED -e "s/__NAME__/$_NAME/g" \
		> "`GetTmp`/license.en"
	EvalFunct
}

# Przygotowuje pliki binarne (grafiki i biblioteki dll) dla instalatora
function PrepareBinaries() {
	# grafiki
	cp "$CURDIR"/*.ico "`GetTmp`"
	cp "$CURDIR"/*.bmp "`GetTmp`"
	# biblioteki
	cp "$CURDIR"/*.dll "`GetTmp`"
}

# Dodaje przy instalacji i usuwa podczas deinstalacji katalogi ze 
# zmiennej PATH
function PathEnv() {
	cat >> "`GetTmp`/FileList.nsh" <<EOF
Push \$INSTDIR
Call AddToPath
EOF
	cat >> "`GetTmp`/DeleteList.nsh" <<EOF
Push \$INSTDIR
Call un.RemoveFromPath
EOF

}

# G³ówny program
function Main() {
	mkdir "`GetTmp`/bin" || Error "Cannot create dir `GetTmp`/bin"
	
	# Pobierz nazwê programu
	local _NAME=$1; shift
	
	if [ "x$_NAME" == "x-c" ]; then
		COMMON=yes
	elif [ "x$_NAME" == "x-m" ]; then
		MULTI=yes
	else
	
		[ -z "$_NAME" ] && Usage "Program name not specified"
		echo "Program name: $_NAME"

	
		# Skopiuj g³ówny program
		local _EXEC=$1; shift
		if [ -z "$_EXEC" ]; then
			Usage "Executable name not specified!"
		fi
		if [ ! -f "$_EXEC" ]; then
			Error "Main executable '$_EXEC' not found"
		fi
		cp "$_EXEC" "`GetTmp`/bin"
		AddFile "bin/`basename \"$_EXEC\"`"
		$STRIP -g "`GetTmp`/bin/`basename $_EXEC`"
	fi

	# Skopiuj pozosta³e pliki
	local _DIR="`GetTmp`/o"
	while [ -n "$1" ] ; do
		if [ "$1" = "-d" ] ; then
			shift
			_DIR="$1"
			[ -n "$_DIR" ] || Usage "-d option without argument"
			mkdir -p "`GetTmp`/$_DIR" || \
				Error "Cannot create directory `GetTmp`/$_DIR"
		elif [ "$1" = "-f" ]; then
			shift
			DEST_FILE="$1";
			[ -n "$DEST_FILE" ] || Usage "-c option without first argument"
			shift
			SRC_FILE="$1";
			[ -n "$SRC_FILE" ] || Usage "-c option without second argument"
			[ -f "$1" ] || Error "File '$1' does not exist";

			cp "$SRC_FILE" "`GetTmp`/$_DIR/$DEST_FILE"
			AddFile "$_DIR/$DEST_FILE"
		else 
			[ -f "$1" ] || Error "File '$1' does not exist";
			cp "$1" "`GetTmp`/$_DIR"
			AddFile "$_DIR/`basename \"$1\"`"
		fi
		shift
	done

	# Sprawd¼ potrzebne narzêdzia
	CheckProgram "$MAKENSIS"
	CheckProgram "$STRIP"
	CheckProgram "$GUNZIP"
	CheckProgram "$GREP"
	CheckProgram "$SED"
	CheckProgram "$ICONV"
	CheckProgram "$SORT"
	
	# Skopiuj biblioteki DLL
	if [ "x$COMMON" == "xyes" ]; then
		CopyDlls "`GetTmp`"
	fi

	# Stwórz listê katalogów do usuniêcia podczas deinstalacji
	ListRmdir

	# Dodaj usuñ katalogi ze zmiennej PATH
	if [ "x$COMMON" == "xyes" ]; then
		PathEnv
	fi

	# Utwórz skrypty
	echo "Preparing scripts..."
	local _SCRIPT=
	if [ "x$COMMON" == "xyes" ]; then
		_NAME=Szarp
		PrepareCommonScript "$_NAME" "`GetTmp`"
	elif [ "x$MULTI" == "xyes" ]; then
		_NAME=Programs
		PrepareMultiScript "$_NAME" "`GetTmp`"

	else
		PrepareScript "$_NAME" `basename "$_EXEC"`
	fi

	# Przygotuj grafiki
	PrepareBinaries
	
	# Kompilacja
	echo "Runing makensis..."
	$MAKENSIS `GetTmp`/script.nsi || \
		Error "makensis failed! - see output above"

	# Kopiuj plik wynikowy
	echo "Copying installer file"
	cp `GetTmp`/${_NAME}Setup.exe . || \
		Error "Cannot copy result file to current dir!"

	# Czyszczenie
	RmTmp

	# Chyba siê uda³o...
	echo "++++++++++++++++++++++++++++++++++++++++++++"
	echo "Success - ${_NAME}Setup.exe created!"
	echo "++++++++++++++++++++++++++++++++++++++++++++"
}

Main $*

