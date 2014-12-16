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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 MA  02110-1301, USA """

__author__    = "Tomasz Pieczerak <tph AT newterm.pl>"
__copyright__ = "Copyright (C) 2014-2015 Newterm"
__version__   = "2.0"
__status__    = "devel"
__email__     = "coders AT newterm.pl"


import sys
import datetime
from PyQt4.QtCore import *
from PyQt4.QtGui import *

from filler2 import Ui_MainWindow
from ipkparser import IPKParser
from DatetimeDialog import Ui_DatetimeDialog
from AboutDialog import Ui_AboutDialog

try:
    _encoding = QApplication.UnicodeUTF8
    def _translate(context, text, disambig=None):
        return QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig=None):
        return QApplication.translate(context, text, disambig)

class StartQT4(QMainWindow):
	def __init__(self, parent=None):
		QWidget.__init__(self, parent)
		self.ui = Ui_MainWindow()
		self.ui.setupUi(self)
		try:
			self.parser = IPKParser("/etc/szarp/default/config/params.xml")
		except IOError:
			self.criticalError(_translate("MainWindow",
						"Cannot read SZARP configuration (params.xml)"))
			sys.exit(1)

		self.ui.titleLabel.setText(self.parser.getTitle())

		self.ui.listOfSets.addItem(
				_translate("MainWindow", "--- Choose a set of parameters ---"))
		self.ui.listOfSets.addItems(self.parser.getSets())
		self.ui.listOfSets.model().setData(
				self.ui.listOfSets.model().index(0,0),
				QVariant(0), Qt.UserRole-1)
		self.ui.listOfSets.setEnabled(True)

		self.ui.valueEdit.setValidator(QDoubleValidator())

		self.fromDate = None
		self.toDate = None

		self.changesCount = 0

		self.ui.changesTable.setColumnCount(5)
		self.ui.changesTable.setColumnWidth(0, 390)
		self.ui.changesTable.setColumnWidth(1, 205)
		self.ui.changesTable.setColumnWidth(2, 210)
		self.ui.changesTable.setColumnWidth(3, 130)
		self.ui.changesTable.setColumnWidth(4, 50)
		self.ui.changesTable.horizontalHeader().setVisible(False)
		self.ui.changesTable.setRowCount(self.changesCount)

	def criticalError(self, msg, title = "SZARP Filler 2 - " +
					  _translate("MainWindow", "Critical Error")):
		QMessageBox.critical(self, title, msg)
		sys.exit(1)

	def onSetChosen(self, text):
		self.ui.paramList.clear()
		self.ui.paramList.addItem(
				_translate("MainWindow", "--- Choose a parameter ---"))
		self.ui.paramList.model().setData(
				self.ui.paramList.model().index(0,0),
				QVariant(0), Qt.UserRole-1)

		for name in self.parser.getParams(unicode(text)):
			self.ui.paramList.addItem(name[1])

		self.ui.paramList.setEnabled(True)

	def onParamChosen(self, text):
		self.ui.fromDate.setEnabled(True)
		self.ui.toDate.setEnabled(True)
		self.ui.valueEdit.setEnabled(True)
		self.ui.valueEdit.setReadOnly(False)

	def onFromDate(self):
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

		if dlg.exec_():
			self.fromDate = dlg.getValue()
			self.ui.fromDate.setText(_translate("MainWindow", "From:") + " " +
					self.fromDate.strftime('%Y-%m-%d %H:%M'))
			self.validateInput()

	def onToDate(self):
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

		if dlg.exec_():
			self.toDate = dlg.getValue()
			self.ui.toDate.setText(_translate("MainWindow", "To:") + " " +
					self.toDate.strftime('%Y-%m-%d %H:%M'))
			self.validateInput()

	def onValueChanged(self):
		new_value = self.ui.valueEdit.text()
		try:
			self.ui.valueEdit.setText(str(float(new_value)))
		except ValueError:
			self.ui.valueEdit.setText("")
		self.validateInput()

	def validateInput(self):
		if self.fromDate is not None and self.toDate is not None \
			and len(self.ui.valueEdit.text()) > 0:
				self.ui.addButton.setEnabled(True)
		else:
				self.ui.addButton.setEnabled(False)

	def aboutQt(self):
		QMessageBox.aboutQt(self)

	def about(self):
		AboutDialog_impl().exec_()

	def addChange(self):
		self.changesCount += 1
		self.ui.changesTable.setRowCount(self.changesCount)

		self.addRow(self.changesCount - 1,
					self.ui.paramList.currentText(),
					self.fromDate, self.toDate,
					self.ui.valueEdit.text())

	def addRow(self, row, pname, from_date, to_date, value):
		item_pname = QTableWidgetItem(unicode(pname))
		item_pname.setFlags(Qt.ItemIsEnabled)
		item_from_date = QTableWidgetItem(from_date.strftime('%Y-%m-%d %H:%M'))
		item_from_date.setFlags(Qt.ItemIsEnabled)
		item_from_date.setTextAlignment(Qt.AlignCenter)
		item_to_date = QTableWidgetItem(to_date.strftime('%Y-%m-%d %H:%M'))
		item_to_date.setFlags(Qt.ItemIsEnabled)
		item_to_date.setTextAlignment(Qt.AlignCenter)
		item_value = QTableWidgetItem(str(value))
		item_value.setFlags(Qt.ItemIsEnabled)
		item_value.setTextAlignment(Qt.AlignCenter)

		self.ui.changesTable.setItem(row, 0, item_pname)
		self.ui.changesTable.setItem(row, 1, item_from_date)
		self.ui.changesTable.setItem(row, 2, item_to_date)
		self.ui.changesTable.setItem(row, 3, item_value)

		rm_button = QPushButton(QIcon.fromTheme("window-close"), "")
		rm_button.setEnabled(False)
		QObject.connect(rm_button, SIGNAL("clicked()"), self.removeChange)
		self.ui.changesTable.setCellWidget(row, 4, rm_button)

	def removeChange(self):
		# TODO
		pass

	def clearChanges(self):
		self.ui.changesTable.setRowCount(0)
		self.changesCount = 0

class DatetimeDialog_impl(QDialog, Ui_DatetimeDialog):
	def __init__(self, parent=None, start_date=datetime.datetime.now()):
		QDialog.__init__(self,parent)
		self.setupUi(self)
		self.calendarWidget.setLocale(QLocale.system())

		# load current date and time
		start_date -= datetime.timedelta(minutes=start_date.minute % 10,
									seconds=start_date.second,
									microseconds=start_date.microsecond)

		# set values in widgets
		self.calendarWidget.setSelectedDate(start_date)
		self.hourSpinBox.setValue(start_date.hour)
		self.minuteSpinBox.setValue(start_date.minute)
		self.currentDate.setText(start_date.strftime('%Y-%m-%d %H:%M'))

		self.currentDatetime = start_date

	def getValue(self):
		return self.currentDatetime

	def updateDate(self):
		caldate = self.calendarWidget.selectedDate().toPyDate()
		current = datetime.datetime.combine(caldate,
					datetime.time(self.hourSpinBox.value(),
								  self.minuteSpinBox.value()))
		self.currentDate.setText(current.strftime('%Y-%m-%d %H:%M'))

		self.currentDatetime = current

class AboutDialog_impl(QDialog, Ui_AboutDialog):
	def __init__(self, parent=None):
		QDialog.__init__(self,parent)
		self.setupUi(self)
		self.setWindowTitle(_translate("AboutDialog", "About ") + "Filler 2")
		self.versionInfo.setText("Filler " + __version__ + " (%s)" % __status__)
		self.info.setText(_translate("MainWindow",
			"Filler 2 is a tool for manual szbase data editing."))
		self.copyright.setText(__copyright__)
		self.website.setText('<a href="http://newterm.pl/">http://newterm.pl/</a>')

	def showLicense(self):
		l = QMessageBox()
		l.setWindowTitle("SZARP Filler 2 - " +
						 _translate("AboutDialog", "License"))
		l.setText(__license__)
		l.exec_()

	def showCredits(self):
		l = QMessageBox()
		l.setWindowTitle("SZARP Filler 2 - " +
						 _translate("AboutDialog", "Credits"))
		l.setText(__author__)
		l.exec_()

if __name__ == "__main__":
	app = QApplication(sys.argv)

	qt_translator = QTranslator()
	qt_translator.load("qt_" + QLocale.system().name(),
			QLibraryInfo.location(QLibraryInfo.TranslationsPath))
	qt_translator.load("filler2_" + QLocale.system().name(),
			"/opt/szarp/resources/locales/qt4")
	app.installTranslator(qt_translator)

	QIcon.setThemeName("Tango")

	myapp = StartQT4()
	myapp.show()
	sys.exit(app.exec_())

