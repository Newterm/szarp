#!/usr/bin/python
# SZARP: SCADA software 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# $Id$
#
# 2011 Krzysztof Golinski
#
# Script creates samples for szbwriter. See usage funcion for more information.
#

import string
import datetime
import time
import random
import optparse

def main():

	parser = optparse.OptionParser()
	parser.add_option('-n', '--name', help='REQUIRED. Converted name like in SzarpBase: "Group:Unit:Parameter"',dest='name')
	parser.add_option('-b', '--begin-date', help='REQUIRED. Set begin date for data: "YYYY-MM-DD"', dest='beginDate')
	parser.add_option('-d', '--days', help='create data for DAYS days. Default value [%default]',
		dest='days', default=30)
	parser.add_option('-s', '--step', help='Step between two samples. Default value [%default]', dest='step', default=1)
	parser.add_option('-m', '--min', help='Minimal value for data. Default value [%default]', dest='minVal', default=0)
	parser.add_option('-M', '--max', help='Max value for data. Default value [%default]', dest='maxVal', default=100)
	parser.add_option('-p', '--percent', help='How much percent of data create (0 < PERCENT < 100). Default value [%default]', dest='percent', default=80)
	parser.add_option('-t', '--type', help='Type of data [ float | int ]. Default value [%default]', dest='type', default='float')

	(opts, args) = parser.parse_args()

	# check required options
	if opts.name is None or opts.beginDate is None:
		print 'Required option is missing'
		parser.print_help()
		exit(-1)

	if opts.type == "float":
		opts.minVal = float (opts.minVal)
		opts.maxVal = float (opts.maxVal)
		opts.step = float (opts.step)
	else:
		opts.minVal = int (opts.minVal)
		opts.maxVal = int (opts.maxVal)
		opts.step = int (opts.step)

	# create begin/end date
	tmp = opts.beginDate.split('-')
	beginTime = datetime.datetime(int (tmp[0]), int (tmp[1]), int (tmp[2]))
	difference = datetime.timedelta(days=int (opts.days))
	endTime   = beginTime + difference

	# max number of samples
	maxData = int (opts.days) * 6 * 24
	maxData = int ( (maxData * int (opts.percent) ) / 100 )

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
	if opts.type == "float":
		oldValue = random.uniform(opts.minVal,opts.maxVal)
	else:
		oldValue = random.randint(opts.minVal,opts.maxVal)

	# print data
	for i in sorted(data):
		print '"' + opts.name +'"', i,
		if opts.type == "int":
			print oldValue
		else:
			print round(oldValue,2)

		# make step
		if  random.randint(0,1) == 0:
			tmp = oldValue + opts.step
		else:
			tmp = oldValue - opts.step
		
		if opts.minVal <= tmp and tmp <= opts.maxVal:
			oldValue = tmp

if __name__ == "__main__":
	main()
