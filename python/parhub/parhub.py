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
from libpar import LibparReader
from daemon import runner

class ParHub:
	def __init__(self):
		lpr = LibparReader()
		self.sub_addr = lpr.get("parhub", "sub_addr")
		self.pub_addr = lpr.get("parhub", "pub_addr")

		self.stdin_path = "/dev/null"
		self.stdout_path = "/var/log/szarp/parhub.stdin.log"
		self.stderr_path = "/var/log/szarp/parbub.stderr.log"
		self.pidfile_path = "/var/run/parhub.pid"
		self.pidfile_timeout = 5

	def run(self):
		dev = zmq.devices.Device(zmq.FORWARDER, zmq.SUB, zmq.PUB)

		dev.bind_in(self.sub_addr)
		dev.setsockopt_in(zmq.SUBSCRIBE, "")

		dev.bind_out(self.pub_addr)

		dev.start()

if __name__ == "__main__":
	runner = runner.DaemonRunner(ParHub())	
	runner.do_action()

