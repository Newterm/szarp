#!/usr/bin/python
# -*- coding: utf-8 -*-

import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui , QtCore

from .utils import * 

from .ui.map_view import Ui_MapView
from .ui.map_diag import Ui_MapDialog

class MapView( QtGui.QWidget , Ui_MapView ) :

	def __init__( self , parent ) :
		QtGui.QWidget.__init__( self , parent )
		self.setupUi(self)
		self.l = []
		self.view.setColumnCount( 2 )
		self.view.setColumnWidth( 1 , 400 )
		self.view.cellChanged.connect( self.onChange )

	def set_map( self , m ) :
		self.fill( list( m.items() ) )

	def set_list( self , l ) :
		self.fill( l )

	def get_map( self ) :
		return dict( self.l )

	def get_list( self ) :
		return self.l

	def onChange( self , r , c ) :
		i0 , i1 = self.view.item(r,0) , self.view.item(r,1)
		if i0 == None or i1 == None : return
		self.l[r] = (i0.text(),i1.text())

	def add_item( self , i , k , v ) :
		self.view.insertRow(i)
		self.l.append( (k,v) )
		self.view.setItem(i,0,QtGui.QTableWidgetItem( k ))
		self.view.setItem(i,1,QtGui.QTableWidgetItem( v ))

	def del_item( self , i ) :
		self.view.removeRow( i )
		self.l.pop(i)

	def fill( self , l ) :
		i = 0
		for k , v in l :
			self.add_item( i , k , v )
			i+=1

	def contextMenuEvent(self, event):
		menu = QtGui.QMenu(self)
		addAction = menu.addAction("Add")
		delAction = menu.addAction("Del")

		action = menu.exec_(self.mapToGlobal(event.pos()))

		if action == addAction :
			self.add_item( len(self.l) , "" , "" )
		elif action == delAction :
			r = self.view.currentRow()
			self.del_item( r )

class MapDialog( QtGui.QDialog , Ui_MapDialog ) :

	def __init__( self , parent ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi(self)

		self.view = MapView( parent )

		self.lay.addWidget( self.view )

		self.set_map = self.view.set_map
		self.get_map = self.view.get_map
		self.set_list = self.view.set_list
		self.get_list = self.view.get_list

