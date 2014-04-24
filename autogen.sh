#!/bin/sh
# $Id$

echo "Running autoheader"
autoheader
echo "Running aclocal"
aclocal -I /usr/share/aclocal -I m4
echo "Running libtoolize"
libtoolize
echo "Running automake"
automake -a -c
echo "Running autoconf"
autoconf
echo "Run ./configure to configure SZARP"
