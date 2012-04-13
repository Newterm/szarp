#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import os , sys , inspect

import libipk.errors as errors
import libipk.plugin as plg

def except_no_plugin( func ) :
	def wrapper( *l , **m ) :
		try :
			return func( *l , **m )
		except KeyError as e :
			raise errors.NoSuchPlugin( e[0] )
	return wrapper

class Plugins :
	def __init__( self ) :
		self.plugins = {}

	def names( self ) :
		return self.plugins.keys()

	def load( self , path ) :
		sys.path.append( path )
		for modname in os.listdir(path) :
			if modname.endswith("plg.py") :
				modname = modname[:-3]
				self.import_module( modname )
		sys.path.pop()

	def import_module( self , modname ) :
		__import__( modname )
		plugin = sys.modules[ modname ]

		for c in inspect.getmembers( plugin , inspect.isclass ) :
			if issubclass(  c[1] , plg.Plugin ) and \
			   not  c[1] == plg.Plugin :
				   if c[0] in self.plugins :
					   raise ImportError('Plugin already loaded')
				   else :
					   self.plugins[c[0]] = c[1]

	@except_no_plugin
	def get( self , name , args = {} ) :
		return self.plugins[name]( **args )

	@except_no_plugin
	def get_args( self , name ) :
		return self.plugins[name].get_args()

	@except_no_plugin
	def get_section( self , name ) :
		return self.plugins[name].section()

	def available( self , name ) :
		return name in self.plugins

	def help( self , name ) :
		try :
			h = self.plugins[name].__doc__
			return h if h != None else 'documentation not provided'
		except KeyError as e :
			raise errors.NoSuchPlugin(name)

