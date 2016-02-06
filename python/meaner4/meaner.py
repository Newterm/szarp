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
from meanerbase import MeanerBase
from heartbeat import create_hearbeat_param, create_meaner4_heartbeat_param

FIRST_HEART_BEAT = 0
NON_FIRST_HEART_BEAT = 1

class Meaner(MeanerBase):
	def __init__(self, path, uri, heartbeat):
		MeanerBase.__init__(self, path)

		self.hub_uri = uri

		self.context = zmq.Context(1)
		self.socket = self.context.socket(zmq.SUB)

		self.last_heartbeat = None

		self.heartbeat_interval = heartbeat
		self.last_meaner4_heartbeat = None

		self.heartbeat_param = saveparam.SaveParam(create_hearbeat_param(), self.szbase_path)
		self.meaner4_heartbeat_param = saveparam.SaveParam(create_meaner4_heartbeat_param(), self.szbase_path)

	def process_msgs(self, msgs):
		latest_time = None;

		for index, batch in msgs.iteritems():
			time = self.save_params[index].process_msg_batch(batch)

			if latest_time is None or time > latest_time:
				latest_time = time

		return latest_time

	def read_socket(self):
		msgs = {}

		try:
			while True:
				msg = self.socket.recv(zmq.NOBLOCK)

				params_values = paramsvalues_pb2.ParamsValues.FromString(msg)

				for param_value in params_values.param_values:
					log_param, index = self.ipk.adjust_param_index(param_value.param_no)
					if log_param:
						#TODO: we ignore logdmn params (for now)
						continue

					if index in msgs:
						msgs[index].append(param_value)
					else:
						msgs[index] = [param_value]

		except zmq.ZMQError as e:
			if e.errno != zmq.EAGAIN:
				raise

		time = self.process_msgs(msgs)

		if time is not None:
			self.heartbeat(time)

	def heartbeat(self, time):
		if self.last_heartbeat is not None and self.last_heartbeat >= time:
			return

		self.heartbeat_param.process_value(
			FIRST_HEART_BEAT if self.last_heartbeat is None else NON_FIRST_HEART_BEAT,
			time)
		self.last_heartbeat = time

	def meaner4_next_heartbeat(self):
		return max(0, self.heartbeat_interval - (time.time() - self.last_meaner4_heartbeat)) * 1000
		
	def meaner4_hearbeat(self, value):
		value = 0 if self.last_heartbeat is None else 1
		self.last_meaner4_heartbeat = int(time.time())
		self.meaner4_heartbeat_param.process_value(value, self.last_meaner4_heartbeat)

	def loop(self):
		while True:
			next_heartbeat = self.meaner4_next_heartbeat()
			if next_heartbeat == 0:
				self.meaner4_hearbeat(NON_FIRST_HEART_BEAT)
				next_heartbeat = self.meaner4_next_heartbeat()

			ready = dict(self.poller.poll(next_heartbeat))
			if self.socket in ready:
				self.read_socket()

	def run(self):
		self.socket.connect(self.hub_uri)
		self.socket.setsockopt(zmq.SUBSCRIBE, "")
		self.poller = zmq.Poller()
		self.poller.register(self.socket, zmq.POLLIN)

		self.meaner4_hearbeat(FIRST_HEART_BEAT)

		self.loop()
			
