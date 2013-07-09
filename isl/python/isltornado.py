#!/usr/bin/python

import logging
import tornado.auth
import tornado.escape
import tornado.ioloop
import tornado.web
import os.path

import time

from tornado import websocket
from tornado.httpclient import HTTPClient, HTTPRequest, HTTPError
from tornado import gen
from tornado.options import define, options, parse_command_line

from lxml import etree
from lxml.etree import XSLT

logger = logging.getLogger('IslTornado')
logger.setLevel(logging.DEBUG)

define("port", default=8088, help="run on the given port", type=int)

class HttpResolver(etree.Resolver):
    def resolve(self, url, id, context):
        if url.startswith('http://'):
            return self.resolve_filename(url, context)
        else:
            return super(HttpResolver, self).resolve(url, id, context)

class MainHandler(tornado.web.RequestHandler):
    def initialize(self):
        self.asdf = time.time()
        logger.info('IslHandler: doing initialize')

    def get(self):
        logger.info('IslHandler: %f', self.asdf)
        self.write("You are now logged out")

class IslHandler(tornado.web.RequestHandler):

    def prepare(self):
        self.parser = self.application.parser
        ns = etree.FunctionNamespace("http://www.praterm.com.pl/ISL/pxslt")
        ns.prefix = 'pxslt'
        ns['isl_vu'] = self.isl_vu

        self.http_client = HTTPClient()


    def _isl_request(self, isl_file):
        fname =  os.path.join(self.application.isl_dir, isl_file)
        if not os.path.exists(fname):
            logger.warn('IslHandler.get: no such file %s', fname)
            self.set_status(404)
            self.write('Nie ma takiego pliku ' + isl_file)
            return

        try:
            self.got_timeout = False
            doc = etree.parse(fname, self.application.parser)

            r = self.stylesheet(doc, uri = "'" + self.application.paramd_uri + "'")
            self.write(str(r))
        except Exception, e:
            logger.exception('_isl_request: : ')
            self.write('Errors: ' + str(stylesheet.error_log))

    def post(self, isl_file):
        self.stylesheet = XSLT(self.application.defs_stylesheet)
        self.set_header('content-type', 'text/json')
        self._isl_request(isl_file)


    def get(self, isl_file):
        _type = self.get_argument('type', None)
        if _type is not None:
            self.post(isl_file)
            return
        self.stylesheet = XSLT(self.application.svg_stylesheet)
        self.set_header('content-type', 'image/svg+xml')
        self._isl_request(isl_file)


    def isl_vu(self, context, args):
        if self.got_timeout:
            logger.info('isl_vu: already got timeout - not doing request')
            return u'----'

        try:
            url = args[0].replace('http://localhost:8081', 'http://10.99.1.58:8083')

            req = HTTPRequest(url, connect_timeout=2.0)

            response = self.http_client.fetch(req)

            doc = etree.parse(response.buffer, self.parser)
            vu = doc.getroot()[0].text

            logger.debug('isl_vu: %s', vu)
            return vu
        except HTTPError, e:
            self.got_timeout = True
            logger.warn('Got timeout')

        except Exception, e:
            logger.exception('isl_vu exception: ')

        return u'----'

class EchoWebSocket(websocket.WebSocketHandler):
    def open(self):
       logger.info("WebSocket opened")

    def on_message(self, message):
        logger.info('EchoWebSocket: on_message: %s', message)
        self.write_message(u"You said: " + message)

    def on_close(self):
        logger.info("WebSocket closed")


class IslTornado(tornado.web.Application):

    def __init__(self, **kwargs):
        shared_path = os.path.abspath(os.path.dirname(__file__))
        logger.info('path: %s' % shared_path)

        handlers = [
            (r"/", MainHandler),
            (r'/shared/(.*)', tornado.web.StaticFileHandler, { 'path': shared_path}),
            (r"/isl/(?P<isl_file>.*\.isl)", IslHandler),
            (r"/ws/", EchoWebSocket),
        ]


        tornado.web.Application.__init__(self, handlers, **kwargs)

        self.paramd_uri = 'http://glw5:8083/'

        resolver = HttpResolver()
        self.parser = etree.XMLParser()
        self.parser.resolvers.add(resolver)

        self.isl_dir = os.path.join('/opt/szarp/', 'jgor', 'config', 'isl')

        # load XSLT only once - at module initialization; cannot create
        # stylesheet objects because of some threading issues
        self.svg_stylesheet = etree.parse('./isl2svg.xsl', self.parser)
        self.defs_stylesheet = etree.parse('./isl2defs2.xsl', self.parser)


def main():
    parse_command_line()
    app = IslTornado(debug=True)
    app.listen(options.port)
    tornado.ioloop.IOLoop.instance().start()


if __name__ == "__main__":
    main()
