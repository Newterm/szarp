#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui

from .main_win import MainWindow

from .config import Config , Configurable

class QtApp( QtGui.QApplication , Configurable ) :
	def __init__( self , argv ) :
		QtGui.QApplication.__init__( self , argv )
		Configurable.__init__( self )

		self.cfg.parse_cfg_file( self.cfg['path:config'] )
		self.cfg.parse_cmd_line( argv )

		self.cfg.save_to_cfg( self.cfg['path:config'] )

		self.main_win = MainWindow( self )

	def run( self ) :
		self.main_win.show()
		code = self.exec_()
		self.cfg.save()
		return code

