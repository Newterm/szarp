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

if [ ! $# -eq 1 -o "$1" == '-h' -o "$1" == '--help' ]; then
	cat <<EOF
Usage $0 USERNAME

Commit changes in SZARP configuration to repository. Uses USERNAME to
access configuration SVN repository.

EOF
	exit 1;
fi

USERNAME=$1;

SVN_SSH="ssh -l $USERNAME" svn commit 
