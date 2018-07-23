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

import curses, os, sys, traceback, unicodedata, threading, string, locale, time
# To allow importing SZARP modules
sys.path.append("/opt/szarp/lib/python")
from libpar import LibparReader

NODATA = -32768 

import zmq
import paramsvalues_pb2

class Param:
	def __init__(self, param):
		self.short_name = param.attrib['short_name'].encode(gb.fileencoding)
		self.value = NODATA
		self.unit = param.get('unit', "").encode(gb.fileencoding)
		self.prec = int(param.get('prec', 0))

	def getValue(self):
		sv = str(self.value)
		if self.prec == 0:
			return (sv).encode(gb.fileencoding)
		
		while len(sv) < self.prec + 1:
			sv = '0'+sv

		return (sv[0:-self.prec]+"."+sv[-self.prec:]).encode(gb.fileencoding)

	def getString(self):
		return self.short_name+" = "+self.getValue()+" "+self.unit

class Report:
	def __init__(self):
		self.params = []

			
class Params:
	def __init__(self):
		self.params = {} # name : Param
		self.reports = {} # title : Report
		self.indexes = {} # index : name
		from lxml import etree
		if gb.base is not None:
			tree = etree.parse("/opt/szarp/"+gb.base+"/config/params.xml")
		else:
			tree = etree.parse("/etc/szarp/default/config/params.xml")

		params2 = tree.xpath('//x:param',namespaces={'x': 'http://www.praterm.com.pl/SZARP/ipk'})
		reports2 = tree.xpath('//x:raport',namespaces={'x': 'http://www.praterm.com.pl/SZARP/ipk'})
		for r in reports2:
			par_elem = r.getparent()
			nm = par_elem.attrib['name'].encode(gb.fileencoding)
			if nm not in self.params.keys():
				self.params[nm] = Param(par_elem)
				self.indexes[params2.index(par_elem)] = nm
			rm = r.attrib['title'].encode(gb.fileencoding)
			if rm not in self.reports.keys():
				self.reports[rm] = Report()
			self.reports[rm].params.append(nm)

		self.initializeZmq()# might do this async-style

	def initializeZmq(self):
		if gb.hostname != "localhost":
			self.hub_uri = "tcp://"+gb.hostname+":"+gb.portnumber
		else:
			self.hub_uri = LibparReader().get("parhub", "pub_conn_addr")

		self.context = zmq.Context(1)
		self.socket = self.context.socket(zmq.SUB)

		self.socket.connect(self.hub_uri)
		self.socket.setsockopt(zmq.SUBSCRIBE, "")
		self.poller = zmq.Poller()
		gb.sz4_ti = threading.Timer(1, update_zmq)
		gb.sz4_ti.start()

	def closeZmq(self):
		try:
			gb.sz4_ti.cancel();
			del gb.sz4_ti
		except:
			pass
	
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
	sz4_ti = None # timer object
	timeinterval = 10 # screen refresh interval, if <=0 automatic screen refresh is off
	sz4 = False
	sz4_params = None
	base = None


def update_zmq():
	gb.sz4_params.poller.register(gb.sz4_params.socket, zmq.POLLIN)
	gb.sz4_params.poller.poll(10)
	read_socket()
	gb.sz4_ti = threading.Timer(gb.timeinterval, update_zmq)
	gb.sz4_ti.start()
	
def read_socket():
	msgs = {}
	try:
		msg = gb.sz4_params.socket.recv(zmq.NOBLOCK)
		params_values = paramsvalues_pb2.ParamsValues.FromString(msg)
		for param_value in params_values.param_values:
			index = param_value.param_no
			if index in gb.sz4_params.indexes.keys():
				param = gb.sz4_params.params[gb.sz4_params.indexes[index]]
				prec = 10**param.prec
				try:
					param.value = int(param_value.int_value) + int(param_value.double_value*prec) + int(param_value.float_value*prec)
				except Exception as e:
					param.value = NODATA
					
			else:
				pass
	except zmq.ZMQError as e:
		if e.errno != zmq.EAGAIN:
			raise


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
	arg2 = unicodedata.normalize('NFKD', unicode(arg2, gb.fileencoding))
	return arg2.replace(u'ł', 'l').replace(u'Ł', 'L').encode('ascii','ignore')
	
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
		print "Invalid xml data from paramd - cannot parse. Please update your paramd. (" , raportPath , ")"
		#traceback.print_exc()
		sys.exit(-1)
	nodes = domtree.childNodes
	s = set()
	for node in nodes[0].getElementsByTagName("list:raport"):
		s.add(node.getAttribute("name").encode(gb.fileencoding))
	for element in s:
		gb.reports.append(element)
	del domtree
	del doc


def readReports(hostname):
	del gb.params
	del gb.reports
	gb.reports = []
	gb.params = []
	gb.reports.append("Available reports: [/] - filter reports  [q or enter here] - quit")	
	
	if gb.sz4 == True:
		for r in gb.sz4_params.reports.keys():
			gb.reports.append(r)
	else:
		readReportsFromParamd(hostname+":"+gb.portnumber)
	
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
		traceback.print_exc()
		sys.exit(-1)
	from xml.dom import minidom
	try:
		domtree = minidom.parseString(doc)
	except:
		restoreScreen()
		print "Invalid xml data from paramd - cannot parse. Please update your paramd. (" , raportPath , ")"
		traceback.print_exc()
		sys.exit(-1)		
	nodes = domtree.childNodes
	for node in nodes[0].getElementsByTagName("list:param"):
		parametr = node.getAttribute("name")
		l=parametr.rfind(":")
		parametr = parametr[l+1:]
		text = parametr+" "+node.getAttribute("rap:short_name")+" = "+node.getAttribute("rap:value")+" "+node.getAttribute("rap:unit")
		gb.params.append(text.encode(gb.fileencoding))
	del domtree
	del doc

def readParams(hostname, raportname):
	del gb.params
	gb.params = []    
	gb.params.append("Available parameters in "+raportname+" [enter here]-back [R/r]-refresh")

	if gb.sz4 == True:
		for n in gb.sz4_params.reports[raportname].params:
			gb.params.append(n+" "+gb.sz4_params.params[n].getString())
	else:
		readParamsFromParamd(hostname+":"+gb.portnumber, gb.activereport)
	return gb.params


def autoRefreshInit():
	"""
	Initializes automatic screen refresh
	"""	
	if gb.timeinterval<=0: return
	try:
		gb.ti = threading.Timer(gb.timeinterval, autoRefreshData)
		gb.ti.start()
	except:
		restoreScreen()
		traceback.print_exc()
		sys.exit(-1)		
	
def autoRefreshData():
	"""
	Performes automatic screen refresh 
	"""	
	if gb.timeinterval<=0: return
	gb.cmdoutlines = readParams(gb.hostname, gb.activereport)
	updateScreen()
	del gb.ti
	gb.ti = threading.Timer(gb.timeinterval, autoRefreshData)
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
				gb.cmdoutlines = readParams(gb.hostname, gb.activereport)
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
				gb.cmdoutlines = readParams(gb.hostname, gb.activereport)
				updateScreen()
		elif c == "q": 
			autoRefreshStop()
			if gb.sz4 == True:
				gb.sz4_params.closeZmq()
			break
		elif c == "/" and gb.whichmenu == 0:
			filteringReports()
	restoreScreen()

def restoreScreen():
	"""
	Restores screen to default
	"""
	if gb.sz4: gb.sz4_params.closeZmq()
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

def getBase(argv):
	"""
	"""
	for i in range(1, len(argv)-1):
		if argv[i]=="-b":
			return argv[i+1]
	return None

def getPortNumber(argv):
	"""
	Returns port number of the paramd  
	"""
	for i in range(1, len(argv)-1):
		if argv[i]=="-p":
			return argv[i+1]
	if gb.sz4:
		return "56662"
	
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
	if gb.sz4:
		return 1
	return 10

if __name__ == "__main__":
	locale.setlocale(locale.LC_ALL, '')
	if "--help" in sys.argv:
		print "usage: raporter.py [-p port] [-h host] [-t timeinterval] [-4 [-b base]] [--help]"
		print "where:"
		print "\t port - port number, if not available, it will be read from szarp.cfg"
		print "\t host - host where paramd or parhub is running, default is localhost"
		print "\t timeinterval - screen refresh parameter, default is 10 seconds"
		print "\t                if timeinterval<=0 then refreshing is off "
		print "\t if -4 is specified, parhub is used instead of paramd"
		print "\t base - will parse local params.xml from that prefix (sz4 only) and szarp default (/etc/szarp) otherwise"
		print "\nexample usage:\n\traporter.py -h 10.99.0.6 -b kepn -4 - read /opt/szarp/kepn/config/params.xml and connect to parhub at 10.99.0.6"
		print "\traporter.py -h 10.99.0.234 - connect to paramd at 10.99.0.234"
		print "\traporter.py -4 - connect to local parhub and parse default config file"
		sys.exit(1)
	
	gb.sz4 = '-4' in sys.argv
	gb.hostname = getHostName(sys.argv)
	gb.portnumber = getPortNumber(sys.argv)
	if gb.sz4 == True:
		gb.base = getBase(sys.argv)
		gb.sz4_params = Params()
	gb.timeinterval = getTimeInterval(sys.argv)
	gb.cmdoutlines = readReports(gb.hostname)
	try:
		main()
	except:
		restoreScreen()
