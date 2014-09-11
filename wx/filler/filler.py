#!/usr/bin/env python
# -*- coding: utf-8 -*-
# SZARP: SCADA software 
# 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

import os
import copy
import re

import gettext
gettext.bindtextdomain('filler', '/opt/szarp/resources/locales')
gettext.textdomain('filler')
_ = gettext.gettext

# SAX parser
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces
from xml.sax.handler import ContentHandler
from xml.sax import saxutils
from xml.sax import SAXException

# date time module form package python-egenix-mxdatetime
from mx.DateTime import *


# reads path to params.xml from szarp.cfg using lpparse
# LPPoptions - options to lpparse
# options - options to be read form szarp.cfg
def getSzarpOptions(LPPoptions,options):
	SzarpBinPATH='/opt/szarp/bin/'
	lpparseExec=SzarpBinPATH + 'lpparse'
	path=os.popen(lpparseExec + LPPoptions + ' ' + options).readline()
	return path.rstrip()

# parametr info - limitations on time and value
class Param:
	def __init__(self):
		self.range=list()
	
	def GetTime(self):
		return self.time
	
	# calculating RelativeDate from 'time' and its 'unit'
	def SetTime(self,time,unit):
		if unit == 'hours':
			self.time=RelativeDate(hours=int(time))	
		elif unit == 'days':
			self.time=RelativeDate(days=int(time))	
		elif unit == 'years':
			self.time=RelativeDate(years=int(time))
		else: # 'months' is default
			self.time=RelativeDate(months=int(time))

	# check if range is OK
	def isRangeOK(self,startDate,endDate):
		for i in self.range: # old ranges
			if not ((startDate <= i[0] and endDate <= i[0]) or 
					(startDate >= i[1] and endDate >= i[1])):
				return False
		return True

# end Param class


class ParamException:
	pass

class ParamsFileException(ParamException):
	pass

class NoParamsException(ParamsFileException):
	def __str__(self):
		print '',_('No params in params.xml.')


class NoEditablesException(ParamsFileException):
	def __str__(self):
		print '',_('No editable parameters in params.xml.')

class BadXMLException(ParamsFileException):
	def __str__(self):
		print '',_('params.xml is not an XML file.')


class ParamRangeException(ParamException):
	pass

class NegativeException(ParamRangeException):
	def __init__(self,param,attribute):
		self.param=param
		self.attribute=attribute

	def __str__(self):
		print '',  self.param, '.',  self.attribute \
				,_('has negative value')

class NoValueException(ParamRangeException):
	def __init__(self,param,attribute):
		self.param=param
		self.attribute=attribute

	def __str__(self):
		print '',  self.param, '.',  self.attribute \
				,_('is not present')
class NonPositiveException(ParamRangeException):
	def __init__(self,param,attribute):
		self.param=param
		self.attribute=attribute

	def __str__(self):
		print '',  self.param, '.',  self.attribute \
				,_('has non-positive value')

class WrongUnitValueException(ParamRangeException):
	def __init__(self,param, okValues):
		self.param=param
		self.okValues=okValues

	def __str__(self):
		print '',_('in'), self.param, _('"unit" attribute is not in'), okValues

class WrongValueException(ParamRangeException):
	def __init__(self,param, attribute):
		self.param=param
		self.attribute=attribute

	def __str__(self):
		print '',  self.param, '.', self.attribute,_('has wrong value')


class WrongRangeException(ParamRangeException):
	def __init__(self,param):
		self.param=param

	def __str__(self):
		print '',_('in') + self.param + _('"min" is greater then "max".') 


class ResourceFileException:
	def __init__(self,file):
		self.file=file

	def __str__(self):
		print '', _('Error while parsing resource file'),': ', self.file

# parsing params.xml in order to get information about editable params and
# their limitations
def readParamsFromFile(xmlFile):
	editableParams=dict()
	extrans = "http://www.praterm.com.pl/SZARP/ipk-extra"

	# finds param with 'editable' element
	class findEditable(ContentHandler):
		def __init__(self):
			self.noParams=True
			self.noEditables=True
		def startElementNS(self, name, qname, attrs):
			(uri, name) = name
			if name == 'param' and uri == 'http://www.praterm.com.pl/SZARP/ipk':
				self.paramName = copy.copy(attrs.getValueByQName('name'))
				try:
					self.paramPrec = copy.copy(attrs.getValueByQName('prec'))
				except KeyError:
					self.paramPrec = 0

				self.noParams=False

			elif name=='editable' and uri == extrans:
				# limitations = editable's attributes
				param=Param()
				try:
					param.min=float(attrs.getValueByQName('min'))
				except ValueError:
					raise WrongValueException(self.paramName,'min')
				except TypeError:
					raise NoValueException(self.paramName,'min')

				try:
					param.max=float(attrs.getValueByQName('max'))
				except ValueError:
					raise WrongValueException(self.paramName,'max')
				except TypeError:
					raise NoValueException(self.paramName,'max')

				try:
					param.prec=int(self.paramPrec)
				except ValueError:
					raise WrongValueException(self.paramName,'time')
				except TypeError:
					raise NoValueException(self.paramName,'prec')

				unitVal=attrs.get('unit',None)
				
				try:
					timeVal=int(attrs.getValueByQName('time'))
				except ValueError:
					raise WrongValueException(self.paramName,'time')

				if param.prec < 0:
					raise NegativeException(self.paramName,'min')
				if param.max < 0:
					raise NegativeException(self.paramName, 'max')
				if timeVal <= 0:
					raise NonPositiveException(self.paramName, 'time')
				okUnitVals=('hours','days','months','years')
				if (not unitVal in okUnitVals) and unitVal!=None:
					raise WrongUnitValueException(self.paramName, okUnitVals)
				if param.max <= param.min :
					raise WrongRangeException(self.paramName)

				param.SetTime(timeVal, unitVal)
				
				
				editableParams[self.paramName]=param
				self.noEditables=False
			else:
				return
		def getFlags(self):
			return (self.noParams,self.noEditables)


	# setting up parser
	parser = make_parser()
	parser.setFeature(feature_namespaces, 1) # use namespaces
	handler = findEditable()
	parser.setContentHandler(handler)

	# parsing
	try:
		parser.parse(xmlFile)
	except SAXException:
		raise BadXMLException

	flags=handler.getFlags()

	if flags[0]: # no params
		raise NoParamsException
	elif flags[1]: # no editables
		raise NoEditablesException

	return editableParams

# how many digt number this is
def digt(number):
	n=10
	d=1
	while number % n < number:
		n*=10
		d+=1
	return d

# getting font from resource file
def getFont(fileName):
	rFile=file(fileName)
	while not re.match('.*szarpfont.*',rFile.readline()):
		pass
	
	t = rFile.readline().strip().split(' ',2)

	if t[0] != 'font_name' or t[1] != '=':
		raise ResourceFileException(rFile)
	
	t = t[2][1:len(t[2])-1]
	return t

# getting ~ dir
def getHome():
	return os.popen('echo ~').readline().strip()


def getVersion():
	vFile=file('/opt/szarp/resources/version.info')
	return vFile.readline().split(' ',2)[0]


	
# generating data for szbwirter
def gen(param, value, beginning, end, file):
	diff=RelativeDateTime(minutes=10) # every 10 minutes
	curTime=beginning
	# change " to \"
	niceParam=param.replace('\\\"','"')
	niceParam=niceParam.replace('"','\\\"')
	# writing to output file
	while curTime<=end:
		record='"'+niceParam.encode('utf-8')+'"' + curTime.strftime(" %Y %m %d %H %M ") + \
		str(value) + '\n'
		file.write(record)
		
		curTime+=diff

# mkdir -p
def mkPath(path):
	p=''
	for i in path.split('/'):
		p+=i+'/'
		if not os.access(p,os.F_OK):
			os.mkdir(p)
