import logging

from pylons import request, response, session, tmpl_context as c
from pylons.controllers.util import abort, redirect_to

from sssweb.lib.base import *
import sys

log = logging.getLogger(__name__)

class AutherrorController(BaseController):

	def __before__(self, action, **params):
		if action != 'index':
			return redirect_to(h.url_for(controller = 'autherror', action = 'index'))
	
	def index(self, id):
		return 'dupa'
		return str(sys.exc_info())

