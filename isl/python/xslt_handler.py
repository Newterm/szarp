"""
  Apache mod_python handler for serving ISL documents - replacing buggy
  mod_xslt.

  SZARP: SCADA software 
  Copyright (C) 2007  PRATERM S.A.
  Pawel Palucha <pawel@praterm.com.pl>

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
"""
from mod_python import apache
from mod_python import util
from lxml import etree
from lxml.etree import XSLT
from StringIO import StringIO

# load XSLT only once - at module initialization; cannot create
# stylesheet objects because of some threading issues
svg_stylesheet = etree.parse(
		'/opt/szarp/resources/xsltd/stylesheets/isl2svg.xsl')
defs_stylesheet = etree.parse(
		'/opt/szarp/resources/xsltd/stylesheets/isl2defs.xsl')

class XsltHandler:
	"""
	Request handler - checks for 'type' and serves document using
	proper stylesheet.
	"""
	def __init__(self, req):
		"""
		Initialization - save request object
		"""
		self.req = req

	def content(self):
		"""
		Create response
		"""
		global svg_stylesheet
		global defs_stylesheet
		
		fc = util.FieldStorage(self.req)
		type = fc.getfirst('type')
		if type == 'defs':
			stylesheet = XSLT(defs_stylesheet)
		else:
			stylesheet = XSLT(svg_stylesheet)
		# this should be set in Apache configuration file using
		# PythonOption directive, for example:
		#   PythonOption szarp.paramd.uri "http://localhost:8081/"
		paramd_uri = self.req.get_options()['szarp.paramd.uri']

		fname = '/etc/szarp/default/config' + self.req.parsed_uri[apache.URI_PATH]

		# Following code should look like:
		#  doc = etree.parse(fname)
		# but there's a bug and parsed files are never closed. This bug
		# is fixed in lxml 1.1.2 but there's no packages of that version
		# available for Debian stable.
		fd = file(fname, "r")
		doc = etree.parse(StringIO(fd.read()))
		fd.close()
		
		self.req.content_type= 'image/svg+xml'
		self.req.send_http_header()
		self.req.write(str(stylesheet(doc, uri = "'" + paramd_uri + "'")))

def handler(req):
	"""
	Run handler
	"""
	h = XsltHandler(req)
	h.content()
	return apache.OK
			
