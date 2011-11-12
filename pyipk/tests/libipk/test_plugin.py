#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

from libipk.plugin import Plugin

class TestPlugin( unittest.TestCase ) :
	
	def setUp( self ) :
		self.plugin = Plugin( '//ipk:test' )

	def tearDown( self ) :
		pass

	def test_xpath_write( self ) :
		try :
			self.plugin.xpath = '/new'
		except AttributeError :
			self.assertTrue( True )
		else :
			self.assertTrue( False )

	def test_process( self ) :
		self.assertRaises( NotImplementedError , self.plugin.process , None )

