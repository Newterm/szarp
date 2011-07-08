#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
  SZARP: SCADA software 

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

  Based on: http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/81549

  $Id$

  To run this script you need first  to install:
  - Python 2.X for Windows, from http://www.python.org/download/
  - Python for Windows extensions from http://sourceforge.net/projects/pywin32/
  You can also create exe executable file using py2exe http://www.py2exe.org/. Setup
  file setup.py is included.

  You can configure listening port number (default is 8080) by creating file named
  port.cfg in script's working directory, containing just port number.
"""

import xmlrpclib
import socket
import win32ui
import dde
import sys
import SimpleXMLRPCServer
import os.path
import pythoncom
import win32serviceutil
import win32service
import win32event
import servicemanager

# default application name, for compatilibity with old ddespy/ddedmn
__DEFAULT_APPNAME__ = "MBENET"
""" Port to listen on """
__PORT__ = 8080

"""
This is XMLRPC to DDE proxy, intended to work with SZARP ddedmn driver. It listens
for XMLRPC requests and passes them as DDE Request messages. RPC request arguments
are in form of:
[ '' : APPNAME, TOPIC1 : [ ITEM11, ITEM12, ... ], TOPIC2 : [ ITEM21, ITEM22, .. ] .. ]
where:
APPNAME is name of application to connect to, if this argument is ommitted, __DEFAULT_APPNAME__
is used instead
TOPIC is DDE topic name
ITEM is DDE item name

DDE request are REQUEST(APPNAME, TOPIC, ITEM). List of return values from DDE Requests is returned.
"""

class ShutdownError(Exception):
	def __init__(self):
		pass

	def __str__(self):
		return "Shutdown"

class DDESpy:
        def __init__(self):
                self.appname = ""
                self.lasttopic = ""
                self.server = dde.CreateServer()
                self.server.Create("DDEPROXY")
                self.convers = dde.CreateConversation(self.server)
                
        def __del__(self):
                self.server.Shutdown()
        
        def set_appname(self, appname):
                if (self.appname != appname):
                        self.appname = appname
                        self.lasttopic = ""

	def shutdown(self):
		raise ShutdownError()
              
	def get_values(self, params):
                if self.appname == "":
                        self.set_appname(__DEFAULT_APPNAME__);
		v = [];
		for topic, items in params.iteritems():
                        if topic == '':
                                self.set_appname(items)
                                continue
			# if topic changed or last request was not succesfull
                        if self.lasttopic != topic:
				# mark connection as invalid
                                self.lasttopic = ""
                                self.convers.ConnectTo(self.appname, topic)
				# if there were no exception, we can mark connection as valid
                                self.lasttopic = topic
                        for item in items:
                                self.lasttopic = ""
                                ret = self.convers.Request(str(item)).rstrip("\0")
                                self.lasttopic = topic
                                v.append(ret)

		return v;

class SilentRequestHandler(SimpleXMLRPCServer.SimpleXMLRPCRequestHandler):
        """
        Simple logging.
        Default logging method tries to resolv client address - that can be
        time-consuming.
        """
        def log_message(self, format, args, par1 = None, par2 = None):
                pass
                #print format % (args, par1, par2)


class AppServerSvc (win32serviceutil.ServiceFramework):
    _svc_name_ = "DDEPROXY"
    _svc_display_name_ = "DDE proxy service"

    def __init__(self,args):
        win32serviceutil.ServiceFramework.__init__(self,args)
        self.hWaitStop = win32event.CreateEvent(None,0,0,None)
        self.server = SimpleXMLRPCServer.SimpleXMLRPCServer(("", 8080),
			requestHandler = SilentRequestHandler)
	self.server.register_instance(DDESpy())

    def SvcStop(self):
        self.ReportServiceStatus(win32service.SERVICE_STOP_PENDING)

	proxy = xmlrpclib.ServerProxy("http://127.0.0.1:8080")
	proxy.shutdown()

    def SvcDoRun(self):
        servicemanager.LogMsg(servicemanager.EVENTLOG_INFORMATION_TYPE,
                              servicemanager.PYS_SERVICE_STARTED,
                              (self._svc_name_,''))
        self.main()

    def main(self):
	while True:
		try:
			self.server.serve_forever()
		except ShutdownError, e:
			return
		except Exception, e:
			print "Cought exception", e
			print "Trying to restart"


def usage():
        print """
SZARP DDE Proxy.
Put port number in port.cfg file in folder containing ddeproxy executable.
Default port number is %d
""" % (__PORT__)

if __name__ == '__main__':
	
	win32serviceutil.HandleCommandLine(AppServerSvc)
	
