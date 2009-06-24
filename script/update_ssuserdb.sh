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
#
# update_ssuserdb.sh - update ssuserdb files on sss mirror servers
#
# Michal Blajerski <nameless@praterm.com.pl>
# (c) Praterm S.A 2006 

HOSTNAME=`/bin/hostname -s`

/usr/bin/touch /opt/szarp/$HOSTNAME/sss_userdb.xml

OLD_SUM=`/usr/bin/md5sum < /opt/szarp/$HOSTNAME/sss_userdb.xml`
/usr/bin/rsync -az $HOSTNAME@praterm:/opt/szarp/prat/sss_user* /opt/szarp/$HOSTNAME/
NEW_SUM=`/usr/bin/md5sum < /opt/szarp/$HOSTNAME/sss_userdb.xml`
if [ "$OLD_SUM" != "$NEW_SUM" ] ; then
	/usr/bin/killall -s HUP sss
fi

