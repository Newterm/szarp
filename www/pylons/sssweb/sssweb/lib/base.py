"""The base Controller API

Provides the BaseController class for subclassing.
"""
from pylons.controllers import WSGIController
from pylons.controllers.util import abort, redirect
from pylons.i18n import _, ungettext, N_

from pylons import session, url

from pylons.templating import render_mako as render

from sssweb.model.meta import Session

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
            Session.remove()

# Include the '_' function in the public names
__all__ = [__name for __name in locals().keys() if not __name.startswith('_') or __name == '_']

