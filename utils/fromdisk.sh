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

#
# Pobieranie bazy z dyskietki (ca³ej!)
# Stanis³aw SAWA, PRATERM 2004
#


if [ -z $DISPLAY ]; then
  DIALOG=dialog
else
  DIALOG=Xdialog
fi

if ! $DIALOG --yesno "W³ó¿ dyskietkê" 5 32; then
  clear
  exit 1
fi

tmp_dir=/tmp/db.tmp.$$

rm -fr $tmp_dir
mkdir -p $tmp_dir

umount /media/floppy0

if ! mcopy a:*.tbz $tmp_dir; then
  $DIALOG --msgbox "B³±d no¶nika lub brak danych!" 5 32
  clear
  rm -fr $tmp_dir
  exit 1
fi

#prefix=`hostname -s`
szarp_root=/opt/szarp

cd $szarp_root

for file in $tmp_dir/*.tbz; do
  prefix=`basename $file | sed 's#\.tbz$##'`
  bunzip2 $tmp_dir/$prefix.tbz
  tar xf $tmp_dir/$prefix.tar
  $DIALOG --msgbox "Uaktualniono bazê $prefix" 5 32
done

rm -fr $tmp_dir
$DIALOG --msgbox "Dane skopiowane - mo¿esz zabraæ dyskietkê" 5 48
