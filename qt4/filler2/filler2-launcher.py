#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Filler 2 is a tool for manual editing of SZARP databases. It is written in Qt4
library and meant to be easy to use.

WARNING! Although all changes should be reversible, they are committed to live
database and therefore should be considered as risky.
"""

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 MA  02110-1301, USA """

__author__    = "Tomasz Pieczerak <tph AT newterm.pl>"
__copyright__ = "Copyright (C) 2014-2015 Newterm"
__version__   = "2.0"
__status__    = "beta"
__email__     = "coders AT newterm.pl"


# imports
import os
import sys
import time
import math
import types
import fcntl
import signal
import getpass
import datetime
import argparse
import subprocess
from collections import namedtuple

# local imports
from filler2 import Ui_MainWindow
from ipkparser import IPKParser
from DatetimeDialog import Ui_DatetimeDialog
from AboutDialog import Ui_AboutDialog
from HistoryDialog import Ui_HistoryDialog
from ValueDialogs import ValueDialogFactory

# pyQt4 imports
from PyQt4.QtCore import *
from PyQt4.QtGui import *

# other imports
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

# constants
LOCKFILE = '/tmp/.filler2_lock'

# global variables
__script_name__ = os.path.basename(sys.argv[0])

# containers definitions
SzChangeInfo = namedtuple('SzChangeInfo',
                          'draw_name name dvalues lswmsw remark')

# signal handlers
signal.signal(signal.SIGINT, signal.SIG_DFL)

# translation function for QTranslator
try:
    _encoding = QApplication.UnicodeUTF8
    def _translate(context, text, disambig=None):
        return QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig=None):
        return QApplication.translate(context, text, disambig)

### Parsing arguments ###

# Method overrides for argparse #
def format_usage(self):
	formatter = self._get_formatter()
	formatter.add_usage(self.usage, self._actions,
		self._mutually_exclusive_groups)
	formatter.add_text("Try `%s --help' for more options." % self.prog)

	return formatter.format_help()

### End of argparse overrides ###

class CustomFormatter(argparse.RawTextHelpFormatter):
	def _format_action_invocation(self, action):
		if not action.option_strings:
			metavar, = self._metavar_formatter(action, action.dest)(1)
			return metavar
		else:
			parts = []
			# if the Optional doesn't take a value, format is:
			#	-s, --long
			if action.nargs == 0:
				parts.extend(action.option_strings)

			# if the Optional takes a value, format is:
			#	-s, --long ARGS
			else:
				default = action.dest.upper()
				args_string = self._format_args(action, default)
				for option_string in action.option_strings:
					parts.append('%s' % option_string)
				parts[-1] += '=%s'%args_string
			return ', '.join(parts)

		return help

	def add_usage(self, usage, actions, groups, prefix="Usage: "):
		if usage is not argparse.SUPPRESS:
			args = usage, actions, groups, prefix
			self._add_item(self._format_usage, args)

# end of CustomFormatter

def format_version():
	"""Format version string."""
	return \
"""Filler %(version)s (status: %(status)s)

%(copy)s

Written and maintained by %(author)s.
Please send bug reports and questions to <%(email)s>.""" \
% {"version": __version__, "status": __status__, "copy": __copyright__,
	"author": __author__, "email": __email__}

# end of format_version()

### End of parsing arguments ###


class SingleInstance:
	"""Assure single-instance process in system."""
	def __init__(self, lockfile):
		self.initialized = False
		self.lockfile = os.path.abspath(lockfile)

		self.lockfd = open(self.lockfile, 'w')
		fcntl.lockf(self.lockfd, fcntl.LOCK_EX | fcntl.LOCK_NB)

		self.initialized = True

	# end of __init__()

	def __del__(self):
		if not self.initialized:
			return

		fcntl.lockf(self.lockfd, fcntl.LOCK_UN)
		if os.path.isfile(self.lockfile):
			os.unlink(self.lockfile)

	# end of __del__()

# end of SingleInstance class


class Filler2(QMainWindow):
	"""SZARP Filler 2 application's main window (pyQt4)."""

	def __init__(self, szprefix, no_warn=False, parent=None):
		"""Filler 2 class constructor."""
		QWidget.__init__(self, parent)
		self.ui = Ui_MainWindow()
		self.ui.setupUi(self)

		# check single instance
		try:
			self.single_instance = SingleInstance(LOCKFILE)
		except IOError:
			self.criticalError(_translate("MainWindow",
				"Cannot acquire a single instance lock. There is "
				" other Filler 2 program running on the system."))

		# parse local SZARP configuration
		try:
			self.parser = IPKParser(szprefix)
		except IOError as err:
			self.criticalError(_translate("MainWindow",
						"Cannot read SZARP configuration")
						+ " (%s)." % err.bad_path)
			sys.exit(1)
		except ValueError as err:
			self.criticalError(_translate("MainWindow",
						"Non-valid SZARP database prefix."))
			sys.exit(1)

		# verify SZARP prefix versus hostname
		if no_warn == False:
			try:
				process = subprocess.Popen(
						["/bin/hostname", "-s"], stdout=subprocess.PIPE)
				out, err = process.communicate()
				if err == None:
					hostname = out[:-1]

				if hostname != self.parser.ipk_prefix:
					self.warningBox(_translate("MainWindow",
						"Warning! Database prefix differs from hostname, "
						"you are probably modifying non-local database."))
			except OSError:
				pass

		### initialize Qt4 widgets ##

		# name of the local configuration
		self.ui.titleLabel.setText(self.parser.getTitle())

		# list of parameter sets
		self.ui.listOfSets.addItem(
				_translate("MainWindow", "--- Choose a set of parameters ---"))
		self.ui.listOfSets.addItems(self.parser.getSets())
		self.ui.listOfSets.model().setData(
				self.ui.listOfSets.model().index(0,0),
				QVariant(0), Qt.UserRole-1)
		self.ui.listOfSets.setEnabled(True)

		# parameter's type combobox
		self.dialog_factory = ValueDialogFactory()

		for name, icon, desc in self.dialog_factory.get_dialogs():
			self.ui.valueType.addItem(icon, desc, QVariant((name, desc)))

		self.ui.valueType.model().setData(
				self.ui.valueType.model().index(0,0),
				QVariant(0), Qt.UserRole-1)

		# date interval variables
		self.fromDate = None
		self.toDate = None

		# table of changes
		base = 64
		self.ui.changesTable.setColumnCount(6)
		self.ui.changesTable.setColumnWidth(0, 6*base-24) # param's draw_name
		self.ui.changesTable.setColumnWidth(1, 3*base)    # "from" date
		self.ui.changesTable.setColumnWidth(2, 3*base)    # "to" date
		self.ui.changesTable.setColumnWidth(3, 3*base)    # param's value type
		self.ui.changesTable.setColumnWidth(4, base-20)   # remove entry button
		self.ui.changesTable.setColumnHidden(5, True)     # param's full name

		self.ui.changesTable.horizontalHeader().setVisible(False)
		self.ui.changesTable.setRowCount(0)

	# end of __init__()

	def criticalError(self, msg, title = None):
		"""Display critical error dialog and terminate.

		Arguments:
			msg - text message to be displayed.
			title - optional dialog title.
		"""
		if title is None:
			title = "SZARP Filler 2 - " + \
				_translate("MainWindow", "Critical Error")
		QMessageBox.critical(self, title, msg)
		sys.exit(1)

	# end of criticalError()

	def warningBox(self, msg, title = None):
		"""Display warning dialog.

		Arguments:
			msg - text message to be displayed.
			title - optional dialog title.
		"""
		if title is None:
			title = "SZARP Filler 2 - " + _translate("MainWindow", "Warning")
		QMessageBox.warning(self, title, msg)

	# end of warningBox()

	def infoBox(self, msg, title = None):
		"""Display information dialog.

		Arguments:
			msg - text message to be displayed.
			title - optional dialog title.
		"""
		if title is None:
			title = "SZARP Filler 2 - " + _translate("MainWindow", "Information")
		QMessageBox.information(self, title, msg)

	# end of infoBox()

	def questionBox(self, msg, title = None):
		"""Display question dialog.

		Arguments:
			msg - text message to be displayed.
			title - optional dialog title.

		Returns:
			QMessageBox.Yes | QMessageBox.No - user's answer.
		"""
		if title is None:
			title = "SZARP Filler 2 - " + _translate("MainWindow", "Question")
		return QMessageBox.question(self, title, msg,
				QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

	# end of questionBox()

	def onSetChosen(self, text):
		"""Slot for signal activated(QString) from 'listOfSets' (QComboBox).

		Arguments:
			text - name of the chosen set.
		"""
		self.ui.paramList.clear()
		self.ui.paramList.addItem(
				_translate("MainWindow", "--- Choose a parameter ---"))
		self.ui.paramList.model().setData(
				self.ui.paramList.model().index(0,0),
				QVariant(0), Qt.UserRole-1)

		# insert a list of parameters from given set
		for p in self.parser.getParams(unicode(text)):
			self.ui.paramList.addItem(p.draw_name, p)

		# activate/reset other ui elements
		self.ui.paramList.setEnabled(True)
		self.ui.paramList.setFocus()
		self.ui.valueType.setCurrentIndex(0)
		self.ui.valueType.setEnabled(False)
		self.ui.addButton.setEnabled(False)

	# end of onSetChosen()

	def onParamChosen(self, index):
		"""Slot for signal activated(QString) from 'paramList' (QComboBox).

		Arguments:
			text - name of chosen parameter (not used).
		"""
		# activate other ui elements
		self.ui.fromDate.setEnabled(True)
		self.ui.toDate.setEnabled(True)
		self.ui.valueType.setEnabled(True)
		self.type_dialog = None
		self.validateInput()

	# end of onParamChosen()

	def onFromDate(self):
		"""Slot for signal clicked() from 'fromDate' (QPushButton). Displays dialog
		for choosing date and time.
		"""
		if self.fromDate is None:
			if self.toDate is None:
				dlg = DatetimeDialog_impl()
			else:
				dlg = DatetimeDialog_impl(start_date=
						(self.toDate - datetime.timedelta(minutes=10)))
		else:
			if self.toDate is None or self.fromDate < self.toDate:
				dlg = DatetimeDialog_impl(start_date=self.fromDate)
			else:
				dlg = DatetimeDialog_impl(start_date=
						(self.toDate - datetime.timedelta(minutes=10)))

		# execute DatetimeDialog
		if dlg.exec_():
			self.fromDate = dlg.getValue()
			self.ui.fromDate.setText(_translate("MainWindow", "From:") + " " +
					self.fromDate.strftime('%Y-%m-%d %H:%M'))
			self.validateInput()

	# end of onFromDate()

	def onToDate(self):
		"""Slot for signal clicked() from 'toDate' (QPushButton). Displays dialog
		for choosing date and time.
		"""
		if self.toDate is None:
			if self.fromDate is None:
				dlg = DatetimeDialog_impl()
			else:
				dlg = DatetimeDialog_impl(start_date=
						(self.fromDate + datetime.timedelta(minutes=10)))
		else:
			if self.fromDate is None or self.toDate > self.fromDate:
				dlg = DatetimeDialog_impl(start_date=self.toDate)
			else:
				dlg = DatetimeDialog_impl(start_date=
						(self.fromDate + datetime.timedelta(minutes=10)))

		# execute DatetimeDialog
		if dlg.exec_():
			self.toDate = dlg.getValue()
			self.ui.toDate.setText(_translate("MainWindow", "To:") + " " +
					self.toDate.strftime('%Y-%m-%d %H:%M'))
			self.validateInput()

	# end of onToDate()

	def onTypeChosen(self, index):
		"""Slot for signal activated() from 'valueType' (QComboBox). Constructs
		appropriate dialog from factory and collects chosen plot parameters.
		"""
		# fetch dialog and parameter data
		dlg_name = self.ui.valueType.itemData(index).toPyObject()[0]
		param_info = self.ui.paramList.itemData(self.ui.paramList.currentIndex()).toPyObject()

		dlg = self.dialog_factory.construct(str(dlg_name),
				param_info.prec, param_info.lswmsw,
				parent = self)

		# reset type items' texts
		for i in range(self.ui.valueType.count()):
			desc = self.ui.valueType.itemData(i).toPyObject()[1]
			self.ui.valueType.setItemText(i, desc)

		if dlg.exec_():
			self.type_dialog = dlg
			self.ui.valueType.setItemText(index, dlg.get_value_desc())
		else:
			self.ui.valueType.setCurrentIndex(0)
			self.type_dialog = None
		self.validateInput()

	# end of onTypeChosen()

	def validateInput(self):
		"""Check whether all needed data is filled and valid. If do, "Add"
		button is activated.
		"""
		if  self.ui.paramList.currentIndex() != 0 and \
			self.fromDate is not None and \
			self.toDate is not None and \
			self.ui.valueType.currentIndex() != 0:
				self.ui.addButton.setEnabled(True)
		else:
				self.ui.addButton.setEnabled(False)

	# end of validateInput()

	def aboutQt(self):
		"""Display about Qt4 dialog."""
		QMessageBox.aboutQt(self)

	def about(self):
		"""Display about Filler 2 dialog."""
		AboutDialog_impl().exec_()

	def addChange(self):
		"""Slot for signal clicked() from addButton (QPushButton). Adds
		change-entry to changesTable (QTableWidget)."""
		if self.fromDate >= self.toDate:
			self.warningBox(_translate("MainWindow",
				"\"To\" date is earlier (or equals) \"From\" date.\nAdding change aborted."))
			return

		param_info = self.ui.paramList.itemData(
				self.ui.paramList.currentIndex()).toPyObject()

		self.ui.changesTable.setRowCount(self.ui.changesTable.rowCount()+1)
		self.addRow(self.ui.changesTable.rowCount() - 1,
					param_info.name,
					self.ui.paramList.currentText(),
					self.fromDate,
					self.toDate,
					self.type_dialog,
					param_info.lswmsw)

		# reset type items' texts
		for i in range(self.ui.valueType.count()):
			desc = self.ui.valueType.itemData(i).toPyObject()[1]
			self.ui.valueType.setItemText(i, desc)
		self.ui.valueType.setCurrentIndex(0)

	# end of addChange()

	def addRow(self, row, fname, pname, from_date, to_date, typedlg, lswmsw):
		"""Add row to changesTable (QTableWidget).

		Arguments:
			row - number of row to be set.
			fname - full parameter name
			pname - parameter's draw name
			from_date - beginning of time period
			to_date - end of time period
			value - parameter's value
			typedlg - object of input value dialog
			lswmsw - whether parametr is a combined lsw/msw
		"""
		# visible columns
		item_pname = QTableWidgetItem(unicode(pname))
		item_pname.setFlags(Qt.ItemIsEnabled)

		item_from_date = QTableWidgetItem(from_date.strftime('%Y-%m-%d %H:%M'))
		item_from_date.setFlags(Qt.ItemIsEnabled)
		item_from_date.setTextAlignment(Qt.AlignCenter)

		item_to_date = QTableWidgetItem(to_date.strftime('%Y-%m-%d %H:%M'))
		item_to_date.setFlags(Qt.ItemIsEnabled)
		item_to_date.setTextAlignment(Qt.AlignCenter)

		item_value = QTableWidgetItem(typedlg.get_value_desc())
		item_value.setFlags(Qt.ItemIsEnabled)
		item_value.setIcon(QIcon(typedlg.qicon_path))
		item_value.setTextAlignment(Qt.AlignCenter)

		# hidden column
		item_fname = QTableWidgetItem(unicode(fname))
		item_fname.setData(Qt.UserRole, QVariant((lswmsw, typedlg)))
		item_fname.setFlags(Qt.ItemIsEnabled)

		self.ui.changesTable.setItem(row, 0, item_pname)
		self.ui.changesTable.setItem(row, 1, item_from_date)
		self.ui.changesTable.setItem(row, 2, item_to_date)
		self.ui.changesTable.setItem(row, 3, item_value)
		self.ui.changesTable.setItem(row, 5, item_fname)

		# "remove" button widget
		rm_button = QPushButton(QIcon.fromTheme("window-close"), "")
		rm_button.setToolTip(_translate("MainWindow", "Remove entry"))
		rm_button.row_id = row
		QObject.connect(rm_button, SIGNAL("clicked()"), self.removeChange)
		self.ui.changesTable.setCellWidget(row, 4, rm_button)

	# end of addRow()

	def removeChange(self):
		"""Slot for signal clicked() from rm_button (QPushButton). Removes
		entry from changesTable (QTableWidget).
		"""
		if QMessageBox.Yes != \
				self.questionBox(_translate("MainWindow", "Remove change?")):
			return

		row = self.sender().row_id
		self.ui.changesTable.removeRow(row)

		# update row_id for every row
		for i in range(row, self.ui.changesTable.rowCount()):
			self.ui.changesTable.cellWidget(i, 4).row_id -= 1

	# end of removeChange()

	def clearChanges(self):
		"""Slot for action 'actionClear'. Removes all entries from
		changesTable (QTableWidget).
		"""
		txt = _translate("MainWindow",
				"Are you sure you want to clear all changes?")

		if QMessageBox.Yes == self.questionBox(txt):
			self.ui.changesTable.setRowCount(0)

	# end of clearChanges()

	def commitChanges(self):
		"""Slot for action 'actionSaveData'. Commits all scheduled changes
		to local szbase.
		"""
		if self.ui.changesTable.rowCount() == 0:
			self.warningBox(_translate("MainWindow", "No changes to commit."))
			return

		# list and confirm
		txt = _translate("MainWindow",
				"Following parameters will be modified:") + "\n\n"

		for i in range(0, self.ui.changesTable.rowCount()):
			txt.append("   * %s\n\n" \
					% (self.ui.changesTable.item(i,0).text()))

		txt.append(_translate("MainWindow", "Commit changes?"))

		if QMessageBox.Yes == self.questionBox(txt):
			# construct list of changes to be committed
			changes_list = []

			for i in range(0, self.ui.changesTable.rowCount()):

				start_date = datetime.datetime.strptime(
					str(self.ui.changesTable.item(i,1).text()),
					'%Y-%m-%d %H:%M')

				end_date = datetime.datetime.strptime(
					str(self.ui.changesTable.item(i,2).text()),
					'%Y-%m-%d %H:%M')

				lswmsw, typedlg = \
					self.ui.changesTable.item(i,5).data(Qt.UserRole).toPyObject()

				# generate list of 10-minute probes in given interval
				dsec = int((end_date-start_date).total_seconds())
				dts = [start_date + datetime.timedelta(minutes=x) \
						for x in range(0, dsec / 60 + 1, 10)]
				# generate probes' values
				dvals = typedlg.generate(dts)

				rmrk = typedlg.get_remark()

				changes_list.append(SzChangeInfo(
						draw_name = unicode(self.ui.changesTable.item(i,0).text()),
						name = unicode(self.ui.changesTable.item(i,5).text()),
						dvalues = dvals,
						lswmsw = lswmsw,
						remark = rmrk
						))

			# do the job (in a new thread)
			szbw = SzbWriter(changes_list, self.parser)
			szbp = SzbProgressWin(szbw, parent = self)

			# reset all GUI elements
			self.ui.changesTable.setRowCount(0)
			self.fromDate = None
			self.toDate = None
			self.ui.listOfSets.setCurrentIndex(0)
			self.ui.paramList.clear()
			self.ui.paramList.setEnabled(False)
			self.ui.fromDate.setText(_translate("MainWindow", "From:"))
			self.ui.fromDate.setEnabled(False)
			self.ui.toDate.setText(_translate("MainWindow", "To:"))
			self.ui.toDate.setEnabled(False)
			self.ui.valueType.setCurrentIndex(0)
			self.ui.valueType.setEnabled(False)
			self.ui.addButton.setEnabled(False)

			# disable main window until job is finished
			self.setEnabled(False)

	# end of commitChanges()

	def contextHelp(self):
		"""Slot for action 'actionContextHelp'. Activates "What is that?" mode."""
		QWhatsThis.enterWhatsThisMode()

	def onViewHistory(self):
		"""Slot for action 'actionViewHistory'. Shows history of committed changes."""
		HistoryDialog_impl(self.parser, parent=self).exec_()

# end of Filler2 class


class DatetimeDialog_impl(QDialog, Ui_DatetimeDialog):
	"""Dialog for choosing date and time (pyQt4)."""

	def __init__(self, parent=None, start_date=datetime.datetime.now()):
		"""DatetimeDialog_impl class constructor.

		Arguments:
			start_date - initial widget date and time (defaults to now).
		"""
		# initialization
		QDialog.__init__(self,parent)
		self.setupUi(self)
		self.calendarWidget.setLocale(QLocale.system())

		# pad date/time to 10 minutes (floor)
		start_date -= datetime.timedelta(minutes=start_date.minute % 10,
									seconds=start_date.second,
									microseconds=start_date.microsecond)

		# set values in widgets
		self.calendarWidget.setSelectedDate(start_date)
		self.hourSpinBox.setValue(start_date.hour)
		self.minuteSpinBox.setValue(start_date.minute)
		self.currentDate.setText(start_date.strftime('%Y-%m-%d %H:%M'))

		self.currentDatetime = start_date

	# end of __init__()

	def getValue(self):
		"""Return chosen date/time."""
		date = self.currentDatetime
		date -= datetime.timedelta(minutes=date.minute % 10)
		return date

	def updateDate(self):
		"""Slot for signals clicked() from calendarWidget (QCalendarWidget) and
		valueChanged(int) from minuteSpinBox (QSpinBox) and hourSpinBox
		(QSpinBox). Updates currentDate (QLineEdit)."""
		caldate = self.calendarWidget.selectedDate().toPyDate()
		current = datetime.datetime.combine(caldate,
					datetime.time(self.hourSpinBox.value(),
								  self.minuteSpinBox.value()))
		self.currentDate.setText(current.strftime('%Y-%m-%d %H:%M'))
		self.currentDatetime = current

	# end of updateDate()

# end of DatetimeDialog_impl class


class AboutDialog_impl(QDialog, Ui_AboutDialog):
	"""Dialog for showing application info (pyQt4)."""

	def __init__(self, parent=None):
		"""AboutDialog_impl class constructor."""
		QDialog.__init__(self, parent)
		self.setupUi(self)

		# set app info
		self.setWindowTitle(_translate("AboutDialog", "About ") + "Filler 2")
		version_str = "Filler " + __version__
		if __status__ != "production": version_str += " (%s)" % __status__
		self.versionInfo.setText(version_str)
		self.info.setText(_translate("MainWindow",
			"Filler 2 is a tool for manual szbase data editing."))
		self.copyright.setText(__copyright__)
		self.website.setText('<a href="http://newterm.pl/">http://newterm.pl/</a>')

	# end of __init__()

	def showLicense(self):
		"""Show license in message box."""
		l = QMessageBox()
		l.setWindowTitle("SZARP Filler 2 - " +
						 _translate("AboutDialog", "License"))
		l.setText(__license__)
		l.exec_()

	# end of showLicense()

	def showCredits(self):
		"""Show credits in message box."""
		l = QMessageBox()
		l.setWindowTitle("SZARP Filler 2 - " +
						 _translate("AboutDialog", "Credits"))
		l.setText(__author__)
		l.exec_()

	# end of showCredits()

# end of AboutDialog_impl class


class HistoryDialog_impl(QDialog, Ui_HistoryDialog):
	"""Dialog for undoing changes committed to szbase by Filler 2 (pyQt4)."""

	def __init__(self, parser, parent=None):
		"""HistoryDialog_impl class constructor.  """
		# initialization
		QDialog.__init__(self, parent)
		self.setupUi(self)
		self.parent = parent
		self.parser = parser

		# table of changes
		self.changesTable.setColumnCount(6)
		self.changesTable.setColumnWidth(0, 350)   # parameter's draw_name
		self.changesTable.setColumnWidth(1, 170)   # change date
		self.changesTable.setColumnWidth(2, 100)   # user
		self.changesTable.setColumnWidth(3, 50)    # show graph
		self.changesTable.setColumnWidth(4, 50)    # undo button
		self.changesTable.setColumnHidden(5, True) # parameter's full name
		self.changesTable.setHorizontalHeaderLabels([
			_translate("HistoryDialog", "Parameter name (set name)"),
			_translate("HistoryDialog", "Change date"),
			_translate("HistoryDialog", "User"),
			"", ""])

		self.refresh()

	# end of __init__()

	def addRows(self, szfs_list):
		"""Add rows to changesTable (QTableWidget).

		Arguments:
			szfs_list - a list of changes present in szbase directory (*.szf)
			            as read by IPKParser.
		"""
		self.changesTable.setRowCount(len(szfs_list))

		row = 0
		for szf in szfs_list:
			# visible columns
			item_pname = QTableWidgetItem(
					unicode("%s (%s)" % (szf.draw_name, szf.set_name)))
			item_pname.setFlags(Qt.ItemIsEnabled)

			item_user = QTableWidgetItem(unicode(szf.user))
			item_user.setFlags(Qt.ItemIsEnabled)
			item_user.setTextAlignment(Qt.AlignCenter)

			item_date = QTableWidgetItem(szf.date.strftime('%Y-%m-%d %H:%M'))
			item_date.setFlags(Qt.ItemIsEnabled)
			item_date.setTextAlignment(Qt.AlignCenter)

			# hidden columns
			item_fname = QTableWidgetItem(unicode(szf.name))
			item_fname.setFlags(Qt.ItemIsEnabled)
			item_fname.setData(Qt.UserRole, QVariant((szf.vals, szf.lswmsw)))

			self.changesTable.setItem(row, 0, item_pname)
			self.changesTable.setItem(row, 1, item_date)
			self.changesTable.setItem(row, 2, item_user)
			self.changesTable.setItem(row, 5, item_fname)

			# "show graph" button widget
			show_button = QPushButton(
					QIcon.fromTheme("utilities-system-monitor"), "")
			show_button.setToolTip(_translate("HistoryDialog",
				"Show parameter's value chart before this change"))
			show_button.row_id = row
			QObject.connect(show_button, SIGNAL("clicked()"), self.showGraph)
			self.changesTable.setCellWidget(row, 3, show_button)

			# "undo" button widget
			undo_button = QPushButton(QIcon.fromTheme("undo"), "")
			undo_button.setToolTip(_translate("HistoryDialog",
				"Recover state of before change was made (new change will be introduced)"))
			undo_button.row_id = row
			QObject.connect(undo_button, SIGNAL("clicked()"), self.undoChange)
			self.changesTable.setCellWidget(row, 4, undo_button)

			row +=1

	# end of addRows()

	def refresh(self):
		"""Refresh list of changes present in database."""
		self.changesTable.setRowCount(0)
		szfs = self.parser.readSzfRecords()
		sorted_szfs = sorted(szfs, key=lambda x: x.date, reverse=True)
		self.addRows(sorted_szfs)

	# end of refresh()

	def undoChange(self):
		"""Slot for signal clicked() from undo_button (QPushButton). Reverts
		change listed in changesTable (QTableWidget).
		"""
		if QMessageBox.Yes != \
				self.parent.questionBox(_translate("HistoryDialog",
					"Are you sure you want to undo this change?\n"
					"(new overwriting change will be introduced)")):
			return

		row = self.sender().row_id

		# fetch changes info
		name = unicode(self.changesTable.item(row, 5).text())
		draw_name = self.changesTable.item(row, 0).text()
		vals, lswmsw = self.changesTable.item(row, 5).data(Qt.UserRole).toPyObject()
		dts = [d for d,v in vals]

		try:
			self.parser.recordSzf(name, dts, lswmsw)
			self.parser.szbWriter(name, vals)
			self.parent.infoBox(_translate("HistoryDialog",
				"Undoing change finished successfully."))
		except IPKParser.SzfRecordError:
			self.parent.warningBox(
					_translate("HistoryDialog",
						"Failed to undo change for parameter ")
					+ draw_name + ". " +
					_translate("HistoryDialog",
						"Encountered an error while making change record."))
		except IPKParser.SzbWriterError:
			self.parent.warningBox(
					_translate("HistoryDialog",
						"Failed to undo change for parameter ")
					+ draw_name + ". " +
					_translate("HistoryDialog",
						"Encountered an error while writing to the database."))
		self.refresh()

	# end of undoChange()

	def showGraph(self):
		"""Slot for signal clicked() from show_button (QPushButton). Shows
		graph of a parameter listed in changesTable (QTableWidget).
		"""
		row = self.sender().row_id

		# fetch parameter's data
		draw_name = self.changesTable.item(row, 0).text()
		name = self.changesTable.item(row, 5).text()
		vals, lswmsw = self.changesTable.item(row, 5).data(Qt.UserRole).toPyObject()
		orgi_d = [i[0] for i in vals]
		v = [i[1] for i in vals]
		d = orgi_d
		curr_d = d
		curr_v = self.parser.extrszb10(name, curr_d)

		# validate data
		only_nan = True
		for i in v:
			if not math.isnan(i):
				only_nan = False
		if only_nan:
			v = []
			d = []

		only_nan = True
		for i in curr_v:
			if not math.isnan(i):
				only_nan = False
		if only_nan:
			curr_v = []
			curr_d = []

		if len(v) == 0 and len(curr_v) == 0:
			self.parent.infoBox(_translate("HistoryDialog", "No data to be "
				"plotted - parameter is empty (both before and after the "
				"change) in period from ") +
				orgi_d[0].strftime('%Y-%m-%d %H:%M') +
				_translate("HistoryDialog", " to ") +
				orgi_d[-1].strftime('%Y-%m-%d %H:%M') +
				".")
			return

		# show graph of past and current values
		plt.cla()
		plt.clf()
		plt.plot(d, v, linewidth=2, color='blue', linestyle='--',
				label=unicode(_translate("HistoryDialog", "before change")))
		plt.plot(curr_d, curr_v, linewidth=2, color='red',
				label=unicode(_translate("HistoryDialog", "current value")))

		plt.gcf().autofmt_xdate(rotation=-30, ha='left')
		hfmt = mdates.DateFormatter('%Y-%m-%d %H:%M')
		plt.gca().xaxis.set_major_formatter(hfmt)
		plt.ylabel(unicode(_translate("HistoryDialog", "Parameter's value")))

		plt.legend()
		plt.title(unicode(draw_name))
		plt.show()

# end of HistoryDialog_impl class


class SzbWriter(QThread):
	"""Worker class for doing szbase modifications in a separate thread."""

	jobDone = pyqtSignal()                # signal: all jobs finished
	paramDone = pyqtSignal(int, str, int) # signal: one parameter done

	def __init__(self, changes_list, parser):
		"""SzbWriter class constructor.

		Arguments:
			changes_list - list of parameter changes to be introduced to SZARP
			               database as a list of tuples (param_draw_name,
			               param_full_name, start_date, end_date, value).
		"""
		QThread.__init__(self)
		self.chlist = changes_list
		self.parser = parser
		self.parser.initRemarks('gcwp-filler2', '85064efb60a9601805dcea56ec5402f7')

	# end of __init__()

	def run(self):
		"""Run SzbWriter jobs."""
		nr = 1
		for ch in self.chlist:
			dts = [dv[0] for dv in ch.dvalues]
			title = u"Modyfikacja parameteru %s (Filler 2)" % ch.draw_name
			remark = ch.remark % \
					(datetime.datetime.now().strftime('%Y/%m/%d %H:%M'),
					 getpass.getuser())

			try:
				self.parser.recordSzf(ch.name, dts, ch.lswmsw)
				self.parser.szbWriter(ch.name, ch.dvalues)
				self.parser.postRemark(ch.name, dts[0], title, remark)
				self.paramDone.emit(nr, ch.draw_name, 0)
			except IPKParser.SzfRecordError:
				self.paramDone.emit(nr, ch.draw_name, 1)
			except IPKParser.SzbWriterError:
				self.paramDone.emit(nr, ch.draw_name, 2)

			time.sleep(1)
			nr += 1

		self.parser.closeRemarks()
		self.jobDone.emit()

	# end of run()

# end of SzbWriter class


class SzbProgressWin(QDialog):
	"""Dialog showing SzbWriter's work progress (pyQt4)."""

	def __init__(self, szb_writer, parent=None):
		"""SzbProgressWin class constructor.

		Arguments:
			szb_writer - SzbWriter object.
		"""
		QDialog.__init__(self, parent)

		self.szb_thread = szb_writer
		self.len = len(szb_writer.chlist)
		self.parentWin = parent
		self.failed_jobs = []

		# construct and set dialog's elements
		self.nameLabel = QLabel("0 / %s" % (self.len))
		self.paramLabel = QLabel(_translate("MainWindow", "Preparing..."))

		self.progressbar = QProgressBar()
		self.progressbar.setMinimum(0)
		self.progressbar.setMaximum(self.len)

		self.setMinimumSize(400, 100)

		mainLayout = QVBoxLayout()
		mainLayout.addWidget(self.paramLabel)

		pbarLayout = QHBoxLayout()
		pbarLayout.addWidget(self.progressbar)
		pbarLayout.addWidget(self.nameLabel)
		mainLayout.addLayout(pbarLayout)

		self.setLayout(mainLayout)
		self.setWindowTitle(_translate("MainWindow", "Writing to szbase..."))

		self.szb_thread.paramDone.connect(self.update)
		self.szb_thread.jobDone.connect(self.fin)

		# show dialog and start processing jobs
		self.show()
		self.szb_thread.start()

	# end of __init__()

	def update(self, val, pname, status):
		"""Slot for signal paramDone() from szb_thread (SzbWriter). Updates
		progress bar state.

		Arguments:
			val - number of processed parameter.
			pname - processed parameter's name.
			status - job exit status.
		"""
		self.progressbar.setValue(val)
		progress = "%s / %s" % (val, self.len)
		done = ""

		if status != 0:
			done = _translate("MainWindow", "failed.")
			self.failed_jobs.append((pname, status))
		else:
			done = _translate("MainWindow", "done.")

		self.nameLabel.setText(progress)
		self.paramLabel.setText(_translate("MainWindow", "Writing parameter") +
				" " + pname + "... " + done)

	# end of update()

	def fin(self):
		"""Slot for signal jobDone() from szb_thread (SzbWriter). Shows info
		about a finished job.
		"""
		self.hide()
		if len(self.failed_jobs) == 0:
			self.parentWin.infoBox(_translate("MainWindow", "Writing to szbase done."))
		else:
			err = [
				"",
				_translate("MainWindow", "failed to make change record, writing aborted"),
				_translate("MainWindow", "failed to write to database, data may be inconsistent")
				]
			estr = ""
			for p,e in self.failed_jobs:
				estr += "* %s - %s\n\n" % (p, err[e])
			self.parentWin.warningBox(_translate("MainWindow",
				"Writing to szbase done. Following parameters failed to proceed:")
				+ "\n\n" + estr)
		self.parentWin.setEnabled(True)

	# end of fin()

# end of SzbProgressWin class

def parse_arguments(argv):
	"""Parse arguments for Filler 2."""
	parser = argparse.ArgumentParser(
			prog=__script_name__,
			description=__doc__,
			epilog="Mail bug reports and suggestions to <%s>." % __email__,
			formatter_class=CustomFormatter
			)
	parser._optionals.title = "Startup"

	# optional startup arguments
	parser.add_argument('-V', '--version', action='version',
						version=format_version())
	parser.add_argument('-n', '--no-warn', action='store_true',
						help="do not display safety warnings")
	parser.add_argument('-Dprefix', type=str, metavar='PREFIX',
						help="SZARP database prefix")

	# override argparse functions
	parser.format_usage = types.MethodType(format_usage, parser)

	args, uargs = parser.parse_known_args(args=argv)

	return parser, args, uargs

# end of parse_arguments()

def main(argv=None):
	"main() function."
	if argv is None:
		argv = sys.argv

	# deal with command line arguments
	parser, args, uargs = parse_arguments(argv)

	app = QApplication(uargs)

	argv2 = app.arguments()
	if len(argv2) != 1:
		for i in argv2[1:]:
			print >>sys.stderr, \
				'%s: warning: unknown option \'%s\'' % (__script_name__, str(i))

	# initialize standard system translator
	qt_translator = QTranslator()
	qt_translator.load("qt_" + QLocale.system().name(),
			QLibraryInfo.location(QLibraryInfo.TranslationsPath))
	app.installTranslator(qt_translator)

	# initialize filler2-specific translator
	filler2_translator = QTranslator()
	filler2_translator.load("filler2_" + QLocale.system().name(),
			"/opt/szarp/resources/locales/qt4")
	app.installTranslator(filler2_translator)

	# start Filler 2 application
	QIcon.setThemeName("Tango")
	filler2app = Filler2(args.Dprefix, args.no_warn)
	filler2app.show()

	return app.exec_()

# end of main()

if __name__ == "__main__":
	sys.exit(main())
