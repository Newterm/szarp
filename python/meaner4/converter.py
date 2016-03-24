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


import multiprocessing
import lxml.etree
import cStringIO
import calendar
import datetime
import os.path
import config
import struct
import syslog
import lxml
import sys
import os

from heartbeat import heartbeat_param_name, create_hearbeat_param
import saveparam
import parampath
import lastentry
import timedelta
import param
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
	def __init__(self, config_dir, szc_dir, offset, current, queue):
		self.offset = offset
		self.current = current

		self.szbase_dir = os.path.join(config_dir, "szbase")

		self.ipk = ipk.IPK(config_dir + "/config/params.xml")
		self.szc_dir = szc_dir

		self.s_params = {}

		del parampath.ParamPath.find_latest_path 
		parampath.ParamPath.find_latest_path = lambda self: None

		delta_cache = {}
		def get_time_delta_cached(self, time_from, time_to):
			diff = time_to - time_from
			if diff < 0:
				raise TimeError(time_from, time_to)

			if diff in delta_cache:
				return delta_cache[diff]
			else:
				delta = timedelta.encode(diff)
				delta_cache[diff] = delta
				return delta	

		del lastentry.LastEntry.get_time_delta
		lastentry.LastEntry.get_time_delta = get_time_delta_cached

		for p in self.ipk.params:
			sp = saveparam.SaveParam(p, self.szbase_dir, FileFactory(), False)
			self.s_params[p.param_name] = sp
	
		self.s_params[heartbeat_param_name] = saveparam.SaveParam(create_hearbeat_param(), self.szbase_dir, FileFactory())

		self.queue = queue

	def param_sz4_time_start(self, param_path):
		try:
			sz4_files = [ x for x in os.listdir(os.path.join(self.szbase_dir, param_path)) if x.endswith(".sz4") ]
			sz4_files.sort()
			return int(sz4_files[0][:-4])
		except:
			return None

	def read_file(self, param_path, file_path, is_szc):
		try:
			path = self.szc_dir if is_szc else self.szbase_dir

			f = open(os.path.join(path, param_path, file_path))
			data = f.read()
			f.close()

			l = len(data) / 2
			values = struct.unpack_from(("<%dh" % (l,)), data)
			return values

		except e:
			print e
			return []

	def file_start_end_time(self, szbase_path):
		year, month = szbase_file_path_to_date(szbase_path)
		nyear, nmonth = (year, month + 1) if month != 12 else (year + 1, 1)

		time = calendar.timegm(datetime.datetime(year, month, 1).utctimetuple())
		ntime = calendar.timegm(datetime.datetime(nyear, nmonth, 1).utctimetuple())

		return time, ntime

	def get_szbase_files(self,  param_path):
		szb = [ x[:-4] for x in os.listdir(os.path.join(self.szbase_dir, param_path)) if x.endswith(".szb") ]
		
		szc = []
		if self.szc_dir:
			try:
				szc = [ x[:-4] for x in os.listdir(os.path.join(self.szc_dir, param_path)) if x.endswith(".szc") ]
			except OSError:
				pass

		ret = []	
		for s in szb:
			if szc.count(s):
				ret.append(s + ".szc")
			else:
				ret.append(s + ".szb")
		ret.sort()

		return ret

	def convert_param(self, sp, pname, pno):
		value = None
		prev_time = None
		param_path = sp.param_path.param_path
		sz4_time = self.param_sz4_time_start(param_path)

		szbase_files = self.get_szbase_files(param_path)

		for j, path in enumerate(szbase_files):
			self.queue.put((self.offset, param_path, pno + 1,
					len(self.s_params), path , j + 1,
					len(szbase_files)))

			time, ntime = self.file_start_end_time(path)
			if sz4_time is not None and sz4_time <= time:
				sp.close()
				return
			
			if prev_time is not None and time > prev_time:
				sp.process_value(sp.param.nan(), time, 0)

			is_szc = path.endswith(".szc")
			delta = 10 if is_szc else 600 

			values = self.read_file(param_path, path, is_szc)
			for v in values:
				if value is None:
					sp.process_value(v, time, 0)
					v = value

				if value != v:
					sp.update_last_time_unlocked(time, 0)
					sp.write_value(v, time, 0)
					value = v

				time += delta
				if not time < ntime:
					break

 				if sz4_time is not None and sz4_time <= time:
					sp.close()
					return
				
			prev_time = time

		if value is not None:
			sp.update_last_time_unlocked(time, 0)

		sp.close()


	def convert(self):
		keys = self.s_params.keys()

		with self.current.get_lock():
			pno = self.current.value
			self.current.value += 1

		while pno < len(keys):
			pname = keys[pno]
			sp = self.s_params[pname]
			try:
				self.convert_param(sp, pname, pno)
			except OSError, e:
				syslog.syslog(syslog.LOG_ERR | syslog.LOG_USER, str(e))	

			with self.current.get_lock():
				pno = self.current.value
				self.current.value += 1

		self.queue.put((self.offset, None))

def help():
	print """Invocation:
%s configuration_dir [szc cache dir]
Program converts database from szbase to sz4 format.
Acceppts one mandatory command line argument - directory
to szbase configuration or '-h' which make it print this 
help""" % (sys.argv[0],)


def main():
	if len(sys.argv) != 2 and len(sys.argv) != 3 or sys.argv[1] == '-h':
		help()
		return

	queue = multiprocessing.Queue()
	value = multiprocessing.Value('i', 0)

	processes = []
	count = multiprocessing.cpu_count()

	szc_dir = sys.argv[2] if len(sys.argv) == 3 else None

	for i in range(count):
		converter = Converter(sys.argv[1], szc_dir, i, value, queue)
		p = multiprocessing.Process(target=converter.convert)
		processes.append(p)
		p.start()

	sys.stdout.write("\033[2J")

	while count:
		m = queue.get()
		if m[1] is None:
			count -= 1
		else:
			sys.stdout.write("\033[%d;0H" % (int(m[0])+1))
			print "%60.60s %s/%s f:%s %s/%s   " % m[1:]
	
	for p in processes:
		p.join()

	sys.stdout.write("\033[%d;0H" % (multiprocessing.cpu_count() + 1,))
	print("DONE!!!            ")

main()
