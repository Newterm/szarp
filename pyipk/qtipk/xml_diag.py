#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui , QtCore

from lxml import etree

from .utils import * 

from .ui.xml_diag import Ui_XmlDialog

class XmlDialog( QtGui.QDialog , Ui_XmlDialog ) :

	def __init__( self , parent ) :
		QtGui.QWidget.__init__( self , parent )
		self.setupUi(self)

	def add_nodes( self , nodes ) :
		s = self.xml.toPlainText()

		for n in nodes :
			s += toUtf8( n.tostring() )

		self.xml.setPlainText( s )

