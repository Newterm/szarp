"""The base Controller API

Provides the BaseController class for subclassing, and other objects
utilized by Controllers.
"""
from pylons import tmpl_context as c
from pylons import app_globals
from pylons import cache, request, response, session, url

from pylons.controllers import WSGIController
from pylons.controllers.util import abort, redirect
from pylons.decorators import jsonify, validate
from pylons.i18n import _, ungettext, N_

from pylons.templating import render_mako as render

import sssweb.lib.helpers as h
import sssweb.model as model
from sssweb.model import meta

class BaseController(WSGIController):
    requires_auth = False

    def __before__(self):
        # Authentication required?
	if self.requires_auth and 'user' not in session:
		return redirect(url(controller='login', qualified = True))

    def __call__(self, environ, start_response):
        """Invoke the Controller"""
        # WSGIController.__call__ dispatches to the Controller method
        # the request is routed to. This routing information is
        # available in environ['pylons.routes_dict']
        try:
            return WSGIController.__call__(self, environ, start_response)
        finally:
            meta.Session.remove()

# Include the '_' function in the public names
__all__ = [__name for __name in locals().keys() if not __name.startswith('_') \
           or __name == '_']
