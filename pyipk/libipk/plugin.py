#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

class Plugin :
	def __init__( self , **args ) :
		try :
			self.set_args( **args )
		except KeyError as e:
			raise ValueError('%s: Your argument is invalid: %s' % (self.__class__.__name__,repr(e)) )

	def set_args( self ) :
		pass

	@staticmethod
	def get_args() :
		return []

	def process( self , node , **args ) :
		raise NotImplementedError('You must implement precess method')

	def result( self ) :
		raise NotImplementedError('You must implement result method')

