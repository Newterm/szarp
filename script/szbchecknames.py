#!/usr/bin/python
# -*- coding: utf-8 -*-
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

# $Id$
#
# 2011 Krzysztof Golinski
#
# A script looking for names which are in database but not in params.xml

import libxml2
import sys
import os
import string
import optparse
import re

def convertToSzarp(str):
	if len(str) == 0:
		return str

	di = dict([(u' ',u'_'),(u'.',u'_'),(u',',u'_'),(u'°',u'_'),(u'/',u'_'),(u'\\',u'_'),(u'-',u'_'),(u'(',u'_'),
		(u')',u'_'), (u'+',u'_'),
		(u'ą',u'a'),(u'ć',u'c'),(u'ę',u'e'),(u'ł',u'l'),(u'ń',u'n'),(u'ó',u'o'),(u'ś',u's'),(u'ź',u'z'),(u'ż','z'),
		(u'Ą',u'A'),(u'Ć',u'C'),(u'Ę',u'E'),(u'Ł',u'L'),(u'Ń',u'N'),(u'Ó',u'O'),(u'Ś',u'S'),(u'Ź',u'Z'),(u'Ż','Z')
])
	s = list(str)
	for i in range(0,len(str)):
		if di.has_key(s[i]):
			s[i] = di[s[i]]
	ans = "".join(s)
	return ans

names_from_xml = set()
names_from_base = set()

def getNames(szbase_path):
	if not os.path.exists(szbase_path):
		print "Path %s not exists" % szbase_path
		exit(-1)

	dirnames = [x[0] for x in os.walk(szbase_path)]
	for dirn in dirnames:
		dirpath = dirn.replace(szbase_path,'')
		dirpath = dirpath.replace(r'/',':')
		if len(dirpath.split(':')) == 3:
			names_from_base.add(unicode(dirpath, 'UTF-8'))

#	for root, dirnames, files in os.walk(szbase_path):
#		for dirn in dirnames:
#			dirpath = os.path.join(root,dirn)
#			dirpath = dirpath.replace(szbase_path,'')
#			dirpath = dirpath.replace(r'/',':')
#			if len(dirpath.split(':')) == 3:
#				names_from_base.add(unicode(dirpath, 'UTF-8'))

def processNode(reader):
	if reader.Name() == "param" or reader.Name() == "send":
		if reader.NodeType() == 1:
			while reader.MoveToNextAttribute():
				if reader.Name() == "name":
					uval = convertToSzarp(unicode(reader.Value(), 'UTF-8'))
					_name = uval.split(':')
					if len(_name) == 3:
#						print "PARAM: ", _name
						names_from_xml.add(_name[0] + ":" + _name[1] + ":" + _name[2])
	if reader.Name() == "define":
		if reader.NodeType() == 1:
			while reader.MoveToNextAttribute():
				if reader.Name() == "formula":
					allform = re.findall(r'\([^\(]*\)',reader.Value())
#					print "allform: ", allform
					for f in allform:
						fo = f[1:-1]
						fo = convertToSzarp(unicode(fo, 'UTF-8'))
						_name = fo.split(':')
						if len(_name) == 3:
#							print "DEFINE: ", _name
							names_from_xml.add(_name[0] + ":" + _name[1] + ":" + _name[2])
	if reader.Name() == "#cdata-section":
		allform = re.findall(r'"[^"]*"',reader.Value())
#		print "allform:", allform
		for f in allform:
			fo = f[1:-1]
			fo = convertToSzarp(unicode(fo,'UTF-8'))
			_name = fo.split(':')
			if len(_name) == 4: # sic! ->  base:*:*:*
				_name = _name[1:]
#				print "CDATA:", _name
				names_from_xml.add(_name[0] + ":" + _name[1] + ":" + _name[2])

def checkNames():
	for bname in sorted(names_from_base):
		if bname not in names_from_xml:
			# second chance
			_tmp = bname.split(':')
			_name = _tmp[0] + ':*:' + _tmp[2]
			if _name not in names_from_xml:
				# third chance
				_name = '*:*:' + _tmp[2]
				if _name not in names_from_xml:
					# last one chance
					_name = '*:' + _tmp[1] + ':' + _tmp[2]
					if _name not in names_from_xml:
						print bname

def streamFile(filename):
	try:
		reader = libxml2.newTextReaderFilename(filename)
	except:
		print "unable to open: %s" % (filename)
		exit(-1)

	ret = reader.Read()
	while ret == 1:
		processNode(reader)
		ret = reader.Read()
	
	if ret != 0:
		print "%s: failed to parse" % (filename)
		exit(-1)

def main():
	epilog = """Usage example:
./szbchecknames.py -b "snew" -x "/tmp/params.xml"
"""

	parser = optparse.OptionParser(epilog=epilog)
	parser.add_option('-b', '--base', type='string',  help='REQUIRED. Base name.', dest='base')
	parser.add_option('-x', '--xml', type='string', help='Full path to a xml file with configuration. Default value [$BASE/config/params.xml]', dest='xmlfile')
	parser.add_option('-p', '--prefix', type='string', help='Base prefix. Default value [%default]',
		dest='prefix', default='/opt/szarp/')

	(opts, args) = parser.parse_args()

	# check required option
	if opts.base is None:
		print 'Required option is missing'
		parser.print_help()
		exit(-1)

	basepath = opts.prefix + opts.base

	if opts.xmlfile is None:
		opts.xmlfile = basepath + "/config/params.xml"

#	print "basepath: %s, xmlfile: %s" % (basepath, opts.xmlfile)

	streamFile(opts.xmlfile)
	getNames(basepath + "/szbase/")
	checkNames()

if __name__ == "__main__":
	main()
