#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui , QtCore

from lxml import etree

from .utils import * 

from .ui.xml_view import Ui_XmlView

class XmlView( QtGui.QWidget , Ui_XmlView ) :
	changedSig = QtCore.pyqtSignal()
	runSig = QtCore.pyqtSignal( list )

	def __init__( self , name , parent ) :
		QtGui.QWidget.__init__( self , parent )
		self.setupUi(self)

		self.model = XmlTreeModel()
		self.view.setModel( self.model )
		self.model.changedSig.connect(self.changedSig)

		self.set_name( name )
	
	def contextMenuEvent(self, event):
		idxes = self.view.selectedIndexes()
		print [ i.internalPointer().node for i in idxes ] 

		if len( idxes ) == 0 : return
		menu = QtGui.QMenu(self)

		runAction = menu.addAction("Run")
		action = menu.exec_(self.mapToGlobal(event.pos()))

		if action == runAction :
			self.runSig.emit( [ i.internalPointer().node for i in idxes ] )

	def set_name( self , name ) :
		pass

	def clear( self ) :
		self.model.clear()

	def add_node( self , node , parent = None ) :
		if node == None : raise ValueError('Node cannot be None')
		self.model.add_node( node , parent )
		self.view.update()
		self.update()

class NodeInfo :
	def __init__( self , node , parent ) :
		self.node   = node
		self.parent = parent

class DragInfo :
	def __init__( self , nodeinfo , parent , parentid ) :
		self.nodeinfo = nodeinfo
		self.parent = parent
		self.parentid = parentid

class XmlTreeModel(QtCore.QAbstractItemModel):
	changedSig = QtCore.pyqtSignal()

	def __init__(self):
		QtCore.QAbstractItemModel.__init__(self)
		self.nodes = []
		self.tab   = []
		self.drag  = []

	def add_node( self , node , parent ) :
		self.beginInsertRows(QtCore.QModelIndex(),len(self.nodes),len(self.nodes)+1)
		self.nodes.append( NodeInfo(node,QtCore.QModelIndex()) )
		self.endInsertRows()

	def clear( self ) :
		self.nodes = []

	def index(self, row, column, parent):
		ptr = parent.internalPointer()
		if ptr == None :
			return self.createIndex(row,column,self.nodes[row])
		if row >= len(ptr.node) : return QtCore.QModelIndex()
		obj = NodeInfo(ptr.node[row],parent)
		self.tab.append(obj) #FIXME: this is nasty hack for GC. Must be fixed soon.
		return self.createIndex(row, column, obj)

	def parent(self, index):
		ptr = index.internalPointer()
		return ptr.parent if ptr != None else QtCore.QModelIndex()

	def rowCount(self, index):
		ptr = index.internalPointer()
		if ptr == None :
			return len(self.nodes)
		else :
			return len(ptr.node)

	def columnCount(self, index):
		return 1

	def data(self, index, role):
		if not index.isValid() : return None

		if role == QtCore.Qt.DisplayRole :
			return index.internalPointer().node.tag
		else:
			return None

	def supportedDropActions(self): 
		return QtCore.Qt.MoveAction | QtCore.Qt.IgnoreAction

	def flags(self, index):
		if not index.isValid():
			return QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsDropEnabled
		return QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable | \
		       QtCore.Qt.ItemIsDragEnabled | QtCore.Qt.ItemIsDropEnabled

	def mimeTypes(self):
		return ['pyipk/indexes']

	def mimeData(self, indexes):
		for i in indexes :
			ni = i.internalPointer()
			node = ni.node
			parent = node.getparent()
			parentidx = self.parent(i)

			if parent == None :
				continue

			# remove from old parent if not same
			row = parent.index( node )
			self.beginRemoveRows( parentidx , row , row )
			parent.remove( node )
			self.endRemoveRows()
			self.drag.append( DragInfo( ni , parent , row ) )

		# create dummy data
		mime = QtCore.QMimeData()
		mime.setData('pyipk/indexes','')
		return mime

	def dropMimeData(self, data, action, row, column, parent):
		if not data.hasFormat('pyipk/indexes') : return False

		if action == QtCore.Qt.IgnoreAction or not parent.isValid() :
			for d in self.drag :
				# insert into old place
				self.beginInsertRows(d.nodeinfo.parent,d.parentid,d.parentid)
				d.parent.insert(d.parentid,d.nodeinfo.node)
				self.endInsertRows()
			del self.drag[:]

			return False

		if column > 0 : return False

		parentinfo = parent.internalPointer()

		for d in self.drag :
			# insert into new node
			row = row if row >= 0 else len(parentinfo.node)
			self.beginInsertRows(parent,row,row)
			parentinfo.node.insert(row,d.nodeinfo.node)
			self.endInsertRows()
		del self.drag[:]

		self.changedSig.emit()

		return True

