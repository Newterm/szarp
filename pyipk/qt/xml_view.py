#!/usr/bin/python
# -*- coding: utf-8 -*-

from PyQt4 import QtGui , QtCore

from lxml import etree
from ui.xml_view import Ui_XmlView

from utils import * 

class XmlView( QtGui.QWidget , Ui_XmlView ) :
	def __init__( self , parent ) :
		QtGui.QWidget.__init__( self , parent )
		self.setupUi(self)

	def clear( self ) :
		self.xml.setText('')

	def add_node( self , node ) :
		xmlstr = etree.tostring(node, pretty_print=True, encoding=unicode)
		self.xml.setText( self.xml.toPlainText() + fromUtf8(xmlstr) )

