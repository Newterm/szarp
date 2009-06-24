#!/bin/sh
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

#
# Stanis³aw Sawa, PRATERM
# $Id$
#
# ma³y, g³upiutki skrypcik dla poolingu szeregu stacji na isdn'ie
# i pobieraniu z nich bazek
# do wywo³ywania z cron'a w trybie */10 * * * *

# Argumenty - nazwy wêz³ów do obdzwonienia

# Je¿eli plik istnieje, to nie dzwonimy
[ -f /etc/szarp/nocall ] && exit

export PATH=/bin:/usr/bin:/sbin:/usr/sbin

ic=`which isdnctrl`
rs=`which rsync`

export RSYNC_RSH=ssh

# Usuwamy stare numery telefonów
T=`isdnctrl list ippp0 | grep Outgoing | cut -d ':' -f 2`
for i in $T ; do 
	$ic delphone ippp0 out $i
done

# lecimy po wêz³ach zapodanych z commandlinea
for base in $*; do
  phone=`grep $base /etc/szarp/phones | cut -f2`
  echo "$base $phone"
  if [ $phone -ne 0 ]; then 
    $ic addphone ippp0 out $phone
    $ic dial ippp0
    sleep 5
    if $ic status ippp0 | grep -q "ippp0 connected to"; then
      $rs -az --delete --force --delete-excluded --exclude='.*' root@192.168.8.1:/opt/szarp/$base/ /opt/szarp/$base/
      ssh root@192.168.8.1 '/usr/sbin/ntpdate -u 192.168.8.8'
    fi
    $ic hangup ippp0
    $ic delphone ippp0 out $phone
  fi
done

