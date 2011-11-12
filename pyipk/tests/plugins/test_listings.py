#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

import sys , os

from lxml import etree

sys.path.append(os.path.abspath('plugins'))
from listings_plg import *
sys.path.pop()

class TestMissingSubTag( unittest.TestCase ) :
	def setUp( self ) :
		self.plg = MissingSubTag()
		self.doc = etree.parse( 'tests/params.xml' )
		self.nsmap = { 'ipk' : 'http://www.praterm.com.pl/SZARP/ipk' }

	def tearDown( self ) :
		pass

	def test_args( self ) :
		self.assertListEqual( self.plg.get_args() , ['tagname'] )

	def test_wrong_args( self ) :
		self.assertRaises( ValueError , self.plg.process , None )

	def test_no_draw( self ) :
		for node in self.doc.xpath( self.plg.xpath , namespaces = self.nsmap ) :
			self.plg.process( node , tagname = 'draw' )
		self.assertEqual( len(self.plg.result()) , 1 )
		self.assertEqual( self.plg.result_pretty() , 'empty:empty:empty' )


	def test_no_report( self ) :
		for node in self.doc.xpath( self.plg.xpath , namespaces = self.nsmap ) :
			self.plg.process( node , tagname = 'report' )
		self.assertEqual( len(self.plg.result()) , 80 )


class TestCountSubTag( unittest.TestCase ) :
	def setUp( self ) :
		self.plg = CountSubTag()
		self.doc = etree.parse( 'tests/params.xml' )
		self.nsmap = { 'ipk' : 'http://www.praterm.com.pl/SZARP/ipk' }

	def tearDown( self ) :
		pass

	def test_args( self ) :
		self.assertListEqual( self.plg.get_args() , ['tagname'] )

	def test_count_draws( self ) :
		for node in self.doc.xpath( self.plg.xpath , namespaces = self.nsmap ) :
			self.plg.process( node , tagname = 'draw' )
		res = self.plg.result()
		zero = [ r for r in res if res[r] == 0 ]
		one  = [ r for r in res if res[r] == 1 ]
		self.assertEqual( len(zero) , 1 )
		self.assertEqual( len(one) , 79 )

	def test_count_reports( self ) :
		for node in self.doc.xpath( self.plg.xpath , namespaces = self.nsmap ) :
			self.plg.process( node , tagname = 'report' )
		self.assertEqual( len(self.plg.result_pretty().splitlines()) , 80 )

