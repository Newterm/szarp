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
import config
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

	month = c2d(path[4]) * 10
	month += c2d(path[5])

	return year, month

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

		def tell(self):
			return self.file.tell()

		def close(self):
			file = open(self.path, "w+b")
			file.write(self.file.getvalue())
			self.file.close()

	def open(self, path, mode):
		return self.File(path)


class Converter:
	def __init__(self, config_dir, offset, count):
		self.offset = offset
		self.count = count

		sz4_dir = os.path.join(config_dir, "sz4")
		self.szbase_dir = os.path.join(config_dir, "szbase")

		self.ipk = ipk.IPK(config_dir + "/config/params.xml")

		self.s_params = {}

		for p in self.ipk.params:
			sp = saveparam.SaveParam(p, sz4_dir, FileFactory(), False)
			self.s_params[p.param_name] = sp
	
		self.s_params[heartbeat_param_name] = saveparam.SaveParam(create_hearbeat_param(), sz4_dir, FileFactory())

	def convert_param(self, sp, pname):
		param_path = sp.param_path.param_path if pname != heartbeat_param_name else "Status/Meaner3/program_uruchomiony"

		try:
			szbase_files = [ x for x in os.listdir(os.path.join(self.szbase_dir, param_path)) if x.endswith(".szb")]
		except OSError:
			print "No values stored in db for that param"
			return	
		szbase_files.sort()

		value = None

		for j, szbase_path in enumerate(szbase_files):
			sys.stdout.write("\rFile: %s, %d/%d    " % (szbase_path, j + 1, len(szbase_files)))

			year, month = szbase_file_path_to_date(szbase_path)
			nyear, nmonth = (year, month + 1) if month != 12 else (year + 1, 1)

			time = calendar.timegm(datetime.datetime(year, month, 1).utctimetuple())
			ntime = calendar.timegm(datetime.datetime(nyear, nmonth, 1).utctimetuple())

			f = open(os.path.join(self.szbase_dir, param_path, szbase_path)).read()
			try: 
				l = len(f) / 2
				values = struct.unpack_from(("<%dh" % (l,)), f)

			except struct.error as e:
				print e
				continue

			for v in values:
				if value is None:
					sp.process_value(v, time, 0)
					v = value

				if value != v:
					sp.update_last_time_unlocked(time, 0)
					sp.write_value(v, time, 0)
					value = v

				time += 600
				#meaner bug fix
				if not time < ntime:
					break
				

		if value is not None:
			sp.update_last_time_unlocked(time, 0)

		sp.close()


	def convert(self):
		sys.stdout.write("Converting...")

		pno = 0
		for pname in self.s_params.iterkeys():
			if pno % self.count == self.offset:
				sp = self.s_params[pname]
				print
				print "Converting values for param: %s, param %d out of %d" % (sp.param_path.param_path, pno + 1, len(self.s_params))
				try:
					self.convert_param(sp, pname)
				except OSError, e:
					print e
			pno += 1

def help():
	print """Invocation:
%s configuration_dir [offset partition]
Program converts database from szbase to sz4 format.
Acceppts one mandatory command line argument - directory
to szbase configuration or '-h' which make it print this 
help""" % (sys.argv[0],)


def main():
	if len(sys.argv) != 2 and len(sys.argv) != 4 or sys.argv[1] == '-h':
		help()
	else:
		if len(sys.argv) == 4:
			offset = int(sys.argv[2])
			count = int(sys.argv[3])
		else:
			offset = 0
			count = 1
		Converter(sys.argv[1], offset, count).convert()


main()
