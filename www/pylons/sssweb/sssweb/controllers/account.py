import logging
import hashlib

from pylons import app_globals
from pylons import request, response, session, tmpl_context as c
from pylons.controllers.util import abort, redirect
from pylons.decorators import rest

from pylons.i18n import set_lang
from sssweb.lib.base import *

log = logging.getLogger(__name__)

class AccountController(BaseController):
	requires_auth = True	# http://wiki.pylonshq.com/display/pylonscookbook/Simple+Homegrown+Authentication

	def __before__(self):
		BaseController.__before__(self)
		set_lang(app_globals.config['lang'])

	def index(self):
		# Return a rendered template
		c.user = app_globals.rpcservice.user_get_info(session['user'], session['passhash'])
		return render('/account/index.mako')

        @rest.dispatch_on(POST='change_password')
	def password(self):
		return render('/account/password.mako')

        @rest.restrict('POST')
	def change_password(self):
		c.action = 'index'
		m = hashlib.md5()
		m.update(request.params['password'])
		if m.hexdigest() != session['passhash']:
			c.message = _("Incorrect current password!")
			return render('/info.mako')
		new = request.params['new_password']
		new2 = request.params['new_password2']
		if new != new2:
			c.message = _("Passwords do not match!")
			return render('/info.mako')
		bad_password = h.check_password(session['user'], new)
		if bad_password is not None:
			c.message = bad_password
			return render('/info.mako')
		m = hashlib.md5()
		m.update(new)
		new = m.hexdigest()
		app_globals.rpcservice.user_change_password(session['user'], session['passhash'], new)
		session['passhash'] = new
		session.save()
		c.message = _("Password set succesfully")
		return render('/info.mako')

