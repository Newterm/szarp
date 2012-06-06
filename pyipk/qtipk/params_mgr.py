#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui , QtCore

from libipk.params import params_fs
from libipk.filesource import FS_local

from .params import QNode
from .xml_view import XmlView
from .xml_diag import XmlDialog

from .map_view import MapDialog
from .plug_diag import PluginsDialog
from .params_editor import ParamsEditor

DEFAULT_EDITOR = 'gvim'

class CopyBuffer :
	def __init__( self ) :
		self.nodes = None

	def set( self , nodes ) :
		self.nodes = nodes

	def get( self ) :
		return self.nodes 

	def pop( self ) :
		nodes = self.nodes
		self.nodes = None
		return nodes

	def __len__( self ) :
		return 0 if self.nodes == None else 1

class ParamsManager( QtGui.QTabWidget ) :
	def __init__( self , plugins , parent = None ) :
		QtGui.QTabWidget.__init__( self , parent )

		self.setTabsClosable( True )
		self.tabCloseRequested.connect( self.closeTab )

		self.editor = ParamsEditor( self , DEFAULT_EDITOR )
		self.editor.changedSig.connect( self.touch_params )

		self.copybuf = CopyBuffer()

		self.plugins = plugins
		self.paramss = []
		self.views = []

	def newParams( self , filesource ) :
		params = params_fs( filesource , treeclass = QNode )

		index = len(self.paramss)

		wdg = QtGui.QWidget( self )
		hlay_xml = QtGui.QHBoxLayout( wdg )
		view_full = XmlView( 'params' , self , self.copybuf )
		view_result = XmlView( 'result' , self , self.copybuf )

		views = (view_full,view_result)

		view_full.changedSig.connect( params.touch )
		view_full.runSig.connect(lambda ns : self.runOnNodes(params,views,ns))
		view_full.showSig.connect( self.showOnNodes )
		view_full.editSig.connect( self.editOnNodes )
		view_full.attribSig.connect( self.attribOnNodes )
		hlay_xml.addWidget( view_full )

		view_result.changedSig.connect( params.touch )
		view_result.runSig.connect(lambda ns : self.runOnNodes(params,views,ns))
		view_result.showSig.connect( self.showOnNodes )
		view_result.editSig.connect( self.editOnNodes )
		view_result.attribSig.connect( self.attribOnNodes )
		hlay_xml.addWidget( view_result )

		self.addTab( wdg , str(filesource) )

		view_full.add_node( params.getroot() )

		self.paramss.append( params )
		self.views.append( (view_full,view_result) )
		return index

	def currentParams( self ) :
		if self.currentIndex() == -1 :
			return None
		else :
			return self.paramss[ self.currentIndex() ]

	def closeTab( self , index ) :
		if self.closeParams( self.paramss[index] ) :
			params = self.paramss.pop(index)
			views  = self.views.pop(index)
			views[0].clear()
			views[1].clear()
			self.removeTab(index)

	def touch_params( self , index ) :
		self.paramss[index].touch()

	def closeAllParams( self ) :
		for p in self.paramss :
			if not self.closeParams( p ) :
				return False
		return True

	def closeParams( self , params ) :
		if params.close() :
			return True
		else :
			reply = QtGui.QMessageBox.question(self, 'Message',
				"File %s not saved. Are you sure you want to close it?" % str(params.get_filesource()),
				QtGui.QMessageBox.Yes | QtGui.QMessageBox.No,
				QtGui.QMessageBox.No)

			if reply == QtGui.QMessageBox.Yes:
				return True
			else :
				return False

	def save( self , filesource = None ) :
		params = self.currentParams()
		if params != None : params.save( filesource )

	def runOnNodes( self , params , views , nodes ) :
		result = self.openRunDialog()
		if result == None : return

		plug = self.plugins.get( result[0] , result[1] )
		view = views[1]

		view.clear()

		for n in nodes :
			plug.process( n.node )

		lxml_nodes = plug.result()
		ipk_nodes = params.rebuild_tree( lxml_nodes )
		for r in ipk_nodes :
			view.add_node( r )

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

	def attribOnNodes( self , nodes ) :
		if len(nodes) != 1 :
			print('You may edit attribs only on one node at once')
			return

		n = nodes[0]
		diag = MapDialog( self )
		diag.set_list( n.node.items() )
		if diag.exec_() == QtGui.QDialog.Accepted :
			n.node.attrib.clear()
			for k , v in diag.get_list() :
				n.node.set(k,v)

