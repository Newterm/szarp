from sssweb.tests import *

class TestSsconfController(TestController):

    def test_index(self):
        response = self.app.get(url_for(controller='ssconf'))
        # Test response...
