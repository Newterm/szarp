# -*- coding: utf-8 -*-

from lxml import etree

class IPKParser:
#	def __init__(self, parent=None):
		#params_xml = etree.parse(filename)
		#root = params_xml.getroot()

	def getTitle(self):
		return u"Ciepłownia WP"

	def getSets(self):
		return ["Zestaw 1", "Zestaw 2", "Zestaw 3", "Zestaw 4"]

	def getParams(self):
		return ["Parameter 7", "Parameter 8", "Parameter 9",
				"Parameter 10", "Parameter 11", "Parameter 13"]

