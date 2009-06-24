import logging

from sssweb.lib.base import *
from pylons import config

log = logging.getLogger(__name__)

class SsconfController(BaseController):
	requires_auth = True	# http://wiki.pylonshq.com/display/pylonscookbook/Simple+Homegrown+Authentication
	
	def index(self):
		c.list = g.rpcservice.list_users(session['user'], session['passhash'])
		return render('/index.mako')
	
	def servers(self):
		c.list = g.rpcservice.list_servers(session['user'], session['passhash'])
		return render('/servers.mako')

	def server(self, id):
		c.server = g.rpcservice.get_server(session['user'], session['passhash'], id)
		return render('/server.mako')

	def save_server(self, id):
		server = dict()
		# for adding new user
		if id is None:
			server['name'] = request.params['name']
		else:
			server['name'] = id
		server['ip'] = request.params['ip']
		if id is None:
			g.rpcservice.add_server(session['user'], session['passhash'], server)
		else:
			g.rpcservice.set_server(session['user'], session['passhash'], server)
		return h.redirect_to(action='servers')

	def add_server(self, id):
		c.server = dict()
		c.server['name'] = ''
		c.server['ip'] = ''
		c.server['main'] = False
		return render('/server.mako')

	def remove_server(self, id):
		c.id = id
		c.message = "Are you sure you want to remove server %s?" % c.id
		c.action = "remove_server_confirmed"
		c.no_action = "server"
		return render('/confirm.mako')

	def remove_server_confirmed(self, id):
		g.rpcservice.remove_server(session['user'], session['passhash'], id)
		return h.redirect_to(action='servers')

	def user(self, id):
		c.user = g.rpcservice.get_user(session['user'], session['passhash'], id)
		c.all_bases = g.rpcservice.get_available_bases(session['user'], session['passhash'])
		c.servers = map(lambda x: x['name'], g.rpcservice.list_servers(session['user'], session['passhash']))
		return render('/user.mako')
	
	def save_user(self, id):
		user = dict()
		# for adding new user
		if id is None:
			user['name'] = request.params['name']
		else:
			user['name'] = id
		for f in ('email', 'server', 'hwkey', 'comment'):
			user[f] = request.params[f]
		user['sync'] = []
		for f in g.rpcservice.get_available_bases(session['user'], session['passhash']):
			try:
				if request.params[f]:
					user['sync'].append(f)
			except KeyError, e:
				pass

		if request.params['exp'] == 'date':
			user['expired'] = request.params['expired']
		else:
			user['expired'] = '-1'
		if id is None:
			password = g.rpcservice.add_user(session['user'], session['passhash'], user)
			msg = _("Your SZARP Synchronization account has been created.\n\nYour login: %s\nYour password: %s\nVisist %s to change your password and view your settings.\n\nSZARP Synchronization Server\n") % (user['name'], password, h.url_for(controller = '/', qualified = True))
			h.send_email(user['email'], _("SZARP sync new account"), msg) 
		else:
			g.rpcservice.set_user(session['user'], session['passhash'], user)
		return h.redirect_to(action='index')
		
	def add_user(self):
		c.all_bases = g.rpcservice.get_available_bases(session['user'], session['passhash'])
		c.servers = map(lambda x: x['name'], g.rpcservice.list_servers(session['user'], session['passhash']))
		c.user = dict()
		c.user['name'] = ''
		c.user['email'] = ''
		c.user['expired'] = '-1'
		c.user['hwkey'] = '-1'
		c.user['sync'] = []
		c.user['server'] = c.servers[0]
		c.user['comment'] = ''
		return render('/user.mako')

	def reset_password(self, id):
		"""
		Reset password
		"""
		c.id = id
		c.message = "Are you sure you want to reset password for user %s?" % c.id
		c.action = "reset_password_confirmed"
		c.no_action = "user"
		return render('/confirm.mako')

	def reset_password_confirmed(self, id):
		"""
		Reset user password and send e-mail with password.
		"""
		password = g.rpcservice.reset_password(session['user'], session['passhash'], id)
		user = g.rpcservice.get_user(session['user'], session['passhash'], id)
		msg = _("Your password for SZARP Synchronization Server has been reset by administrator.\nYour login is '%s', your new password is '%s'.\n\nSZARP Synchronization Server\n") % (user['name'], password)
		h.send_email(user['email'], _("SZARP sync new password"), msg) 

		c.message = "New password has been sent to user %s" % c.id
		c.action = 'user'
		c.id = id
		return render('/info.mako')

			
	def remove_user(self, id):
		c.id = id
		c.message = "Are you sure you want to remove user %s?" % c.id
		c.action = "remove_user_confirmed"
		c.no_action = "user"
		return render('/confirm.mako')
	
	def remove_user_confirmed(self, id):
		g.rpcservice.remove_user(session['user'], session['passhash'], id)
		return h.redirect_to(action='index')

	def disable_key(self, id):
		g.rpcservice.disable_key(session['user'], session['passhash'], id)
		return self.user(id)

	def reset_key(self, id):
		g.rpcservice.reset_key(session['user'], session['passhash'], id)
		return self.user(id)

	def turnoff_key(self, id):
		g.rpcservice.turnoff_key(session['user'], session['passhash'], id)
		return self.user(id)

