import logging

from pylons import app_globals
from pylons import request, response, session, tmpl_context as c
from pylons.decorators import rest

from sssweb.lib.base import *

log = logging.getLogger(__name__)

class SyncserverController(BaseController):
    requires_auth = True	# http://wiki.pylonshq.com/display/pylonscookbook/Simple+Homegrown+Authentication

    def index(self):
        c.list = app_globals.rpcservice.list_servers(session['user'], session['passhash'])
        return render('/syncserver/index.mako')
    
    @rest.dispatch_on(POST='save')
    def edit(self, id):
        log.warn('edit')
        c.server = app_globals.rpcservice.get_server(session['user'], session['passhash'], id)
        return render('/syncserver/edit.mako')

    @rest.restrict('POST')
    def save(self, id):
        log.warn('save')
        server = dict()
        server['name'] = id
	server['ip'] = request.params['ip']
        app_globals.rpcservice.set_server(session['user'], session['passhash'], server)
        log.warn('save doing redirect')
        return redirect(url(controller='syncserver', action='index'))
    
    @rest.dispatch_on(POST='create')
    def new(self):
        log.warn('new')
        c.server = dict()
        c.server['name'] = ''
        c.server['ip'] = ''
        c.server['main'] = False
        return render('/syncserver/edit.mako')
    
    @rest.restrict('POST')
    def create(self):
        log.warn('create')
        server = dict()
        server['name'] = request.params['name']
	server['ip'] = request.params['ip']
        log.warn('request: %s' % str(request.params))
        try:
            app_globals.rpcservice.add_server(session['user'], session['passhash'], server)
        except Exception, e:
            log.error(str(e))
        log.warn('create doing redirect')
        return redirect(url(controller='syncserver', action='index'))
    
    def delete(self, id):
        c.id = id
        c.message = "Are you sure you want to remove server %s?" % c.id
        c.action = "delete_confirmed"
        c.no_action = "index"
        return render('/confirm.mako')

    @rest.restrict('POST')
    def delete_confirmed(self, id):
        app_globals.rpcservice.remove_server(session['user'], session['passhash'], id)
        return redirect(url(controller='syncserver', action='index'))

