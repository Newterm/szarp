#!/usr/bin/python
# -*- coding: iso-8859-2 -*-

# Script fetches and parses weather forecast data from meteoprog.ua website
# in format suitable for szbwriter input.
# There should be parameters configured in SZARP, 3*2 = 6 parameters for 3-days forecast.
#
# Config file /etc/szarp/meteoprog.cfg is required:
#
# [Main]
# url=<direct url to forecast XML data>
# user=<user name to access data>
# password=<password>

from lxml import etree
from ConfigParser import SafeConfigParser
import httplib
import base64, string
import datetime
import re

# Config file path
CONFIG = "/etc/szarp/meteoprog.cfg"

# Patterns for creating parameters names
strs = [
		'"Sieć:Prognoza temperatury:temperatura %s 1 dzień"',
		'"Sieć:Prognoza temperatury:temperatura %s 2 dni"',
		'"Sieć:Prognoza temperatury:temperatura %s 3 dni"'
		]
minmax = [ "minimalna", "maksymalna" ]

# Starting hours of forcast periods
hours = dict()
hours["night"] = 0
hours["morning"] = 5
hours["day"] = 11
hours["evening"] = 17

shift = dict()
shift["night"] = datetime.timedelta(hours=1)
shift["morning"] = datetime.timedelta()
shift["day"] = datetime.timedelta()
shift["evening"] = datetime.timedelta()
shift["last"] = datetime.timedelta(hours=1)

# parsing config file
config = SafeConfigParser()
config.read(CONFIG)
url = config.get('Main', 'url')
user = config.get('Main', 'user')
passwd = config.get('Main', 'password')

def url_split(url):
	# get url base (without trailing request part)
	reg_result = re.match("(http://)?(?P<host>[^/]+)(?P<middle>.*/)(?P<end>[^/]*)\s*", url)
	if reg_result == None:
		raise Exception("URL: '%s' doesn't match expected form" % url)
	url_host = reg_result.group('host')
	url_middle = reg_result.group('middle')
	url_end = reg_result.group('end')
	return [url_host, url_middle, url_end]

# establish connection, which will be used without closing
[url_host, url_middle, url_end] = url_split(url)
connection = httplib.HTTPConnection(url_host)
auth = base64.encodestring('%s:%s' % (user, passwd)).replace('\n', '')
# authorisation request, authorisation will be kept together with open connection
connection.request("GET", url_middle, headers={"Authorization" : "Basic %s" % auth})
response = connection.getresponse()

# handle redirections
redir_depth = 0
while response.status in [301, 302]:
	redir_depth = redir_depth + 1
	if redir_depth > 10:
		raise Exception("Redirected %d times, giving up" % redir_depth)
	headers = dict(response.getheaders())
	location = headers['location']
	[url_host, url_middle, unused] = url_split(location)
	connection = httplib.HTTPConnection(url_host)
	connection.request("GET", url_middle, headers={"Authorization" : "Basic %s" % auth})
	response = connection.getresponse()

# get XML data using open connection
connection.request("GET", url_middle + url_end, headers={"Authorization" : "Basic %s" % auth})
meteo = connection.getresponse()
xml = etree.XML(meteo.read().lstrip(' \t\n'))

# save output
cache_min = []
cache_max = []
i = 0
for date in xml:
	(year, month, day) =  date.get('date').split('-')
	for time in date:
		tmin = time.find('tmin')
		tmax = time.find('tmax')
		cache_min.append((int(tmin.text), datetime.datetime(int(year), int(month), int(day), hours[time.get('name')], 0) - shift[time.get('name')]))
		cache_max.append((int(tmax.text), datetime.datetime(int(year), int(month), int(day), hours[time.get('name')], 0) - shift[time.get('name')]))
	# add data for end of period
	cache_min.append((int(tmin.text), datetime.datetime(int(year), int(month), int(day)) +
		datetime.timedelta(days=1) - shift['last']))
	cache_max.append((int(tmax.text), datetime.datetime(int(year), int(month), int(day)) +
		datetime.timedelta(days=1) - shift['last']))
	i += 1

interval = datetime.timedelta(minutes=10)

t = minmax[0]
for cache in (cache_min, cache_max):
	start = cache[0]
	for end in cache[1:]:
		curr = start[1]
		while curr < end[1]:
			print (strs[(start[1] - cache[0][1]).days] % t), 
			print "%04d %02d %02d %02d %02d" % (curr.year, curr.month, curr.day, curr.hour, curr.minute),
			print start[0]
			curr += interval
		start = end
	t = minmax[1]

