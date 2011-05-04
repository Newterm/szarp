import logging
import hashlib

from pylons import app_globals
from pylons import request, response, session, tmpl_context as c
from pylons.controllers.util import abort, redirect
from pylons.i18n import set_lang
from pylons.decorators import rest

from sssweb.lib.base import *

logging.basicConfig(level=logging.DEBUG)

log = logging.getLogger(__name__)

class SyncuserController(BaseController):
	requires_auth = True	# http://wiki.pylonshq.com/display/pylonscookbook/Simple+Homegrown+Authentication

	def __before__(self):
		BaseController.__before__(self)
		set_lang(app_globals.config['lang'])

        def _get_selected_bases(self, requrest):
            selected = []
            for f in app_globals.rpcservice.get_available_bases(session['user'], session['passhash']):
                try:
                    if request.params[f]:
                        selected.append(f)
                except KeyError, e:
                    pass
            return selected

	def index(self):
            c.list = app_globals.rpcservice.list_users(session['user'], session['passhash'])
            return render('/syncuser/index.mako')

	@rest.dispatch_on(POST='save')
	def new(self):
            c.all_bases = app_globals.rpcservice.get_available_bases(session['user'], session['passhash'])
            c.servers = map(lambda x: x['name'], app_globals.rpcservice.list_servers(session['user'], session['passhash']))
            c.user = dict()
            c.user['name'] = ''
            c.user['email'] = ''
            c.user['expired'] = '-1'
            c.user['hwkey'] = '-1'
            c.user['sync'] = []
            c.user['server'] = c.servers[0]
            c.user['comment'] = ''
            return render('/syncuser/edit.mako')

	@rest.dispatch_on(POST='save')
	def edit(self, id):
		c.user = app_globals.rpcservice.get_user(session['user'], session['passhash'], id)
		c.all_bases = app_globals.rpcservice.get_available_bases(session['user'], session['passhash'])
		c.servers = map(lambda x: x['name'], app_globals.rpcservice.list_servers(session['user'], session['passhash']))
		return render('/syncuser/edit.mako')

	@rest.restrict('POST')
	def save(self, id=None):
		log.debug('save: id %s params %s', id, str(request.params))
		user = dict()
		if id is None:
			user['name'] = request.params['name']
		else:
			user['name'] = id

		for f in ('email', 'server', 'hwkey', 'comment'):
			user[f] = request.params[f]

		user['sync'] = self._get_selected_bases(request)

		if request.params['exp'] == 'date':
			user['expired'] = request.params['expired']
		else:
			user['expired'] = '-1'
		if id is None:
			try:
				password = app_globals.rpcservice.add_user(session['user'], session['passhash'], user)
				msg = _("Your SZARP Synchronization account has been created.\n\nYour login: %s\nYour password: %s\nVisist %s to change your password and view your settings.\n\nSZARP Synchronization Server\n") % (user['name'], password, h.url_for(controller = '/', qualified = True))
				h.send_email(user['email'], _("SZARP sync new account"), msg)
			except Exception, e:
				log.error(str(e))
				raise e
			return redirect(url(controller='syncuser', action='edit', id=user['name']))
		else:
			app_globals.rpcservice.set_user(session['user'], session['passhash'], user)
		return redirect(url(controller='syncuser', action='index'))

	def reset_password(self, **kwargs):
		"""
		Reset password
		"""
		c.id = kwargs.get('id')
		c.message = "Are you sure you want to reset password for user %s?" % c.id
		c.action = "reset_password_confirmed"
		c.no_action = "index"
		return render('/confirm.mako')

	@rest.restrict('POST')
	def reset_password_confirmed(self, id):
		"""
		Reset user password and send e-mail with password.
		"""
		password = app_globals.rpcservice.reset_password(session['user'], session['passhash'], id)
		user = app_globals.rpcservice.get_user(session['user'], session['passhash'], id)
		msg = _("Your password for SZARP Synchronization Server has been reset by administrator.\nYour login is '%s', your new password is '%s'.\n\nSZARP Synchronization Server\n") % (user['name'], password)
		h.send_email(user['email'], _("SZARP sync new password"), msg)
                
		c.message = "New password has been sent to user %s" % id
		c.action = 'edit'
		return render('/info.mako')

	def delete(self, **kwargs):
		if not kwargs.has_key('id'):
			return redirect(url(controller='syncuser',action='index'))
		c.id = kwargs.get('id')
		c.message = "Are you sure you want to remove user %s?" % c.id
		c.action = "delete_confirmed"
		c.no_action = "index"
		return render('/confirm.mako')

	@rest.restrict('POST')
	def delete_confirmed(self, id):
		app_globals.rpcservice.remove_user(session['user'], session['passhash'], id)
		return redirect(url(controller='syncuser', action='index'))

	def disable_key(self, id):
		app_globals.rpcservice.disable_key(session['user'], session['passhash'], id)
		return redirect(h.url_for(action='edit'))

	def reset_key(self, id):
		app_globals.rpcservice.reset_key(session['user'], session['passhash'], id)
		return redirect(h.url_for(action='edit'))

	def turnoff_key(self, id):
		app_globals.rpcservice.turnoff_key(session['user'], session['passhash'], id)
		return redirect(h.url_for(action='edit'))

