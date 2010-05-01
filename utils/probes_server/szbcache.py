"""
  Module for SZARP 10-seconds probes cache searching/reading. 

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

import os
import time
from calendar import timegm
import glob
import array
from subprocess import Popen, PIPE

def debug(s):
	print "DEBUG:", s

def format_time(tm):
	if not isinstance(tm, time.struct_time):
		tm = time.gmtime(tm)
	return "%04d-%02d-%02d %02d:%02d" % (tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min)


class SzbException(Exception):
	pass

class PathIterator:
	"""
	Iterates through possible next/previous szbase/szcache files.
	"""
	def __init__(self, path):
		"""
		@param path path to start file
		"""
		self.path = path
		self.ext = path.rsplit(".", 2)[1]

	def next(self):
		"""
		Returns path to next file, set this path as current.
		"""
		(dirname, year, month) = self.split()
		if month > 12:
			month = 1
			year += 1
		self.join(dirname, year, month)
		return self.path

	def prev(self):
		"""
		Returns path to previous file, set this path as current.
		"""
		(dirname, year, month) = self.split()
		if month <= 0:
			month = 12
			year -= 1
		self.join(dirname, year, month)
		return self.path

	def split(self):
		"""
		Returns current path splitted into (dirname, year, month) tuple.
		"""
		(dirname, base) = os.path.split(self.path)
		year = int(base[0:4])
		month = int(base[4:6]) - 1
		return (dirname, year, month)

	def join(self, dirname, year, month):
		"""
		Creates new path from dirname, year and month, set is as current.
		"""
		self.path = os.path.join(dirname, '%04d%02d.%s' % (year, month, self.ext))

class LibparReader:
	"""
	Class for reading parameters from szarp.cfg files.
	"""
	def __init__(self):
		pass

	def get(self, section, parameter):
		"""
		Return value of parameter from section, empty if parameter is not found.
		"""
		return Popen(["/opt/szarp/bin/lpparse", "-s", section, parameter],
				stdout=PIPE).communicate()[0].strip()

class SzbCache:
	"""
	Class for accessing szbase cache. Allows for data searching and reading.
	"""
	SZBCACHE_PROBE = 10
	SZBCACHE_SIZE = 2
	SZBCACHE_TYPE = 'h'
	SZBCACHE_NODATA = -32768
	SZBCACHE_EXT = ".szc"

	def __init__(self):
		lpr = LibparReader()
		self.cachedir = lpr.get("prober", "cachedir")
		if not os.path.isdir(self.cachedir):
			raise SzbException("incorrect cache dir '" + self.cachedir + "'")

	def search(self, start, end, direction, parampath):
		"""
		Search for data. Public interface.
		@param start start of search as time_t
		@param end end of search as time_t
		@param direction 1 for right search, -1 for left search, 0 for in-place
		@param parampath path to directory with parameter, relative to main cache directory

		start should be grater then end for direction equal to -1 and less for direction equal 1;
		end is ignored for direction equal 0; if start or end is equal to -1, appropriate first/last
		time is used.

		"""
		debug("SEARCH PARAMS: start %s, end %s, direction %d" % (format_time(start), format_time(end), direction))
		dpath = self.check_path(parampath)
		if not os.path.isdir(dpath):
			return -1
		if direction == 0:
			if start == -1:
				(start, last) = self.search_first_last()
			return self.search_at(start, dpath)
		(first, last) = self.search_first_last(dpath)
		if direction > 0:
			if start == -1 or start < first:
				start = first
			if end == -1 or end > last:
				end = last
		else:
			if start == -1 or start > last:
				start = last
			if end == -1 or end < first:
				end = first
		debug("START END: %s %s" % (format_time(start), format_time(end)))
		return self.search_for(start, end, direction, dpath)

	def get_size(self, starttime, endtime, parampath):
		"""
		Returns number of bytes that will be written when calling write_data
		with the same arguments.
		@param starttime first time to write (as time_t)
		@param endtime last time to write (as time_t)
		@param parampath path to parameter, relative to main szbcache directory
		@return number of bytes to write
		"""
		dpath = self.check_path(parampath)
		if not os.path.isdir(dpath):
			return 0
		return (((endtime - starttime) / self.SZBCACHE_PROBE) + 1) * self.SZBCACHE_SIZE

	def write_data(self, starttime, endtime, parampath, output):
		"""
		Writes data (or part of it) to output file. You should call this method
		with starttime set to return value of previous call until starttime <= endtime.
		Public interface.
		@param starttime first time to write (as time_t)
		@param endtime last time to write (as time_t)
		@param parampath path to parameter, relative to main szbcache directory
		@param output file to write to (object with 'write' method)
		@return starttime for next call, grater then endtime after last call
		"""
		debug("WRITE_DATA: %d %d" % (starttime, endtime))
		dpath = self.check_path(parampath)
		if not os.path.isdir(dpath):
			return endtime + 1
		startpath, startindex = self.time2index(starttime, dpath)
		endpath, endindex = self.time2index(endtime, dpath)
		if startpath == endpath:
			# write part of file
			self.write_file(startindex, endindex, startpath, output)
			return self.index2time(startpath, endindex) + self.SZBCACHE_PROBE
		else:
			# write to the end of file
			pi = PathIterator(startpath)
			nextpath = startpath.next()
			nexttime = self.index2time(nextpath, 0)
			endpath, endindex = self.time2index(nexttime - self.SZBCACHE_PROBE, startpath)
			self.write_file(startindex, endindex, startpath, output)
			return nexttime

	def write_file(self, startindex, endindex, path, output):
		"""
		Writes data from startindex to endindex from path to output. If path file is
		to short, no-data is written to fill missing space.
		@param startindex first index to write
		@param endindex last index to write
		@param path path to file
		@param output output file
		"""
		debug("WRITE_FILE %d %d %s" % (startindex, endindex, str(output)))
		lastindex = min(endindex, self.last_index(path))
		towrite = ((lastindex - startindex) + 1) * self.SZBCACHE_SIZE
		debug("WRITE_FILE TOWRITE %d" % towrite)
		f = open(path, "rb")
		output.write(f.read(towrite))
		f.close()
		arr = array.array(self.SZBCACHE_TYPE)
		arr.append(self.SZBCACHE_NODATA)
		while lastindex < endindex:
			output.write(arr.tostring())
			lastindex += 1
		debug("WRITE_FILE FINISH")

	def search_first_last(self, dirpath):
		"""
		Return tupple of first and last data available for param from given directory.
		"""
		l = sorted(glob.glob(dirpath + '/[0-9][0-9][0-9][0-9][0-9][0-9]%s' % (self.SZBCACHE_EXT)))
		if len(l) == 0:
			return -1
		return (self.index2time(l[0]), self.index2time(l[len(l) - 1], -1) )

	def check_path(self, parampath):
		"""
		Check for directory for given path, returns absolute path to parameter directory.
		@param parampath path to parameter directory, relative to main cache directory
		"""
		try:
			splitted = parampath.split('/')
			(p1, p2, p3) = splitted
		except ValueError:
			raise SzbException("Incorrect parameter path")
		for p in splitted:
			if p in ['', '.', '..']:
				raise SzbException("Incorrect relative parameter path")
		return os.path.abspath(self.cachedir + "/" + parampath)


	def search_at(self, start, dpath):
		"""
		Searches for data exactly at given position.
		@param start time_t time
		@param path absolute path to directory for param
		@return -1 if data is not found, start if found
		"""
		(path, index) = self.time2index(start, dpath)
		if not os.path.isfile(path):
			return -1
		index *= self.SZBCACHE_SIZE
		if os.path.getsize(path) < index:
			return -1
		f = open(path, "rb")
		f.seek(index)
		arr = array.array(self.SZBCACHE_TYPE)
		arr.fromfile(f, 1)
		if arr[0] == self.SZBCACHE_NODATA:
			return -1
		return start

	def search_for(self, start, end, direction, dpath):
		"""
		Search for data in files.
		@param start time_t start time
		@param end time_t end time
		@param direction serach direction, -1 or +1
		@param dpath path to parameter data directory
		@return -1 if data not found, otherwise time_t time of data found
		"""
		(startpath, startindex) = self.time2index(start, dpath)
		pi = PathIterator(startpath)
		(endpath, endindex) = self.time2index(end, dpath)

		while start * direction <= end * direction:
			debug("SEARCH LOOP: start, startpath, startindex: %s %s %d" %
					(format_time(start), startpath, startindex))
			(found, start) = self.search_file(startpath, startindex, endpath, endindex, direction)
			if found:
				return start
			if direction > 0:
				(startpath, startindex) = (pi.next(), 0)
			else:
				(startpath, startindex) = (pi.prev(), -1)
			print "DEBUG startpath, direction", startpath, direction
			start = self.index2time(startpath, startindex)
		return -1

	def search_file(self, path, startindex, endpath, endindex, direction):
		"""
		Search for data in file.
		@param path path to file to search in
		@startindex starting search index
		@param endpath final path to search, meaningfull only if equal to path
		@param endindex final index to search, meaningfull only if endpath equals path
		@param direction search direction
		@return (found, found_time) tupple
		"""
		if not os.path.isfile(path):
			return (False, -1)
		if endpath != path:
			# search all file
			if direction > 0:
				endindex = self.last_index(path)
			else:
				endindex = 0
		else:
			if direction > 0:
				endindex = min(endindex, self.last_index(path))
			else:
				startindex = min(startindex, self.last_index(path))
		if startindex == -1 and direction < 0:
			startindex = self.last_index(path)
		return self.real_search_file(path, startindex, endindex, direction)

	def file_seek(self, f, index):
		"""
		Seek to specified position in file.
		@param f file to seek in
		@param index index of position to seek to
		"""
		f.seek(index * self.SZBCACHE_SIZE)

	def real_search_file(self, path, startindex, endindex, direction):
		"""
		Search for data in file with readl start/end indexes.
		@param path of file to search
		@param startindex first index, within file size
		@param endindex lastindex, within file size
		@param direction search direction
		@return (found, found_time) tupple
		"""
		bsize = 32768 / self.SZBCACHE_SIZE
		
		f = open(path, "rb")
		while startindex * direction <= endindex * direction:
			rsize = min(bsize, (endindex - startindex) * direction)
			if direction < 0:
				self.file_seek(f, startindex - rsize)
			else:
				self.file_seek(f, startindex)
			arr = array.array(self.SZBCACHE_TYPE)
			arr.fromfile(f, rsize)
			found, foundindex = self.search_array(arr, direction)
			if found:
				return (True, self.index2time(path, foundindex))
			startindex += rsize * direction
		return (False, -1)

	def search_array(self, arr, direction):
		"""
		Search for no-data index in array.
		@param arr array to search in
		@param direction search direction
		@return (found, found_index) tupple
		"""
		if direction > 0:
			start = 0
			stop = len(arr)
		else:
			start = len(arr) - 1
			stop = - 1
		debug("SEARCH_ARRAY start %d stop %d direction %d" % (start, stop, direction))
		for i in range(start, stop, direction):
			if arr[i] != self.SZBCACHE_NODATA:
				return (True, i)
		return (False, -1)

	def last_index(self, path):
		"""
		Return number of last index of file (or size of file in indexes minus 1).
		"""
		return os.path.getsize(path) / self.SZBCACHE_SIZE - 1

	def time2index(self, t, dirpath):
		"""
		Returns absolute file name and index for given time and params directory.
		"""
		ts = time.gmtime(t)
		path = "%s/%04d%02d%s" % (dirpath, ts.tm_year, ts.tm_mon, self.SZBCACHE_EXT)
		index = (ts.tm_mday - 1) * 24 * 3600 + ts.tm_hour * 3600 + ts.tm_min * 60 + ts.tm_sec
		index = index // self.SZBCACHE_PROBE
		print path, index
		return (path, index)

	def index2time(self, path, index = 0):
		"""
		Returns time_t for given file and index. If index - 1, look for last index in file.
		"""
		if index == -1:
			index = os.path.getsize(path) / self.SZBCACHE_SIZE
		(dirname, filename) = os.path.split(path)
		print "FILENAME:", filename
		year = int(filename[0:4])
		month = int(filename[4:6])
		return timegm((year, month, 1, 0, 0, 0)) + index * self.SZBCACHE_PROBE

