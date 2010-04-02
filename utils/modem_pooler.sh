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

# ma³y, g³upiutki skrypcik dla poolingu szeregu stacji modemowych
# i pobieraniu z nich bazek

# Wa¿ne aby, nazwy wêz³ów podawane byly w kolejno¶ci rosn±cej. np
# modem_pooler.sh glw1 glw3 glw4 glw6

export PATH=/bin:/usr/bin:/sbin:/usr/sbin

DEBUG=""
if [ "$1" = "-m" ]; then
	DEBUG=debug
	shift
fi

szpppon='/opt/szarp/bin/szppp-on'
szpppoff='/opt/szarp/bin/szppp-off'
rs=`which rsync`
rs_options="-az --delete --force --exclude='.*' --exclude='nohup.out' --timeout=$POOLER_RSYNC_TIMEOUT"
szbase_options=" --include=`date +%Y%m.szb` --include=`date +%Y%m -d '1 month ago'`.szb --exclude='*.szb'"
host=`hostname -s`
nr_of_phnones=`grep -c glw /etc/szarp/phones`
nr_of_finished_phones=1
results=

#checking if /etc/szarp/nocall exist and exit
nocall=`ls /etc/szarp | grep nocall`
if [ -z "$DEBUG" -a -n "$nocall" ]; then
	echo "Do nothing - /etc/szarp/nocall exist"
	exit
fi

#export results to file
function export_results () {
	nr_of_finished_phones=1;
	while [ $nr_of_finished_phones -le $nr_of_phnones ]; do
		echo ${results[$nr_of_finished_phones]} >> /opt/szarp/$host/modem_stats
		nr_of_finished_phones=$[nr_of_finished_phones + 1]		
	done
}	

#closing session by another modem_pooler and waitng
#for testdmn to read data and erse file
function wait_for_close_session() {
	sleep 30
}

trap wait_for_close_session SIGINT
killall -s INT modem_pooler.sh


function finish_session() {
	killall -s INT pppd
	sleep 3
	killall -s KILL pppd
	sleep 3
	results[$nr_of_finished_phones]="-9"
	nr_of_finished_phones=$[nr_of_finished_phones + 1]		
	while [ $nr_of_finished_phones -le $nr_of_phnones ]; do
		results[$nr_of_finished_phones]="NO_DATA"
		nr_of_finished_phones=$[nr_of_finished_phones + 1]		
	done
	export_results
	exit
}



trap finish_session SIGINT


export RSYNC_RSH=ssh

# lecimy po wêz³ach
for base in $*; do
  phone=`grep $base /etc/szarp/phones | cut -f2`
  nr_of_line=`grep -n $base /etc/szarp/phones | cut -b1`
  if [ -n "$phone" ]; then 
  	while [ $nr_of_finished_phones -lt $nr_of_line ]; do
		results[$nr_of_finished_phones]="NO_DATA"
		nr_of_finished_phones=$[nr_of_finished_phones + 1]
	done
  
    echo -n "$base: calling($phone) "
    if [ -z $DEBUG ]; then
	    $szpppon -l -n $phone ppp &>/dev/null
    else
	    $szpppon -l -n $phone ppp
    fi
    if [ $? -eq 0 ]; then
      echo -n "rsync "
      if [ -z $DEBUG ]; then
	      $rs $rs_options $szbase_options root@192.168.8.1:/opt/szarp/$base/ /opt/szarp/$base/ &>/dev/null
      else
	      $rs $rs_options $szbase_options root@192.168.8.1:/opt/szarp/$base/ /opt/szarp/$base/ 
      fi
      results[$nr_of_finished_phones]="$?" 
      if [ -z $DEBUG ]; then
	      ssh root@192.168.8.1 "/usr/sbin/ntpdate -u 192.168.8.8" &>/dev/null 
      else
	      ssh root@192.168.8.1 "/usr/sbin/ntpdate -u 192.168.8.8"
      fi
      if [ -z $DEBUG ]; then
	      $szpppoff &>/dev/null
      else
	      $szpppoff
      fi
      echo "done"
    else
      results[$nr_of_finished_phones]="-5"
	  echo "failed"
    fi
	nr_of_finished_phones=`expr $nr_of_finished_phones + 1`
    sleep 15
  fi
done

#put NO_DATA for modems, who wasn't present on argument list, and it number is great than
#last argument
while [ $nr_of_finished_phones -le $nr_of_phnones ]; do
	results[$nr_of_finished_phones]="NO_DATA"
	nr_of_finished_phones=$[nr_of_finished_phones + 1]
done
export_results
