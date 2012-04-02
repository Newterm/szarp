#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import unittest

import os , shutil , tempfile , filecmp

from lxml import etree

import libipk.filesource as fs

TEST_PARAMS = './tests/params.xml'

class TestSsh( unittest.TestCase ) :
	'''tests ssh connection. This test requires ssh server and exchanged keys with lo'''

	def setUp( self ) :
		fd , fn = tempfile.mkstemp()
		os.close(fd)
		self.fn = fn

	def tearDown( self ) :
		pass

	def test_read( self ) :
		shutil.copyfile( TEST_PARAMS , self.fn )
		s = fs.FS_ssh( 'localhost' , self.fn )
		self.assertTrue( filecmp.cmp( self.fn , s.filename() ) )

	def test_sync( self ) :
		s = fs.FS_ssh( 'localhost' , self.fn )
		shutil.copyfile( TEST_PARAMS , s.filename() )
		s.sync()
		self.assertTrue( filecmp.cmp( TEST_PARAMS , self.fn ) )

