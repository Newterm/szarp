#!/usr/bin/python
# -*- encoding: utf-8 -*-
#python-psutil

# simple python script to get RAM usage

import sys                                  # exit()
import time                                 # sleep()
import logging                              # logowanie (istotne!)
import psutil
from logging.handlers import SysLogHandler  #

MiB = 1024 * 1024

class RAMReader:
	def update_values(self):
		try:
			memory = psutil.virtual_memory()
			ipc.set_read(0, int(memory.total / MiB) )
			ipc.set_read(1, int(memory.used / MiB) )
			ipc.set_read(2, int(memory.available / MiB) )
		except Exception as err:
			logger.error("failed to fetch data, %s", str(err).splitlines()[0])
			ipc.set_no_data(0)
			ipc.set_no_data(1)
			ipc.set_no_data(2)

		ipc.go_parcook()

def main(argv=None):
	global logger
	logger = logging.getLogger('pdmn-ram.py')
	logger.setLevel(logging.DEBUG)
	handler = logging.handlers.SysLogHandler(address='/dev/log', facility=SysLogHandler.LOG_DAEMON)
	handler.setLevel(logging.WARNING)
	formatter = logging.Formatter('%(filename)s: [%(levelname)s] %(message)s')
	handler.setFormatter(formatter)
	logger.addHandler(handler)

	if 'ipc' not in globals():
		global ipc
		sys.path.append('/opt/szarp/lib/python')
		from test_ipc import TestIPC
		ipc = TestIPC("example-base", "/opt/szarp/example-base/pdmn_ram.py")

	ex = RAMReader()

	while True:
		time.sleep(10)
		ex.update_values()


# end of main()

if __name__ == "__main__":
	sys.exit(main())
