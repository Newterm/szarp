#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest
import sys , os.path

from lxml import etree

import sip
sip.setapi('QString', 2)

from PyQt4.QtGui import QApplication , QWidget
from PyQt4.QtTest import QTest
from PyQt4.QtCore import Qt

from libipk import params as lp
from qtipk  import params as qp

from .trees import TestCaseTree

class TestModelTree( TestCaseTree ) :
	def setUp( self ) :
		self.params = lp.params_file( './tests/params.xml' , treeclass = qp.QNode )
		self.root = self.params.getroot()
	
	def tearDown( self ) :
		pass

class TestQNode( TestCaseTree ) :
	def setUp( self ) :
		self.params = lp.params_file( './tests/params.xml' , treeclass = qp.QNode )
	
	def tearDown( self ) :
		pass

	def test_remove_node( self ) :
		root = self.params.getroot()
		root[2].remove( root[2][10] )
		root[2][10].remove( root[2][10][0] )
		self.assertEqQTree(self.params.getroot() , self.params.getroot().node)

		self.assertRaises( ValueError , root[2].remove , root[2] )
		self.assertEqQTree(self.params.getroot() , self.params.getroot().node)

	def test_insert_node( self ) :
		root = self.params.getroot()
		node = qp.QNode( self.params.doc , root , etree.Element( 'sweetchild' ) )
		root.insert( 1 , node )
		self.assertEqQTree(self.params.getroot() , self.params.getroot().node)

		root.insert( 0 , root[0] )
		self.assertEqQTree(self.params.getroot() , self.params.getroot().node)

	def test_delete_node( self ) :
		root = self.params.getroot()
		node = root[0]

		node.delete()

		self.assertRaises( ValueError , root.remove , node )
		self.assertEqQTree(self.params.getroot() , self.params.getroot().node)

