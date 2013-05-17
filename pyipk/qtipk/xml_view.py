#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

import string

from copy import copy

import libipk.params

from PyQt4 import QtGui , QtCore

from lxml import etree

from .utils import * 

from .ui.xml_view import Ui_XmlView

from .params import ModelTree

import re , threading , time

class SearchEngine( QtCore.QObject ) :
	foundSig = QtCore.pyqtSignal( object )

	def __init__( self , parent = None ) :
		QtCore.QObject.__init__( self , parent )
		self.thread  = threading.Thread()
		self._stop = threading.Event()

	def breaker( self ) :
#        time.sleep(0.001)
		return not self._stop.isSet()

	def run( self ) :
		i = 0
		for r in self.roots :
			se = libipk.params.ParamsSearch( [r] , self.pattern )
			while True :
				path = se.next( breaker = self.breaker )
				if path == None :
					break
				path.insert( 0 , i )
				self.foundSig.emit( path )
			i += 1

	def stop( self ) :
		self._stop.set()
		self.thread.join()
		self._stop.clear()

	def new( self , roots , string ) :
		if self.thread.isAlive() : 
			self.stop()

		if string == '' : return

		self.roots   = roots
		self.pattern = string

		self.thread = threading.Thread( target = self.run )
		self.thread.start()

class XmlView( QtGui.QWidget , Ui_XmlView ) :
	changedSig = QtCore.pyqtSignal()
	runSig = QtCore.pyqtSignal( list )
	showSig = QtCore.pyqtSignal( list )
	editSig = QtCore.pyqtSignal( list )
	attribSig = QtCore.pyqtSignal( list )

	def __init__( self , name , parent , buf = None ) :
		QtGui.QWidget.__init__( self , parent )
		self.setupUi(self)

		self.copybuf = buf 

		self.model = XmlTreeModel()
		self.view.setModel( self.model )
		self.view.expanded.connect(self.model.expand)
		self.view.collapsed.connect(self.model.collapse)
		self.view.header().moveSection(1,0)

		self.view.doubleClicked.connect( self.doubleClick )

		self.model.changedSig.connect(self.changedSig)

		self.set_name( name )

		self.search = SearchEngine()
		self.search.foundSig.connect( self.on_found )

		self.found_paths = []
		self.current_path_id = None
		self.lineFind.hide()
		self.butNext.hide()
		self.butPrev.hide()
		self.view.setFocus()

		self.set_shortcut( self.view     , "Ctrl+F" , self.on_search_start )
		self.set_shortcut( self.view     , "Esc"    , self.on_search_end   )
		self.set_shortcut( self.view     , "Ctrl+N" , self.on_next         )
		self.set_shortcut( self.view     , "Ctrl+P" , self.on_prev         )
		self.set_shortcut( self.lineFind , "Esc"    , self.on_search_end   )
		self.set_shortcut( self.lineFind , "Ctrl+N" , self.on_next         )
		self.set_shortcut( self.lineFind , "Ctrl+P" , self.on_prev         )

	def set_shortcut( self , parent , key , func ) :
		sc = QtGui.QShortcut( parent )
		sc.setKey( key )
		sc.setContext( QtCore.Qt.WidgetWithChildrenShortcut )
		sc.activated.connect( func )

	def on_search( self , string ) :
		self.found_paths = []
		self.current_path_id = None
		self.search.new( [ r.node for r in self.model.getroots() ] , string )

	def on_search_start( self ) :
		self.lineFind.show()
		self.butNext.show()
		self.butPrev.show()
		self.lineFind.setFocus()

	def on_search_end( self ) :
		self.lineFind.hide()
		self.butNext.hide()
		self.butPrev.hide()
		self.view.setFocus()

		self.found_paths = []
		self.current_path_id = None

	def on_found( self , p ) :
		self.found_paths.append( p )
		if self.current_path_id == None :
			self.current_path_id = len(self.found_paths)-1
			self.set_by_path( p )

	def on_next( self ) :
		if self.current_path_id == None : return
		self.current_path_id += 1
		if self.current_path_id >= len(self.found_paths) :
			self.current_path_id = 0
		self.set_by_path( self.found_paths[ self.current_path_id ] )

	def on_prev( self ) :
		if self.current_path_id == None : return
		self.current_path_id -= 1
		if self.current_path_id < 0 :
			self.current_path_id = len(self.found_paths)-1 
		self.set_by_path( self.found_paths[ self.current_path_id ] )
	
	def set_by_path( self , p ) :
		i = self.model.index_by_path( p )
		self.view.setCurrentIndex( i )

	def doubleClick( self , idx ) :
		if idx.column() == 0 :
			self.attribSig.emit( [idx.internalPointer().node] )
	
	def contextMenuEvent(self, event):
		idxes = self.view.selectedIndexes()

		if len( idxes ) == 0 : return

		nodes = [ i.internalPointer().node for i in idxes if i.column() == 0 ]
		menu = QtGui.QMenu(self)

		runAction     = menu.addAction(stdIcon("system-run"),"Run")
		attribAction  = menu.addAction("Attributes")
		menu.addSeparator()
		copyAction    = menu.addAction(stdIcon("edit-copy"),"Copy")
		pasteAction   = menu.addAction(stdIcon("edit-paste"),"Paste")
		cutAction     = menu.addAction(stdIcon("edit-cut"),"Cut")
		menu.addSeparator()
		deleteAction  = menu.addAction(stdIcon("list-remove"),"Remove")
		newAction     = menu.addAction(stdIcon("list-add"),"Insert new")
		menu.addSeparator()
		showAction    = menu.addAction(stdIcon("document-print-preview"),"Show")
		editnewAction = menu.addAction(stdIcon("accessories-text-editor"),"Edit new")
		editAction    = menu.addAction("Edit with")


		pasteAction.setEnabled(len(self.copybuf) > 0 and len(nodes) == 1)
		attribAction.setEnabled(len(nodes) == 1)

		action = menu.exec_(self.mapToGlobal(event.pos()))

		def child_elem( node ) :
			return etree.Element(node.tag+"_child")

		if action == runAction :
			self.runSig.emit( nodes )
		elif action == attribAction :
			self.attribSig.emit( nodes )
		elif action == copyAction :
			self.copybuf.set( nodes )
		elif action == pasteAction :
			nnodes = self.copybuf.get()
			for n in nnodes :
				nodes[0].create_child( copy(n.node) )
		elif action == cutAction :
			self.copybuf.set( nodes )
			for n in nodes : n.delete()
		elif action == showAction :
			self.showSig.emit( nodes )
		elif action == editAction :
			self.editSig.emit( nodes )
		elif action == deleteAction :
			for n in nodes : n.delete()
		elif action == newAction :
			for n in nodes : n.create_child(child_elem(n.node))
		elif action == editnewAction :
			newnodes = [ n.create_child(child_elem(n.node)) for n in nodes ]
			self.editSig.emit( newnodes )
			

	def set_name( self , name ) :
		pass

	def clear( self ) :
		self.model.clear()

	def add_node( self , node ) :
		if node == None : raise ValueError('Node cannot be None')
		self.model.add_node( node )
		self.view.update()
		self.update()

	def set_buf( self , buf ) :
		self.copybuf = buf

class DragInfo :
	def __init__( self , node , nodeidx , parent ) :
		self.node = node
		self.nodeidx = nodeidx
		self.parent = parent

class XmlTreeModel(QtCore.QAbstractItemModel):
	changedSig = QtCore.pyqtSignal()

	def __init__(self):
		QtCore.QAbstractItemModel.__init__(self)
		self.drag  = []
		self.roots = []

	def getroots( self ) :
		return self.roots

	def add_node( self , node ) :
		row = len(self.roots)
		self.beginInsertRows(QtCore.QModelIndex(),row,row+1)
		self.roots.append( node.create_mtnode(row,self) )
		self.endInsertRows()

	def clear( self ) :
		self.beginRemoveRows( QtCore.QModelIndex() , 0 , len(self.roots) )
		for r in self.roots : r.close()
		del self.roots[:]
		self.endRemoveRows()

	def expand( self , index ) :
		mt = index.internalPointer()
		if mt != None : mt.fill()

	def collapse( self , index ) :
		mt = index.internalPointer()
		if mt != None : mt.purge()

	def index_by_path( self , path ) :
		path = list(path)
		node = QtCore.QModelIndex()
		while len(path) > 0 :
			parent = node
			node   = self.index( path.pop(0) , 0 , parent )
		return node

	def index(self, row, column, parentidx ):
		parent = parentidx.internalPointer()
		if parent == None :
			if row >= 0 and row < len(self.roots) :
				return self.createIndex(row,column,self.roots[row])
			else :
				return QtCore.QModelIndex()
		if row >= len(parent.node) : return QtCore.QModelIndex()
		if parent.rows() <= row :
			for i in range(parent.rows(),row+1) :
				parent.add( parent.node[i].create_mtnode( i ) )
		return self.createIndex(row,column,parent.children_at(row))

	def parent(self, index):
		child  = index.internalPointer()
		if child == None or child.parent == None : return QtCore.QModelIndex()
		row = child.parent.row if child.parent != None else 0
		return self.createIndex( row , 0 , child.parent )

	def rowCount(self, index):
		if index.column() > 0 : return 0
		ptr = index.internalPointer()
		return len(ptr.node) if ptr != None else len(self.roots)

	def columnCount(self, index):
		return 2

	def headerData( self , section , orientation , role ) :
		if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole :
			return [ 'xml' , 'line' ][section]

	def data(self, index, role):
		if not index.isValid() : return None

		if role == QtCore.Qt.DisplayRole :
			# FIXME: this method is probably to havy for lage xml files
			n = index.internalPointer().node
			if index.column() == 1 :
				return str(n.getline())
			else :
				return n.toline()
		elif role == QtCore.Qt.BackgroundRole :
			if index.column() == 0 :
				color = index.internalPointer().node.node.get('color')
				return QtGui.QColor( color ) if color != None else None
		else:
			return None

	def supportedDropActions(self): 
		return QtCore.Qt.MoveAction | QtCore.Qt.IgnoreAction

	def flags(self, index):
		flags = QtCore.Qt.ItemIsEnabled
		flags |= QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsDropEnabled
		flags |= QtCore.Qt.ItemIsDragEnabled
		return flags
		       

	def mimeTypes(self):
		return ['pyipk/indexes']

	def mimeData(self, indexes):

		del self.drag[:]

		for nodeidx in indexes :
			if nodeidx.column() != 0 :
				continue

			self.drag.append( nodeidx )

		# create dummy data
		mime = QtCore.QMimeData()
		mime.setData('pyipk/indexes','')
		return mime

	def dropMimeData(self, data, action, row, column, parentidx):
		result = True

		if not data.hasFormat('pyipk/indexes') :
			result = False
		elif action == QtCore.Qt.IgnoreAction :
			result = False
		elif column > 0 :
			result = False
		elif all( n.internalPointer().parent == None for n in self.drag ) and not parentidx.isValid() :
			self.dropRoot( row )
		elif all( n.internalPointer().parent != None for n in self.drag ) and parentidx.isValid() :
			self.dropNodes( row , parentidx )
			self.changedSig.emit()
		else :
			result = False

		del self.drag[:]

		return result

	def dropRoot( self , row ) :
		tomove = [ self.roots[i.row()] for i in self.drag ]
		off = 0
		add = 0
		for d in sorted( self.drag , key = lambda i : i.row() ) :
			self.beginRemoveRows( QtCore.QModelIndex() , d.row()-off , d.row()-off )
			del self.roots[ d.row() - off ]
			self.endRemoveRows()
			off += 1
			if row > d.row() :
				add += 1

		row -= add

		self.beginInsertRows( QtCore.QModelIndex(), row, row+len(tomove)-1 )
		self.roots[row:row] = tomove
		self.endInsertRows()

	def dropNodes( self , row ,parentidx ) :
		qparent = parentidx.internalPointer().node

		for idx in self.drag :
			mtnode = idx.internalPointer()
			qnode = mtnode.node

			# insert into new node
			row = row if row >= 0 else self.rowCount(parentidx)
			if qnode.parent == qparent :
				if mtnode.row == row  : continue
				elif mtnode.row < row : row -= 1

			qnode.delete()
			qparent.insert( row , qnode )

