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
from urlparse import urlparse
import httplib
import base64, string
import datetime

# Config file path
CONFIG = "/etc/szarp/meteoprog.cfg"

# Patterns for creating parameters names
strs = [
		'"Sieć:Prognoza temperatury:temperatura %s 1 dzień"',
		'"Sieć:Prognoza temperatury:temperatura %s 2 dni"',
		'"Sieć:Prognoza temperatury:temperatura %s 3 dni"',
		'"Sieć:Prognoza temperatury:temperatura %s 4 dni"'
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
	parse_res = urlparse(url)
	url_host = parse_res.netloc
	url_path = parse_res.path
	if parse_res.query != "":
		url_path = url_path + "?" + parse_res.query
	return [url_host, url_path]

# establish connection
[url_host, url_path] = url_split(url)
connection = httplib.HTTPConnection(url_host)
auth = base64.encodestring('%s:%s' % (user, passwd)).replace('\n', '')
connection.request("GET", url_path, headers={"Authorization" : "Basic %s" % auth})
response = connection.getresponse()

# handle redirections
redir_depth = 0
while response.status in [301, 302]:
	redir_depth = redir_depth + 1
	if redir_depth > 10:
		raise Exception("Redirected %d times, giving up" % redir_depth)
	headers = dict(response.getheaders())
	location = headers['location']
	[url_host, url_path] = url_split(location)
	connection = httplib.HTTPConnection(url_host)
	connection.request("GET", url_path, headers={"Authorization" : "Basic %s" % auth})
	response = connection.getresponse()

# get XML data using open connection
xml = etree.XML(response.read().lstrip(' \t\n'))

# save output
cache_min = []
cache_max = []
i = 0
for date in xml:
	(year, month, day) =  date.get('date').split('-')
	for time in date:
		tmin = time.find('tmin')
		tmax = time.find('tmax')
		cache_min.append((int(tmin.text), datetime.datetime(int(year), int(month), int(day), int(time.get('time')[:2]) , 0) ))
		cache_max.append((int(tmax.text), datetime.datetime(int(year), int(month), int(day), int(time.get('time')[:2]) , 0) ))

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

