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
import ipk

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
ipk = IPK.ipk(config_dir + "/config/params.xml")
sys.stdout.write("Converting...")

param_map = {}
for i, pnode in enumerate(params):
	p = param.Param(pnode)
	sp = saveparam.SaveParam(param.Param(p), sz4_dir, False)
	param_map[p.param_name] = p

for i, p in enumerate(param_map):
	try:

		print
		print "Converting values for param: %s, param %d out of %d" % (sp.param_path.param_path, i + 1, len(params))

		if p.param.msw_combined_referencing_param is not None or p.param.lsw_combined_referencing_param is not None:
			continue

		combined = p.param.combined

		if combined:
			lp = p.lsw_param
			mp = p.msw_param
		else:
			lp = sp

		szbase_files = [ x for x in os.listdir(os.path.join(szbase_dir, lp.param_path.param_path)) if x.endswith(".szb")]
		szbase_files.sort()

		for j, szbase_path in enumerate(szbase_files):
			sys.stdout.write("\rFile: %s, %d/%d    " % (szbase_path, j, len(szbase_files)))
			sys.stdout.flush()
			date = szbase_file_path_to_date(szbase_path)

			f = open(os.path.join(szbase_dir, lp.param_path.param_path, szbase_path)).read()
			if combined:
				f2 = open(os.path.join(szbase_dir, mp.param_path.param_path, szbase_path)).read()
			for i in xrange(len(f) / 2):
				v = struct.unpack_from("<h", f, i * 2)[0]
				if combined:
					v2 = struct.unpack_from("<h", f2, i * 2)[0]
					v = v2 * 65536 + v
				lp.process_value(v, calendar.timegm(date.utctimetuple()), None)
				date += datetime.timedelta(minutes=10)
	except OSError, e:
		pass


