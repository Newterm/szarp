#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim: set fileencoding=utf-8 :

import lxml

from libipk import errors

from lxml import etree

toUtf8 = lambda s: s.decode('utf8')

class PNode :
	def __init__( self , doc , parent , node ) :
		self.doc = doc
		self.parent = parent
		self.node = node
		self.__build_children()

	def __len__( self ) :
		return len(self.children)

	def __getitem__( self, key ) :
		return self.children[key]

	def touch( self ) :
		self.doc.touch()

	def getline( self ) :
		return self.node.sourceline

	def toline( self ) :
#        ns = n.nsmap[None]
#        n.nsmap[None] = ''
#        out = toUtf8( etree.tostring( n , pretty_print = True , encoding = 'utf8' , method = 'xml' ) ).partition('\n')[0]
#        n.nsmap[None] = ns
#        return out
		if isinstance(self.node.tag,str) :
			tag = self.node.tag.replace( '{%s}' % self.node.nsmap[self.node.prefix] , '%s:' % self.node.prefix if self.node.prefix != None else  '' )
			out = '<%s '% tag + ' '.join(['%s="%s"'%(a,self.node.get(a)) for a in self.node.attrib]) + '>'
		else :
			out = toUtf8( etree.tostring( self.node , pretty_print = True , encoding = 'utf8' , method = 'xml' ) ).partition('\self.node')[0]
		return out

	def remove( self , child ) :
		try :
			self.children.remove( child )
			self.node.remove( child.node )
			child.parent = None
		except ValueError :
			raise ValueError('That child is not mine')

	def insert( self , pos , child ) :
		if child in self.children :
			return
		self.children.insert( pos , child )
		self.node.insert( pos , child.node )

	def __build_children( self ) :
		if self.node == None :
			raise ValueError('Node must be defined')

		self.children = []

		for n in self.node :
			self.children.append( PNode( self.doc , self , n ) )

class Params :
	def __init__( self , filename = None , mode = None , namespaces = None , treeclass = PNode ) :
		self.filename = filename
		self.changed = False
		self.doc = None
		self.nsmap = {}
		if filename :
			self.open( filename , mode , namespaces , treeclass )

	def open( self , filename , mode = None , namespaces = None , treeclass = PNode ) :
		if self.doc :
			raise IOError('File already opened')
		self.doc = etree.parse( filename )
		self.root = treeclass( self , None , self.doc.getroot() )
		self.changed = False

	def getroot( self ) :
		return self.root

	def touch( self ) :
		self.changed = True

	def save( self , fn = None ) :
		if fn == None : fn = self.filename
		self.doc.write( fn , pretty_print=True , method = 'xml' , xml_declaration=True , encoding=self.doc.docinfo.encoding )
		self.changed = False

	def close( self ) :
		return not self.changed 

