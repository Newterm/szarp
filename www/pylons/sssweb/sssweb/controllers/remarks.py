import logging
import hashlib

from pylons import request, response, session, url, tmpl_context as c
from pylons.controllers.util import abort, redirect
from pylons.decorators import rest

from sssweb.lib.base import BaseController, render

from sssweb.model import meta
import sssweb.model as model

log = logging.getLogger(__name__)

class RemarksController(BaseController):

	def index(self):
		c.users_q = meta.Session.query(model.Users).order_by('real_name')
		return render('/remarks/index.mako')

        @rest.dispatch_on(POST='save')
	def new(self):
		c.bases_q = [ (p.id, p.prefix) for p in meta.Session.query(model.Prefix) ]
		c.user = model.Users()
		c.user.name = ""
#		c.user.id = None
		c.user.real_name = ""
		c.user_ids = []
		c.password  = ""
		return render('/remarks/edit.mako')

        @rest.dispatch_on(POST='save')
	def edit(self, id):
		c.user = meta.Session.query(model.Users).get(id)
		c.bases_q = [ (p.id, p.prefix) for p in meta.Session.query(model.Prefix) ]
		c.password = ""
		c.user_ids = [ p.prefix.id for p in c.user.base ]
		return render('/remarks/edit.mako')

        @rest.restrict('POST')
	def save(self, id=None):
                log.info('id: %s', id)
                user_id = id
		users_q = meta.Session.query(model.Users)

		if user_id is None:
			user = model.Users()
			user.name = request.params['name']
		else:
			user = users_q.get(user_id)
		user.real_name = request.params['real_name']
		if len(request.params['password']) > 0:
			m = hashlib.md5()
			m.update(request.params['password'])
			user.password = m.hexdigest()

		if user_id is None:
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

		return redirect(url(controller='remarks', action='index'))

        def delete(self, id):
                user = meta.Session.query(model.Users).get(id)
                meta.Session.query(model.UsersPrefixAssociation).filter_by(user_id=user.id).delete()
                meta.Session.delete(user)
                meta.Session.commit()
                return redirect(url(controller='remarks', action='index'))

