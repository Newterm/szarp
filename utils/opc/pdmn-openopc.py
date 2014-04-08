#!/usr/bin/python

#
#  OPC Client for pythondmn based on OpenOPC project.
#  http://openopc.sourceforge.net/
#  requires python-pyro package 
#
#  Configuration example:
#
#    <device
#	daemon="/opt/szarp/bin/pythondmn"
#	path="/opt/szarp/bin/pdmn-openopc.py"
#	extra:pyro_host="192.168.0.1"
#	extra:opc_server="ArchestrA.FSGateway.2">
#		<unit id="1" type="1" subtype="1" bufsize="1">
#			<param
#				name="Intouch:Generator:Ilość tagów"
#				extra:item="INTOUCH.$SYS$ItemCount"
#				extra:val_op="LSW"
#				...
#				prec="0">
#			</param>
#		</unit>
#	</device>
#
#	extra:pyro_host - addres of windows machine with OpenOPCService
#	extra:opc_server - name of server to send request
#	extra:item - item to read
#	extra:val_op - LSW/MSW (optional)
	

import sys
import Pyro
import time

sys.path.append('/opt/szarp/lib/python')
import OpenOPC

from lxml import etree

nsmap = {'p':'http://www.praterm.com.pl/SZARP/ipk'}

import logging
from logging.handlers import SysLogHandler

logger = logging.getLogger("pdmn-openopc")
hdlr = SysLogHandler(address='/dev/log', facility=SysLogHandler.LOG_DAEMON)
formatter = logging.Formatter('%(filename)s: %(levelname)s: %(message)s')
hdlr.setFormatter(formatter)
logger.addHandler(hdlr)
logger.setLevel(logging.DEBUG)

class IPCdumy:
	def __init__(self):
		pass

	def get_line_number(self):
		return 2

	def get_ipk_path(self):
		return '/opt/szarp/szop/config/params.xml'

	def set_read(self, ind, val):
		if val is None:
			logger.debug("set_read: %d => None", ind)
		else:
			logger.debug("set_read: %d => %d", ind, val)

	def go_parcook(self):
		return

if 'ipc' not in globals():
	global ipc
	ipc = IPCdumy()

class Param:
	def __init__(self, item, ind, name, prec):
		self.item = item
		self.index = ind
		self.name = name
		self.prec_m = 10 ** prec
		self.shift = 0
		self.scale = 1
		self.val_op = None

	def update_value(self, value):
		logger.debug("update_value: %d %s %s", self.index, self.name, str(value))
		if value is None:
			ipc.set_read(self.index, None)
			return

		v = (value * self.scale) + self.shift
		if self.val_op == 'LSW':
			v = v % 65536
		elif self.val_op == 'MSW':
			v = v / 65536

		ipc.set_read(self.index, int(v * self.prec_m))

class OpcDaemon(object):

	def __init__(self, ipk_path, line_number):
		self.ipk_path = ipk_path
		self.ipk_line_number = line_number
		self.tx_pause = 0 # pasue between transactions
		self.group_size = None
		self.data_source = 'hybrid'
		self.sync = 'async'
		self.update_rate = 1000
		self.timeout = 10000
		self.items = dict()
		self.params = list()
		self.pyro_host = None
		self.pyro_port = 7766
		self.opc_host = 'localhost'
		self.pyro_connected = False
		self.com_connected = False

	def configure(self):
		doc = etree.parse(self.ipk_path)

		xp = "/p:params/p:device[%d]" % self.ipk_line_number
		edev = doc.xpath(xp, namespaces=nsmap)[0]

		self.pyro_host = edev.get('{http://www.praterm.com.pl/SZARP/ipk-extra}pyro_host')
		self.pyro_port = edev.get('{http://www.praterm.com.pl/SZARP/ipk-extra}pyro_port', 7766)
		self.opc_server = edev.get('{http://www.praterm.com.pl/SZARP/ipk-extra}opc_server')
			

		for i, ep in enumerate(edev.xpath('p:unit[1]/p:param', namespaces=nsmap)):
			item = ep.get('{http://www.praterm.com.pl/SZARP/ipk-extra}item')
			if not item in self.items:
				self.items[item] = None
			p = Param(item, i, ep.get('name'), int(ep.get('prec')))
			p.scale = float(ep.get('{http://www.praterm.com.pl/SZARP/ipk-extra}scale', 1))
			p.shift = float(ep.get('{http://www.praterm.com.pl/SZARP/ipk-extra}shift', 0))
			self.params.append(p)
	
	def _update(self, row):
		item = row[0]
		status = row[2]
		v = row[1]
		if v is not None:
			v = float(v)

		if status == 'Good':
			self.items[item] = v
		else:
			self.items[item] = None
		

	def do_read(self):
		logger.info('Doing read')
		try:
			if not self.pyro_connected:
				self.opc = OpenOPC.open_client(self.pyro_host, self.pyro_port)
				self.opc.connect(self.opc_server, self.opc_host)
				self.pyro_connected = True
				self.com_connected = True

			if not self.com_connected:
				self.opc.connect(opc_server, opc_host)
				self.com_connected = True

			data = self.opc.read(self.items.keys(),
					group='opcdmn',
					size=self.group_size,
					pause=self.tx_pause,
					source=self.data_source,
					update=self.update_rate,
					timeout=self.timeout,
					sync=self.sync,
					include_error=True)

			for row in data:
				logger.debug(str(row))
				self._update(row)

						
		except OpenOPC.TimeoutError, error_msg:
			logger.error(error_msg[0])
			self.success = False

		except OpenOPC.OPCError, error_msg:
			logger.error(error_msg[0])
			self.success = False

			if self.opc.ping():
				self.com_connected = True
			else:
				self.com_connected = False
		
		except (Pyro.errors.ConnectionClosedError, Pyro.errors.ProtocolError), error_msg:
			logger.error('Gateway Service: %s', error_msg)
			self.success = False
			self.pyro_connected = False

		except TypeError, error_msg:
			logger.error(error_msg[0])
			return

		except IOError:
			logger.exception('IOError')
			opc.close()
			sys.exit(1)
		
		else:
			success = True


	def go_parcook(self):
		logger.info('Sending to parcook')
		for p in self.params:
			p.update_value(self.items[p.item])

		ipc.go_parcook()




def main():
	dmn = OpcDaemon(ipc.get_ipk_path(), ipc.get_line_number())
	dmn.configure()

	while True:
		dmn.do_read()
		dmn.go_parcook()
		time.sleep(10)


if __name__ == '__main__':
	main()
