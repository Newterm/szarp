from sssweb.tests import *

class TestLoginController(TestController):

    def test_index(self):
        response = self.app.get(url_for(controller='Login'))
        # Test response...
