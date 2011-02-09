#!/usr/bin/python
#-*- coding: utf-8 -*-
"""
  Text version of Reporter3 - replacing rap.txt

  SZARP: SCADA software 
  Adam Smyk <asmyk@praterm.com.pl>

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

import curses, os, sys, traceback, unicodedata, threading, string, locale
# To allow importing SZARP modules
sys.path.append("/opt/szarp/lib/python")
from libpar import LibparReader

class gb:
	"""
	Global variables.
	"""
	scrn = None # window object
	cmdoutlines = [] # output 
	winrow = None # current row position in screen
	startrow = None # index of first row in cmdoutlines to be displayed
	reports = [] # list of reports displayed = menu - whichmenu = 0
	areports = [] # list of all available reports = menu - whichmenu = 0
	filter = "" # just a filter for reports, it is activated when '/' is pressed
	params = [] # list of parameters - whichmenu = 1
	whichmenu = 0 # which menu is on the screen now
	activereport = None # active report name
	hostname = None # host name
	portnumber = None # paramd 
	user = None # user name
	passwd = None # password
	fileencoding = locale.getpreferredencoding()
	if not fileencoding:
		fileencoding = "UTF-8"
	fileencoding1 = "UTF-8"
	ti = None # timer object
	timeinterval = 10 # screen refresh interval, if <=0 automatic screen refresh is off

def getLocalParamdPort():
	"""
	Returns local paramd port.
	"""
	lp = LibparReader()
	port = lp.get('paramd_for_raporter', 'port')
	if not port: 
		port = "8081"
	return port

def converseRaportName(arg1):
	"""
	Returns converse to raport name from unicode to ascii.
	All spaces are replaced by underlined character.
	"""
	from string import maketrans
	arg2 = arg1.translate(maketrans(" ","_"))
	arg2 = arg2.replace('Ł','L')
	arg2 = arg2.replace('ł','l')
	return  unicodedata.normalize('NFKD', unicode(arg2,gb.fileencoding)).encode('ascii','ignore')
	
def readReportsFromParamd(hostname):
	"""
	Returns reports list received from paramd server working on given hostname.
	"""
	from urllib import urlopen
	try:
		raportPath = "http://"+hostname+"/xreports"
		doc = urlopen(raportPath).read()
	except:
		restoreScreen()
		print "Cannot find host: " + hostname + " or list of reports is unavailable. Check if paramd is running."
		#traceback.print_exc()
		sys.exit(-1)
	from xml.dom import minidom
	try:
		domtree = minidom.parseString(doc)
	except:
		restoreScreen()
		print "Invalid xml data from paramd - cannot parse. Please update your paramd."
		#traceback.print_exc()
		sys.exit(-1)
	nodes = domtree.childNodes
	del gb.params
	del gb.reports
	gb.reports = []
	gb.params = []
	gb.reports.append("Available reports: [/] - filter reports  [q or enter here] - quit")	
	s = set()
	for node in nodes[0].getElementsByTagName("list:raport"):
		s.add(node.getAttribute("name").encode(gb.fileencoding))
	for element in s:
		gb.reports.append(element)
	del domtree
	del doc
	return gb.reports


def readParamsFromParamd(hostname, raportname):
	"""
	Returns data for 'raportname' received from paramd server working on hostname.
	"""
	from urllib import urlopen
	try:
		raportPath = "http://"+hostname+"/xreports?title="+converseRaportName(raportname)
		doc = urlopen(raportPath).read()
	except:
		restoreScreen()
		print "Cannot find host: " + hostname + " or report "+converseRaportName(raportname)+" is unavailable. Check if paramd is running."
		#traceback.print_exc()
		sys.exit(-1)
	from xml.dom import minidom
	try:
		domtree = minidom.parseString(doc)
	except:
		restoreScreen()
		print "Invalid xml data from paramd - cannot parse. Please update your paramd."
		#traceback.print_exc()
		sys.exit(-1)		
	nodes = domtree.childNodes
	del gb.params
	gb.params = []    
	gb.params.append("Available parameters in "+raportname+" [enter here]-back [R/r]-refresh")
	for node in nodes[0].getElementsByTagName("list:param"):
		parametr = node.getAttribute("name")
		l=parametr.rfind(":")
		parametr = parametr[l+1:]
		text = parametr+" "+node.getAttribute("rap:short_name")+" = "+node.getAttribute("rap:value")+" "+node.getAttribute("rap:unit")
		gb.params.append(text.encode(gb.fileencoding))
	del domtree
	del doc
	return gb.params

def autoRefreshInit():
	"""
	Initializes automatic screen refresh
	"""	
	if gb.timeinterval<=0: return
	try:
		gb.ti = threading.Timer(gb.timeinterval, autoRefreshDataFromParamd)
		gb.ti.start()
	except:
		restoreScreen()
		traceback.print_exc()
		sys.exit(-1)		
	
def autoRefreshDataFromParamd():
	"""
	Performes automatic screen refresh 
	"""	
	if gb.timeinterval<=0: return
	gb.cmdoutlines = readParamsFromParamd(gb.hostname+":"+gb.portnumber, gb.activereport)
	updateScreen()
	del gb.ti
	gb.ti = threading.Timer(gb.timeinterval, autoRefreshDataFromParamd)
	gb.ti.start()

def autoRefreshStop():
	"""
	Stops automatic screen refresh
	"""	
	if gb.timeinterval<=0: return
	gb.ti.cancel();
	del gb.ti

def updateScreen():
	"""
	Refreshes screen
	"""
	ncmdlines = len(gb.cmdoutlines)
	if ncmdlines < curses.LINES:	nwinlines = ncmdlines
	else:				nwinlines = curses.LINES - 1
	lastrow = gb.startrow + nwinlines 
	gb.scrn.clear()
	i=0
	for ln in gb.cmdoutlines[gb.startrow:lastrow]:
		gb.scrn.addstr(i,0,ln)
		i += 1
	gb.scrn.addstr(gb.winrow,0,gb.cmdoutlines[gb.winrow+gb.startrow],curses.A_BOLD)
	gb.scrn.refresh()

def goUpByOne():
	"""
	Refreshes screen when key UP is pressed
	"""
	if gb.winrow > 0: #in the centre of screen
		gb.scrn.addstr(gb.winrow,0,gb.cmdoutlines[gb.startrow+gb.winrow])
		gb.winrow = gb.winrow - 1		
		ln = gb.cmdoutlines[gb.startrow+gb.winrow]
		gb.scrn.addstr(gb.winrow,0,ln,curses.A_BOLD)
		gb.scrn.refresh()	
		return		
	if gb.startrow - 1 < 0: return
	gb.startrow = gb.startrow - 1
	gb.winrow = 0
	ncmdlines = len(gb.cmdoutlines)
	if ncmdlines < curses.LINES:	nwinlines = ncmdlines
	else:				nwinlines = curses.LINES - 1
	lastrow = gb.startrow + nwinlines 
	gb.scrn.clear()
	for ln in gb.cmdoutlines[gb.startrow:lastrow]:
		gb.scrn.addstr(gb.winrow,0,ln)
		gb.winrow += 1
	gb.winrow = 0
	gb.scrn.addstr(gb.winrow,0,gb.cmdoutlines[gb.startrow],curses.A_BOLD)
	gb.scrn.refresh()


def goDownByOne():
	"""
	Refreshes screen when key DOWN is pressed
	"""
	ncmdlines = len(gb.cmdoutlines)
	if ncmdlines < curses.LINES - 1:
		nwinlines = ncmdlines
		if gb.winrow + 1 >= ncmdlines: return
	else:				nwinlines = curses.LINES - 1 
	if gb.winrow < curses.LINES - 2: 
		gb.scrn.addstr(gb.winrow,0,gb.cmdoutlines[gb.startrow+gb.winrow])
		gb.winrow = gb.winrow + 1		
		ln = gb.cmdoutlines[gb.startrow+gb.winrow]
		gb.scrn.addstr(gb.winrow,0,ln,curses.A_BOLD)
		gb.scrn.refresh()		
		return		
	if gb.startrow + nwinlines  >= ncmdlines: return
	gb.startrow = gb.startrow + 1
	lastrow = gb.startrow + nwinlines 
	gb.scrn.clear()
	gb.winrow = 0
	for ln in gb.cmdoutlines[gb.startrow:lastrow]:
		gb.scrn.addstr(gb.winrow,0,ln)
		gb.winrow += 1
	gb.winrow -= 1;
	gb.scrn.addstr(gb.winrow,0,gb.cmdoutlines[lastrow-1],curses.A_BOLD)
	gb.scrn.refresh()

def filteringReports():
	"""
	Performs filtering operation. Filtering is active when gb.filter!=""
	"""
	s = gb.filter
	gb.scrn.addstr(curses.LINES-1,0,"                                                                           ")
	gb.scrn.addstr(curses.LINES-1,0,"Report prefix: "+gb.filter)
	gb.scrn.refresh() 
	while True:
		try:
			c = gb.scrn.getch()
			cr=chr(c)
			if cr==chr(10):
				updateScreen()
				gb.scrn.refresh()
				return
			if cr==chr(27):
				updateScreen()
				gb.scrn.refresh()
				return
			if cr==chr(127):
				if len(s)>0:
					s=s[:len(s)-1]
			else:
				s=s+cr
			gb.filter = s
			del gb.cmdoutlines
			gb.cmdoutlines = doFiltering(gb.filter)
			if len(gb.cmdoutlines)>1:
				gb.winrow=1	
			else:
				gb.winrow=0
			updateScreen()
			gb.scrn.addstr(curses.LINES-1,0,"                                                                           ")
			gb.scrn.addstr(curses.LINES-1,0,"Report prefix: "+s)
			gb.scrn.refresh()
		except:
			restoreScreen()
			traceback.print_exc()
			sys.exit(1)

def copyReports():
	del gb.areports
	gb.areports=[]
	for ln in gb.reports:
		gb.areports.append(ln)
	
def doFiltering(prefix):
	del gb.reports
	gb.reports=[]
	i=0;
	for ln in gb.areports:
		if i==0: 
			gb.reports.append(ln)
			i=1
		else:
			if ln.startswith(prefix): 
				gb.reports.append(ln)
	gb.winrow = 0
	gb.startrow = 0
	return gb.reports


def main():
	"""
	Main function
	"""
	gb.scrn = curses.initscr()
	curses.noecho()
	curses.cbreak()
	gb.startrow = 1
	goUpByOne();
	try:
		copyReports()
	except:
		restoreScreen()
		traceback.print_exc()

	while True:
		c = gb.scrn.getch()
		c = chr(c)		
		if   c[0] == "A": 	goUpByOne();
		elif c[0] == "B":	goDownByOne();
		elif c == "\n": 
			if gb.whichmenu == 0 and gb.startrow == 0 and gb.winrow == 0: break
			elif gb.whichmenu == 0 :
				gb.whichmenu = 1 
				gb.activereport = gb.cmdoutlines[gb.winrow+gb.startrow]
				gb.cmdoutlines = readParamsFromParamd(gb.hostname+":"+gb.portnumber, gb.activereport)
				gb.startrow = 1
				gb.winrow = 0
				goUpByOne()
				autoRefreshInit()

			elif gb.whichmenu == 1 and gb.startrow == 0 and gb.winrow == 0:
				gb.whichmenu = 0 
				autoRefreshStop()
				gb.cmdoutlines = gb.reports
				gb.startrow = 1
				gb.winrow = 0
				goUpByOne()
		elif c == "r" or c == "R": 
			if gb.whichmenu == 1:	
				gb.cmdoutlines = readParamsFromParamd(gb.hostname+":"+gb.portnumber, gb.activereport)
				updateScreen()
		elif c == "q": 
			autoRefreshStop()
			break
		elif c == "/" and gb.whichmenu == 0:
			filteringReports()
	restoreScreen()

def restoreScreen():
	"""
	Restores screen to default
	"""
	if gb.scrn != None: 
		curses.nocbreak()
		curses.echo()
		curses.endwin()

def getHostName(argv):
	"""
	Returns name of the host where paramd is running 
	"""
	for i in range(1, len(argv)-1):
		if argv[i]=="-h":
			return argv[i+1]
	return "localhost"		

def getPortNumber(argv):
	"""
	Returns port number of the paramd  
	"""
	for i in range(1, len(argv)-1):
		if argv[i]=="-p":
			return argv[i+1]
	return getLocalParamdPort()

def getTimeInterval(argv):
	"""
	Returns time interval for automatic screen refresh
	"""
	for i in range(1, len(argv)-1):
		if argv[i]=="-t":
			try:
				val = string.atoi(argv[i+1], base=10)
			except:
				print "Invalid time interval"
				sys.exit(-1)
			return val
	return 10

if __name__ == "__main__":
	locale.setlocale(locale.LC_ALL, '')
	if len(sys.argv) == 2 and sys.argv[1]=="--help":
		print "usage: raporter.py [-p port] [-h hostname] [-t timeinterval] [--help]"
		print "where:"
		print "\t port - port number, if not available, it will be read from szarp.cfg"
		print "\t hostname - hostname where paramd is running, default is localhost"
		print "\t timeinterval - screen refresh parameter, default is 10 seconds"
		print "\t                if timeinterval<=0 then refreshing is off "
		sys.exit(1)
	if len(sys.argv) % 2 == 0:
		print "usage: raporter.py [-p port] [-h hostname] [-t timeinterval] [--help]"
		sys.exit(1)

	gb.hostname = getHostName(sys.argv)
	gb.portnumber = getPortNumber(sys.argv)
	gb.timeinterval = getTimeInterval(sys.argv)
	gb.cmdoutlines = readReportsFromParamd(gb.hostname+":"+gb.portnumber)
	try:
		main()
	except:
		restoreScreen()
		#traceback.print_exc()
