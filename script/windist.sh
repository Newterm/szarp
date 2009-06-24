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

# Skrypt do tworzenia paczek instalacyjnych wersji dla Windows programów
# wx'owych. Powinien wyszukiwaæ biblioteki, ale na razie zak³ada ¿e instalacja
# by³a wed³ug INSTALL.Windows.

# 2005 Praterm S.A.
# Pawe³ Pa³ucha <pawel@praterm.com.pl>

# Powinien byæ odpalany przez make'a w katalogu z programem, 
# argumentem jest nazwa programu, katalog ze ¼ród³ami oraz ewentualne
# dodatkowe pliki które maja wej¶æ w sk³ad instalki.

PROGRAM="$1"
SRC_DIR="$2"

function Usage() {
	echo -e "\
Usage: windist PROGRAM_NAME SRC_DIR\n"
	exit 1
}


if [ -z "$PROGRAM" ] ; then
	Usage
fi

if [ -z "$SRC_DIR" ] ; then
	Usage
fi

ZIP="which zip"
if [ -z "$ZIP" ] ; then
	echo "'zip' program not found"
	exit 1
fi

[ -f "$PROGRAM.exe" ] || {
	echo "File not found: $PROGRAM.exe"
	exit 1
}

TMPDIR=`mktemp -d /tmp/windist.XXXXXX` || { 
	echo "Cannot create temp dir"
	exit 1 
}
mkdir "$TMPDIR/$PROGRAM"

MINGW="/usr/share/doc/mingw32-runtime/mingwm10.dll.gz"
DDLS="\
/usr/local/i586-mingw32msvc/lib/wxbase28u_gcc_custom.dll \
/usr/local/i586-mingw32msvc/lib/wxmsw28u_adv_gcc_custom.dll \
/usr/local/i586-mingw32msvc/lib/wxmsw28u_core_gcc_custom.dll \
/usr/local/i586-mingw32msvc/lib/wxmsw28u_html_gcc_custom.dll \
/usr/local/i586-mingw32msvc/lib/wxmsw28u_aui_gcc_custom.dll \
/usr/local/i586-mingw32msvc/lib/wxmsw28u_gl_gcc_custom.dll \
/usr/local/i586-mingw32msvc/lib/wxmsw28u_richtext_gcc_custom.dll \
/usr/local/i586-mingw32msvc/lib/libfreetype-6.dll \
/usr/local/i586-mingw32msvc/bin/libiconv-2.dll \
/usr/local/i586-mingw32msvc/bin/libcharset-1.dll \
/usr/local/i586-mingw32msvc/bin/libxml2-2.dll \
/usr/local/i586-mingw32msvc/bin/libcurl-3.dll \
$MINGW"

for i in $DDLS ; do 
	if [ ! -f $i ] ; then
		echo "Can't find $i"
		rm -rf $TMPDIR
		exit 1
	fi
	cp $i $TMPDIR/$PROGRAM
done

gunzip $TMPDIR/$PROGRAM/*.gz

LOCDIR="$TMPDIR/$PROGRAM/pl_PL/LC_MESSAGES"
mkdir -p "$LOCDIR" || {
	echo "Cannot create dir $LOCDIR"
	rm -rf $TMPDIR
	exit 1
}

if [ -f "$SRC_DIR/$PROGRAM.mo" ] ; then
	cp "$SRC_DIR/$PROGRAM.mo" "$LOCDIR"
fi
cp "$SRC_DIR/../common/wx.mo" "$LOCDIR"
cp "$SRC_DIR/../common/common.mo" "$LOCDIR"

while [ -n "$3" ] ; do
	cp $3 "$TMPDIR/$PROGRAM"
	shift
done

cp "$PROGRAM.exe" "$TMPDIR/$PROGRAM"

CURDIR=`pwd`
rm -f "$PROGRAM.zip"
cd $TMPDIR
zip -r "$CURDIR"/$PROGRAM.zip $PROGRAM/* 
cd "$CURDIR"
rm -rf "$TMPDIR"

echo "$PROGRAM.zip created"
