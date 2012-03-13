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

		nodes = [ i.internalPointer().node for i in idxes ]

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

globaltab = []

class ModelTree :
	def __init__( self , node , row , parent=None ) :
		globaltab.append( self )
		
		self.parent = None if parent == None else parent.add( self )
		self.node = node 
		self.row  = row

		self.children = []
		# TODO: should be done dinamicly
		for i in range(len(self.node)) :
			self.add( ModelTree( self.node[i] , i ) )

	def insert( self, row , child ) :
		prev = child.parent.children.index( child )

		if child.parent == self and prev < row : row -= 1

		child.parent.children.pop(prev)
		self.children.insert( row , child )

		self.__update_rows( row )
		child.parent.__update_rows( prev )

		child.parent = self

	def __update_rows( self , beg = 0 ) :
		# Update row index.
		# TODO: Could be done faster like this: for i in ragne(beg,len(self.children))
		for i in range(len(self.children)) :
			self.children[i].row = i

	def add( self , child ) :
		self.children.append( child )
		child.parent = self

	def __repr__( self ) :
		return unicode( (self.row,self.node) )

class XmlTreeModel(QtCore.QAbstractItemModel):
	changedSig = QtCore.pyqtSignal()

	def __init__(self):
		QtCore.QAbstractItemModel.__init__(self)
#        self.root      = ModelTree(None,0)
#        self.root.node = etree.Element('root')
		self.drag  = []
		self.roots = []

	def add_node( self , node ) :
#        row = len(self.root.children)
		row = len(self.roots)
		self.beginInsertRows(QtCore.QModelIndex(),row,row+1)
		self.roots.append( ModelTree(node,row) )
#        self.root.node.append( node )
#        self.root.add_child( ModelTree(node,row) )
		self.endInsertRows()

#    def createIndex( self , row , column , ptr ) :
#        i = QtCore.QAbstractItemModel.createIndex( self , row , column , ptr )
#        print i
#        return i

#    def create_model_tree( self , parentmt , node , row , column ) :
#        mt = ModelTree(parentmt,node)
#        idx= self.createIndex(row,column,mt)
#        mt.set_index(idx)
#        return mt

	def clear( self ) :
		self.beginRemoveRows( QtCore.QModelIndex() , 0 , len(self.roots) )
		del self.roots[:]
#        self.root = ModelTree(None,0)
#        self.root.node = etree.Element('root')
		self.endRemoveRows()

	def index(self, row, column, parentidx ):
		parent = parentidx.internalPointer()
#        print 'index' , row , column , parent
		if parent == None :
			if row >= 0 and row < len(self.roots) :
#                print 'row>=0 and row<len(self.roots)' ,  self.roots[row]
				return self.createIndex(row,column,self.roots[row])
			else :
#                print 'Empty'
				return QtCore.QModelIndex()
		if row >= len(parent.node) : return QtCore.QModelIndex()
		if len(parent.children) <= row :
			for i in range(len(parent.children),row+1) :
#                print
#                print 'add'
#                print
				parent.add( ModelTree( parent.node[i] , i ) )
#        print 'Normal' , row , parent.children[row] , parent.children
		return self.createIndex(row,column,parent.children[row])

	def parent(self, index):
#        print 'parent' , index.internalPointer()
		child  = index.internalPointer()
		if child == None or child.parent == None : return QtCore.QModelIndex()
		row = child.parent.row if child.parent != None else 0
#        print row ,child.parent, child.node.getparent()
#        print row ,child.parent , child.parent.children
#        print row ,child.parent.node , child.parent.node.getchildren()
		return self.createIndex( row , 0 , child.parent )

	def rowCount(self, index):
		ptr = index.internalPointer()
		print 'rowCount' , ptr , len(ptr.node) if ptr != None else 0
		return len(ptr.node) if ptr != None else len(self.roots)

	def columnCount(self, index):
		return 1

	def data(self, index, role):
#        print 'data' , index , index.internalPointer() , role
		if not index.isValid() : return None

		if role == QtCore.Qt.DisplayRole :
			# FIXME: this method is probably to havy for lage xml files
#            print 'Display'
			n = index.internalPointer().node
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
#            print 'None'
			return None

	def supportedDropActions(self): 
		return QtCore.Qt.MoveAction | QtCore.Qt.IgnoreAction

	def flags(self, index):
#        print 'flags'
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
#        print 'mime'
		for nodeidx in indexes :
			child = nodeidx.internalPointer()
			parent = child.parent

			if parent == None :
				continue

#            print 'drag: ' , child , parent
			self.drag.append( DragInfo( child, nodeidx, parent ) )

		# create dummy data
		mime = QtCore.QMimeData()
		mime.setData('pyipk/indexes','')
		return mime

	def dropMimeData(self, data, action, row, column, parentidx):
#        print 'drag'
		if not data.hasFormat('pyipk/indexes') : return False

		if action == QtCore.Qt.IgnoreAction or not parentidx.isValid() :
			del self.drag[:]
			return False

		if column > 0 : return False

		parent = parentidx.internalPointer()

		for d in self.drag :
			# insert into new node
			row = row if row >= 0 else self.rowCount(parentidx)
			if d.parent == parent and (row == d.node.row+1 or row == d.node.row) : continue
#            print
#            print 'drop: ' , d.node , d.parent , d.node.row , '\t->\t' , parent , row
#            print
			self.beginMoveRows(self.parent(d.nodeidx),d.node.row,d.node.row,parentidx,row)

#            print 'beginned'
			d.parent.node.remove( d.node.node )
			parent.node.insert(row,d.node.node )

#            print parent , d.node , parent.children
#            print d.parent , d.node , d.parent.children

			parent.insert( row , d.node )

#            print parent , d.node , parent.children
#            print d.parent , d.node , d.parent.children

#            print 'ending'

			self.endMoveRows()
#            print 'ended'
		del self.drag[:]

		self.changedSig.emit()

		return True

