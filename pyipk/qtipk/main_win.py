#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui , QtCore

from libipk.filesource import FS_local
from libipk.plugins import Plugins

from .params_mgr import ParamsManager

from .base_diag import BaseDialog
from .remote_diag import RemoteDialog
from .validate_diag import ValidateDialog

from .utils import *

from .ui.main_win import Ui_MainWindow

SZARP_PATH = '/opt/szarp/'
DEFAULT_RELAXNG = '/opt/szarp/resources/dtd/ipk-params.rng'

class MainWindow( QtGui.QMainWindow , Ui_MainWindow ) :
	def __init__( self , plugins ) :
		QtGui.QMainWindow.__init__( self )
		self.setupUi( self )

		self.plugins = Plugins()
		self.plugins.load(plugins)

		self.params_mgr = ParamsManager( self.plugins , self )

		self.validateDiag = ValidateDialog( self , DEFAULT_RELAXNG )

		self.setCentralWidget( self.params_mgr )

	def rlyClose( self ) :
		return self.params_mgr.closeAllParams()

	def closeEvent(self, event):
		if self.rlyClose() :
			event.accept()
		else :
			event.ignore()

	def save( self ) :
		self.params_mgr.save()

	def saveAs( self ) :
		fn = QtGui.QFileDialog.getSaveFileNameAndFilter( self , 
				'Save File' , '.' ,
				'XML Files (*.xml);;All (*)' )

		if fn[0] != '' :
			self.params_mgr.save( FS_local(fromUtf8(fn[0]) ) )

	def openParamsDialog( self ) :
		fn = QtGui.QFileDialog.getOpenFileNameAndFilter( self ,
				'Open File' , '.' ,
				'XML Files (*.xml);;All (*)' )

		if fn[0] != '' :
			self.openParams( FS_local( fromUtf8(fn[0]) ) )

	def openParams( self , fs ) :
		self.params_mgr.newParams( fs )

	def openBaseDialog( self ) :
		diag = BaseDialog( self , SZARP_PATH )
		if diag.exec_() == QtGui.QDialog.Accepted :
			self.openParams( FS_local(fromUtf8(diag.selected_params())) )

	def openRemoteDialog( self ) :
		diag = RemoteDialog( self , SZARP_PATH )
		if diag.exec_() == QtGui.QDialog.Accepted :
			self.openParams( diag.get_fs() )

	def validate( self ) :
		self.validateDiag.exec_( self.params_mgr.currentParams() )

