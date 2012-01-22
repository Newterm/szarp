#!/usr/bin/python
# -*- coding: utf-8 -*-

from PyQt4 import QtGui

from main_win import MainWindow

class QtApp( QtGui.QApplication ) :
	def __init__( self , argv , plugins ) :
		QtGui.QApplication.__init__( self , argv )

		self.main_win = MainWindow( plugins )

	def run( self ) :
		self.main_win.show()
		return self.exec_()

