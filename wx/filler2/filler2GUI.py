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

__author__      = "Tomasz Pieczerak <tph AT newterm.pl>"
__copyright__   = "Copyright (C) 2014 Newterm"
__version__     = "2.1"
__status__      = "beta"
__email__       = "coders AT newterm.pl"

import sys, wx

def main (argv=None):
    "main() function."
    if argv is None:
        argv = sys.argv

	app = wx.App()

	frame = wx.Frame(None, -1, "Filler 2")
	frame.Show()

	app.MainLoop()

# end of main()

if __name__ == '__main__':
	sys.exit(main())

