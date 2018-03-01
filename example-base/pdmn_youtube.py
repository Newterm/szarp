#!/usr/bin/python
# -*- encoding: utf-8 -*-

#script to track your video information - rating, views, likes and dislikes
#remember to use correctly
#no SZARP warranty

#uses pafy python libary to simplify youtube requests
#details : https://github.com/mps-youtube/pafy
#requires pafy and youtube-dl to be installed
#easiest way is to do this with pip
#fill extra:url in config/params.xml to gather data with this script

import pafy

import sys                                  # exit()
import time                                 # sleep()
import logging                              # logging (important!)
sys.path.append("/opt/szarp/lib/python")    #
from pydaemontimer import ReTimer           

import psutil
from lxml import etree
from logging.handlers import SysLogHandler  #

BASE_PREFIX = ""

class YTReader:
	#parsing configuration xml to get video url
	def __init__(self):
		conf_str = ipc.get_conf_str()
		self.xml_root = etree.fromstring(conf_str)
		namespaces = self.xml_root.nsmap
		root_namespace = namespaces[None]
		extra_namespace = namespaces["extra"]
		self.rn = root_namespace
		self.en = extra_namespace
		query = etree.ETXPath("{%s}device/{%s}unit/@{%s}url" % (self.rn, self.rn, self.en))
		self.URL = str(query(self.xml_root)[0])
		self.URL2 = str(query(self.xml_root)[1])

	def update_values(self):
		try:
			video = pafy.new( self.URL )
			ipc.set_read(0, video.viewcount)
			ipc.set_read(1, video.likes)
			ipc.set_read(2, video.dislikes)
			ipc.set_read(3, int(10 * video.rating) )
			video2 = pafy.new( self.URL2 )
			ipc.set_read(4, video2.viewcount)
			ipc.set_read(5, video2.likes)
			ipc.set_read(6, video2.dislikes)
			ipc.set_read(7, int(10 * video2.rating) )

		except Exception as err:
			logger.error("failed to fetch data, %s", str(err).splitlines()[0])
			ipc.set_no_data(0)
			ipc.set_no_data(1)
			ipc.set_no_data(2)
			ipc.set_no_data(3)
			ipc.set_no_data(4)
			ipc.set_no_data(5)
			ipc.set_no_data(6)
			ipc.set_no_data(7)

		ipc.go_parcook()

def main(argv=None):
	global logger
	logger = logging.getLogger('pdmn_youtube.py')
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
		ipc = TestIPC("%s"%BASE_PREFIX, "/opt/szarp/%s/pdmn_youtube.py"%BASE_PREFIX)

	ex = YTReader()
	time.sleep(10)

	ReTimer(3600, ex.update_values).go()


# end of main()

if __name__ == "__main__":
	sys.exit(main())
