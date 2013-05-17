#!/usr/bin/python
# -*- coding: UTF-8 -*-
# vim: set fileencoding=utf-8 :

"""
Given a params.xml assure that for every param (except drawdefinables)
exists szbase dir and at least one .szb file
"""

import unicodedata
from optparse import OptionParser
import os
import xml.sax

class ParamsContentHandler(xml.sax.ContentHandler):
	"""
	Parse params.xml and get all params (except drawdefinables)
	"""
	def __init__(self):
		xml.sax.ContentHandler.__init__(self)
		self.DRAWDEF = "drawdefinable"
		self.ignored_section = False
		self.params = []

	def startElement(self, name, attrs):
		if name == self.DRAWDEF:
			self.ignored_section = True
		elif name == "param":
			if not self.ignored_section:
				param_name = attrs.getValue("name")
				self.params.append(param_name)

	def endElement(self, name):
		if name == self.DRAWDEF:
			self.ignored_section = False

def get_param_names(filename):
	handler = ParamsContentHandler()
	try:
		with open(filename) as f:
			xml.sax.parse(f, handler)
	except Exception, e:
		printerr("! Error: not a correct XML file: %s" % filename)
		printerr(e)
		return False
	return handler.params

def unicode_to_ascii(string):
	"""
	Converts unicode string to printable ascii form, removes polish characters
	"""
	return unicodedata.normalize('NFKD', string).replace(u'ł', 'l').replace(u'Ł', 'L').encode('ascii', 'ignore')

def name_to_path(name):
	ascii_name = unicode_to_ascii(name)
	prev_char = '/'
	new_name = ''
	for i in range(len(ascii_name)):
		if ascii_name[i] == ':':
			if prev_char == '/':
				new_name = new_name + '_'
			else:
				new_name = new_name + '/'
		else:
			if ascii_name[i].isalnum():
				new_name = new_name + ascii_name[i]
			else:
				new_name = new_name + '_'
		prev_char = ascii_name[i]
	return new_name

def get_dir_names(filename):
	param_names = get_param_names(filename)
	dir_names = []
	for name in param_names:
		dir_names.append(name_to_path(name))
	return dir_names

def create_dir_and_deffile(dir_path, szb_filename):
	if not os.path.exists(dir_path):
		os.makedirs(dir_path)	# recursive
		print "\tpath: " + dir_path + " created"
	if len(os.listdir(dir_path)) == 0:
		filename = dir_path.rstrip('/') + '/' + szb_filename
		f = open(filename, 'a')
		f.close()
		print "\tfile: " + filename + " created"

parser = OptionParser(usage="usage: %prog params_filename\nCreates szbase dirs for all non-drawdefinable params.\nFor every param script checks if its directory exists in szbase. If not, directory is created. If for any param there are no .szb files, script creates single empty .szb file in params directory.")
parser.add_option("-b", "--szbase_path", help="path to szbase directory (e.g. /opt/szarp/location/szbase)", 
		action="store", dest="base_path")
parser.add_option("-f", "--szb_filename", help="filename for default .szb file (e.g. 201201.szb)", 
		action="store", dest="szb_filename")

(options, args) = parser.parse_args()
if len(args) != 1:
	print "ERROR: params.xml filename not provided"
	exit(1)

if options.base_path == None:
	print "ERROR: path to szbase dir not provided"
	exit(1)

if options.szb_filename == None:
	print "ERROR: default .szb filename not provided"
	exit(1)

filename = args[0]
base_path = options.base_path
print "szbase path set to: " + base_path

dir_names = get_dir_names(filename)
for name in dir_names:
	print name
	full_path = base_path.rstrip('/') + '/' + name
	create_dir_and_deffile(full_path, options.szb_filename)
