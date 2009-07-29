#!/bin/sh
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
Usage: $0 <REPOSITORY_ADDRESS> <USER_NAME> [ <CONFIG_PREFIX> ]

Script for replacing SZARP configuration with version fetched from SVN server.
Configuration must be first imported to repository using conf-import.py script.
REPOSITORY_ADDRESS must be full address of SVN repository. CONFIG_PREFIX, if
not given, is guessed from server hostname.
USER_NAME is used to access repository.

EOF
}

REPO=
PREFIX=
if [ $# -eq 3 ]; then
	REPO=$1
	USERNAME=$2;
	PREFIX=$3;
	shift;
elif [ $# -eq 2 ]; then
	REPO=$1
	USERNAME=$2;
	echo -n "Guessing prefix... ";
	PREFIX=$(hostname -s);
	echo $PREFIX;
fi
if [ -z "$REPO" -o "$REPO" == "-h" -o "$REPO" == "--help" ] ; then
	Usage $0
	exit 1
fi

if [ ! -e /opt/szarp/$PREFIX/config/params.xml ]; then
	echo "File /opt/szarp/$PREFIX/config/params.xml does not exists, exiting...";
	exit 1;
fi

if [ $# -ne 2 ]; then
	Usage $0
	exit 1;
fi


DIRPATH=/opt/szarp/$PREFIX
if [ -e $DIRPATH/.svn ]; then
	echo "This configuration is already managed by svn, exiting";
	exit 1;
fi
	
TMPDIR=$(mktemp -d "/opt/szarp/$PREFIX.XXXXXX")

cat <<EOF
Hello, that's what I'm gonna to do:
1. I will move all files and dirs from $DIRPATH except for 'szbase_stamp' and 'szbase' to $TMPDIR
2. Check out configuration files for $PREFIX from the repository into $DIRPATH
You'll have to type your password few times. Please check that after this operation completes
contents of $DIRPATH looks ok. SZARP must be turned off during execution of this script.
Press Enter to continue or Ctrl-C to exit.
EOF
read 

for i in $DIRPATH/*; do 
	if [ "$i" == "$DIRPATH/szbase" ]; then
		continue;
	elif [ "$i" == "$DIRPATH/szbase_stamp" ]; then
		continue;
	fi
		
	echo "Moving: $i to $TMPDIR";
	mv $i $TMPDIR || { echo "Failed to move $i, help!"; exit 1;}
done


echo "Checking out configuration"
CDIR=`pwd`
cd $DIRPATH;

echo Running SVN_SSH="ssh -l $USERNAME" svn co $REPO/$PREFIX . 
SVN_SSH="ssh -l $USERNAME" svn co $REPO/$PREFIX . 

if [ $? -ne 0 ]; then 
	echo "Failed to fetch configuration, help!";
else
	echo "Ok";
fi

cd $CDIR
