from sssweb.tests import *

class TestUserController(TestController):

    def test_index(self):
        response = self.app.get(url(controller='User', action='index'))
        # Test response...
