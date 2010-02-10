from zope.interface import implements

from twisted.internet import protocol, reactor, interfaces
from twisted.protocols import basic
import time
import os

import szbcache

class ErrorCodes():
	BadRequest = 400

class Producer():
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
	def __init__(self, proto, code, msg):
		Producer.__init__(self, proto)
		self._code = code
		self._msg = msg

	def resumeProducing(self):
		Producer.resumeProducing(self)
		self._proto.transport.write("%d %s\n" % (self._code, self._msg))
		self.finish()

class SearchProducer(Producer):
	def __init__(self, proto, start, end, direction, param):
		Producer.__init__(self, proto)
		self._start = start
		self._end = end
		self._direction = direction
		self._param = param

	def resumeProducing(self):
		Producer.resumeProducing(self)
		try:
			ret = self._proto.factory.szbcache.search(self._start, self._end, self._direction, self._param)
			print "RET:", ret
			self._proto.transport.write("%d\n" % (ret))
		except szbcache.SzbException, e:
			self._proto.transport.write("%d %s\n" % (ErrorCodes.BadRequest, str(e)))
		self.finish()

class GetProducer:
	implements(interfaces.IPushProducer)
	def __init__(self, proto, data):
		self._proto = proto
		self._paused = False
		self._data = data
		path = '/home/pawel/ftp/debian-503-i386-netinst.iso'
		self._f = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
		self._size = os.fstat(self._f).st_size
	def pauseProducing(self):
		self._paused = True
		#print('pausing connection from %s' % (self._proto.transport.getPeer()))

	def resumeProducing(self):
		self._paused = False
		if not self._size is None:
			self._proto.transport.write("%d\n" % self._size)
			self._size = None
		while not self._paused:
			buf = os.read(self._f, 64000)
			if buf == '':
				self.finish()
				os.close(self._f)
				break
			else:
				self._proto.transport.write(buf)
	
	def stopProducing(self):
		os.close(self._f)

class ProbesProtocol(basic.LineReceiver):
	"""
	Parses request line, returns apropriate response producer.
	"""
	def parse(self, line):
		line = line.strip()
		if line.startswith('GET '):
			return self.parseGET(line)
		elif line.startswith('SEARCH '):
			return self.parseSEARCH(line)
		else:
			return ErrorProducer(self, 
					ErrorCodes.BadRequest, 
					"Incorrect request, must be GET or SEARCH")

	def parseGET(self, line):
		(req, start, end, path) = line.split(None, 3)
		return path

	def parseSEARCH(self, line):
		try:
			(req, start, end, direction, path) = line.split(None, 4)
		except ValueError:
			return ErrorProducer(self, ErrorCodes.BadRequest, "Not enough request tokens")
		try:
			start = int(start)
			if start < 0:
				raise ValueError()
			start = start - start % 10
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
		if end != -1:
			if direction < 0 and start < end:
				return ErrorProducer(self, ErrorCodes.BadRequest, "Direction < 0 and start < end")
			elif direction > 0 and start > end:
				return ErrorProducer(self, ErrorCodes.BadRequest, "Direction > 0 and start > end")

		return SearchProducer(self, start, end, direction, path)

	def connectionMade(self):
		print('connection made from %s' % (self.transport.getPeer()))
		# queue of producers for not-yet-served requests
		self._requests = []
		self._producer = None

	"""
	One producer finished, run another
	"""
	def nextProducer(self):
		if len(self._requests) > 0:
			print "STARTING PRODUCER"
			self._producer = self._requests.pop(0)
			self.transport.registerProducer(self._producer, True)
			self._producer.resumeProducing()
		else:
			self._producer = None

	"""
	Called when new request line was received. Adds request to the queue
	and start serving it if no other is currently served.
	"""
	def addRequest(self, producer):
		self._requests.append(producer)
		if self._producer is None:
			self.nextProducer()

	def lineReceived(self, line):
		print "LINE:", line

		self.addRequest(self.parse(line))
		
	def connectionLost(self, reason):
		print('connection lost from %s' % (self.transport.getPeer()))


class ProbesFactory(protocol.ServerFactory):
	protocol = ProbesProtocol

	def __init__(self):
		self.szbcache = szbcache.SzbCache()

	def getCacheDir():
		return self.cachedir




reactor.listenTCP(8090, ProbesFactory())
reactor.run()


