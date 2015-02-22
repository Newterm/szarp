#!/usr/bin/env python
# -*- coding: utf-8 -*-
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

# setting gettext - translates EN -> PL
import gettext
gettext.bindtextdomain('filler', '/opt/szarp/resources/locales')
gettext.bind_textdomain_codeset('filler','utf-8')
gettext.textdomain('filler')
_ = lambda s : gettext.gettext(s).decode('utf-8')

import sys
import os
import wx
import wx.html

import popen2

# date time module form package python-egenix-mxdatetime
from mx.DateTime import *


from filler import *
from DateDialog import *
from cmdLineParser import *

# parameters with their time ranges, new values and graphical (wx.list)
# representation
class ParamList(wx.ListCtrl):
	uID=0 # unique ID
	params=dict()
	curListPos=0 # position of last param
	listSelection=list() # selected params in list

	def setDialog(self,dialog):
		self.szDialog=dialog

	def __init__(self, parent, *args, **kwds):
		wx.ListCtrl.__init__(self, *args, **kwds)
		self.main=parent # main frame
	
	# removes selected params (not only form list)
	def cleanSelected(self, event=None):
		ans=self.szDialog( _('Question') ,wx.YES_NO| wx.ICON_QUESTION,\
			_('Do you really want to delete selected data?')) 
		if ans == wx.ID_YES: # user really wants to remove
			# index correction (they change when
			# params are removed)
			correction=0
			
			# sorting is needed for correction
			self.listSelection.sort()

			for pNr in self.listSelection:
				n=pNr-correction

				# only for existing data
				if n < self.curListPos:
					
					pName = self.GetItem(n,0).GetText()
					rID = self.GetItem(n,0).GetData()

					# removing range
					range=self.params[pName].range
					for i in range:
						if i[2]==rID:
							range.remove(i)
							break
					
					self.DeleteItem(n)
					self.curListPos-=1
					correction+=1
			self.listSelection = list()

	# context menu
	def menu(self, event):
		# one time only
		if not hasattr(self, "menuID1"):
			self.menuID1 = wx.NewId()

			self.Bind(wx.EVT_MENU, self.cleanSelected, id=self.menuID1)

		menu = wx.Menu()
		rem = wx.MenuItem(id=self.menuID1,text= _('Remove'))
		
		menu.AppendItem(rem)
	
		if(self.curListPos==0): # enabled only if there are parametres
			rem.Enable(False)
		
		self.PopupMenu(menu)
		menu.Destroy()

	
	# on key down
	def OnKeyDown(self, event):
		if event.GetKeyCode()==wx.WXK_DELETE:
			self.cleanSelected()
		else:
			event.Skip()
	
	# removes all params
	def cleanList(self):
		self.curListPos=0
		self.listSelection=list()
		self.DeleteAllItems()
		for k,v in self.params.iteritems():
			v.range = list()
	
	# removes all params, but asks first
	def askAndCleanList(self, event):
		if self.curListPos>0: # there is something to be removed
			ans=self.szDialog(_('Question'),wx.YES_NO|wx.ICON_QUESTION,\
			_('Do you really want to delete all data?')) 
			if ans == wx.ID_YES:
				self.cleanList() # removes all params
	
	# adding new param
	def addToList(self,value,param,paramName,startDate,endDate):
		self.params[paramName]=param
		param.range.append([startDate,endDate,self.uID,value])
		
		# columns: name, value, from, to
		self.InsertStringItem(self.curListPos,paramName)
		self.SetStringItem(self.curListPos,1,startDate.strftime('%Y-%m-%d %H:%M'))
		self.SetStringItem(self.curListPos,2,endDate.strftime('%Y-%m-%d %H:%M'))
		self.SetStringItem(self.curListPos,3,unicode(value))
		self.SetItemData(self.curListPos,self.uID)
		
		# added new param
		self.curListPos+=1
		self.uID+=1

	# number of params
	def size(self):
		return self.curListPos
	
	# saving data to database
	def generate(self,event):
		if self.curListPos==0: # no params to be written
			self.szDialog(_('No data'), wx.OK|wx.ICON_INFORMATION, _('No data to be written to database.'))
		elif self.szDialog(_('Are you sure?'), wx.YES_NO|wx.ICON_INFORMATION, _('Do you really want to write all data to database'))==wx.ID_YES:
			
			# temporary file (for szbwriter)
			fileName='/tmp/filler-'
			fileName+=os.popen('date +%F-%R-%N').readline().rstrip()+'.tmp'
			try:
				outFile=file(fileName, 'w')
			except:
				self.szDialog(_('Temporary file'), wx.OK|wx.ICON_ERROR, _('Cannot open temporary file'),': ', filename)
				return 

			# writing to file
			for k,v in self.params.iteritems():
				for i in v.range:
					gen(k,i[3],i[0],i[1],outFile)

			outFile.close()

			command = '/opt/szarp/bin/szbwriter -Dlog="/tmp/szbwriter-by-filler"'
			if "FILLER_COMMAND" in os.environ:
				command = os.environ["FILLER_COMMAND"]
			# writing to database
			writer = popen2.Popen3(command + self.main.options + ' < ' + fileName, True)
			result = writer.wait()/256
			err = writer.childerr.read().strip()

			if result == 0:
				self.szDialog(_('Database'),
				wx.OK|wx.ICON_INFORMATION, _('Data was successfully written to database.'))
				self.cleanList() # removing all (written) params
			elif err=='':
				self.szDialog(_('Database'),
				wx.OK|wx.ICON_ERROR, \
				_('Error writing database:'),' ','szbwriter',\
				' ',_('returned'),' ', result ,'.\n',\
				_('For more informations see'),' ',\
				'/tmp/szbwriter-by-filler',' ', _('log file'),'.')

			else:
				self.szDialog(_('Database'),
				wx.OK|wx.ICON_ERROR, err)

			os.remove(fileName) # removing temporary file
		
	# row selected
	def rowSelected(self,event):
		self.listSelection.append(event.GetIndex())
	
	# deselecting - clearing selection list
	def rowDeselected(self,event):
		self.listSelection=list()

class MainFrame(wx.Frame):
	def __init__(self, *args, **kwds):
		okParams=True # file prams.xml was read without problems
		curParamName='' # chosen param name

		self.focusList=list()

		CLP=CommandLineParser(sys.argv,len(sys.argv))
		self.options=' '
		for i in CLP.getAllDs():
			self.options+=i
		self.options+=' '
		
		kwds['style'] = wx.DEFAULT_FRAME_STYLE & ~ (wx.RESIZE_BORDER
				| wx.RESIZE_BOX | wx.MAXIMIZE_BOX)
		wx.Frame.__init__(self, *args, **kwds)
		
		# setting font from ~/.szarp/gtk.rc
		try:
			font = self.GetFont()
			newFont = getFont(getHome()+'/.szarp/gtk.rc')
			font.SetNativeFontInfoFromString(newFont)
			self.SetFont(font)
		except:
			pass
		
		# setting icon
		self.SetIcon( wx.Icon("/opt/szarp/resources/wx/icons/szarp16.xpm",wx.BITMAP_TYPE_XPM))
				
		
		self.chapter=4
		self.Bind(wx.EVT_HELP, self.showMyHelp, self)
		
		self.beginButton = wx.Button(self, 1001 , _('Choose from date'))
		self.beginButton.SetToolTipString(_('Choose from date'))
		self.beginButton.chapter=6
		self.Bind(wx.EVT_HELP, self.showMyHelp, self.beginButton)
		self.Bind(wx.EVT_BUTTON, self.runDateDialog, self.beginButton) 
		
		self.endButton = wx.Button(self, 1002, _('Choose till date'))
		self.endButton.SetToolTipString(_('Choose till date'))
		self.endButton.chapter=6
		self.endButton.MoveAfterInTabOrder(self.beginButton)
		self.Bind(wx.EVT_HELP, self.showMyHelp, self.endButton)
		self.Bind(wx.EVT_BUTTON, self.runDateDialog, self.endButton) 

		self.valueTextCtrl = wx.TextCtrl(self, -1, '')
		self.valueTextCtrl.SetToolTipString(_('Choose value'))
		self.valueTextCtrl.chapter=8
		self.Bind(wx.EVT_HELP, self.showMyHelp, self.valueTextCtrl)
		
		self.okButton = wx.BitmapButton(self, -1,wx.Bitmap("/opt/szarp/resources/wx/icons/add16.xpm",wx.BITMAP_TYPE_XPM) )
		self.Bind(wx.EVT_BUTTON, self.add, self.okButton) 
		self.okButton.chapter=8
		self.Bind(wx.EVT_HELP, self.showMyHelp, self.okButton)
		self.okButton.SetToolTipString(_('Add'))
		
		self.paramList = ParamList(self,self,-1,style=wx.LC_REPORT|wx.LC_SORT_ASCENDING)
		self.paramList.setDialog(self.szDialog)
		self.paramList.chapter=9
		self.Bind(wx.EVT_HELP, self.showMyHelp, self.paramList)

		self.Bind(wx.EVT_LIST_ITEM_RIGHT_CLICK,
				self.paramList.menu, self.paramList) 
		self.Bind(wx.EVT_LIST_ITEM_SELECTED,
				self.paramList.rowSelected, self.paramList) 
		
		self.Bind(wx.EVT_LIST_ITEM_DESELECTED,
				self.paramList.rowDeselected, self.paramList) 
		
		self.Bind(wx.EVT_LIST_KEY_DOWN,
				self.paramList.OnKeyDown, self.paramList) 
		


		# Menu Bar
		self.menu = wx.MenuBar()
		self.SetMenuBar(self.menu)
		fileMenu = wx.Menu()
		#Plik
		fileMenu.Append(101, _('&Write\tCtrl+W'), '')
		fileMenu.Append(102, _('&Clear\tCtrl+C'), '')
		fileMenu.AppendSeparator();
		fileMenu.Append(103, _('&Exit\tCtrl+Q'), '')
		self.menu.Append(fileMenu, _('&File'))
		#Pomoc
		helpMenu = wx.Menu()
		helpMenu.Append(201, _('Table of contents\tF1'))
		helpMenu.Append(202, _('Context help\tShift+F1'))
		helpMenu.Append(203, _('About'))
		self.menu.Append(helpMenu, _('&Help'))
	
		self.Bind(wx.EVT_MENU, self.paramList.generate, id=101)
		self.Bind(wx.EVT_MENU, self.paramList.askAndCleanList, id=102)
		self.Bind(wx.EVT_MENU, self.closeWindow, id=103)

		self.Bind(wx.EVT_MENU, self.showHelp, id=201)
		self.Bind(wx.EVT_MENU, self.contextHelp, id=202)
		self.Bind(wx.EVT_MENU, self.about, id=203)
		# Menu Bar end



		# Help system
		self.help=wx.html.HtmlHelpController()
		
		helpFileName='/opt/szarp/resources/documentation/new/filler/html/filler.hhp'
		if os.access(helpFileName,os.F_OK):
			self.help.AddBook(helpFileName)
		else:
			helpMenu.Enable(201,False)
			helpMenu.Enable(202,False)

		self.cHelp=wx.ContextHelp(None,False)
		


		# toolbar
		tb = self.CreateToolBar(wx.TB_HORIZONTAL)
		tsize = (16,16)
		newPic = wx.ArtProvider.GetBitmap(wx.ART_NEW, wx.ART_TOOLBAR, tsize)
		savePic = wx.ArtProvider.GetBitmap(wx.ART_FILE_SAVE, wx.ART_TOOLBAR, tsize)
		
		tb.AddSimpleTool(10, newPic, _('Clear'), _('Clears all selected parameters.'))
		tb.AddSimpleTool(20, savePic, _('Write'), _('Write to database.'))
		self.Bind(wx.EVT_TOOL, self.paramList.askAndCleanList, id=10)
		self.Bind(wx.EVT_TOOL, self.paramList.generate, id=20)
		
		try:
			self.setParams(getSzarpOptions(self.options,'IPK'))
		except ParamException:
			self.Close()
			return

		names=list()
		for v in self.params:
			names.append(v)
		
		self.paramSelect = wx.Choice(self, -1, choices=names, style=wx.CB_SORT)
		self.paramSelect.SetToolTipString(_('Choose parameter'))
		self.Bind(wx.EVT_CHOICE, self.paramSelected, self.paramSelect)
		self.paramSelect.chapter=5
		self.Bind(wx.EVT_HELP, self.showMyHelp, self.paramSelect)
		
		self.Bind(wx.EVT_CHAR, self.OnChar)

		# first param is selected
		self.curParamName = self.paramSelect.GetString(0)
		self.setDefaultValue()
		
		# default dates
		yesterday = chopDate(now()- RelativeDate(days=1))
		if chopDate(now()-self.params[self.curParamName].GetTime()) <= yesterday:
			self.startDate = yesterday
		else:
			# one hour ago
			self.startDate=chopDate(now()-RelativeDate(hours=1))

		self.endDate=chopDate(now())
		self.isStartDateSet=True  
		self.isEndDateSet=True 
		
		self.__set_properties()
		self.__do_layout()
		
		self.Show()
		
		self.beginButton.SetLabel(_('From')+': '+self.startDate.strftime('%Y-%m-%d %H:%M')) 
		self.endButton.SetLabel(_('Till')+': '+self.endDate.strftime('%Y-%m-%d %H:%M')) 

		# focus list
		self.paramSelect.SetFocus()
		self.focusList.append(self.paramSelect)
		self.focusList.append(self.beginButton)
		self.focusList.append(self.endButton)
		self.focusList.append(self.valueTextCtrl)
		self.focusList.append(self.okButton)
		self.focusList.append(self.paramList)


	def __set_properties(self):
		self.SetTitle(_('Filler'))
		self.paramList.InsertColumn(0, _('Parameter'))
		self.paramList.SetColumnWidth(0,
				self.paramSelect.GetSize()[0]+10)
		self.paramList.InsertColumn(1, _('From'))
		self.paramList.SetColumnWidth(1, self.beginButton.GetSize()[0]+20)
		self.paramList.InsertColumn(2, _('Till'))
		self.paramList.SetColumnWidth(2, self.endButton.GetSize()[0]+20)
		self.paramList.InsertColumn(3, _('Value'))
		self.paramList.SetColumnWidth(3, self.valueTextCtrl.GetSize()[0]+20)
		w=0
		for i in range(0,4):
			w+=self.paramList.GetColumnWidth(i)
		self.paramList.SetMinSize((w,300))

	def __do_layout(self):
		mainSizer = wx.BoxSizer(wx.VERTICAL)
		
		self.topGridSizer = wx.BoxSizer(wx.HORIZONTAL)
		
		self.topGridSizer.Add(self.paramSelect, flag=wx.ALIGN_CENTER|wx.ALL, border=10)
		self.topGridSizer.Add(self.beginButton, flag=wx.ALIGN_CENTER|wx.ALL, border=10)
		self.topGridSizer.Add(self.endButton, flag=wx.ALIGN_CENTER|wx.ALL, border=10)
		self.topGridSizer.Add(self.valueTextCtrl, flag=wx.ALIGN_CENTER|wx.ALL, border=10)
		self.topGridSizer.Add(self.okButton, flag=wx.ALIGN_CENTER|wx.ALL, border=10)

		mainSizer.Add(self.topGridSizer,flag=wx.ALL, border=0)
		mainSizer.Add(self.paramList, flag=wx.ALL, border=10)
		
		
		self.SetAutoLayout(True)
		self.SetSizer(mainSizer)
		mainSizer.Fit(self)
		mainSizer.SetSizeHints(self)
		self.Layout()
	
	
	# menu events
	
	# showing help
	def showHelp(self,event):
		self.help.DisplayContents()
	
	# context help
	def contextHelp(self,event):
		self.cHelp.BeginContextHelp()

	# about
	def about(self,event):
		ans=self.szDialog(_('About'),wx.OK|wx.ICON_INFORMATION,\
			_('Filler version'),': ', getVersion(), '\n',
			u'SZARP Scada System', '\n',
			_('Author'),
			u': Maciej Zbi\u0144kowski <maciek@praterm.com.pl>')
	# exit
	def closeWindow(self, event):
		if self.paramList.size()>0:
			# are you sure?
			ans=self.szDialog(_('Exit'),wx.YES_NO|wx.ICON_QUESTION,\
			_('You will lose all not saved data'),'\n',\
			_('Are you sure you want to exit?'))
			if ans == wx.ID_YES: # yes!
				self.Close()
		else: # no params in list - no problem
			self.Close()
	
	# other events

	# choosing date and time
	def runDateDialog(self, event):
		# begin or end  date button
		button = event.GetEventObject()
		
		if self.curParamName == '': # have to chose param first
			self.szDialog(_('Parameter'),wx.OK|wx.ICON_INFORMATION,_('Choose parameter first.'))
		else:
			dd=DateDialog(self, -1)
			dd.CenterOnScreen()
			if event.GetId() == 1001: # begin date
				dd.setDate(self.startDate)
			else:
				dd.setDate(self.endDate)

			answer = dd.ShowModal()
			chosenDate = dd.getDate() 
			dd.Destroy()
			

			if answer == wx.ID_OK: # user chosen date
				# setting dates - they don't have to be
				# correct. If they are wrong
				# self.is*DateSet will be false.
				if event.GetId() == 1001: # begin date
					self.startDate=chosenDate
				else:
					self.endDate=chosenDate
				
				curParam = self.params[self.curParamName]
				minDate= now()-curParam.GetTime()
				minDate=chopDate(minDate)
				maxDate=chopDate(now())

				# checking if this is good date
				if chosenDate < minDate or chosenDate > maxDate:
					self.szDialog(_('Wrong range'), wx.OK|\
					wx.ICON_ERROR,\
					_('You must choose date between'),' ',\
					minDate.strftime('%Y-%m-%d %H:%M'), ' ',\
					_('and'),' ',maxDate.strftime('%Y-%m-%d %H:%M'))
				else:
					# saving dates
					if event.GetId() == 1001: # begin date
						labTxt=_('From')
						if (not self.isEndDateSet) or  chosenDate<self.endDate:
							self.isStartDateSet=True
						else:
							self.szDialog(_('Wrong date'),
								wx.OK|
								wx.ICON_INFORMATION,
								_('Till date cannot be greater then from date.'))
							return
					elif event.GetId() == 1002: # end date
						labTxt=_('Till')
						if (not self.isStartDateSet) or chosenDate>self.startDate:
							self.isEndDateSet=True
						else:
							self.szDialog(_('Wrong date'), wx.OK| wx.ICON_INFORMATION, _('From date can not be greater then till date.')) 
							return
		
					# buttons label is chosen date
					button.SetLabel(labTxt+': '+chosenDate.strftime('%Y-%m-%d %H:%M')) 


	# event: combo item selected 
	def paramSelected(self, event):
		cb = event.GetEventObject()
		self.curParamName = cb.GetStringSelection() # chosen param
		self.cleanInput()
		self.setDefaultValue()
	
	# even: got char (key is pressed)
	def OnChar(self,event):
		if event.GetKeyCode()==wx.WXK_RETURN:
			self.add()
		elif event.GetKeyCode()==wx.WXK_TAB:
			self.nextWindow().SetFocus()
		else:
			event.Skip()
	
	# returns next window in focus queue
	def nextWindow(self):
		if not hasattr(self, 'curWindow'):
			self.curWindow = 1
		else:
			self.curWindow+=1
		

		if self.curWindow >= len(self.focusList):
			self.curWindow=0

		return self.focusList[self.curWindow]
		

	# setting default value
	def setDefaultValue(self):
		param = self.params[self.curParamName]
		if not hasattr(param, 'previousValue'):
			self.valueTextCtrl.SetValue(digt(param.max)*'0'+'.'+param.prec*'0')
		else:
			self.valueTextCtrl.SetValue(param.previousValue)
	
	# checking if chosen data is good enough to be added to list
	def add(self, event=None):
		if self.curParamName == '': # param must be chosen
			self.szDialog(_('Warning'), wx.OK|
					wx.ICON_INFORMATION, _('Choose parameter first.'))
		# both dates must be chosen (if they are, they are chosen
		# correctly for sure)
		elif (not self.isStartDateSet) or (not self.isEndDateSet):
			self.szDialog(_('Warning'), wx.OK| wx.ICON_INFORMATION, _('Choose both dates first.'))
		else:
			try: # value must be float
				val = float(self.valueTextCtrl.GetValue())
			except:
				self.szDialog(_('Wrong value'), wx.OK| wx.ICON_INFORMATION, _('Value must be float.'))

				return

			param = self.params[self.curParamName]

			# value must be in range
			if(val>=param.min and val<=param.max):
				if param.isRangeOK(self.startDate,self.endDate):

					self.paramList.addToList(val,param,self.curParamName,self.startDate,self.endDate)
					self.cleanInput()
					param.previousValue=unicode(val);
				else:
					self.szDialog(_('Wrong date') ,\
					wx.OK|wx.ICON_ERROR,\
					_('This date range has aleady been used')) 
			else:
				self.szDialog(_('Wrong value') ,wx.OK|\
					wx.ICON_ERROR,
					_('Choose value between'),' ',\
					param.min,' ', _('and'),' ', \
					param.max)


	# reading params.xml
	def setParams(self,xmlFilename):
		try: # opening params.xml file
			xmlFile=file(xmlFilename)
		except:
			self.szDialog('params.xml', wx.OK|wx.ICON_ERROR,\
					_('Can\'t find file "params.xml".'))
			raise ParamException

		try: # parsing params.xml
			self.params=readParamsFromFile(xmlFile)
		# excepting parsing (XML) errors
		except NoParamsException: 
			self.szDialog(_('Error in params.xml')\
			,wx.OK|wx.ICON_ERROR,\
			_('No params in params.xml'))
			raise ParamException
		except NoEditablesException:
			self.szDialog(_('Error in params.xml')\
			,wx.OK|wx.ICON_ERROR,\
			_('No editable parameters in params.xml'))
			raise ParamException
		except BadXMLException:
			self.szDialog(_('Error in params.xml')\
			,wx.OK|wx.ICON_ERROR,\
			_('params.xml is not an XML file.'))
			raise ParamException
		# excepting editable's attributres' value errors
		except NoValueException, e:
			self.szDialog(_('Error in params.xml')\
			,wx.OK|wx.ICON_ERROR,\
			_('Couldn\'t find attribute'),' "', e.attribute, \
			'" ',_('in parameter'),' "', e.param,'"')
			raise ParamException
		except WrongValueException, e:
			self.szDialog(_('Error in params.xml')\
			,wx.OK|wx.ICON_ERROR,\
			_('Incorrect value of attribute'),' "', e.attribute, \
			'" ',_('in parameter'),' "', e.param,'"')
			raise ParamException
		except NonPositiveException, e:
			self.szDialog(_('Error in params.xml')\
			,wx.OK|wx.ICON_ERROR,\
			_('Non-Positive value of attribute'),' "', e.attribute, \
			'" ',_('in parameter'),' "', e.param,'"')
			raise ParamException
		except NegativeException, e:
			self.szDialog(_('Error in params.xml')\
			,wx.OK|wx.ICON_ERROR,\
			_('Negative value of attribute'),' "', e.attribute, \
			'" ',_('in parameter'),' "', e.param,'"')
			raise ParamException
		except WrongRangeException, e:
			self.szDialog(_('Error in params.xml') \
			,wx.OK|wx.ICON_ERROR,\
			_('"min" attribute is not less then "max" in parameter.'),\
			' "', e.param,'"')
			raise ParamException
		except WrongUnitValueException, e:
			self.szDialog(_('Error in params.xml')\
			,wx.OK|wx.ICON_ERROR,\
			_('"unit" attribute is not in'),' ', e.okValues,\
				' ',_('in parameter'),' "', e.param,'"')
			raise ParamException
			

	# clearing chosen dates
	def cleanInput(self):
		self.isStartDateSet=False
		self.isEndDateSet=False
		self.beginButton.SetLabel(_('Choose from date'))
		self.endButton.SetLabel(_('Choose till date'))
	
	# show context help
	def showMyHelp(self,event):
		self.help.DisplayID(event.GetEventObject().chapter)

	
	# wx.MessageDialog wrapper - szDialog can have many text
	# arguments, which are glued together. Dialog is shown, and
	# destroyed after getting answer form user.
	def szDialog(self, title, flags, *txts):
		dia=wx.MessageDialog(self, ''.join(map(unicode,txts)) ,title, flags )
		ans = dia.ShowModal() # pokazanie dialogu
		dia.Destroy() # zniszczenie dialogu
		return ans
	
# end of class MainFrame

if __name__ == '__main__':
	app = wx.App(False)
	mainFrame = MainFrame(None, -1, '')
	app.SetTopWindow(mainFrame)
	app.MainLoop()
