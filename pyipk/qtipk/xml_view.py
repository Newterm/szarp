#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

import string

from PyQt4 import QtGui , QtCore

from lxml import etree

from .utils import * 

from .ui.xml_view import Ui_XmlView

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
		self.model.changedSig.connect(self.changedSig)

		self.set_name( name )
	
	def contextMenuEvent(self, event):
		idxes = self.view.selectedIndexes()

		if len( idxes ) == 0 : return
		menu = QtGui.QMenu(self)

		runAction = menu.addAction("Run")
		showAction = menu.addAction("Show")
		editAction = menu.addAction("Edit with")

		action = menu.exec_(self.mapToGlobal(event.pos()))

		nodes = [ i.internalPointer() for i in idxes ]

		if action == runAction :
			self.runSig.emit( nodes )
		elif action == showAction :
			self.showSig.emit( nodes )
		elif action == editAction :
			for i in idxes : self.view.collapse( self.model.parent(i) )
			self.editSig.emit( nodes )

	def set_name( self , name ) :
		pass

	def clear( self ) :
		self.model.clear()

	def add_node( self , node , parent = None ) :
		if node == None : raise ValueError('Node cannot be None')
		self.model.add_node( node , parent )
		self.view.update()
		self.update()

class DragInfo :
	def __init__( self , node , nodeidx , parent , parentid ) :
		self.node = node
		self.nodeidx = nodeidx
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
		self.nodes.append( node )
		self.endInsertRows()

	def clear( self ) :
		self.beginRemoveRows( QtCore.QModelIndex() , 0 , len(self.nodes) )
		self.nodes = []
		self.endRemoveRows()

	def index(self, row, column, parent):
		ptr = parent.internalPointer()
		if ptr == None :
			if row < 0 or row >= len(self.nodes) :
				return QtCore.QModelIndex()
			return self.createIndex(row,column,self.nodes[row])
		if row >= len(ptr) : return QtCore.QModelIndex()
		obj = ptr[row]
		self.tab.append(obj) #FIXME: this is nasty hack for GC. Must be fixed soon.
		return self.createIndex(row, column, obj)

	def parent(self, index):
		ptr = index.internalPointer()
		if ptr == None :
			return QtCore.QModelIndex()
		parent  = ptr    .getparent()
		if parent == None :
			return QtCore.QModelIndex()
		grandpa = parent .getparent()
		if grandpa == None :
			row = self.nodes.index(parent)
		else :
			row = grandpa.index( parent )
		column  = 0
		return self.createIndex( row , column , parent )

	def rowCount(self, index):
		ptr = index.internalPointer()
		if ptr == None :
			return len(self.nodes)
		else :
			return len(ptr) 

	def columnCount(self, index):
		return 1

	def insertRows(self, row, count, parent=None ):
		self.beginInsertRows()
		self.endInsertRows()

	def data(self, index, role):
		if not index.isValid() : return None

		if role == QtCore.Qt.DisplayRole :
			# FIXME: this method is probably to havy for lage xml files
			n = index.internalPointer()
#            ns = n.nsmap[None]
#            n.nsmap[None] = ''
#            out = toUtf8( etree.tostring( n , pretty_print = True , encoding = 'utf8' , method = 'xml' ) ).partition('\n')[0]
#            n.nsmap[None] = ns
#            return out
			if isinstance(n.tag,basestring) :
				out = '<%s '%n.tag + ' '.join(['%s="%s"'%(a,n.get(a)) for a in n.attrib]) + '>'
				out = string.replace(out,'{'+n.nsmap[None]+'}','') if None in n.nsmap else out
			else :
				out = toUtf8( etree.tostring( n , pretty_print = True , encoding = 'utf8' , method = 'xml' ) ).partition('\n')[0]
			return out
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
		for nodeidx in indexes :
			node = nodeidx.internalPointer()
			parent = node.getparent()

			if parent == None :
				continue

			row = parent.index( node )
			self.drag.append( DragInfo( node , nodeidx , parent , row ) )

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

		parent = parentidx.internalPointer()

		for d in self.drag :
			# insert into new node
			row = row if row >= 0 else len(parent)
			self.beginMoveRows(self.parent(d.nodeidx),d.parentid,d.parentid,parentidx,row)
			d.parent.remove( d.node )
			parent.insert(row,d.node )
			self.endMoveRows()
		del self.drag[:]

		self.changedSig.emit()

		return True

