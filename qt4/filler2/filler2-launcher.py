#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import datetime
from PyQt4.QtCore import *
from PyQt4.QtGui import *

from filler2 import Ui_MainWindow
from ipkparser import IPKParser
from DatetimeDialog import Ui_DatetimeDialog

try:
    _encoding = QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QApplication.translate(context, text, disambig)

class StartQT4(QMainWindow):
	def __init__(self, parent=None):
		QWidget.__init__(self, parent)
		self.ui = Ui_MainWindow()
		self.ui.setupUi(self)
		self.parser = IPKParser()

		self.ui.titleLabel.setText(self.parser.getTitle())

		self.ui.listOfSets.addItems(self.parser.getSets())
		self.ui.listOfSets.setEnabled(True)

	def onSetChosen(self, text):
		self.ui.paramList.clear()
		self.ui.paramList.addItems(self.parser.getParams())

		self.ui.paramList.setEnabled(True)

	def onParamChosen(self, text):
		self.ui.fromDate.setEnabled(True)
		self.ui.toDate.setEnabled(True)
		self.ui.valueEdit.setEnabled(True)

	def onFromDate(self):
		dlg = DatetimeDialog_impl()
		if dlg.exec_():
			dt = dlg.getValue()
			self.ui.fromDate.setText(_translate("MainWindow", "From:", None)
					+ " " + dt.strftime('%Y-%m-%d %H:%M'))

	def onToDate(self):
		dlg = DatetimeDialog_impl()
		if dlg.exec_():
			dt = dlg.getValue()
			self.ui.toDate.setText(_translate("MainWindow", "To:", None)
					+ " " + dt.strftime('%Y-%m-%d %H:%M'))

class DatetimeDialog_impl(QDialog, Ui_DatetimeDialog):
	def __init__(self,parent=None):
		QDialog.__init__(self,parent)
		self.setupUi(self)
		self.calendarWidget.setLocale(QLocale.system())

		# load current date and time
		now = datetime.datetime.now()
		now -= datetime.timedelta(minutes=now.minute % 10,
									seconds=now.second,
									microseconds=now.microsecond)

		# set values in widgets
		self.hourSpinBox.setValue(now.hour)
		self.minuteSpinBox.setValue(now.minute)
		self.currentDate.setText(now.strftime('%Y-%m-%d %H:%M'))

		self.currentDatetime = now

	def getValue(self):
		return self.currentDatetime

	def updateDate(self):
		caldate = self.calendarWidget.selectedDate().toPyDate()
		current = datetime.datetime.combine(caldate,
					datetime.time(self.hourSpinBox.value(),
								  self.minuteSpinBox.value()))
		self.currentDate.setText(current.strftime('%Y-%m-%d %H:%M'))

		self.currentDatetime = current

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

