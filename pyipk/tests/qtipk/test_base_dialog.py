#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest
import sys , os.path

import sip
sip.setapi('QString', 2)

from PyQt4.QtGui import QApplication , QWidget
from PyQt4.QtTest import QTest
from PyQt4.QtCore import Qt

from qtipk.base_diag import BaseDialog

class TestBaseDialog( unittest.TestCase ) :
	def setUp( self ) :
		self.app = QApplication(sys.argv)
		self.parent = QWidget()
		self.diag = BaseDialog( self.parent , 'tests/szarp/' ) 
		self.okbut = self.diag.buttonBox.button(self.diag.buttonBox.Ok)
	
	def tearDown( self ) :
		self.app.deleteLater()

	def test_select_arrow( self ) :
		QTest.keyClick(self.diag.listView,Qt.Key_Down)
		QTest.keyClick(self.diag.listView,Qt.Key_Down)
		QTest.mouseClick(self.okbut,Qt.LeftButton)
		self.assertEqual( self.diag.selected_base() , 'three' )

	def test_select_text( self ) :
		QTest.keyClicks(self.diag.listView,'thr')
		QTest.mouseClick(self.okbut,Qt.LeftButton)
		self.assertEqual( self.diag.selected_base() , 'three' )

	def test_params_files( self ) :
		for b in self.diag.bases :
			self.assertTrue( os.path.isfile( self.diag.params_path(b) ) )


