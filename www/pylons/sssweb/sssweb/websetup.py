"""Setup the sssweb application"""
import logging

from paste.deploy import appconfig
from pylons import config
from sssweb.model import meta

from sssweb.config.environment import load_environment

log = logging.getLogger(__name__)

def setup_config(command, filename, section, vars):
    """Place any commands to setup sssweb here"""
    conf = appconfig('config:' + filename)
    load_environment(conf.global_conf, conf.local_conf)

    # Create the tables if they don't already exist
    meta.metadata.create_all(bind=meta.engine)
