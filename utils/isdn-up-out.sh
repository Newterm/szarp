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
# SZARP 2003 Stanis³aw Sawa
# Prosty skrypcik ustawiaj±cy wszystko co potrzeba do dzwonienie gdzie¶ po
# isdn'ie ...
#

if [ x$1 = x ]; then
  devicenum=0
  device=ippp0
else
  devicenum=$1
  device=ippp$1
fi

#numer telefonu
number=0202422

#konfiguracja dla dial-out
isdnctrl addif $device
isdnctrl addphone $device out $number
isdnctrl huptimeout $device 0
isdnctrl l2_prot $device hdlc
isdnctrl l3_prot $device trans
isdnctrl encap $device syncppp
/usr/sbin/ipppd \
  user ppp \
  ipcp-accept-local ipcp-accept-remote noipdefault lock \
  mru 1500 mtu 1500 usepeerdns useifip \
  /dev/$device
isdnctrl pppbind $device $devicenum
ifconfig $device up
