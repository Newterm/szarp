#!/bin/sh
# $Id$

echo "Running autoheader"
autoheader
echo "Running aclocal"
aclocal -I m4 -I /usr/share/aclocal
echo "Running libtoolize"
libtoolize
echo "Running automake"
automake -a -c
echo "Running autoconf"
autoconf
echo "Run ./configure to configure SZARP"
