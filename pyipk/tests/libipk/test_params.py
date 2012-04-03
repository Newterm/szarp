#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

import os , filecmp
import platform

from lxml import etree

import libipk.params
from libipk.filesource import FS_local

TMP_FILE = '/tmp/test_pyipk_params.xml'

class TestOpen( unittest.TestCase ) :

	def setUp( self ) :
		pass

	def tearDown( self ) :
		pass

	def test_construct( self ) :
		params = libipk.params.params_file('tests/params.xml')
		self.assertTrue(True)

	def test_double_open( self ) :
		params = libipk.params.params_file('tests/params.xml')
		self.assertRaises( IOError , params.open , FS_local("unknown.xml") )

	def test_fail_open( self ) :
		params = libipk.params.Params()
		self.assertRaises( IOError , params.open , FS_local("unknown.xml") )

class TestTouching( unittest.TestCase ) :
	def setUp( self ) :
		self.params = libipk.params.params_file('tests/params.xml')

	def tearDown( self ) :
		pass

	def test_not_touched( self ) :
		self.assertTrue( self.params.close() )

	def test_touch( self ) :
		self.params.touch()
		self.assertFalse( self.params.close() )

	def test_save( self ) :
		self.params.touch()
		self.assertFalse( self.params.close() )
		self.params.save( FS_local(TMP_FILE) )
		self.assertTrue( self.params.close() )

	def test_touch_node( self ) :
		self.params.getroot()[1][0][1].touch()
		self.assertFalse( self.params.close() )

class TestSave( unittest.TestCase ) :
	def setUp( self ) :
		self.params = libipk.params.params_file('tests/params.xml')

	def tearDown( self ) :
		os.remove( TMP_FILE )

	def test_save_same( self ) :
		self.params.save( FS_local(TMP_FILE) )
		self.assertTrue( filecmp.cmp( 'tests/params.xml' , TMP_FILE ) )

class TestParams( unittest.TestCase ) :
	def setUp( self ) :
		self.params = libipk.params.params_file( './tests/params.xml' )
	
	def tearDown( self ) :
		pass

	def test_find_node_path( self ) :
		r = self.params.getroot().node
		n = r[1][0][2][0]
		e = etree.Element('asdf')
		p = self.params.find_node_path( n )
		self.assertListEqual( p , [1,0,2,0] )

		p = self.params.find_node_path( r )
		self.assertListEqual( p , [] )

		p = self.params.find_node_path( r )
		self.assertListEqual( p , [] )

		self.assertRaises( ValueError , self.params.find_node_path , e )

	def test_get_pnode( self ) :
		r = self.params.getroot()
		n = r[2][17][0]

		p = []
		self.assertEqual( self.params.get_pnode( p ) , r )

		p = [ 2 , 17 , 0 ]
		self.assertEqual( self.params.get_pnode( p ) , n )

		p = [ 2, 17 , 0 , 0 ]
		self.assertEqual( self.params.get_pnode( p ) , None )

class TestPNode( unittest.TestCase ) :
	def setUp( self ) :
		self.params = libipk.params.params_file( './tests/params.xml' )
	
	def tearDown( self ) :
		pass

	def assertEqTrees( self ) :
		def rec_test( pnode , lnode ) :
			self.assertEqual( pnode.node , lnode )
			self.assertEqual( len(pnode) , len(lnode) )
			for i in range(len(pnode)) :
				self.assertEqual( pnode , pnode[i].parent )
				rec_test( pnode[i] , lnode[i] )
			
		rec_test( self.params.getroot() , self.params.getroot().node )

	def test_static_tree( self ) :
		self.assertEqTrees()

	def test_remove_node( self ) :
		root = self.params.getroot()
		root[2].remove( root[2][10] )
		root[2][10].remove( root[2][10][0] )
		self.assertEqTrees()

		self.assertRaises( ValueError , root[2].remove , root[2] )
		self.assertEqTrees()

	def test_insert_node( self ) :
		root = self.params.getroot()
		node = libipk.params.PNode( self.params.doc , root , etree.Element( 'sweetchild' ) )
		root.insert( 1 , node )
		self.assertEqTrees()

		root.insert( 0 , root[0] )
		self.assertEqTrees()

	def test_replace( self ) :
		r = self.params.getroot()
		r[2].replace( etree.Element('cze1') )
		self.assertEqTrees()
		
		r.replace( etree.Element('cze2') )
		self.assertEqTrees()

	def test_rebuild_one( self ) :
		r = self.params.getroot().node

		r.insert( 0 , r[3] )
		self.params.rebuild_tree()
		self.assertEqTrees()

		r.insert( 0 , etree.Element('cze') )
		self.params.rebuild_tree()
		self.assertEqTrees()

		r.remove( r[3] )
		self.params.rebuild_tree()
		self.assertEqTrees()

	def test_rebuild_all( self ) :
		r = self.params.getroot().node

		r[0].insert( 0 , r[3] )
		r[1].insert( 0 , etree.Element('cze') )
		r[2].remove( r[2][0] )

		self.params.rebuild_tree()
		self.assertEqTrees()

	def test_rebuild_root( self ) :
		self.params.root.node = etree.Element('newroot')
		self.params.rebuild_tree()
		self.assertEqTrees()

	def test_rebuild_same( self ) :
		q = self.params.getroot()[3]
		r = self.params.getroot().node
		r.insert( 0 , r[3] )
		r.insert( 0 , etree.Element('cze') )
		r.remove( r[2] )
		self.params.rebuild_tree()

		self.assertEqual( q.node , self.params.getroot()[1].node )
		self.assertEqual( q , self.params.getroot()[1] )

	def test_rebuild_result( self ) :
		e = etree.Element('cze')
		q = self.params.getroot()[3]
		r = self.params.getroot().node
		n = r[3]
		r.insert( 0 , r[3] )
		r.insert( 0 , e )
		r.remove( r[2] )

		res = self.params.rebuild_tree( [e,n,r] )
		self.assertEqual( res[0].node , e )
		self.assertEqual( res[1] , q )
		self.assertEqual( res[2] , self.params.getroot() )

	def test_rebuild_insert( self ) :
		s = self.params.getroot().node[3]
		s.insert( 0 , etree.Element('cze') )
		self.params.rebuild_tree()
		self.assertEqTrees()

