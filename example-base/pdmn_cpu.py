#!/usr/bin/python
# -*- encoding: utf-8 -*-
# script to monitor system resources usage
# shows cores usage, running processes and load
# cpu_usage len is different on each processor so remember to change accordingly!

import sys                                  # exit()
import time                                 # sleep()
import logging                              # logowanie (istotne!)
from logging.handlers import SysLogHandler  #
sys.path.append("/opt/szarp/lib/python")    #
from pydaemontimer import ReTimer           # opcjonalnie zamiast sleepa

import psutil
import os
import subprocess
from subprocess import Popen, PIPE

class CPUReader:
	def get_running_processes(self):
		p = subprocess.Popen("ps ux | wc -l", shell=True,stdout=PIPE)
		if p.wait():
			raise Exception("failed to get running processes")
		return int(p.communicate()[0].strip())

	def update_values(self):
		try:
			rp = self.get_running_processes()
			cpu_usage = psutil.cpu_percent(percpu=True)
			ipc.set_read_sz4_int(0, rp)
			ipc.set_read_sz4_float(1, cpu_usage[0] )
			ipc.set_read_sz4_float(2, cpu_usage[1] )
			ipc.set_read_sz4_float(3, cpu_usage[2] )
			ipc.set_read_sz4_float(4, cpu_usage[3] )
			ipc.set_read_sz4_double(5, os.getloadavg()[0] )
		except Exception as err:
			logger.error("failed to fetch data, %s", str(err).splitlines()[0])
			ipc.set_no_data(0)
			ipc.set_no_data(1)
			ipc.set_no_data(2)
			ipc.set_no_data(3)
			ipc.set_no_data(4)
			ipc.set_no_data(5)

def main(argv=None):
	psutil.cpu_percent(percpu=True)
	global logger
	logger = logging.getLogger('pdmn-cpu.py')
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
		ipc = TestIPC("example-base", "/opt/szarp/example-base/pdmn_cpu.py")

	ex = CPUReader()

	#use sz4 interface
	ipc.autopublish_sz4()
	ipc.force_sz4()

	while not ipc.sz4_ready():
		time.sleep(1)

	while True:
		time.sleep(10)
		ex.update_values()


# end of main()

if __name__ == "__main__":
	sys.exit(main())
