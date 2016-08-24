#!/usr/bin/python
"""
  SZARP: SCADA software 
  Pawel Palucha <pawel@praterm.com.pl>

  Twisted-based server for serving values of SZARP 10-seconds probes cache.
  See ProbesProtocol class for protocol description.

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
from zope.interface import implements
from twisted.internet import protocol, reactor, interfaces
from twisted.protocols import basic
from twisted.python.log import msg

# To allow importing SZARP modules
import sys
sys.path.append("/opt/szarp/lib/python")

import szbcache
from libpar import LibparReader

def debug(s):
	"""
	"""
	# Uncomment following line to turn on debug
	# print "DEBUG:", s

class ErrorCodes():
	BadRequest = 400

class Producer():
	"""
	Abstract producer class, common for all server producers.
	"""
	implements(interfaces.IPushProducer)
	def __init__(self, proto):
		self._proto = proto
		self._paused = True

	def pauseProducing(self):
		self._paused = True

	def resumeProducing(self):
		self._paused = False
	
	def stopProducing(self):
		pass

	def finish(self):
		self._paused = True
		self._proto.transport.unregisterProducer()
		self._proto.nextProducer()

class ErrorProducer(Producer):
	"""
	Producer writing error message.
	"""
	def __init__(self, proto, code, msg):
		Producer.__init__(self, proto)
		self._code = code
		self._msg = msg

	def resumeProducing(self):
		Producer.resumeProducing(self)
		self._proto.transport.write("ERROR %d %s\n" % (self._code, self._msg))
		self.finish()

class SearchProducer(Producer):
	"""
	Producer creating response for SEARCH request.
	"""
	def __init__(self, proto, start, end, direction, param):
		Producer.__init__(self, proto)
		self._start = start
		self._end = end
		self._direction = direction
		self._param = param
		debug("SEARCH PRODUCER CREATE: start %s end %s, direction %d" %
				(szbcache.format_time(start), szbcache.format_time(end), direction))

	def resumeProducing(self):
		Producer.resumeProducing(self)
		debug("SEARCH PRODUCER RESUME")
		try:
			ret = self._proto.factory.szbcache.search(self._start, self._end, self._direction, self._param)
			self._proto.transport.write("%d %d %d\n" % ret)
		except szbcache.SzbException, e:
			self._proto.transport.write("ERROR %d %s\n" % (ErrorCodes.BadRequest, str(e)))
		debug("SEARCH PRODUCER FINISH")
		self.finish()

class RangeProducer(Producer):
	"""
	Producer creating respons for RANGE request
	"""
	def __init__(self, proto):
		Producer.__init__(self, proto)

	def resumeProducing(self):
		Producer.resumeProducing(self)
		try:
			ret = self._proto.factory.szbcache.available_range()
			self._proto.transport.write("%d %d\n" % ret)
		except szbcache.SzbException, e:
			self._proto.transport.write("ERROR %d %s\n" % (ErrorCodes.BadRequest, str(e)))
		self.finish()


class GetProducer(Producer):
	"""
	Producer creating response for GET request.
	"""
	implements(interfaces.IPushProducer)
	def __init__(self, proto, start, end, param):
		Producer.__init__(self, proto)
		self._start = start
		self._end = end
		self._param = param
		self._proto = proto
		self._paused = False
		self._size, self._last = self._proto.factory.szbcache.get_size_and_last(
				self._start, self._end, self._param)

	def pauseProducing(self):
		self._paused = True

	def resumeProducing(self):
		self._paused = False
		if not self._size is None:
			self._proto.transport.write("%d %d\n" % (self._last, self._size))
			if self._size == 0:
				self.finish()
				debug("GET PRODUCER FINISH")
				return
			self._size = None
		while not self._paused and self._start <= self._end:
			self._start = self._proto.factory.szbcache.write_data(
					self._start, self._end, self._param, self._proto.transport)
		if self._start >= self._end:
			self.finish()
			debug("GET PRODUCER FINISH")
	
	def stopProducing(self):
		pass

class ProbesProtocol(basic.LineReceiver):
	"""
	Parses request line, returns apropriate response producer.
	Protocol description:
	There are three kinds of requestes, SEARCH, GET and RANGE.

	SEARCH request:

	SEARCH start_time end_time direction param_path\r\n

	where:
	start_time is search starting time as time_t,
	end_time is search finish time (inclusive) as time_t
	direction is search direction: -1 for left (start_time > end_time), 0 for in-place, 1 for right.
	param_path is path to parameter directory relative to main cache directory
	You can pass -1 as start_time and/or end_time, it means searching from/to all range.

	Response for SEARCH request is:

	found_time first_time last_time\r\n

	where found_time is -1 if search failed, or date/time of data found as time_t, first_time
	and last_time are date/time of first/ast available data or -1 if no data for parameter
	is available

	GET request:

	GET start_time end_time param_path\r\n

	where:
	start_time and end_time (as time_t) is inclusive range of data to fetch,
	param_path is path to parameter to fetch.

	Response for GET request is:

	last_time size\r\n
	data

	Where:
	- last_time is time of last available probe for parameter as time_t
	- size is size (in bytes) of following data
	- data is raw values of fetched parameter as little-endian short int (16 bits) numbers

	RANGE request:

	RANGE\r\n

	Reponse for RANGE request is:
	first_time last_time\r\n

	Where:
	- first_time is approximate date/time of first globally available probe as time_t
	- last_time is approximate date/time of last globally available probe as time_t

	Optionally, response for client request may be:

	ERROR code description\r\n

	It denotes error with specified code and description.
	"""
	def parse(self, line):
		"""
		Parse request line.
		"""
		line = line.strip()
		if line.startswith('GET '):
			return self.parseGET(line)
		elif line.startswith('SEARCH '):
			return self.parseSEARCH(line)
		elif line.startswith('RANGE'):
			return self.parseRANGE(line)
		else:
			return ErrorProducer(self, 
					ErrorCodes.BadRequest, 
					"Incorrect request, must be GET or SEARCH")

	def parseGET(self, line):
		"""
		Parse GET request.
		"""
		(req, start, end, path) = line.split(None, 3)
		try:
			start = int(start)
			end = int(end)
			if start > end:
				raise ValueError()
		except ValueError:
			return ErrorProducer(self, ErrorCodes.BasRequest, "Incorrect start/end time")
		return GetProducer(self, start, end, path)

	def parseSEARCH(self, line):
		"""
		Parse SEARCH request.
		"""
		try:
			(req, start, end, direction, path) = line.split(None, 4)
		except ValueError:
			return ErrorProducer(self, ErrorCodes.BadRequest, "Not enough request tokens")
		try:
			start = int(start)
			if start < 0 and start != -1:
				raise ValueError()
		except ValueError:
			return ErrorProducer(self, ErrorCodes.BadRequest, "Incorrect start time")
		try:
			end = int(end)
			if end <= 0 and end != -1:
				raise ValueError()
		except ValueError:
			return ErrorProducer(self, ErrorCodes.BadRequest, "Incorrect end time")
		try:
			direction = int(direction)
		except ValueError:
			return ErrorProducer(self, ErrorCodes.BadRequest, "Incorrect direction")
		return SearchProducer(self, start, end, direction, path)

	def parseRANGE(self, line):
		"""
		Parse RANGE request.
		"""
		req = line.strip()
		if req != "RANGE":
			return ErrorProducer(self, ErrorCodes.BasRequest, "Unnecesarry arguments for RANGE request")
		return RangeProducer(self)

	def connectionMade(self):
		debug('connection made from %s' % (self.transport.getPeer()))
		# queue of producers for not-yet-served requests
		self._requests = []
		self._producer = None

	def nextProducer(self):
		"""
		One producer finished, run another
		"""
		if len(self._requests) > 0:
			self._producer = self._requests.pop(0)
			debug("STARTING PRODUCER %s" % (self._producer))
			self.transport.registerProducer(self._producer, True)
			self._producer.resumeProducing()
		else:
			self._producer = None

	def addRequest(self, producer):
		"""
		Called when new request line was received. Adds request to the queue
		and start serving it if no other is currently served.
		"""
		self._requests.append(producer)
		if self._producer is None:
			self.nextProducer()

	def lineReceived(self, line):
		"""
		Callback called when line was received.
		"""
		self.addRequest(self.parse(line))
		
	def connectionLost(self, reason):
		debug('connection lost from %s' % (self.transport.getPeer()))


class ProbesFactory(protocol.ServerFactory):
	protocol = ProbesProtocol

	def __init__(self):
		self.szbcache = szbcache.SzbCache()

	def getCacheDir():
		return self.cachedir

def get_port_address():
	"""
	Return (port, interface) tupple for reactor.listenTCP or internet.TCPServer.
	"""
	lpr = LibparReader()
	try:
		port = int(lpr.get("probes_server", "port"))
	except ValueError:
		raise RuntimeError("missing 'port' parameter in 'probes_server' section of szarp.cfg file")
	address = lpr.get("probes_server", "address")
	return (port, address)

if __name__ == "__main__":
	port, addr = get_port_address()

	reactor.listenTCP(port, ProbesFactory(), interface=addr)
	reactor.run()

