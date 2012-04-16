#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

import string

from PyQt4 import QtGui , QtCore

from lxml import etree

from .utils import * 

from .ui.xml_view import Ui_XmlView

from .params import ModelTree

class XmlView( QtGui.QWidget , Ui_XmlView ) :
	changedSig = QtCore.pyqtSignal()
	runSig = QtCore.pyqtSignal( list )
	showSig = QtCore.pyqtSignal( list )
	editSig = QtCore.pyqtSignal( list )

	def __init__( self , name , parent ) :
		QtGui.QWidget.__init__( self , parent )
		self.setupUi(self)

		self.model = XmlTreeModel()
		self.view.setModel( self.model )
		self.view.expanded.connect(self.model.expand)
		self.view.collapsed.connect(self.model.collapse)
		self.view.header().moveSection(1,0)
		self.model.changedSig.connect(self.changedSig)

		self.set_name( name )
	
	def contextMenuEvent(self, event):
		idxes = self.view.selectedIndexes()

		if len( idxes ) == 0 : return
		menu = QtGui.QMenu(self)

		runAction = menu.addAction("Run")
		showAction = menu.addAction("Show")
		delAction = menu.addAction("Delete")
		newAction = menu.addAction("Insert new")
		editnewAction = menu.addAction("Edit new")
		editAction = menu.addAction("Edit with")

		action = menu.exec_(self.mapToGlobal(event.pos()))

		nodes = [ i.internalPointer().node for i in idxes if i.column() == 0 ]

		def child_elem( node ) :
			return etree.Element(node.tag+"_child")

		if action == runAction :
			self.runSig.emit( nodes )
		elif action == showAction :
			self.showSig.emit( nodes )
		elif action == editAction :
			self.editSig.emit( nodes )
		elif action == newAction :
			for n in nodes : n.create_child(child_elem(n.node))
		elif action == editnewAction :
			newnodes = [ n.create_child(child_elem(n.node)) for n in nodes ]
			self.editSig.emit( newnodes )
		elif action == delAction :
			for n in nodes : n.delete()
			

	def set_name( self , name ) :
		pass

	def clear( self ) :
		self.model.clear()

	def add_node( self , node ) :
		if node == None : raise ValueError('Node cannot be None')
		self.model.add_node( node )
		self.view.update()
		self.update()

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
		else:
			return None

	def supportedDropActions(self): 
		return QtCore.Qt.MoveAction | QtCore.Qt.IgnoreAction

	def flags(self, index):
		flags = QtCore.Qt.ItemIsEnabled
		if not index.isValid():
			return flags
		flags |= QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsDropEnabled
		if index.internalPointer().parent == None :
			return flags
		flags |= QtCore.Qt.ItemIsDragEnabled
		return flags
		       

	def mimeTypes(self):
		return ['pyipk/indexes']

	def mimeData(self, indexes):
		for nodeidx in indexes :
			if nodeidx.column() != 0 :
				continue

			child = nodeidx.internalPointer()
			parent = child.parent

			if parent == None :
				continue

			self.drag.append( child )

		# create dummy data
		mime = QtCore.QMimeData()
		mime.setData('pyipk/indexes','')
		return mime

	def dropMimeData(self, data, action, row, column, parentidx):
		if not data.hasFormat('pyipk/indexes') : return False

		if action == QtCore.Qt.IgnoreAction or not parentidx.isValid() :
			del self.drag[:]
			return False

		if column > 0 : return False

		qparent = parentidx.internalPointer().node

		for mtnode in self.drag :
			qnode = mtnode.node

			# insert into new node
			row = row if row >= 0 else self.rowCount(parentidx)
			if qnode.parent == qparent :
				if mtnode.row == row  : continue
				elif mtnode.row < row : row -= 1

			qnode.delete()
			qparent.insert( row , qnode )

		del self.drag[:]

		self.changedSig.emit()

		return True

