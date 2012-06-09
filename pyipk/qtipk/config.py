#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import sip
sip.setapi('QString', 2)

from optparse import OptionParser

try :
	import configparser
except ImportError : # for python 2.X 
	import ConfigParser as configparser

import os , os.path

from PyQt4 import QtGui , QtCore

def fullpath( path ) :
	return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))

class Config( QtCore.QObject )  :

	editedSig  = QtCore.pyqtSignal( str , str )
	addedSig   = QtCore.pyqtSignal( str , str )
	removedSig = QtCore.pyqtSignal( str , str )

	def connect_key( self , sig , key , func ) :
		sig.connect( lambda k , v : k == key and func(v) )

	def on_edited( self , key , func ) :
		self.connect_key( self.editedSig , key , func )

	def on_added( self , key , func ) :
		self.connect_key( self.addedSig , key , func )

	def on_remove( self , key , func ) :
		self.connect_key( self.removedSig , key , func )

	def __init__( self ) :
		QtCore.QObject.__init__( self )

		self.m = {}
		
		self.default_values()

	def __getitem__( self , key ) :
		return self.m[key]

	def __delitem__( self , key ) :
		val = self.m[key]
		del self.m[key]
		self.removedSig.emit( key , val )

	def __setitem__( self , key , val ) :
		if key not in self.m :
			self.m[key] = val
			self.addedSig.emit( key , val )
		else :
			oval = self.m[key]
			self.m[key] = val
			if oval != val :
				self.editedSig.emit( key , val )

	def __contains__( self , key ) :
		return key in self.m

	def items( self ) :
		return self.m.items()

	def default_values( self ) :
		self['path:szarp']    = '/opt/szarp/'
		self['path:relax_ng'] = '/opt/szarp/resources/dtd/ipk-params.rng'
		self['path:plugins']  = '/opt/szarp/lib/plugins/'
		self['path:config']   = fullpath('~/.pyipk.ini')

		self['gui:editor']    = 'gvim'

	def parse_cmd_line( self , argv ) :
		self.parser = OptionParser()
		self.parser.add_option("-b" , "--base" ,
				metavar="<prefix>" ,
				help = "SZARP base prefix" )
		self.parser.add_option("-p" , "--params" ,
				metavar = "<params.xml>" ,
				help = "xml file with configuration" )
		self.parser.add_option("-u" , "--url" ,
				metavar="<url>" ,
				help="url to params.xml file. Currently only file and ssh protocols supported. Url format: protocol://user:password@host:port/path/to/file>" )
		self.parser.add_option("-g" , "--relax-ng" ,
				dest="relax_ng" ,
				metavar="<ipk-params.rng>" ,
				help="specify relax-ng schema. Defaults to %s." % self['path:relax_ng'] ,
				default = self['path:relax_ng'] )
		self.parser.add_option("" , "--load" ,
				metavar = "<path>" ,
				help = "directory where plugins are stored. Defaults to %s." % self['path:plugins'] ,
				default = self['path:plugins'] )
		self.parser.add_option("" , "--prefix" ,
				metavar = "<path>" ,
				help = "directory where SZARP is installed. Defaults to %s." % self['path:szarp'] ,
				default = self['path:szarp'] )
		self.parser.add_option("" , "--config" ,
				metavar = "<path>" ,
				help = "path to config file. Defaults to %s." % self['path:config'] ,
				default = self['path:config'] )
		options , self.args = self.parser.parse_args()

		if options.base :
			self['url:params'] = 'file:///opt/szarp/%s/config/params.xml' % options.base
		elif options.params :
			self['url:params'] = 'file:///%s' % fullpath( options.params )
		elif options.url :
			self['url:params'] = options.url

		self['path:szarp']    = fullpath( options.prefix )
		self['path:relax_ng'] = fullpath( options.relax_ng )
		self['path:plugins']  = fullpath( options.load )
		self['path:config']   = fullpath( options.config )

	def parse_cfg_file( self , path , force = True ) :
		cfg = configparser.SafeConfigParser()
		cfg.read( path )
		for sec in cfg.sections() :
			for k , v in cfg.items( sec ) :
				key = '%s:%s'%(sec,k)
				if force or key not in self :
					self[key] = v

	def save_to_cfg( self , path ) :
		self.to_save = path

	def save( self ) :
		cfg = configparser.SafeConfigParser()
		for k , v in self.m.items() :
			s , k = k.split(':',1)
			if not cfg.has_section(s) :
				cfg.add_section(s) 
			cfg.set(s,k,v)
		with open( self.to_save , 'wb' ) as f :
			cfg.write( f )

class Configurable :
	def __init__( self , parent = None ) :
		if parent != None :
			self._cfg = parent.cfg
		else :
			self._cfg = Config()

	@property
	def cfg( self ) :
		return self._cfg

	@cfg.setter
	def cfg( self , cfg ) :
		for k , v in cfg.items() :
			self._cfg[k] = v

