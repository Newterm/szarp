#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui , QtCore

from libipk.filesource import FS_local , FS_url
from libipk.plugins import Plugins

from .params_mgr import ParamsManager

from .base_diag import BaseDialog
from .remote_diag import RemoteDialog
from .validate_diag import ValidateDialog
from .map_view import MapDialog

from .utils import *
from .config import Config , Configurable

from .ui.main_win import Ui_MainWindow

class MainWindow( QtGui.QMainWindow , Ui_MainWindow , Configurable ) :
	def __init__( self , parent ) :
		QtGui.QMainWindow.__init__( self )
		self.setupUi( self )
		Configurable.__init__( self , parent )

		self.plugins = Plugins()
		self.plugins.load( self.cfg['path:plugins'] )
		self.cfg.on_edited( 'path:plugins' , self.plugins.reload )

		self.params_mgr = ParamsManager( self.plugins , self )

		self.validateDiag = ValidateDialog( self )

		self.setCentralWidget( self.params_mgr )

		if 'url:params' in self.cfg :
			self.openParams( FS_url(self.cfg['url:params']) )

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
		diag = BaseDialog( self , self.cfg['path:szarp'] )
		if diag.exec_() == QtGui.QDialog.Accepted :
			self.openParams( FS_local(fromUtf8(diag.selected_params())) )

	def openRemoteDialog( self ) :
		diag = RemoteDialog( self , self.cfg['path:szarp'] )
		if diag.exec_() == QtGui.QDialog.Accepted :
			self.openParams( diag.get_fs() )

	def openConfigDialog( self ) :
		diag = MapDialog( self )
		diag.set_map( self.cfg )
		# lambda cannot contain assignment:O
		def update( m ) : self.cfg = m
		diag.on_apply( update )
		if diag.exec_() == QtGui.QDialog.Accepted :
			self.cfg = diag.get_map()

	def validate( self ) :
		self.validateDiag.exec_( self.params_mgr.currentParams() )

