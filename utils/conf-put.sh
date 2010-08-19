#!/bin/bash
# SZARP: SCADA software 
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
# $Id$


Usage() {
	cat <<EOF
Usage $0 [OPTION]... USERNAME

Commit changes in SZARP configuration to repository. Uses USERNAME to
access configuration SVN repository.

Options:
	-h, --help		print usage info and exit
	-p, --port=PORT		set ssh port number to PORT, default is 22

EOF
}

PORT=22
BASENAME=`basename "$0"`
TEMP=`getopt -o hp: --long help,port: -n "$BASENAME" -- "$@"`
if [ $? != 0 ] ; then echo "Terminating..." >&2 ; exit 1 ; fi
eval set -- "$TEMP"

while true ; do
	case "$1" in
		-h|--help)
			Usage $BASENAME
			exit 0
			shift
			;;
		-p|--port)
			PORT=$2
			shift 2
			;;
		--) shift; break;;
		*) echo "Internal error!" ; exit 1 ;;
	esac
done

if [ $# != 1 ] ; then Usage $BASENAME; exit 1; fi

USERNAME=$1;
env SVN_SSH="ssh -l $USERNAME -p $PORT"  svn commit

