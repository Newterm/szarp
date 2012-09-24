"""
  SZARP: SCADA software 
  Darek Marcinkiewicz <reksio@newterm.pl>

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
import os
import os.path
import lxml
import struct
import lxml.etree
import calendar
import datetime

import param
import saveparam

def szbase_file_path_to_date(path): 
	def c2d(c):
		return ord(c) - ord('0')

	year = c2d(path[0]) * 1000
	year += c2d(path[1]) * 100
	year += c2d(path[2]) * 10
	year += c2d(path[3]) 

	month = c2d(path[4])
	month += c2d(path[5])

	return datetime.datetime(year, month, 1)


if len(sys.argv) != 2 or sys.argv[1] == '-h':
	print """Invocation:
%s configuration_dir
Program converts database from szbase to sz4 format.
Acceppts one mandatory command line argument - directory
to szbase configuration or '-h' which make it print this 
help""" % (sys.argv[0],)

config_dir = sys.argv[1]
szbase_dir = os.path.join(config_dir, "szbase")
sz4_dir = os.path.join(config_dir, "sz4")

save_params = []
ipk = lxml.etree.parse(config_dir + "/config/params.xml")
params = ipk.xpath("//s:param[@base_ind]", namespaces={'s':'http://www.praterm.com.pl/SZARP/ipk'})
sys.stdout.write("Converting...")
for i, p in enumerate(params):
	try:
		sp = saveparam.SaveParam(param.Param(p), sz4_dir, False)

		print
		print "Converting values for param: %s, param %d out of %d" % (sp.param_path.param_path, i + 1, len(params))

		szbase_files = [ x for x in os.listdir(os.path.join(szbase_dir, sp.param_path.param_path)) if x.endswith(".szb")]
		szbase_files.sort()

		for j, szbase_path in enumerate(szbase_files):
			sys.stdout.write("\rFile: %s, %d/%d    " % (szbase_path, j, len(szbase_files)))
			sys.stdout.flush()
			date = szbase_file_path_to_date(szbase_path)

			f = open(os.path.join(szbase_dir, sp.param_path.param_path, szbase_path)).read()
			for i in xrange(len(f) / 2):
				v = struct.unpack_from("<h", f, i * 2)[0]
				sp.process_value(v, calendar.timegm(date.utctimetuple()), None)
				date += datetime.timedelta(minutes=10)
	except OSError, e:
		pass


