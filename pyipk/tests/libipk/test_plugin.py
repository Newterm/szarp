#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

import sys

from libipk.plugin import Plugin

class TestPlugin( unittest.TestCase ) :
	
	def setUp( self ) :
		self.plugin = Plugin( '//ipk:test' )

	def tearDown( self ) :
		pass

	@unittest.skipIf( sys.version_info < (3,0) ,
			"Seems that python2 allows property writing"  )
	def test_xpath_write( self ) :
		try :
			self.plugin.xpath = '/new'
		except Exception as e :
			self.assertIsInstance( e , AttributeError )
		else :
			self.assertTrue( False )

	def test_process( self ) :
		self.assertRaises( NotImplementedError , self.plugin.process , None )

