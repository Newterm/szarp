#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui , QtCore

from libipk.params import params_fs
from libipk.filesource import FS_local
from libipk.plugins import Plugins

from .params import QNode

from .xml_view import XmlView
from .xml_diag import XmlDialog
from .plug_diag import PluginsDialog
from .base_diag import BaseDialog
from .remote_diag import RemoteDialog
from .validate_diag import ValidateDialog

from .params_editor import ParamsEditor

from .utils import *

from .ui.main_win import Ui_MainWindow

SZARP_PATH = '/opt/szarp/'
DEFAULT_EDITOR = 'gvim'
DEFAULT_RELAXNG = '/opt/szarp/resources/dtd/ipk-params.rng'

class MainWindow( QtGui.QMainWindow , Ui_MainWindow ) :
	def __init__( self , plugins ) :
		QtGui.QMainWindow.__init__( self )
		self.setupUi( self )

		self.editor = ParamsEditor( self , DEFAULT_EDITOR )
		self.editor.changedSig.connect( self.touch_params )

		self.view_full = XmlView( 'params' , self.centralwidget )
		self.view_full.changedSig.connect( self.touch_params )
		self.view_full.runSig.connect( self.runOnNodes )
		self.view_full.showSig.connect( self.showOnNodes )
		self.view_full.editSig.connect( self.editOnNodes )
		self.hlay_xml.addWidget( self.view_full )

		self.view_result = XmlView( 'result' , self.centralwidget )
		self.view_result.changedSig.connect( self.touch_params )
		self.view_result.showSig.connect( self.showOnNodes )
		self.view_result.editSig.connect( self.editOnNodes )
		self.hlay_xml.addWidget( self.view_result )

		self.params = None
		self.plugins = Plugins()
		self.plugins.load(plugins)

		self.validateDiag = ValidateDialog( self , DEFAULT_RELAXNG )

	def touch_params( self ) :
		if self.params != None : self.params.touch()

	def rlyClose( self ) :
		return self.closeParams()

	def closeEvent(self, event):
		if self.rlyClose() :
			event.accept()
		else :
			event.ignore()

	def save( self ) :
		if self.params != None :
			self.params.save()

	def saveAs( self ) :
		fn = QtGui.QFileDialog.getSaveFileNameAndFilter( self , 
				'Save File' , '.' ,
				'XML Files (*.xml);;All (*)' )

		if fn[0] != '' and self.params != None :
			self.params.save( fromUtf8(fn[0]) )

	def openParamsDialog( self ) :
		fn = QtGui.QFileDialog.getOpenFileNameAndFilter( self ,
				'Open File' , '.' ,
				'XML Files (*.xml);;All (*)' )

		if fn[0] != '' :
			self.openParams( FS_local( fromUtf8(fn[0]) ) )

	def openParams( self , fs ) :
		if not self.closeParams() : return
		self.params = params_fs( fs , treeclass = QNode )
		self.view_full.clear()
		self.view_result.clear()
		self.view_full.add_node( self.params.getroot() )

	def closeParams( self ) :
		if self.params == None or self.params.close() :
			return True
		else :
			reply = QtGui.QMessageBox.question(self, 'Message',
				"Params not saved. Are you sure you want to do this?",
				QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
				QtGui.QMessageBox.No)

			if reply == QtGui.QMessageBox.Yes:
				return True
			else :
				return False

	def runOnNodes( self , nodes ) :
		result = self.openRunDialog()
		if result == None : return

		plug = self.plugins.get( result[0] , result[1] )

		self.view_result.clear()

		for n in nodes :
			plug.process( n.node )
			lxml_nodes = plug.result()
			ipk_nodes = self.params.rebuild_tree( lxml_nodes )
			for r in ipk_nodes :
				self.view_result.add_node( r )

	def editOnNodes( self , nodes ) :
		self.editor.edit( nodes )

	def showOnNodes( self , nodes ) :
		diag = XmlDialog( self )
		diag.add_nodes( nodes )
		diag.exec_()

	def openRunDialog( self ) :
		diag = PluginsDialog( self , self.plugins )
		if diag.exec_() == QtGui.QDialog.Accepted :
			return ( diag.selected_name() , diag.selected_args() )
		return None

	def openBaseDialog( self ) :
		diag = BaseDialog( self , SZARP_PATH )
		if diag.exec_() == QtGui.QDialog.Accepted :
			self.openParams( FS_local(fromUtf8(diag.selected_params())) )

	def openRemoteDialog( self ) :
		diag = RemoteDialog( self , SZARP_PATH )
		if diag.exec_() == QtGui.QDialog.Accepted :
			self.openParams( diag.get_fs() )

	def validate( self ) :
		self.validateDiag.exec_( self.params )

