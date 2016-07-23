#!/usr/bin/python
"""
  SZARP: SCADA software 
  Darek Marcinkiewicz <reksio@newterm.pl>

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

from daemon import runner
import meaner4

class Meaner4App:
	def __init__(self):
		self.stdin_path = "/dev/null"
		self.stdout_path = "/var/log/szarp/meaner4.stdin.log"
		self.stderr_path = "/var/log/szarp/meaner4.stderr.log"
		self.pidfile_path = "/var/run/meaner4.pid"
		self.pidfile_timeout = 5

	def run(self):
		meaner4.go()

if __name__ == "__main__":
	app = runner.DaemonRunner(Meaner4App())
	try:
		app.do_action()
	except runner.DaemonRunnerStopFailureError as ex:
		# if the script was not running, the pid file won't be locked
		# and we don't want the stop action to fail
		if str(ex).find("not locked") < 0:
			raise ex
