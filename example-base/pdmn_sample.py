#!/usr/bin/python
# -*- encoding: utf-8 -*-

# example showing how to create your own python daemon for SZARP
# if you're interested in full SZARP capabilities check documentation
# https://szarp.org//documentation/new/ipk/html/szarp.html

import sys                                  # exit()
import time                                 # sleep()
import logging                              # logging ( important ! )
from logging.handlers import SysLogHandler

BASE_PREFIX = ""

sys.path.append("/opt/szarp/lib/python")    # use with sz4 python functions
#from pydaemontimer import ReTimer           # optional instead of sleep

class Example:
	#to further customize your daemon behaviour you can use
	#configuration from params.xml to pass additional script values
	#to do so use "extra" namespace inside unit(s) 
	def __init__(self):
		conf_str = ipc.get_conf_str()
		self.xml_root = etree.fromstring(conf_str)
		namespaces = self.xml_root.nsmap
		root_namespace = namespaces[None]
		extra_namespace = namespaces["extra"]
		self.rn = root_namespace
		self.en = extra_namespace
		query = etree.ETXPath("{%s}device/{%s}unit/@{%s}attribute" % (self.rn, self.rn, self.en))
		self.attrs = str(query(self.xml_root))

	#data generating class needs to write into IPC shared memory

	#use function ipc.set_read( param_index, value)
	#where index starts from 0 and matches order of params in params.xml
	#while value must be 16-bit integer ( bigger values ought to be merged inside params.xml file )
	#to show no available data one should use ipc.set_no_data(index)
	#not all values have to be changed before calling go_parcook() function
	#as it will simply read previous values
	def update_values(self):
		try:
			ipc.set_read(0, 1 )
			#float also should be sent as an integer
			#with precision set inside params.xml
			#example float with prec="2" inside params.xml
			ipc.set_read(1, int( float_value * 100 ) )
			#if sz4 base is available and was initialized one can use sz4 interface
			#to write bigger values ( 32 bit ) without converting them
			ipc.set_read_sz4_short(2, short_value )
			ipc.set_read_sz4_float(3, float_value )
			ipc.set_read_sz4_int(4, integer_value )
			ipc.set_read_sz4_double(5, double_value )
		except Exception as err:
			logger.error("failed to fetch data, %s", str(err).splitlines()[0])
			ipc.set_no_data(0)

		#function to trigger data reading from parcook
		#not needed if ipc.autopublish_sz4() was used
		ipc.go_parcook()

def main(argv=None):
	#logging settings
	global logger
	logger = logging.getLogger('pdmn_sample.py')
	logger.setLevel(logging.DEBUG)
	handler = logging.handlers.SysLogHandler(address='/dev/log', facility=SysLogHandler.LOG_DAEMON)
	handler.setLevel(logging.WARNING)
	formatter = logging.Formatter('%(filename)s: [%(levelname)s] %(message)s')
	handler.setFormatter(formatter)
	logger.addHandler(handler)

	#IPC is shared memory segment, daemon has access to and writes its data into
	#It is managed by parcook, when daemon is created and its size is equal to 
	#number of parameters declared inside params.xml file
	#
	#If no ipc is specified ( script was called by user not by parcook )
	#daemon will mock IPC to write params on stdout

	#mocking ipc so you can test your script by running it from command line
	if 'ipc' not in globals():
		global ipc
		sys.path.append('/opt/szarp/lib/python')
		from test_ipc import TestIPC
		ipc = TestIPC("%s"%BASE_PREFIX, "/opt/szarp/%s/pdmn_sample.py"%BASE_PREFIX)

	#create data reader instance
	ex = RAMReader()

	#for sz3 database type maximum single parameter size is 16bit
	#if you need bigger parameters you can merge them inside params.xml ( check documentation )
	#however if you use sz4 database type you can use sz4 interface
	#to use bigger sizes  ( 32bit )
	#lines below need to be added to use sz4 interface
	#ipc.autopublish_sz4()
	#ipc.force_sz4()

	#while not ipc.sz4_ready():
	#	time.sleep(1)

	#main loop
	#update values for IPC
	#no shorter than 10 seconds
	while True:
		time.sleep(10)
		ex.update_values()
	#you can also use ReTimer instead of sleep
	#ReTimer(sleep_period, functor_to_call).go()


# end of main()

if __name__ == "__main__":
	sys.exit(main())
