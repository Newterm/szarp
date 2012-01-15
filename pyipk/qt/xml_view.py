#!/usr/bin/python
# -*- coding: utf-8 -*-

from PyQt4 import QtGui , QtCore

from lxml import etree
from ui.xml_view import Ui_XmlView

from utils import * 

class XmlView( QtGui.QWidget , Ui_XmlView ) :
	def __init__( self , name , parent ) :
		QtGui.QWidget.__init__( self , parent )
		self.setupUi(self)

		self.model = XmlTreeModel()
		self.view.setModel( self.model )

		self.set_name( name )

	def set_name( self , name ) :
		pass

	def clear( self ) :
		self.model.clear()

	def add_node( self , node , parent = None ) :
		if node == None : raise ValueError('Node cannot be None')
		self.model.add_node( node , parent )
		self.view.update()
		self.update()

class XmlTreeModel(QtCore.QAbstractItemModel):
	def __init__(self):
		QtCore.QAbstractItemModel.__init__(self)
		self.nodes = []
		self.tab = []

	def add_node( self , node , parent ) :
		self.beginInsertRows(QtCore.QModelIndex(),len(self.nodes),len(self.nodes)+1)
		self.nodes.append( (node,QtCore.QModelIndex()) )
		self.endInsertRows()

	def clear( self ) :
		self.nodes = []

	def index(self, row, column, parent):
		ptr = parent.internalPointer()
		if ptr == None :
			return self.createIndex(row,column,self.nodes[row])
		obj = (ptr[0][row],parent)
		self.tab.append(obj)
		return self.createIndex(row, column, obj)

	def parent(self, index):
		ptr = index.internalPointer()
		return ptr[1]

	def rowCount(self, index):
		ptr = index.internalPointer()
		if ptr == None :
			return len(self.nodes)
		else :
			return len(ptr[0])

	def columnCount(self, index):
		return 1

	def data(self, index, role):
		if not index.isValid() : return QtCore.QVariant()

		if role == QtCore.Qt.DisplayRole :
			return index.internalPointer()[0].tag
		else:
			return QtCore.QVariant()

	def supportedDropActions(self): 
		return QtCore.Qt.CopyAction | QtCore.Qt.MoveAction

	def flags(self, index):
		if not index.isValid():
			return QtCore.Qt.ItemIsEnabled
		return QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable | \
		       QtCore.Qt.ItemIsDragEnabled | QtCore.Qt.ItemIsDropEnabled

	def mimeTypes(self):
		return ['text/xml']

	def mimeData(self, indexes):
		mimedata = QtCore.QMimeData()
		mimedata.setData('text/xml', 'mimeData')
		return mimedata

	def dropMimeData(self, data, action, row, column, parent):
		print 'dropMimeData %s %s %s %s' % (data.data('text/xml'), action, row, parent) 
		return True

