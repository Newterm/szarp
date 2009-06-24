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
 
#
# SZARP Stanis³aw Sawa
# Prosty skrypcik przygotowywuj±cy do wy³±czenia isdn'a
#

if [ x$1 = x ]; then
  echo "podaj numer devicu"
  exit 1
else
  devicenum=$1
  device=ippp$1
fi

ifconfig $device down
isdnctrl delif $device
