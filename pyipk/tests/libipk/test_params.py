#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

import os , filecmp

from lxml import etree

import libipk.params

TMP_FILE = '/tmp/test_pyipk_params.xml'

class TestOpen( unittest.TestCase ) :

	def setUp( self ) :
		pass

	def tearDown( self ) :
		pass

	def test_construct( self ) :
		params = libipk.params.Params('tests/params.xml')
		self.assertTrue(True)

	def test_double_open( self ) :
		params = libipk.params.Params('test/params.xml')
		self.assertRaises( IOError , params.open , "unknown.xml" )

	def test_double_open( self ) :
		params = libipk.params.Params()
		self.assertRaises( IOError , params.open , "unknown.xml" )

class TestTouching( unittest.TestCase ) :
	def setUp( self ) :
		self.params = libipk.params.Params('tests/params.xml')

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
		self.params.save()
		self.assertTrue( self.params.close() )

class TestSave( unittest.TestCase ) :
	def setUp( self ) :
		self.params = libipk.params.Params('tests/params.xml')

	def tearDown( self ) :
		os.remove( TMP_FILE )

	def test_save_same( self ) :
		self.params.save( TMP_FILE )
		filecmp.cmp( 'tests/params.xml' , TMP_FILE )

