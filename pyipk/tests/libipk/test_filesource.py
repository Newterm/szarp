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
		self.fn1 = fn

		fd , fn = tempfile.mkstemp()
		os.close(fd)
		self.fn2 = fn

	def tearDown( self ) :
		os.remove(self.fn1)
		os.remove(self.fn2)

	def test_read( self ) :
		shutil.copyfile( TEST_PARAMS , self.fn1 )
		s = fs.FS_ssh( '127.0.0.1' , self.fn1 , port = 22 )
		with open(self.fn2,'w') as f :
			f.write( s.read() )
		self.assertTrue( filecmp.cmp( TEST_PARAMS , self.fn2 ) )

	def test_write( self ) :
		shutil.copyfile( TEST_PARAMS , self.fn2 )
		s = fs.FS_ssh( 'localhost' , self.fn1 )
		with open(self.fn2,'r') as f :
			s.write( f.read() )
		self.assertTrue( filecmp.cmp( TEST_PARAMS , self.fn1 ) )

	@unittest.skip( "Host finding takes long time, but test should pass." )
	def test_fail_conn( self ) :
		self.assertRaises( fs.HostNotFoundError , fs.FS_ssh , 'unknown.local' , self.fn1 , port = 17 )

	def test_fail_auth( self ) :
		self.assertRaises( fs.AuthenticationError , fs.FS_ssh , 'localhost' , self.fn1 , username = 'root' )

