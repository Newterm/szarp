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

import zmq
import time
import lxml
import lxml.etree
import paramsvalues_pb2
import param
import saveparam
import logging
from logging.handlers import SysLogHandler
import sys
import signal
from meanerbase import MeanerBase

FIRST_HEART_BEAT = 0
NON_FIRST_HEART_BEAT = 1
force_interrupt = False

class Meaner(MeanerBase):
	def __init__(self, path, uri, heartbeat, interval):
		MeanerBase.__init__(self, path)

		self.hub_uri = uri

		self.context = zmq.Context(1)
		self.socket = self.context.socket(zmq.SUB)

		self.last_heartbeat = None

		self.heartbeat_interval = heartbeat
		self.last_meaner4_heartbeat = None

		self.saving_interval = interval
		self.saving_time = 0

		self.msgs = {}

	def process_msgs(self):
		latest_time = None;

		if self.msgs:
			for index, batch in self.msgs.iteritems():
				saving_time = self.save_params[index].process_msg_batch(batch)

				if latest_time is None or saving_time > latest_time:
					latest_time = saving_time
		self.msgs = {}
		return latest_time

	def read_socket(self):
		try:
			while True:
					msg = self.socket.recv(zmq.NOBLOCK)

					params_values = paramsvalues_pb2.ParamsValues.FromString(msg)

					for param_value in params_values.param_values:
						index = param_value.param_no
						if index in self.msgs:
							self.msgs[index].append(param_value)
						else:
							self.msgs[index] = [param_value]

		except zmq.ZMQError as e:
			if e.errno != zmq.EAGAIN:
				raise

		current_time = int(time.mktime(time.gmtime()))
		if current_time - self.saving_time > self.saving_interval:
			saving_time = self.save_msgs()
			if saving_time is not None:
				self.heartbeat(saving_time)
			self.saving_time = current_time

	def save_msgs(self):
		return self.process_msgs()

	def heartbeat(self, time):
		if self.last_heartbeat is not None and self.last_heartbeat >= time:
			return
		self.last_heartbeat = time

	def meaner4_next_heartbeat(self):
		return max(0, self.heartbeat_interval - (time.time() - self.last_meaner4_heartbeat)) * 1000

	def meaner4_hearbeat(self, value):
		value = 0 if self.last_heartbeat is None else 1
		self.last_meaner4_heartbeat = int(time.time())

	def loop(self):
		while not force_interrupt:
			next_heartbeat = self.meaner4_next_heartbeat()
			if next_heartbeat == 0:
				self.meaner4_hearbeat(NON_FIRST_HEART_BEAT)
				next_heartbeat = self.meaner4_next_heartbeat()

			try:
				ready = dict(self.poller.poll(next_heartbeat))
			except zmq.ZMQError as e:
				pass

			if self.socket in ready:
				self.read_socket()

	def signal_handler(self, signal, frame):
		self.logger.info("interrupt signal " + str(signal) + " caught, cleaning up")
		global force_interrupt
		force_interrupt = True

	def on_exit(self):
		self.save_msgs()
		self.socket.close()
		self.context.term()
		self.logger.info("cleanup finished, exiting")
		sys.exit(0)

	def set_logger(self):
		self.logger = logging.getLogger("meaner4")
		hdlr = SysLogHandler(address='/dev/log', facility=SysLogHandler.LOG_DAEMON)
		formatter = logging.Formatter('%(filename)s: meaner4: %(message)s')
		hdlr.setFormatter(formatter)
		self.logger.addHandler(hdlr)
		self.logger.setLevel(logging.INFO)

	def run(self):
		signal.signal(signal.SIGTERM, self.signal_handler)
		signal.signal(signal.SIGINT, self.signal_handler)

		self.set_logger()
		self.socket.connect(self.hub_uri)
		self.socket.setsockopt(zmq.SUBSCRIBE, "")
		self.poller = zmq.Poller()
		self.poller.register(self.socket, zmq.POLLIN)

		self.meaner4_hearbeat(FIRST_HEART_BEAT)

		self.loop()
		self.on_exit()
