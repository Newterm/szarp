#!/usr/bin/python

#
# Script creates samples for szbwriter. See usage funcion for more information :)
#

import getopt
import string
import sys
import datetime
import time
import random

def usage():

	m ="""All listed parameters are required:
--name="Grup:Union:Parameter"
	converted name like in SzarpBase
		Example: Legionowo:wezel:temp1
--begin-date="YYYY-MM-DD"
	Set begin date for data. 
		Example: "2011-05-14"

Optional parameters:
--help
--usage
	Show this message and end script
--days=NUM
	Hom many data for NUM days create -> "End-date" = begin-date + days
		Example: 5
		Default: 30
--percent=NUM
	How much percent of data create (0 < NUM < 100) 
		Example: 90
		Default: 80
--type=[ float | int ]
	Type of data
		Example: int
		default: float
--min=NUM
	Minimal value for data, Examples:
		5
		3.14
	Default: 0 for int ,  0.00 for float
--max=NUM
	Max value for data, Examples:
		127
		128.0
	Default: 100 for int, 100.00 for float
--step=NUM
	Step beetween two samples
	Default 1 for int 1.00 for float

Usage examples:
datagen.py --name="Leg:Union1:temp" --begin-date="2012-12-24"
datagen.py --name="AX:Wezel:temp" --begin-date="2012-03-22" --days=35 --percent=80 --type=int
"""
	print m

def main():

	args = sys.argv[1:]
	if "--help" in args or "--usage" in args:
		usage()
		sys.exit(0)

	try:
		optlist, args = getopt.getopt(args,'x', ['name=','begin-date=','days=','percent=','type=','min=','max=','step='])

	except getopt.GetoptError:
		usage()
		sys.exit(2)

	# default values, if you edit them change usage()

	name = ""
	beginDate=""
	days=30
	percent=80
	typeData="float"
	minVal=0.00
	maxVal=100.00
	step=1.00

	# process parameters
	for k, v in optlist:
		if k == '--name':
			name = v
		if k == '--begin-date':
			beginDate = v
		if k == '--days':
			days = v
		if k == '--percent':
			percent = int(v)
		if k == '--type':
			typeData = v
		if k == '--min':
			minVal = v
		if k == '--max':
			maxVal = v
		if k == '--step':
			step = v

	# quick check parameters
	if (name == "" or beginDate == "") or ( typeData != "float" and typeData != "int" ):
		usage()
		sys.exit(2)

	if typeData == "float":
		minVal = float (minVal)
		maxVal = float (maxVal)
		step = float (step)
	else:
		minVal = int (minVal)
		maxVal = int (maxVal)
		step = int (step)

	print minVal, maxVal

	# create begin/end date
	tmp = beginDate.split('-')
	beginTime = datetime.datetime(int (tmp[0]), int (tmp[1]), int (tmp[2]))
	difference = datetime.timedelta(days=int (days))
	endTime   = beginTime + difference

	# max namber of samles
	maxData = int (days) * 6 * 24
	maxData = int ( (maxData * percent) / 100 )

	# time in seconds
	sBeginTime = time.mktime(beginTime.timetuple())
	sEndTime = time.mktime(endTime.timetuple())

	data = set()
	random.seed()

	# collect data
	while len(data) < maxData:
		r = random.uniform(sBeginTime, sEndTime)
		t = datetime.datetime.fromtimestamp(r).strftime('%Y %m %d %H %M')
		data.add(t)

	# init begin value
	if typeData == "float":
		oldValue = random.uniform(minVal,maxVal)
	else:
		oldValue = random.randint(minVal,maxVal)

	# print data
	for i in sorted(data):
		print '"' + name +'"', i, 
		if (typeData == "int"):
			print oldValue
		else:
			print round(oldValue,2)

		# make step
		if ( random.randint(0,1) == 0):
			tmp = oldValue + step
		else:
			tmp = oldValue - step
		
		if (minVal <= tmp and tmp <= maxVal):
			oldValue = tmp

if __name__ == "__main__":
    main()
