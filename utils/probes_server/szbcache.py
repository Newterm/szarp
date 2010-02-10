import os
import time
from calendar import timegm
import glob
import array
from subprocess import Popen, PIPE

class SzbException(Exception):
	pass

class SzbCache:
	SZBCACHE_EXT = ".szc"
	SZBCACHE_PROBE = 10
	SZBCACHE_SIZE = 2
	SZBCACHE_TYPE = 'h'
	SZBCACHE_NODATA = -32768

	def __init__(self):
		self.cachedir = Popen(["/opt/szarp/bin/lpparse", "-s", "prober", "cachedir"],
				stdout=PIPE).communicate()[0].strip()
		if not os.path.isdir(self.cachedir):
			raise SzbException("incorrect cache dir '" + self.cachedir + "'")

	"""
	Check for directory for given path, returns absolute path to parameter directory.
	"""
	def check_path(self, path):
		try:
			splitted = path.split('/')
			(p1, p2, p3) = splitted
		except ValueError:
			raise SzbException("Incorrect parameter path")
		for p in splitted:
			if p in ['', '.', '..']:
				raise SzbException("Incorrect relative parameter path")
		return os.path.abspath(self.cachedir + "/" + path)

	def search(self, start, end, direction, path):
		path = self.check_path(path)
		if not os.path.isdir(path):
			return -1
		if direction == 0:
			return self.search_at(start, path)
		elif direction > 0:
			return self.search_right(start, end, path)
		return -1

	"""
	Searches for data exactly at given position.
	@param start time_t time
	@param path absolute path to directory for param
	@return -1 if data is not found, start if found
	"""
	def search_at(self, start, path):
		(path, index) = self.time2index(start, path)
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

	"""
	Return tupple of first and last data available for param from given directory.
	"""
	def search_first_last(self, path):
		l = sorted(glob.glob(path + '/[0-9][0-9][0-9][0-9][0-9][0-9]%s' % (self.SZBCACHE_EXT)))
		if len(l) == 0:
			return -1
		return ( self.index2time(l[0]), self.index2time(l[len(l) - 1], -1) )

	"""
	Search for first no-null data.
	@param start start time
	@param end end time (may be -1)
	@param path directory for param
	@return time of data found, -1 if not found
	"""
	def search_right(self, start, end, path):
		first, last = self.search_first_last(path)
		print "FIRST LAST", first, last
		if end == -1:
			end = last
		else:
			end = min(end, last)
		if start < first:
			start = first
		if start > end:
			return -1
		while True:
			(path, index) = self.time2index(start, path)
			(found, start) = self.search_in_file(path, index, 
					index + (end - start) / self.SZBCACHE_PROBE)
			if found:
				return start
		return -1

	"""
	Searches for not-null data in file.
	@param path path to file, may not exist
	@param index probe index to start
	@param max_probes length (in probes) of range to search in (may exceed
	path file length)
	@return tupple (found, time) where found is True if data was found, otherwise 
	time is significant and is next time to search for (start of potential next file)
	"""
	def search_in_file(self, path, index, max_probes):
		print "HERE", index, max_probes
		if index > max_probes:
			return (True, -1)
		# if file does not exists, we try next
		if not os.path.isfile(path):
			return (False, self.index2time(self.next_file(path)))
		# size of current file, in probes
		isize = os.path.getsize(path) / self.SZBCACHE_SIZE
		# maximum range of probes to check
		isize = min(isize, max_probes)
		# file is to short, try next
		if index > isize:
			return (False, self.index2time(self.next_file(path)))
		# move file position to start of search
		f = open(path, "rb")
		f.seek(index * self.SZBCACHE_SIZE)
		# iterate through file
		print "HERE10", index, isize
		while index < isize:
			# buffer size
			bsize = min(32768, isize - index)
			# read data into buffer
			arr = array.array(self.SZBCACHE_TYPE)
			arr.fromfile(f, bsize)
			print "TELL:", f.tell()
			# scan buffer for no-data
			for i in range(0, bsize):
				if arr[i] != self.SZBCACHE_NODATA:
					return (True, self.index2time(path, index + i))
			# move index
			index += bsize
		if isize == max_probes:
			return (True, -1)
		else:
			return (False, self.index2time(self.next_file(path)))


	def next_file(self, path):
		(dirname, base) = os.path.split(path)
		year = int(base[0:4])
		month = int(base[4:6]) + 1
		if month > 12:
			month = 1
			year += 1
		return os.path.join(dirname, '%04d%02d%s' % (year, month, self.SZBCACHE_EXT))


	"""
	Returns absolute file name and index for given time and params directory.
	"""
	def time2index(self, t, dirpath):
		ts = time.gmtime(t)
		path = "%s/%04d%02d%s" % (dirpath, ts.tm_year, ts.tm_mon, self.SZBCACHE_EXT)
		index = (ts.tm_mday - 1) * 24 * 3600 + ts.tm_hour * 3600 + ts.tm_min * 60 + ts.tm_sec
		index = index // self.SZBCACHE_PROBE
		print path, index
		return (path, index)

	"""
	Returns time_t for given file and index. If index - 1, look for last index in file.
	"""
	def index2time(self, path, index = 0):
		if index == -1:
			index = os.path.getsize(path) / self.SZBCACHE_SIZE
		(dirname, filename) = os.path.split(path)
		print "FILENAME:", filename
		year = int(filename[0:4])
		month = int(filename[4:6])
		return timegm((year, month, 1, 0, 0, 0)) + index * self.SZBCACHE_PROBE

