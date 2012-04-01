#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest
import sys , os.path

import sip
sip.setapi('QString', 2)

from PyQt4.QtGui import QApplication , QWidget
from PyQt4.QtTest import QTest
from PyQt4.QtCore import Qt , QModelIndex

from qtipk.xml_view import XmlView , XmlTreeModel
from qtipk.params import QNode , ModelTree
from libipk.params import Params

from lxml import etree

from .trees import TestCaseTree


class TestModelTree( TestCaseTree ) :
	def setUp( self ) :
		self.model = XmlTreeModel()
		self.params = Params( './tests/params.xml' , treeclass=QNode )
	
	def tearDown( self ) :
		pass

	def test_mulit_node( self ) :
		self.model.add_node( self.params.getroot()[0] )
		self.model.add_node( self.params.getroot()[1] )
		i0 = self.model.index(0,0,QModelIndex()).internalPointer().node
		i1 = self.model.index(1,0,QModelIndex()).internalPointer().node
		q0 = self.params.getroot()[0]
		q1 = self.params.getroot()[1]
		self.assertEqual( i0 , q0 )
		self.assertEqual( i1 , q1 )

class TestModelTreeDragNDrop( TestCaseTree ) :
	def setUp( self ) :
		self.model = XmlTreeModel()
		self.params = Params( './tests/params.xml' , treeclass=QNode )

		self.model.add_node( self.params.getroot() )

		self.i0 = self.model.index(0,0,QModelIndex())
		self.i00 = self.model.index(0,0,self.i0)
		self.i01 = self.model.index(1,0,self.i0)
		self.i02 = self.model.index(2,0,self.i0)

		self.n0 = self.params.getroot()
		self.n00 = self.n0[0]
		self.n01 = self.n0[1]
		self.n02 = self.n0[2]
		self.n000 = self.n00[0]
		self.n010 = self.n01[0]
	
	def tearDown( self ) :
		pass

	def getIdx( self , row , parent ) :
		return self.model.index(row,0,parent)

	def assertQNodeAtIndex( self , row , parent , qnode , msg = None ) :
		self.assertEqual( self.model.index(row,0,parent).internalPointer().node , qnode , msg )

	def test_oneup( self ) :
		mime = self.model.mimeData( [self.i01] )
		self.model.dropMimeData( mime , Qt.DropAction , 0 , 0 , self.i0 )

		self.assertQNodeAtIndex( 0, self.i0, self.n01 )
		self.assertQNodeAtIndex( 1, self.i0, self.n00 )

	def test_onedown( self ) :
		mime = self.model.mimeData( [self.i00] )
		self.model.dropMimeData( mime, Qt.DropAction, 2, 0, self.i0 )

		i0 = self.model.index(0,0,self.i0).internalPointer().node
		i1 = self.model.index(1,0,self.i0).internalPointer().node

		self.assertEqual( self.n00 , i1 )
		self.assertEqual( self.n01 , i0 )

	def test_sameplace( self ) :
		mime = self.model.mimeData( [self.i02] )
		self.model.dropMimeData( mime, Qt.DropAction, 2, 0, self.i0 )
		self.assertQNodeAtIndex( 2, self.i0, self.n0[2] )

	def test_double_move( self ) :
		mime = self.model.mimeData( [self.getIdx(2,self.i0)] )
		self.model.dropMimeData( mime, Qt.DropAction, 1, 0, self.i0 )
		self.assertQNodeAtIndex( 1, self.i0, self.n02 )

		mime = self.model.mimeData( [self.getIdx(1,self.i0)] )
		self.model.dropMimeData( mime, Qt.DropAction, 0, 0, self.i0 )
		self.assertQNodeAtIndex( 0, self.i0, self.n02 )

	def test_change_parent( self ) :
		mime = self.model.mimeData( [self.getIdx(0,self.i01)] )
		self.model.dropMimeData( mime, Qt.DropAction, 0, 0, self.i00 )
		self.assertQNodeAtIndex( 0, self.i00, self.n010 )
		self.assertQNodeAtIndex( 1, self.i00, self.n000 )
		self.assertEqQTree(self.n0,self.params.doc.getroot())

		mime = self.model.mimeData( [self.getIdx(1,self.i00)] )
		self.model.dropMimeData( mime, Qt.DropAction, 0, 0, self.i01 )
		self.assertQNodeAtIndex( 0, self.i00, self.n010 )
		self.assertQNodeAtIndex( 0, self.i01, self.n000 )
		self.assertEqQTree(self.n0,self.params.doc.getroot())

class TestXmlView( TestCaseTree ) :
	def setUp( self ) :
		self.app = QApplication(sys.argv)
		self.parent = QWidget()
		self.xml = XmlView( 'name' , self.parent )
		self.view = self.xml.view
		self.model = self.xml.model

		self.params = Params( './tests/params.xml' , treeclass=QNode )
	
	def tearDown( self ) :
		self.app.deleteLater()

