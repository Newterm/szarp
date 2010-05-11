#!/usr/bin/python

"""
  SZARP: SCADA software
  Pawel Palucha <pawel@praterm.com.pl>

  Configuration file for running probes_server using twistd.

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

from twisted.application import internet, service
from twisted.internet import protocol, reactor, defer
from twisted.protocols import basic
from twisted.python.log import ILogObserver, FileLogObserver
from twisted.python.logfile import LogFile

# To allow importing SZARP modules
import sys
sys.path.append("/opt/szarp/lib/python")
import probes_server
from libpar import LibparReader

port, addr = probes_server.get_port_address()

application = service.Application('probes_server')
logfile = LogFile("probes_server.log", "/opt/szarp/logs")
application.setComponent(ILogObserver, FileLogObserver(logfile).emit)

factory = probes_server.ProbesFactory()
internet.TCPServer(port, factory, interface=addr).setServiceParent(
		    service.IServiceCollection(application))

