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

import urllib2


class HttpResolver(etree.Resolver):
    def resolve(self, url, id, context):
        if url.startswith('http://'):
            return self.resolve_filename(url, context)
        else:
            return super(HttpResolver, self).resolve(url, id, context)

resolver = HttpResolver()
parser = etree.XMLParser()
parser.resolvers.add(resolver)

# load XSLT only once - at module initialization; cannot create
# stylesheet objects because of some threading issues
svg_stylesheet = etree.parse(
                    '/opt/szarp/resources/xsltd/stylesheets/isl2svg.xsl',
                    parser)
defs_stylesheet = etree.parse(
                    '/opt/szarp/resources/xsltd/stylesheets/isl2defs.xsl',
                    parser)

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
                self.timeout = 2.0
                self.got_timeout = False

        def isl_vu(self, context, args):
            if self.got_timeout:
                self.req.log_error('isl_vu: already got timeout - not doing request', apache.APLOG_WARNING)
                return u'----'

            url = args[0]

            try:
                r = urllib2.urlopen(url, timeout=self.timeout)

                if (r.getcode() != 200):
                    self.req.log_error('isl_vu status_code: %d' % r.getcode(), apache.APLOG_WARNING)
                    return u'----'

                doc = etree.parse(r, parser)

                vu = doc.getroot()[0].text

                return vu
            except urllib2.URLError, e:
                self.req.log_error('isl_vu http timeout for url: %s' % url, apache.APLOG_WARNING)
                self.got_timeout = True
            except Exception, e:
                self.req.log_error('isl_vu url: %s get error: %s' % (url, str(e)), apache.APLOG_ERR)

            return u'----'

	def content(self):
		"""
		Create response
		"""
		global svg_stylesheet
		global defs_stylesheet

                ns = etree.FunctionNamespace("http://www.praterm.com.pl/ISL/pxslt")
                ns.prefix = 'pxslt'
                ns['isl_vu'] = self.isl_vu

		fc = util.FieldStorage(self.req)
		req_type = fc.getfirst('type')
		if req_type == 'defs':
			stylesheet = XSLT(defs_stylesheet)
		else:
			stylesheet = XSLT(svg_stylesheet)

		# this should be set in Apache configuration file using
		# PythonOption directive, for example:
		#   PythonOption szarp.paramd.uri "http://localhost:8081/"
		paramd_uri = self.req.get_options()['szarp.paramd.uri']

                fname = 'file:///etc/szarp/default/config' + self.req.parsed_uri[apache.URI_PATH]

		doc = etree.parse(fname, parser)

                r = 'dupa'
                try:
                    r = stylesheet(doc, uri = "'" + paramd_uri + "'")
                except Exception, e:
                    self.req.log_error('content: stylesheet failed %s' % str(e), apache.APLOG_ERR)
                    r = stylesheet.error_log

		self.req.content_type= 'image/svg+xml'
		self.req.send_http_header()
		self.req.write(str(r))

def handler(req):
	"""
	Run handler
	"""
	h = XsltHandler(req)
	h.content()
	return apache.OK

