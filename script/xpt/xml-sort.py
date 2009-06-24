#!/usr/bin/python
# -*- coding: utf-8 -*-
# SZARP: SCADA software 
# Copyright (C) 1994-2008  PRATERM S.A.
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

# Sorts XML translation file for proper wildcard treatment. Reads file given as first 
# argument, prints reuslt to stdout.

from lxml import etree
import sys

ns = 'http://www.praterm.com.pl/SZARP/ipk_dictionary'
doc = etree.parse(sys.argv[1])

def getkey(elem):
    return elem.find("./{%s}translation[@lang='pl']" % ns).attrib['value']

container = doc.find(".//{%s}short_name" % ns)
container[:] = sorted(container, key=getkey)
container = doc.find(".//{%s}param_name" % ns)
container[:] = sorted(container, key=getkey)
container = doc.find(".//{%s}draw_title" % ns)
container[:] = sorted(container, key=getkey)
container = doc.find(".//{%s}draw_name" % ns)
container[:] = sorted(container, key=getkey)

print "<?xml version='1.0' encoding='UTF-8'?>"
doc.write(sys.stdout, encoding = 'utf-8')

# thats all!
