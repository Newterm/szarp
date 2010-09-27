#!/usr/bin/python
# -*- coding: utf-8 -*-

# Script for generating skeleton params.xml file from CSV file
# created by p2csv.py script.
#
# Usage: csv2p.py <input.csv> <output.xml>

import csv
import sys
from lxml import etree

fcsv = open(sys.argv[1], "r")
params = open(sys.argv[2], "wb")

def addns(tag):
	return "{http://www.praterm.com.pl/SZARP/ipk}" + tag


reader = csv.DictReader(fcsv)

tree = etree.Element(addns('params'), nsmap = {None: "http://www.praterm.com.pl/SZARP/ipk"})
insert = etree.SubElement(tree, addns('device'))
insert = etree.SubElement(insert, addns('unit'))

drawdefinable = []

for data in reader:
	els = dict()
	name = ':'.join([data["group"].decode('utf-8'), 
		data['unit'].decode('utf-8'), data['name'].decode('utf-8')])
	els['param'] = etree.SubElement(insert, addns('param'))
	if name.endswith("msw"):
		drawdefinable.append(data)
		pass
	elif not name.endswith("lsw"):
		els['raport'] = etree.SubElement(els['param'], addns('raport'))
		els['draw'] = etree.SubElement(els['param'], addns('draw'))

	els['param'].set("name", name)
	for k, v in data.items():
		try:
			el, ignore, attr = k.partition(':')
			els[el].set(attr, v.decode('utf-8'))
		except:
			pass

insert = etree.SubElement(tree, addns("drawdefinable"))

for data in drawdefinable:
	els = dict()
	name = ':'.join([data["group"].decode('utf-8'), 
		data['unit'].decode('utf-8'), data['name'].decode('utf-8')])
	els['param'] = etree.SubElement(insert, addns('param'))
	basename = name.rstrip("msw")

	define = etree.SubElement(els['param'], addns('define'))
	define.set("type", "DRAWDEFINABLE")
	define.set("formula", "(" + basename + "msw) (" + basename + "lsw) :")

	els['param'].set("name", basename)
	els['raport'] = etree.SubElement(els['param'], addns('raport'))
	els['draw'] = etree.SubElement(els['param'], addns('draw'))
	for k, v in data.items():
		try:
			el, ignore, attr = k.partition(':')
			if attr != "base_ind":
				els[el].set(attr, v.decode('utf-8'))
		except:
			pass

etree.ElementTree(tree).write(params, encoding="iso-8859-2", pretty_print=True)

