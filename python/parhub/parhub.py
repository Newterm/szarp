#!/usr/bin/python
"""
  SZARP: SCADA software 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

"""
import zmq
from zmq import devices
from libpar import LibparReader
from daemon import runner
import sys

class ParHub:
	def __init__(self, will_daemonize):
		lpr = LibparReader()
		self.sub_addr = lpr.get("parhub", "sub_addr")
		self.pub_addr = lpr.get("parhub", "pub_addr")
		if (will_daemonize):
			self.stdin_path = "/dev/null"
			self.stdout_path = "/var/log/szarp/parhub.stdin.log"
			self.stderr_path = "/var/log/szarp/parhub.stderr.log"
			self.pidfile_path = "/var/run/parhub.pid"
			self.pidfile_timeout = 5

		self.context = zmq.Context(1)

	def run(self):
		frontend = self.context.socket(zmq.SUB)
		frontend.bind(self.sub_addr)
		frontend.setsockopt(zmq.SUBSCRIBE, "")

		backend = self.context.socket(zmq.PUB)
		backend.bind(self.pub_addr)

		zmq.device(zmq.FORWARDER, frontend, backend)

if __name__ == "__main__":
	try:
		if "--no-daemon" in sys.argv:
			parhub = ParHub(False)
			parhub.run()
		else:
			app = runner.DaemonRunner(ParHub(True))
			app.do_action()
	except Exception as ex:
		# if the script was not running, the pid file won't be locked
		# and we don't want the stop action to fail
		if str(ex).find("not locked") < 0:
			raise ex
