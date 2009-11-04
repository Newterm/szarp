#!/usr/bin/python

# Script for manipulating values in szarp-base files.
# Pass files to modify on command line.

import array
import sys

for file in sys.argv[1:]:
	data = array.array('h')	# array of signed 2 bytes short integers
	print file
	f = open(file, "r")
	try:
		data.fromfile(f, 10000)
	except EOFError:
		pass
	f.close()
	for i in range(0, len(data)):
		if data[i] == -32768:
			continue
		data[i] *= 10	# operation to perform
	f = open(file, "w")
	data.tofile(f)
	f.close()
	
