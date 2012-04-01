#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest
import sys , os.path


class TestCaseTree( unittest.TestCase ) :
	def assertEqMtNode( self , mtnode , lnode ) :
		self.assertEqual( mtnode.node , lnode )
		self.assertEqual( mtnode.rows() , len(lnode) )
		self.assertEqual( mtnode.row , lnode.parent.index(lnode) )

	def assertEqModelTree( self , mtnode , lnode ) :
		self.assertEqNodes( mtnode , lnode )
		for i in range(len(mtnode.children)) :
			self.assertEqual( mtnode , mtnode.children[i].parent )
			assertEqModelTree( mtnode.children[i] , lnode[i] )

	def assertEqQNode( self , qnode , lnode ) :
		self.assertEqual( qnode.node , lnode )
		self.assertEqual( len(qnode) , len(lnode) )

	def assertEqQTree( self , qnode , lnode ) :
		self.assertEqQNode(qnode,lnode)
		for i in range(len(qnode)) :
			self.assertEqual( qnode , qnode[i].parent )
			self.assertEqQTree( qnode[i] , lnode[i] )

