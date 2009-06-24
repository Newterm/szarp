#!/usr/bin/python
"""
  SZARP SCADA
  $Id$
  2009 Pawel Palucha <pawel@praterm.pl>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA


  Common functions for dealing with szarp configurations and times of current data.
"""

import datetime
import glob
import fnmatch
import os
import sys
import time
import xml.sax as sax

SZARPDIR = "/opt/szarp"
PARPATH = SZARPDIR + "/%s/szbase/%s/??????.szb"

def get_prefixes(ignored):
	"""
	@param list of ignored configuration, may contain wildcard characters
	@return list of all non-ignored szarp prefixes
	"""
	ret = []
	for d in glob.glob(SZARPDIR + "/????/config/params.xml"):
        	l = len(SZARPDIR)
	        prefix = d[l+1:l+5]
	        # ommit aggregated configs
	        try:
	                os.stat(SZARPDIR + "/" + prefix + "/config/aggr.xml")
	                continue
	        except OSError:
	                pass
	       	# ommit ignored configs
	        ignore = False
	       	for i in ignored:
	                if fnmatch.fnmatch(prefix, i):
	                        ignore = True
	                        continue
      		if ignore:
	                continue
		ret.append(prefix)
	return ret


# Classes for date manipulation
ZERO = datetime.timedelta(0)

class UTC(datetime.tzinfo):
    """UTC"""

    def utcoffset(self, dt):
        return ZERO

    def tzname(self, dt):
        return "UTC"

    def dst(self, dt):
        return ZERO

# UTC timezone
Utc = UTC()

STDOFFSET = datetime.timedelta(seconds = -time.timezone)
if time.daylight:
    DSTOFFSET = datetime.timedelta(seconds = -time.altzone)
else:
    DSTOFFSET = STDOFFSET

DSTDIFF = DSTOFFSET - STDOFFSET
class LocalTimezone(datetime.tzinfo):

    def utcoffset(self, dt):
        if self._isdst(dt):
            return DSTOFFSET
        else:
            return STDOFFSET

    def dst(self, dt):
        if self._isdst(dt):
            return DSTDIFF
        else:
            return ZERO

    def tzname(self, dt):
        return time.tzname[self._isdst(dt)]

    def _isdst(self, dt):
        tt = (dt.year, dt.month, dt.day,
              dt.hour, dt.minute, dt.second,
              dt.weekday(), 0, -1)
        stamp = time.mktime(tt)
        tt = time.localtime(stamp)
        return tt.tm_isdst > 0

# local timezone
Local = LocalTimezone()

def get_age(d):
	"""
	Returns age (in minutes) of date d.
	"""
	delta = datetime.datetime.now(Local) - d
	return delta.days*60*24 + delta.seconds // 60

def get_lad(prefix, parameter_path):
	"""
	Return date (datetime object) of last data for given parameter, None if parameter is not found;
	only size of szbase file is checked - that should be fine because meaner does not write
	empty values at the end of szbase file.
	@param prefix szarp configuration prefix
	@param parameter_path parameter path (only /opt/szarp/<prefix>/szbase subdirectory part)
	"""

	files = glob.glob(PARPATH % (prefix, parameter_path))
	if len(files) == 0:
		return None
	files.sort(None, None, True)
	path = files[0]
	(tmp, sep, name) = path.rpartition('/')
	name = name[0:6]
	size = os.stat(path).st_size

	d = datetime.datetime(int(name[0:4]), int(name[4:6]), 1, tzinfo = Utc) + datetime.timedelta(minutes = 10*(size//2-1))
	return d.astimezone(Local)

class DeviceHandler(sax.ContentHandler):
	"""
	Sax content handler to scan configuration for all test parameters.
	"""
	def __init__(self):
		self.tests = []
		self.num = 0
		self.name = ""
		self.param = ""

	def startElement(self, name, attrs):
		if name == 'device':
			self.num += 1
			self.name = attrs.get('path', '')
			self.checked = False
		elif name == 'param':
			self.param = attrs.get('name')
		elif name == 'raport' and (attrs.get('title') == u'RAPORT TESTOWY' or attrs.get('filename','') == u'test.rap')  and not self.checked:
			self.tests.append((self.num, self.name, self.param))
			self.checked = True

	def endElement(self, name):
		if name == 'device':
			if not self.checked and not self.param.startswith('fake:'):
				self.tests.append((self.num, self.name, None))

def get_test_params(prefix):
	"""
	Search configuration for test parameters.
	@param prefix szarp configuration prefix
	@return list of tupples (device number, device path, parameter name)
	"""
	dh = DeviceHandler()
	parser = sax.make_parser()
	parser.setContentHandler(dh)
	parser.parse(SZARPDIR + "/" + prefix + "/config/params.xml")
	return dh.tests

class TitleHandler(sax.ContentHandler):
	"""
	Sax handler that scans for SZARP configuration title.
	"""
	def __init__(self):
		self.title = None
	def startElement(self, name, attrs):
		if name == 'params':
			self.title = attrs.get('title')

def get_title(prefix):
	"""
	Returns configuration title for prefix.
	"""
	th = TitleHandler()
	parser = sax.make_parser()
	parser.setContentHandler(th)
	parser.parse(SZARPDIR + "/" + prefix + "/config/params.xml")
	return th.title

def param_to_path(param):
	"""
	Converts unicode parameter name to szbase parameter path (
	/opt/szarp/<prefix>/szbase subdirectory part).
	"""
	path = u''
	prev = None
	for c in param:
		if ((ord(c) in range(ord(u'a'), ord(u'z')+1))
				or (ord(c) in range(ord(u'A'), ord(u'Z')+1))
				or (ord(c) in range(ord(u'0'), ord('9')+1))):
			path += c
		elif c == u'\u0104':
			 path += u'A'
                elif c == u'\u0105':
			 path += u'a'
                elif c == u'\u0106':
			 path += u'C'
                elif c == u'\u0107':
			 path += u'c'
                elif c == u'\u0118':
			 path += u'E'
                elif c == u'\u0119':
			 path += u'e'
                elif c == u'\u0141':
			 path += u'L'
                elif c == u'\u0142':
			 path += u'l'
                elif c == u'\u0143':
			 path += u'N'
                elif c == u'\u0144':
			 path += u'n'
                elif c == u'\u00d3':
			 path += u'O'
                elif c == u'\u00f3':
			 path += u'o'
                elif c == u'\u015a':
			 path += u'S'
                elif c == u'\u015b':
			 path += u's'
                elif c == u'\u0179':
			 path += u'Z'
                elif c == u'\u017a':
			 path += u'z'
                elif c == u'\u017b':
			 path += u'Z'
                elif c == u'\u017c':
			 path += u'z'
		elif c == u':':
			if prev == u':':
				path += u'_'
			else:
				path += u'/'
		else:
			path += u'_'
		prev = c
	return path

