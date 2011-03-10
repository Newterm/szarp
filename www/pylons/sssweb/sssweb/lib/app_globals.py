"""The application's Globals object"""
from sssweb.model import init_model
import xmlrpclib

class Globals(object):
    """Globals acts as a container for objects available throughout the
    life of the application
    """

    def __init__(self, config):
        """One instance of Globals is created during application
        initialization and is available during requests via the 'g'
        variable
        """

        self.config = config
	self.rpcservice = xmlrpclib.ServerProxy(config['rpcserver'])

