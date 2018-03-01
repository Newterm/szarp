#!/usr/bin/python
# -*- encoding: utf-8 -*-
#script to get local weather info
#details : https://openweathermap.org
#uses pyowm libary to simplify observation requests
#to use this script you need your own OWM_TOKEN
#you can test location availability on the given site
#fill OWM_TOKEN and LOCATION to run this script

#python-owm
#https://github.com/csparpa/pyowm
import pyowm


import sys                                  # exit()
import time                                 # sleep()
import logging
from logging.handlers import SysLogHandler

OWM_TOKEN = 'you_OWM_token'
LOCATION = 'Warsaw,PL'
BASE_PREFIX = ""

class WeatherReader:
	def __init__(self):
		self.owm = pyowm.OWM(OWM_TOKEN)

	def update_values(self):
		try:
			observation = self.owm.weather_at_place(LOCATION)
			if observation is None :
				ipc.set_no_data(0)
				ipc.set_no_data(1)
				ipc.set_no_data(2)
			else :
				W = observation.get_weather()
				temp = W.get_temperature('celsius')['temp']
				pressure = W.get_pressure()['press']
				wind_speed = W.get_wind()['speed']
				ipc.set_read(0, int(10 * temp) )
				ipc.set_read(1, pressure       )
				ipc.set_read(2, int(10 * wind_speed)  )
		except Exception as err:
			logger.error("failed to fetch data, %s", str(err).splitlines()[0])
			ipc.set_no_data(0)
			ipc.set_no_data(1)
			ipc.set_no_data(2)

		ipc.go_parcook()

def main(argv=None):
	global logger
	logger = logging.getLogger('pdmn_weather.py')
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
		ipc = TestIPC("%s"%BASE_PREFIX, "/opt/szarp/%s/pdmn_weather.py"%BASE_PREFIX)

	ex = WeatherReader()

	while True:
		time.sleep(10)
		ex.update_values()


# end of main()

if __name__ == "__main__":
	sys.exit(main())
