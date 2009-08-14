from sssweb.tests import *

class TestRemarksController(TestController):

    def test_index(self):
        response = self.app.get(url(controller='remarks', action='index'))
        # Test response...
