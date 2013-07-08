#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

import string

from PyQt4 import QtGui , QtCore

from lxml import etree

from .utils import * 

from .ui.xml_view import Ui_XmlView

from libipk.params import *

# FIXME: this sux, but dont know yet how to fix it. Without it
# pythons GS deletes objects handle by Qt
globaltab = []

class ModelTree :
	def __init__( self , node , row ) :
		globaltab.append( self )
		
		self.parent = None
		self.node = node 
		self.row  = row

		self.children = None

	def node_changed( self ) :
		pass

	def rows( self ) :
		return len(self.node)

	def fill( self ) :
		if self.children == None :
			self.children = []
			for i in range(len(self.node)) :
				self.add( self.node[i].create_mtnode( i ) )

	def purge( self ) :
		for c in self.children : c.close()
		self.children = None

	def close( self ) :
		self.__disconnect()
		if self.children != None:
			for c in self.children : c.close()
			self.children = None

	def remove( self , child ) :
		row = child.row
		self.children.pop(row)
		self.__update_rows(row)

	def __update_rows( self , beg = 0 ) :
		# Update row index.
		# TODO: Could be done faster like this:
		#for i in ragne(beg,len(self.children))
		for i in range(len(self.children)) :
			self.children[i].row = i

	def __update_child( self , child ) :
		child.parent = self
		child.mtmodel = self.mtmodel

	def add( self , child ) :
		self.fill()
		self.children.append( child )
		self.__update_child( child )

	def children_at( self , row ) :
		self.fill()
		return self.children[row]

#    def __repr__( self ) :
#        return unicode( ('ModelTree',self.row,self.node) )

	def delete_beg( self ) :
		''' deletes self from ModelTree structure '''
		self.mtmodel.beginRemoveRows(
				self.mtmodel.createIndex(self.row,0,self.parent) , 
				self.row ,
				self.row )
		self.parent.remove( self )
		self.parent = None

	def delete_end( self ) :
		self.mtmodel.endRemoveRows()
		self.close()

	def __disconnect( self ) :
		self.node.  changedSig.disconnect(self.node_changed)
		self.node.deleteBegSig.disconnect(self.delete_beg)
		self.node.deleteEndSig.disconnect(self.delete_end)
		self.node.insertBegSig.disconnect(self.insert_beg)
		self.node.insertEndSig.disconnect(self.insert_end)

	def insert_beg( self , row , factory ) :
		''' inserts new MtNode based on factory function '''
		self.fill()
		if row < 0 : row = len(self.children)
		self.mtmodel.beginInsertRows(
				self.mtmodel.createIndex(row,0,self) , 
				row ,
				row )
		child = factory( row )
		self.children.insert( row , child )
		self.__update_child( child )
		self.__update_rows( row )

	def insert_end( self ) :
		self.mtmodel.endInsertRows()


class QNode( PNode , QtCore.QObject ) :
	changedSig = QtCore.pyqtSignal()
	deleteBegSig  = QtCore.pyqtSignal()
	deleteEndSig  = QtCore.pyqtSignal()
	insertBegSig  = QtCore.pyqtSignal(int,object)
	insertEndSig  = QtCore.pyqtSignal()

	drag = []

	def __init__( self , doc , parent , node ) :
		PNode.__init__( self , doc , parent , node )
		QtCore.QObject.__init__( self )

	def create_mtnode( self , row , mtmodel = None ) :
		''' factory for ModelTree nodes based on self node '''
		mt = ModelTree( self , row ) 
		self.  changedSig.connect( mt.node_changed )
		self.deleteBegSig.connect( mt.delete_beg )
		self.deleteEndSig.connect( mt.delete_end )
		self.insertBegSig.connect( mt.insert_beg )
		self.insertEndSig.connect( mt.insert_end )
		if mtmodel != None : mt.mtmodel = mtmodel
		return mt

	def touch( self ) :
		self.changedSig.emit()
		PNode.touch( self )

	def delete( self ) :
		PNode.remove( self.parent , self )

	def _remove( self , child ) :
		child.deleteBegSig.emit()
		PNode._remove( self , child )
		child.deleteEndSig.emit()

	def _insert( self , row , child ) :
		self.insertBegSig.emit( row , child.create_mtnode )
		PNode._insert( self , row , child )
		self.insertEndSig.emit()

	@classmethod
	def add_drag( cls , qnode ) :
		cls.drag.append( qnode )

	@classmethod
	def clear_drag( cls ) :
		del cls.drag[:]

	@classmethod
	def list_drag( cls ) :
		return self.drag

	def __repr__( self ) :
		return unicode( ('QNode',self.node) )


