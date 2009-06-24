#!/bin/bash
# SZARP: SCADA software 
# 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# $Id$
# 2006 Praterm S.A.
# Pawe³ Pa³ucha <pawel@praterm.com.pl>

# Input sumator - asks for data from user (using xdialog) and adds obtained value
# to parameter in file. Use with testdmn.
# Configuration in szarp.cfg (section insum)
#	config_prefix - configuration prefix
#	datafile - path to testdmn file
#	title - input box title
#	message - input box message
#	min - minimum acceptable value
#	max - maximum acceptable value
#	double - double (lsw/msw) value if "yes"

PREFIX=`/opt/szarp/bin/lpparse -s insum config_prefix`
if [ -z "$PREFIX" ] ; then
	echo "Cannot read config_prefix from szarp.cfg file" >&2
fi
FILE=`/opt/szarp/bin/lpparse -s insum datafile`
if [ -z "$FILE" ] ; then
	FILE="/opt/szarp/$PREFIX/insum.data"
fi

MSG=`/opt/szarp/bin/lpparse -s insum message`
if [ -z "$MSG" ] ; then
	MSG="Podaj warto¶æ parametru:"
fi
TITLE=`/opt/szarp/bin/lpparse -s insum title`
if [ -z "$TITLE" ] ; then
	TITLE="Wczytywanie warto¶ci"
fi
MIN=`/opt/szarp/bin/lpparse -s insum min`
if [ -z "$MIN" ] ; then
	MIN=0
fi
MAX=`/opt/szarp/bin/lpparse -s insum max`
if [ -z "$MAX" ] ; then
	MAX=32767
fi
DOUBLE=`/opt/szarp/bin/lpparse -s insum double`

while true ; do
	VAL=`Xdialog --stdout --title "$TITLE" --inputbox "$MSG" 0 0` || exit 1
	if [ -z "$VAL" ] ; then
		Xdialog --title "$TITLE" --msgbox "Warto¶æ nie mo¿e byæ pusta!" 0 0
		continue
	fi
	BCVAL=`echo "$VAL" | bc -l 2>/dev/null`
	if [ -z "$VAL" -o "$VAL" != "$BCVAL" ] ; then
		Xdialog --title "$TITLE" --msgbox "Wprowad¼ poprawn± liczbê!" 0 0
		continue
	fi
	if [ "$VAL" -lt "$MIN" ] ; then
		Xdialog --title "$TITLE" --msgbox "Za ma³a warto¶æ - musi byæ co najmniej $MIN" 0 0
		continue
	fi
	if [ "$VAL" -gt "$MAX" ] ; then
		Xdialog --title "$TITLE" --msgbox "Za du¿a warto¶æ - musi byæ co najwy¿ej $MAX" 0 0
		continue
	fi
	break
done
	
ERROR=
OLDVAL=`cat "$FILE" | head -n 1| tr -d ' \n'`
if [ -z "$OLDVAL" ] ; then
	OLDVAL=0
	ERROR=error
fi
if [ "`echo $OLDVAL | bc -l 2>/dev/null`" != "$OLDVAL" ] ; then
	echo "Incorrect value in data file, ignoring" 1>&2
	OLDVAL=0
	ERROR=error
fi

if [ -z "$ERROR" -a "$DOUBLE" = "yes" ] ; then
	OLDVAL2=`cat "$FILE" | head -n 2 | tail -n 1 | tr -d ' \n'`
	if [ -z "$OLDVAL2" ] ; then
		OLDVAL=0
		ERROR=error
	fi
	if [ "`echo $OLDVAL2 | bc -l 2>/dev/null`" != "$OLDVAL2" ] ; then
		echo "Incorrect value in data file, ignoring" 1>&2
		OLDVAL=0
		ERROR=error
	fi
fi

if [ -z "$ERROR" -a "$DOUBLE" = "yes" ] ; then
	OLDVAL=$(($OLDVAL << 16 | $OLDVAL2))
fi

VAL=$(($VAL+$OLDVAL))

if [ "$DOUBLE" = "yes" ] ; then
	echo $(($VAL >> 16)) > "$FILE"
	echo $(($VAL & 0xFFFF)) >> "$FILE"
else
	echo "$VAL" > "$FILE"
fi

Xdialog --title "$TITLE" --msgbox "Zapis poprawny - nowa warto¶æ sumaryczna $VAL" 0 0

