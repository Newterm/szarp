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
		self.doc = etree.parse( 'tests/params.xml' )

	def tearDown( self ) :
		pass

	def test_args( self ) :
		self.assertListEqual( MissingSubTag.get_args() , ['tag','subtag'] )

	def test_proper_args( self ) :
		args = {'tag':'a','subtag':'b'}
		plg = MissingSubTag( **args )
		plg.set_args( **args )

	def test_wrong_args( self ) :
		args = {'tag':'a'}
		plg = MissingSubTag( subtag = 'b' , **args )
		self.assertRaises( ValueError , MissingSubTag , **args )
		self.assertRaises( KeyError , plg.set_args , **args )

	def test_no_draw( self ) :
		args = {'tag':'param','subtag':'draw'}
		plg = MissingSubTag( **args )
		plg.process( self.doc.getroot() )
		self.assertEqual( len(plg.result()) , 1 )


	def test_no_report( self ) :
		args = {'tag':'param','subtag':'raport'}
		plg = MissingSubTag( **args )
		plg.process( self.doc.getroot() )
		self.assertEqual( len(plg.result()) , 71 )


class TestCountSubTag( unittest.TestCase ) :
	def setUp( self ) :
		self.doc = etree.parse( 'tests/params.xml' )

	def tearDown( self ) :
		pass

	def test_args( self ) :
		self.assertListEqual( CountSubTag.get_args() , ['tag','subtag','count'] )

	def test_count_draws( self ) :
		args = {}
		args['tag'] = 'param'
		args['subtag'] = 'draw'
		args['count'] = '0'

		plg = CountSubTag( **args )
		plg.process( self.doc.getroot() )
		zero = plg.result()

		args['count'] = '1'

		plg = CountSubTag( **args )
		plg.process( self.doc.getroot() )
		one = plg.result()

		self.assertEqual( len(zero) , 1 )
		self.assertEqual( len(one) , 79 )

class TestFindAttrib( unittest.TestCase ) :
	def setUp( self ) :
		self.doc = etree.parse( 'tests/params.xml' )

	def tearDown( self ) :
		pass

	def test_args( self ) :
		self.assertListEqual( FindAttrib.get_args() , ['tag','attrib','value'] )

	def test_subsubtag( self ) :
		plg = FindAttrib( tag='param' , attrib='name' , value='A:B:C' )
		plg.process( self.doc.getroot() )

		self.assertEqual( len(plg.result()) , 2 )

class TestFindRepeated( unittest.TestCase ) :
	def setUp( self ) :
		self.doc = etree.parse( 'tests/params.xml' )

	def tearDown( self ) :
		pass

	def test_args( self ) :
		self.assertListEqual( FindRepeated.get_args() , ['tag','attrib'])

	def test_multipledraws( self ) :
		plg = FindRepeated( tag='draw' , attrib='title' )
		plg.process( self.doc.getroot() )

		self.assertEqual( len(plg.result()) , 79 )

