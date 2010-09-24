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

for data in reader:
	els = dict()
	els['param'] = etree.SubElement(insert, addns('param'))
	els['raport'] = etree.SubElement(els['param'], addns('raport'))
	els['draw'] = etree.SubElement(els['param'], addns('draw'))
	els['param'].set("name", ':'.join([data["group"].decode('utf-8'), 
		data['unit'].decode('utf-8'), data['name'].decode('utf-8')]))
	for k, v in data.items():
		try:
			el, ignore, attr = k.partition(':')
			els[el].set(attr, v.decode('utf-8'))
		except:
			pass

etree.ElementTree(tree).write(params, encoding="iso-8859-2", pretty_print=True)

