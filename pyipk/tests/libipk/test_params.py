#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

from lxml import etree

import libipk.params

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

class TestApply( unittest.TestCase ) :
	def setUp( self ) :
		self.params = libipk.params.Params('tests/params.xml')

	def tearDown( self ) :
		pass

	def test_dummy( self ) :
		doc = etree.parse( 'tests/params.xml' )
		l1 = doc.xpath( '//ipk:unit' , namespaces = { 'ipk' : 'http://www.praterm.com.pl/SZARP/ipk' } )
		l2 = []
		self.params.apply( '//ipk:unit' , lambda x : l2.append(x) )
		self.assertEqual( len(l1) , len(l2) )

