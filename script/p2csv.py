#!/usr/bin/python
# -*- coding: utf-8 -*-

# Script parses params.xml file and creates CSV output, suitable for editing
# in Excel and importing again using csv2p.py script.
#
# Usage: p2csv.py <params.xml> <output.csv>

import csv
import sys
from lxml import etree

params = open(sys.argv[1], "r")
fcsv = open(sys.argv[2], "wb")

def mywriteheader(writer, fields):
	d = dict()
	for f in fields:
		d[f] = f
	writer.writerow(d)

def e2ns(element):
	r = element[1:].partition('}')
	return r[0], r[2]

class ParamsTarget:
	def __init__(self, writer, attrs):
		self.mdata = dict()
		self.writer = writer
		self.attrs = attrs

	def start(self, tag, attrib):
		ns, el = e2ns(tag)
		if ns != 'http://www.praterm.com.pl/SZARP/ipk':
			return
		if el == 'param':
			self.mdata['group'], self.mdata['unit'], self.mdata['name'] = attrib['name'].encode('utf-8').split(':')
		if not self.attrs.has_key(el):
			return
		for a in attrib.keys():
			if not a in self.attrs[el]:
				continue
			self.mdata[el + ':' + a] = attrib[a].encode('utf-8')

	def end(self, tag):
		ns, el = e2ns(tag)
		if ns == 'http://www.praterm.com.pl/SZARP/ipk' and el == 'param':
			writer.writerow(self.mdata)
			self.mdata.clear()

	def close(self):
		pass


nattrs = [ 'group', 'unit', 'name' ]
attrs = dict()
attrs['param'] = [ 'short_name', 'draw_name', 'unit', 'prec', 'base_ind' ]
attrs['draw'] = [ 'title', 'min', 'max' ]
attrs['raport'] = [ 'title', ]

fields = nattrs
for k, v in attrs.items():
	for f in v:
		fields.append(k + ":" + f)


writer = csv.DictWriter(fcsv, fields)

data = dict()
data['Group'] = 'test'

try:
	writer.writeheader()
except AttributeError:
	mywriteheader(writer, fields)

parser = etree.XMLParser(target = ParamsTarget(writer, attrs))
etree.parse(params, parser)

