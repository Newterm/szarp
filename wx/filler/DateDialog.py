#!/usr/bin/env python
# -*- coding: utf-8 -*-

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

import wx
import wx.calendar
from mx.DateTime import *

import gettext
gettext.bindtextdomain('filler', '/opt/szarp/resources/locales')
gettext.textdomain('filler')
_ = gettext.gettext


# date and time choosing dialog
class DateDialog(wx.Dialog):
	def __init__(self, *args, **kwds):
		# begin wxGlade: DateDialog.__init__
		kwds['style'] = wx.DEFAULT_DIALOG_STYLE
		wx.Dialog.__init__(self, *args, **kwds)
        
		# date - calendar
		self.cal = wx.calendar.CalendarCtrl(self, 10,\
			wx.DateTime_Now(), pos = (25,50),\
			style = wx.calendar.CAL_SHOW_HOLIDAYS \
			| wx.calendar.CAL_MONDAY_FIRST \
			| wx.calendar.CAL_SEQUENTIAL_MONTH_SELECTION)
		
		# hours
		self.hourLabel = wx.StaticText(self, -1, _('Hour'))
		self.hourSpinCtrl = wx.SpinCtrl(self, -1, '', min=0, max=23, style=wx.SP_WRAP|wx.TE_NOHIDESEL)

		# minutes
		self.minuteLabel = wx.StaticText(self, -1, _('Minute'))
		self.minuteSpinCtrl = wx.SpinCtrl(self, -1, '', min=0, max=59, style=wx.SP_WRAP|wx.TE_NOHIDESEL)

		self.okButton = wx.Button(self, wx.ID_OK)
		self.cancelButton = wx.Button(self, wx.ID_CANCEL)

		self.__set_properties()

		self.__do_layout()

		# end wxGlade

	def __set_properties(self):
		# begin wxGlade: DateDialog.__set_properties
		self.SetTitle(_('Date and time picker'))
		# end wxGlade
	def __do_layout(self):
		mainGridSizer = wx.GridBagSizer(2, 2)
		rightSizer = wx.BoxSizer(wx.VERTICAL)
		mainGridSizer.Add(self.cal,(0,0))
		
		rightSizer.Add(self.hourLabel)
		rightSizer.Add(self.hourSpinCtrl)
		rightSizer.Add(self.minuteLabel)
		rightSizer.Add(self.minuteSpinCtrl)
		
		mainGridSizer.Add(rightSizer,(0,1))
		mainGridSizer.Add(self.okButton,(1,0))
		mainGridSizer.Add(self.cancelButton,(1,1))
		self.SetAutoLayout(True)
		self.SetSizer(mainGridSizer)
		mainGridSizer.Fit(self)
		mainGridSizer.SetSizeHints(self)
		self.Layout()

	# returns chosen date and time (exact to 10 minutes)
	def getDate(self):
		hs=self.hourSpinCtrl.GetValue() # chosen hour
		mis=self.minuteSpinCtrl.GetValue() # chosen minute

		time = RelativeDate(hours=hs,minutes=mis) # chosen time
		
		# chosen date (with time=now().time)
		date = localtime(self.cal.GetDate().GetTicks())
		
		# present time is subtracted form chosen date, so its
		# time becomes midnight
		correction=RelativeDate(hours=date.hour,\
				minutes=date.minute, seconds=date.second)
		
		# returns chosen date and time (after rounding to 10
		# minutes)
		return chopDate(date+time-correction)

	def setDate(self,date):
		self.hourSpinCtrl.SetValue(date.hour) 
		self.minuteSpinCtrl.SetValue(date.minute)
		self.cal.SetDate(wx.DateTimeFromTimeT(date.ticks()))

# end of class DateDialog

# round to 10 minutes
def chopDate(date):
	diff = date.minute % 10 # difference to 10 min
	if diff < 5: # rounding up
		correction=RelativeDateTime(minutes=-diff,\
			seconds=-date.second)
	else: # rounding down
		correction=RelativeDateTime(minutes=10-diff,\
			seconds=-date.second)
	return date+correction
