import logging
import hashlib

from pylons import request, response, session, tmpl_context as c
from pylons.controllers.util import abort, redirect_to
from pylons.i18n import set_lang
from pylons import config
from sssweb.lib.base import *

log = logging.getLogger(__name__)

class UserController(BaseController):
	requires_auth = True	# http://wiki.pylonshq.com/display/pylonscookbook/Simple+Homegrown+Authentication
	
	def __before__(self):
		BaseController.__before__(self)
		set_lang(config['lang'])

	def index(self):
		# Return a rendered template
		c.user = g.rpcservice.user_get_info(session['user'], session['passhash'])
		return render('/u_user.mako')

	def password(self):
		return render('/u_password.mako')

	def change_password(self):
		c.action = 'index'
		cur = request.params['password']
		m = hashlib.md5()
		m.update(cur)
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
		g.rpcservice.user_change_password(session['user'], session['passhash'], new)
		session['passhash'] = new
		session.save()
		c.message = _("Password set succesfully")
		return render('/info.mako')

