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

		self.fromDate = None
		self.toDate = None

	def onSetChosen(self, text):
		self.ui.paramList.clear()
		self.ui.paramList.addItems(self.parser.getParams())

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
			self.ui.fromDate.setText(_translate("MainWindow", "From:", None)
					+ " " + self.fromDate.strftime('%Y-%m-%d %H:%M'))

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
			self.ui.toDate.setText(_translate("MainWindow", "To:", None)
					+ " " + self.toDate.strftime('%Y-%m-%d %H:%M'))

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

