#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

import sys , os

from lxml import etree

sys.path.append(os.path.abspath('plugins'))
from edit_plg import *
sys.path.pop()

class TestEnumerateChoosenAttrib( unittest.TestCase ) :
	def setUp( self ) :
		self.doc = etree.parse( 'tests/params.xml' )
		self.root = self.doc.getroot()

	def tearDown( self ) :
		pass

	def test_args( self ) :
		self.assertListEqual( EnumerateChoosenAttrib.get_args() , ['attrib','start'] )

	def test_increment( self ) :
		plg = EnumerateChoosenAttrib( attrib = 'id' , start='0' )
		for n in self.root :
			plg.process( n )

		i = 0
		for n in self.root :
			self.assertEqual( n.get('id') , '%d' % i )
			i += 1

	def test_decrement( self ) :
		plg = EnumerateChoosenAttrib( attrib = 'number' , start='100' )
		for n in reversed(self.root) :
			plg.process( n )

		i = 100 + len(self.root) - 1
		for n in self.root :
			self.assertEqual( n.get('number') , '%d' % i )
			i -= 1

class TestEnumerateChoosenAttrib( unittest.TestCase ) :
	def setUp( self ) :
		self.doc = etree.parse( 'tests/params.xml' )
		self.root = self.doc.getroot()
		self.defined = self.root.xpath('.//ns:defined[1]',namespaces={'ns':self.root.nsmap[None] } )[0]

	def tearDown( self ) :
		pass

	def test_args( self ) :
		self.assertListEqual( EnumerateSubTagAttrib.get_args() , ['tag','attrib','start'] )

	def test_increment( self ) :
		plg = EnumerateChoosenAttrib( attrib = 'id' , start='-5' , subtag='param' )
		plg.process( self.defined )

		i = -5
		for n in self.root :
			if n.tag == '{%s}:param' % self.root.nsmap[None] :
				self.assertEqual( n.get('id') , '%d' % i )
				i += 1

class TestEnumerate( unittest.TestCase ) :
	def setUp( self ) :
		self.doc = etree.parse( 'tests/params.xml' )
		self.root = self.doc.getroot()
		self.defined = self.root.xpath('.//ns:defined[1]',namespaces={'ns':self.root.nsmap[None] } )[0]

	def tearDown( self ) :
		pass

	def test_args( self ) :
		self.assertListEqual( EnumerateSubTagAttrib.get_args() , ['tag','attrib','start'] )

	def test_increment( self ) :
		plg1 = EnumerateSubTagAttrib( attrib = 'id1' , start='0' , tag='param' )
		plg2 = EnumerateChoosenAttrib( attrib = 'id2' , start='0' )
		plg1.process( self.defined )
		for n in self.root :
			if n.tag == '{%s}:param' % self.root.nsmap[None] :
				plg2.process( n )

		for n in self.root :
			self.assertEqual( n.get('id1') , n.get('id2') )


