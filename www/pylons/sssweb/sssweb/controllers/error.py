import cgi
import os.path
import sys

from paste.urlparser import StaticURLParser
from pylons.middleware import error_document_template, media_path

from sssweb.lib.base import *

class ErrorController(BaseController):
    """Generates error documents as and when they are required.

    The ErrorDocuments middleware forwards to ErrorController when error
    related status codes are returned from the application.

    This behaviour can be altered by changing the parameters to the
    ErrorDocuments middleware in your config/middleware.py file.
    
    """
    def document(self):
        """Render the error document"""
	c.error = _("An error has occured. Please inform server administrator.")
        return render('/info.mako')


