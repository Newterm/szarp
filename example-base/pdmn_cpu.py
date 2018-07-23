#!/usr/bin/python
# -*- encoding: utf-8 -*-
# script to monitor system resources usage
# shows cores usage, running processes and load
# cpu_usage len is different on each processor so remember to change accordingly!
# requires psutil to work
# insert your threads number as NUMBER_OF_THREADS to check CPU usage
# remember to change your params.xml accordingly

import sys                                  # exit()
import time                                 # sleep()
import logging
from logging.handlers import SysLogHandler
sys.path.append("/opt/szarp/lib/python")
from pydaemontimer import ReTimer

import psutil
import os
import subprocess
from subprocess import Popen, PIPE

BASE_PREFIX = ""
NUMBER_OF_THREADS = 4

class CPUReader:
	def get_running_processes(self):
		p = subprocess.Popen("ps ux | wc -l", shell=True,stdout=PIPE)
		return int(p.communicate()[0].strip())

	def update_values(self):
		try:
			rp = self.get_running_processes()
			cpu_usage = psutil.cpu_percent(percpu=True)
			for i in xrange(0, NUMBER_OF_THREADS):
				ipc.set_read_sz4_float(i, cpu_usage[i])
			ipc.set_read_sz4_int(NUMBER_OF_THREADS, rp)
			ipc.set_read_sz4_double(NUMBER_OF_THREADS+1, os.getloadavg()[0] )
		except Exception as err:
			logger.error("failed to fetch data, %s", str(err).splitlines()[0])
			for i in xrange(0, NUMBER_OF_THREADS):
				ipc.set_no_data(i)
			ipc.set_no_data(NUMBER_OF_THREADS)
			ipc.set_no_data(NUMBER_OF_THREADS+1)

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
		ipc = TestIPC("%s"%BASE_PREFIX, "/opt/szarp/%s/pdmn_cpu.py"%BASE_PREFIX)

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
