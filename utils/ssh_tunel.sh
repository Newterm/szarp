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

# $Id$

# maly skrypcik do utrzymywania w dzialaniu tunelikow

# wywo³anie:
# ssh_tunel.sh <remote_host> <port_on_remote_host> [port_to_forward(22)] [direction(R)]

remote=$1
port=$2
rport=$3
direction=$4

if [ x$rport = "x" ]
then
  rport=22
fi

if [ x$direction = "x" ]
then
  direction='R'
fi

cmdline="ssh $remote -$direction$port:localhost:$rport"
keepalive="/bin/sh -c \"while true; do echo .+.; sleep 5s; done\""
if [ x$remote = "x" ] || [ x$port = "x" ]
then
  echo
  echo "Wywo³anie programu: ssh_tunel.sh <nazwa-zdalnego-hosta> <port-na-zdalnym-hoscie> [<port-lokalny>] [<kierunek>L|R]"
  echo
  echo "Uwaga! Parametr <port-lokalny> mozna zwykle pominac (domyslna wartosc 22, czyli port SSH) (to samo dotyczy paramtru kierunek - zwykle R)"
  exit 1
fi

if ! ps axwh | grep -v "grep" | cut -b28- | grep " -$direction$port:localhost:22"
then
  $cmdline "$keepalive" < /dev/null > /dev/null 2>&1 &
  echo "Tunelowanie uruchomione"
# $cmdline "$keepalive"
else
  echo "Tunel ssh JUZ dziala na tym komputerze!"
fi
