import re
# SZARP: SCADA software 
# 
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

class CommandLineParser:
	def __init__(self, args, argc):
		self.args=args
		self.cur=0
		self.argc=argc
	
	def getNext(self):
		self.cur+=1
		if self.cur<self.argc:
			return self.args[self.cur]
		else:
			raise IOError


	def getAllDs(self):
		self.ret=list()
		while self.cur+1<self.argc:
			try:
				option=self.getD()
				if((option!=None) and not (re.match('.*=-D.*',option))):
					self.ret.append(option)

			except IOError:
				pass

		return self.ret
	
	def getD(self):
		t=self.getNext()
		if re.match('^-D',t):
			if re.match('.*=.',t): # -Dname = value
				return t
			elif re.match('.*=',t): # -Dname=
				t+=self.getNext()
				return t
			else:
				eq=self.getNext()
				if re.match('=',eq):
					t+=eq
					if not re.match('.*=.',t):
						t+=self.getNext()
					return t
				else:
					return None
						
		else:
			return None
