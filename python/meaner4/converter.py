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
from heartbeat import heartbeat_param_name, create_hearbeat_param
import cStringIO

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

class FileFactory:
	class File:
		def __init__(self, path):
			self.path = path

			self.file = cStringIO.StringIO()
			try:
				file = open(path, "r+b")
				s = file.read()
				self.file.write(s)
			except:
				pass


		def seek(self, offset, whence):
			self.file.seek(offset, whence)

		def write(self, data):
			self.file.write(data)

		def read(self, len):
			return self.file.read(len)

		def lock(self):
			pass

		def unlock(self):
			pass

		def close(self):
			file = open(self.path, "w+b")
			file.write(self.file.getvalue())
			self.file.close()

	def open(self, path, mode):
		return self.File(path)

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

ipk = ipk.IPK(config_dir + "/config/params.xml")
sys.stdout.write("Converting...")

save_param_map = {}
for i, p in enumerate(ipk.params):
	sp = saveparam.SaveParam(p, sz4_dir, FileFactory())
	save_param_map[p.param_name] = sp
save_param_map[heartbeat_param_name] = saveparam.SaveParam(create_hearbeat_param(), sz4_dir, FileFactory())

pno = 0
for pname in save_param_map.iterkeys():
	try:
		sp = save_param_map[pname]
		if sp.param.msw_combined_referencing_param is not None or sp.param.lsw_combined_referencing_param is not None:
			continue

		sp = save_param_map[pname]
		print
		print "Converting values for param: %s, param %d out of %d" % (sp.param_path.param_path, pno + 1, len(save_param_map))

		combined = sp.param.combined
		if combined:
			lsp = save_param_map[sp.param.lsw_param.param_name]
			msp = save_param_map[sp.param.msw_param.param_name]
		else:
			lsp = sp

		param_path = lsp.param_path.param_path if pname != heartbeat_param_name else "Status/Meaner3/program_uruchomiony"
		szbase_files = [ x for x in os.listdir(os.path.join(szbase_dir, param_path)) if x.endswith(".szb")]
		szbase_files.sort()

		for j, szbase_path in enumerate(szbase_files):
			sys.stdout.write("\rFile: %s, %d/%d    " % (szbase_path, j, len(szbase_files)))
			sys.stdout.flush()
			date = szbase_file_path_to_date(szbase_path)

			f = open(os.path.join(szbase_dir, lsp.param_path.param_path, szbase_path)).read()
			if combined:
				f2 = open(os.path.join(szbase_dir, msp.param_path.param_path, szbase_path)).read()
			try: 
				for i in xrange(len(f) / 2):
					if combined:
						v1 = struct.unpack_from("<H", f, i * 2)[0]
						v2 = struct.unpack_from("<H", f2, i * 2)[0]
						v = (v2 << 16) + v1
					else:
						v = struct.unpack_from("<h", f, i * 2)[0]
					sp.process_value(v, calendar.timegm(date.utctimetuple()), None)
					date += datetime.timedelta(minutes=10)
			except struct.error:
				pass

		sp.close()
	except OSError, e:
		pass

	pno += 1


