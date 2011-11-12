#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

import os , sys

import libipk.plugins 
import libipk.errors

sys.path.append(os.path.abspath('tests/libipk/plugins'))
from basic_plg import BasicPlugin
from another_plg import AnotherPlugin
from double_plg import *
sys.path.pop()

class TestImport( unittest.TestCase ) :

	def setUp( self ) :
		pass

	def tearDown( self ) :
		pass

	def test_basic_import( self ) :
		plugins = libipk.plugins.Plugins()
		plugins.import_module( 'basic_plg' )

		self.assertEqual( plugins.plugins["BasicPlugin"] , BasicPlugin )

	def test_double_import( self ) :
		plugins = libipk.plugins.Plugins()
		plugins.import_module( 'double_plg' )

		self.assertEqual( plugins.plugins["FirstPlugin"] , FirstPlugin )
		self.assertEqual( plugins.plugins["SecondPlugin"] , SecondPlugin )

class TestLoad( unittest.TestCase ) :
	def setUp( self ) :
		sys.path.append(os.path.abspath('tests/libipk/plugins'))

	def tearDown( self ) :
		sys.path.pop()

	def test_basic_load( self ) :
		plugins = libipk.plugins.Plugins()
		plugins.load('tests/libipk/plugins')

		self.assertEqual( plugins.plugins["BasicPlugin"] , BasicPlugin )
		self.assertEqual( plugins.plugins["AnotherPlugin"] , AnotherPlugin )

	def test_redundant_load( self ) :
		plugins = libipk.plugins.Plugins()
		plugins.load('tests/libipk/plugins')
		self.assertRaises( ImportError , plugins.load , 'tests/libipk/plugins')

class TestApply( unittest.TestCase ) :
	def setUp( self ) :
		self.plugins = libipk.plugins.Plugins()
		self.plugins.load('tests/libipk/plugins')

	def tearDown( self ) :
		pass

	def test_apply_plugin( self ) :
		self.assertTrue( isinstance( self.plugins.get( "BasicPlugin" ) , BasicPlugin ) )
		self.assertTrue( isinstance( self.plugins.get( "AnotherPlugin" ), AnotherPlugin ) )

	def test_apply_wrong_plugin( self ) :
		self.assertRaises( libipk.errors.NoSuchPlugin , self.plugins.get , "UnknownPlugin" )

