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
from heartbeat import create_hearbeat_param

FIRST_HEART_BEAT = 0
NON_FIRST_HEART_BEAT = 1

class Meaner(MeanerBase):
	def __init__(self, path, uri, heartbeat):
		MeanerBase.__init__(self, path)

		self.parcook_uri = uri

		self.context = zmq.Context(1)
		self.socket = self.context.socket(zmq.SUB)
		self.heartbeat_interval = heartbeat
		self.last_heartbeat = None

		p = create_hearbeat_param()
		self.heartbeat_param = saveparam.SaveParam(p, self.szbase_path)

	def read_socket(self):
		try:
			while True:
				msg = self.socket.recv(zmq.NOBLOCK)

				params_values = paramsvalues_pb2.ParamsValues()
				params_values.ParseFromString(msg)

				for param_value in params_values.param_values:
					log_param, index = self.ipk.adjust_param_index(param_value.param_no)
					if log_param:
						#TODO: we ignore logdmn params (for now)
						continue
					self.save_params[index].process_msg(param_value)
		except zmq.ZMQError as e:
			if e.errno != zmq.EAGAIN:
				raise

	def time_for_hearbeat(self):
		return max(0, self.heartbeat_interval - (time.time() - self.last_heartbeat)) * 1000
		
	def hearbeat(self, value):
		value = 0 if self.last_heartbeat is None else 1
		self.last_heartbeat = time.time()
		self.heartbeat_param.process_value(value, self.last_heartbeat)

	def loop(self):
		while True:
			ready = dict(self.poller.poll(self.time_for_hearbeat()))
			if self.socket in ready:
				self.read_socket()
			else:
				self.hearbeat(NON_FIRST_HEART_BEAT)

	def run(self):
		self.socket.connect(self.parcook_uri)
		self.socket.setsockopt(zmq.SUBSCRIBE, "")
		self.poller = zmq.Poller()
		self.poller.register(self.socket, zmq.POLLIN)

		self.hearbeat(FIRST_HEART_BEAT)

		self.loop()
			
