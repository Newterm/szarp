#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

class Plugin :
	def __init__( self , xpath = '/' ) :
		self._xpath = xpath

	@property
	def xpath( self ) :
		return self._xpath 

	def get_args( self ) :
		return []

	def process( self , node , **args ) :
		raise NotImplementedError('You must implement precess method')

	def result( self ) :
		return None

	def result_pretty( self ) :
		return repr(self.result())

