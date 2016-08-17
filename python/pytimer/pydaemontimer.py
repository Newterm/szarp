#!/usr/bin/python2.7

from threading import Timer
from time import sleep

class ReTimer:
	def __init__(self, t, f, args = []):
		self._func = f
		self._args = args
		self._period = t
		self._lock = 0

	def _call(self):
		while self._lock:
			pass

		self._timer = Timer(self._period, self._call).start()
		self._lock = 1
		if self._args == []:
			self._func()
		else:
			self._func(self._args)
		self._lock = 0

	def go(self):
		self._call()
	

if __name__ == "__main__":
	def hello():
		print "Hello"

	ReTimer(1, hello).go()
