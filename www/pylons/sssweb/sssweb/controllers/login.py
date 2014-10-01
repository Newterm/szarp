import logging
import hashlib

from pylons import app_globals, request, response, session, tmpl_context as c, url
from pylons.controllers.util import abort, redirect

from pylons.decorators import rest

from sssweb.lib.base import *

from sssweb.lib.helpers import send_email

log = logging.getLogger(__name__)

class LoginController(BaseController):

	def __before__(self):
		BaseController.__before__(self)

	def index(self):
		"""
		Show login form. Submits to /login/submit
		"""
		return render('/login.mako')

	def invalid(self):
		"""
		Show login form after incorrect user/password submission.
		"""
		c.error = _("Invalid user/password")
		return render('/login.mako')

        @rest.restrict('POST')
	def submit(self):
		"""
		Verify username and password
		"""
		# Both fields filled?
		form_username = str(request.params.get('username'))
		form_password = str(request.params.get('password'))
		m = hashlib.md5()
		m.update(form_password)
		passhash = m.hexdigest()
		# Get user data from database
		if app_globals.rpcservice.check_admin(form_username, passhash):
			# Mark admin as logged in
			session['user'] = form_username
			session['passhash'] = passhash
			session.save()
			return redirect(url(controller='syncuser', action='index'))

		if app_globals.rpcservice.check_user(form_username, passhash):
			# Mark user as logged in
			session['user'] = form_username
			session['passhash'] = passhash
			session.save()
			return redirect(url.current(controller='account', action='index'))
		return redirect(url.current(action = 'invalid'))

	def logout(self):
		"""
		Logout the user and redirect to login page
		"""
		if 'user' in session:
			del session['user']
			del session['passhash']
			session.save()
			return redirect(url('home'))

	def reminder(self):
		"""
		Password reminder form
		"""
		return render('/remind.mako')

	def remind_submit(self):
		"""
		Password remind request submitted.
		"""
		user = request.params['username']
		(key, email) = app_globals.rpcservice.get_password_link(user)

		if key:
			msg = _("You (or someone else) requested change of forgotten password to SZARP Synchronization Server.\n\nIf you did not request password change, just ignore this e-mail.\nIf you want to reset your SZARP synchronization password, click (or paste to your WWW browser) following link: \n%s\nLink is valid to the end of current day.\n\nSZARP Synchronization Server\n") % (url(controller = 'login', action = 'activate_link', qualified = True, user = user, key = key))
			send_email(email, _("SZARP sync password reset"), msg) 

		# We display this message also for non-existent user names so this page
		# cannot be used to check existing user names.
		c.message = _("Check your e-mail for password activation link. Link is valid to the end of current day.")
                c.controller = 'syncuser'
		c.action = "index"
		return render("/info.mako")

	def activate_link(self):
		"""
		Password activation link.
		"""
		user = request.params['user']
		key = request.params['key']
		(password, email) = app_globals.rpcservice.activate_password_link(user, key)
		if password is False:
			c.message = _("Link is incorrect or outdated!")
		else:
			msg = _("Your password for SZARP Synchronization Server has been reset.\nYour login is '%s', your new password is '%s'.\n\nSZARP Synchronization Server\n") % (user, password)
			send_email(email, _("SZARP sync new password"), msg) 
			c.message = _("Your password has been reset. New password has been sent to you by e-mail.")
		c.action = "index"
		c.controller = 'login'
		return render("/info.mako")

