from sssweb.tests import *

class TestAutherrorController(TestController):

    def test_index(self):
        response = self.app.get(url(controller='autherror', action='index'))
        # Test response...
