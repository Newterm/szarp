
from webob import Request, Response
from webob import exc
import xmlrpclib
from routes import url_for
class AuthErrorMiddleware(object):
	"""
	Catches XML-RCP exceptions and if it's authorization error, redirects to login page.
	"""
	def __init__(self, app):
		self.app = app
		
	def __call__(self, environ, start_response):
		req = Request(environ)
		try:
			rsp = req.get_response(self.app)
		except  xmlrpclib.Fault, err:
			# faultString is something like: <'__main__.AuthException'>:invalid user or password
			c = err.faultString.split("'",2)[1].partition(".")[2]
			if c == "AuthException":
				rsp = exc.HTTPSeeOther(location = url_for(controller='login', action = 'invalid'))
			else:
				raise err
		return rsp(environ, start_response)


