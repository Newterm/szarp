#!/usr/bin/env python
# -*- coding: utf-8 -*-

__license__ = \
"""
 Filler 2 is a part of SZARP SCADA software

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA"""

__author__      = "Tomasz Pieczerak <tph AT newterm.pl>"
__copyright__   = "Copyright (C) 2014 Newterm"
__version__     = "2.1"
__status__      = "devel"
__email__       = "coders AT newterm.pl"

import sys, wx
import gettext


class MainWindow(wx.Frame):
	def __init__(self, *args, **kwargs):
		wx.Frame.__init__(self, *args, **kwargs)

		self.SetIcon(wx.Icon("/opt/szarp/resources/wx/icons/szarp16.xpm",
					 wx.BITMAP_TYPE_XPM))
		self.CreateInteriorWindowComponents()
		self.CreateExteriorWindowComponents()

	def CreateInteriorWindowComponents(self):
		self.control = wx.TextCtrl(self, style=wx.TE_MULTILINE)

	def CreateExteriorWindowComponents(self):
		self.CreateMenu()
		self.CreateStatusBar()

	def CreateMenu(self):
		# Create program's menu
		menuBar = wx.MenuBar()
		self.SetMenuBar(menuBar)

		# "File" menu
		fileMenu = wx.Menu()
		fileMenu.Append(wx.ID_SAVE, _("&Save\tCtrl+S"), _("Save changes to params.xml"))
		fileMenu.Append(wx.ID_CLEAR, _("&Clear\tCtrl+C"), _("Clear pending actions"))
		fileMenu.AppendSeparator()
		fileMenu.Append(wx.ID_EXIT, _("E&xit\tCtrl+Q"), _("Terminate Filler 2"))
		menuBar.Append(fileMenu, _("&File"))

		self.Bind(wx.EVT_MENU, self.OnExit, id=wx.ID_EXIT)

		# "Help" menu
		helpMenu = wx.Menu()
		helpMenu.Append(wx.ID_HELP, _("User's &manual\tF1"), _("Show user's manual"))
		helpMenu.Append(wx.ID_CONTEXT_HELP, _("Con&text help\tShift+F1"), _("Activate context help"))
		helpMenu.Append(wx.ID_ABOUT, _("&About"), _("Information about Filler 2"))
		menuBar.Append(helpMenu, _("&Help"))

		self.Bind(wx.EVT_MENU, self.OnAbout, id=wx.ID_ABOUT)

	def OnAbout(self, event):
		info = wx.AboutDialogInfo()

		info.SetIcon(wx.Icon('/opt/szarp/resources/wx/images/szarp-logo.png',
					 wx.BITMAP_TYPE_PNG))
		info.SetName("Filler")
		info.SetVersion(__version__ + " (" + __status__ + ")")
		info.SetDescription(_("Filler 2 is a tool for manual szbase data editing."))
		info.SetCopyright(__copyright__)
		info.SetLicence(__license__)
		info.SetWebSite("http://newterm.pl/")
		info.AddDeveloper(__author__)

		wx.AboutBox(info)

	def OnExit(self, event):
		self.Close()  # Close the main window.


def main (argv=None):
	"main() function."
	if argv is None:
		argv = sys.argv

	gettext.install('filler2', '/opt/szarp/resources/locales', unicode=True)

	app = wx.App()

	frame = MainWindow(None, wx.ID_ANY, "Filler 2")
	frame.Show()

	app.MainLoop()

# end of main()

if __name__ == '__main__':
	sys.exit(main())

