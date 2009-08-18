import logging
import md5

from pylons import request, response, session, tmpl_context as c
from pylons.controllers.util import abort, redirect_to

from sssweb.lib.base import BaseController, render

from sssweb.model import meta
import sssweb.model as model

log = logging.getLogger(__name__)

class RemarksController(BaseController):

	def index(self):
		c.users_q = meta.Session.query(model.Users)
		return render('/remarks_users.mako')

	def user(self, id):
		users_q = meta.Session.query(model.Users)

		c.user = users_q.get(id)
		c.bases_q = [ (p.id, p.prefix) for p in meta.Session.query(model.Prefix) ]
		c.password = ""
		c.user_ids = [ p.prefix.id for p in c.user.base ]
		return render('/remarks_user.mako')

	def save_user(self, id):
		users_q = meta.Session.query(model.Users)

		if id == None:
			user = model.Users()
			user.name = request.params['name']
		else:
			user = users_q.get(id)
		user.real_name = request.params['real_name']
		if len(request.params['password']) > 0:
			m = md5.new()
			m.update(request.params['password'])
			user.password = m.hexdigest()

		if id == None:
			meta.Session.add(user)
			meta.Session.flush()


		bases_q = meta.Session.query(model.Prefix)

		for base in bases_q:
			if request.params.has_key(str(base.id)):
				a = [ q for q in user.base if q.prefix.id == base.id ]
				if not a:
					a = model.UsersPrefixAssociation()
					a.prefix = base
					a.write_access = 1
					user.base.append(a)
			else:
				map(lambda x: meta.Session.delete(x), filter(lambda x: x.prefix.id == base.id, user.base))

		meta.Session.commit()

		return redirect_to(controller='remarks', action='index')

	def add_user(self):
		c.bases_q = [ (p.id, p.prefix) for p in meta.Session.query(model.Prefix) ]

		c.user = model.Users()
		c.user.name = ""
		c.user.id = None
		c.user.real_name = ""
		c.user_ids = []
		c.password  = ""
		return render('/remarks_user.mako')

