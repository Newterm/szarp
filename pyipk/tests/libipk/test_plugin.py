#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

import sys

from libipk.plugin import Plugin

class TestPlugin( unittest.TestCase ) :
	
	def setUp( self ) :
		self.plugin = Plugin()

	def tearDown( self ) :
		pass

	def test_not_implemented( self ) :
		self.assertRaises( NotImplementedError , self.plugin.process , None )
		self.assertRaises( NotImplementedError , self.plugin.result )

