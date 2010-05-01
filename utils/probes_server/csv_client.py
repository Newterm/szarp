#!/usr/bin/python
"""
Simple client for probe_server. Reads data and writes as CSV file.
Command line options are similar to extrszb program, use --help for details.
Arguments: param_path start_time end_time
Time in "%Y-%m-%d %H:%M:%S" format

SZARP: SCADA software 
Pawel Palucha <pawel@praterm.com.pl>

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

import sys
import socket 
import time
import array
import calendar
from optparse import OptionParser

f = "%Y-%m-%d %H:%M:%S"

parser = OptionParser(usage="usage: %prog [options] parameter_path\n\nParameter path is path to parameter data directory relative to main cache directory, for example for parameter 'System:Status:average system load' it is 'System/Status/average_system_load'")
parser.add_option("-s", "--start", dest="start", help="first date/time to extract, required")
parser.add_option("-e", "--end", dest="end", help="last date/time to extract, required")
parser.add_option("-D", "--date-format", dest="dateformat", metavar="STRING", default=f,
		help="set date/time format string to STRING, default is %default")
parser.add_option("-E", "--empty", dest="empty", action="store_true", default=False, 
		help="print also empty rows (ignored by default)")
parser.add_option("-N", "--no-data", dest="nodata", metavar="TEXT", default="NO_DATA",
		help="print TEXT in place of no-data values, default is '%default', ignored without -E option")
parser.add_option("-d", "--delimiter", dest="delimiter", metavar="TEXT",
		help="use TEXT as fields delimiter, default is '%default'", default=",")
parser.add_option("-o", "--output", dest="output", metavar="FILE",
		help="write output to FILE, default is standard output")
#parser.add_option("-S", "--dec-separator", metavar="C", dest="separator", default=".",
#		help="use C as decimal separator, default is '%default'")
(options, arguments) = parser.parse_args()
if len(arguments) != 1:
	parser.error("one parameter path required")
if not options.start:
	parser.error("-s options is required")
if not options.end:
	parser.error("-e options is required")

param = arguments[0]
starttime = options.start
endtime = options.end
if options.output:
	output = open(options.output, "w")
else:
	output = sys.stdout

start = calendar.timegm(time.strptime(starttime, options.dateformat))
end = calendar.timegm(time.strptime(endtime, options.dateformat))

res = socket.getaddrinfo('localhost', 8090, socket.AF_UNSPEC, socket.SOCK_STREAM)

(family, socktype, proto, canonname, sockaddr) = res[0]
s = socket.socket(family, socktype, proto)
s.connect(sockaddr)
s.send("GET %d %d %s\r\n" % (start, end, param))
sizestr = ''
c = ''
while c != '\n':
	c = s.recv(1)
	sizestr += c

size = int(sizestr.rstrip())

t = start
while size > 0:
	data = s.recv(4096)
	size -= len(data)
	arr = array.array('h')
	arr.fromstring(data)
	for i in arr:
		lt = time.strftime(options.dateformat, time.localtime(t))
		t += 10
		if i != -32768:
			output.write("%s%s %d\n" % (lt, options.delimiter, i))
		elif options.empty:
			output.write("%s%s %s\n" % (lt, options.delimiter, options.nodata))
s.close()

