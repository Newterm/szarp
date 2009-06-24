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
"""
import xmlrpclib
import socket
import win32ui
import dde
import SimpleXMLRPCServer

__DDE_SERVER__ = "MBENET"

class DDESpy:
	def get_values(self, params):
		v = [];
		server = dde.CreateServer()
		try:
			server.Create(__DDE_SERVER__)
			convers = dde.CreateConversation(server)
			for topic, items in params.iteritems():
				convers.ConnectTo(__DDE_SERVER__, topic)
				for item in items:
					v.append(convers.Request(str(item)).rstrip("\0"))
		finally:
			server.Shutdown()
		return v;


if __name__ == '__main__':
	try:
		server = SimpleXMLRPCServer.SimpleXMLRPCServer(("", 8080))
		server.register_instance(DDESpy())
		server.serve_forever()
	except KeyboardInterrupt:
		pass
	except socket.error, e:
		print "Przykro nie mogę przydzielić żadanego portu dla serwera (%s)"%(e)
