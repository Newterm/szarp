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
import time
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

		# initialize table of changes
		self.ui.changesTable.setColumnCount(6)
		self.ui.changesTable.setColumnWidth(0, 390)
		self.ui.changesTable.setColumnWidth(1, 205)
		self.ui.changesTable.setColumnWidth(2, 210)
		self.ui.changesTable.setColumnWidth(3, 130)
		self.ui.changesTable.setColumnWidth(4, 50)
		self.ui.changesTable.setColumnHidden(5, True)
		self.ui.changesTable.horizontalHeader().setVisible(False)
		self.ui.changesTable.setRowCount(0)

	def criticalError(self, msg, title = None):
		if title is None:
			title = "SZARP Filler 2 - " + \
				_translate("MainWindow", "Critical Error")
		QMessageBox.critical(self, title, msg)
		sys.exit(1)

	def warningBox(self, msg, title = None):
		if title is None:
			title = "SZARP Filler 2 - " + _translate("MainWindow", "Warning")
		QMessageBox.warning(self, title, msg)

	def infoBox(self, msg, title = None):
		if title is None:
			title = "SZARP Filler 2 - " + _translate("MainWindow", "Information")
		QMessageBox.information(self, title, msg)

	def questionBox(self, msg, title = None):
		if title is None:
			title = "SZARP Filler 2 - " + _translate("MainWindow", "Question")
		return QMessageBox.question(self, title, msg,
				QMessageBox.Yes | QMessageBox.No, QMessageBox.Yes)

	def onSetChosen(self, text):
		self.ui.paramList.clear()
		self.ui.paramList.addItem(
				_translate("MainWindow", "--- Choose a parameter ---"))
		self.ui.paramList.model().setData(
				self.ui.paramList.model().index(0,0),
				QVariant(0), Qt.UserRole-1)

		for name in self.parser.getParams(unicode(text)):
			self.ui.paramList.addItem(name[1], name[0])

		self.ui.paramList.setEnabled(True)
		self.ui.addButton.setEnabled(False)

	def onParamChosen(self, text):
		self.ui.fromDate.setEnabled(True)
		self.ui.toDate.setEnabled(True)
		self.ui.valueEdit.setEnabled(True)
		self.ui.valueEdit.setReadOnly(False)
		self.validateInput()

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
		if self.ui.paramList.currentIndex() != 0 and \
			self.fromDate is not None and self.toDate is not None and \
			len(self.ui.valueEdit.text()) > 0:
				self.ui.addButton.setEnabled(True)
		else:
				self.ui.addButton.setEnabled(False)

	def aboutQt(self):
		QMessageBox.aboutQt(self)

	def about(self):
		AboutDialog_impl().exec_()

	def addChange(self):
		if self.fromDate >= self.toDate:
			self.warningBox(_translate("MainWindow",
				"\"To\" date is earlier (or equals) \"From\" date.\nAdding change aborted."))
		else:
			self.ui.changesTable.setRowCount(self.ui.changesTable.rowCount()+1)
			self.addRow(self.ui.changesTable.rowCount() - 1,
						self.ui.paramList.itemData(self.ui.paramList.currentIndex()).toString(),
						self.ui.paramList.currentText(),
						self.fromDate, self.toDate,
						self.ui.valueEdit.text())

	def addRow(self, row, fname, pname, from_date, to_date, value):
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
		item_fname = QTableWidgetItem(unicode(fname))
		item_fname.setFlags(Qt.ItemIsEnabled)

		self.ui.changesTable.setItem(row, 0, item_pname)
		self.ui.changesTable.setItem(row, 1, item_from_date)
		self.ui.changesTable.setItem(row, 2, item_to_date)
		self.ui.changesTable.setItem(row, 3, item_value)
		self.ui.changesTable.setItem(row, 5, item_fname)

		rm_button = QPushButton(QIcon.fromTheme("window-close"), "")
		rm_button.setToolTip(_translate("MainWindow", "Remove entry"))
		rm_button.row_id = row
		QObject.connect(rm_button, SIGNAL("clicked()"), self.removeChange)
		self.ui.changesTable.setCellWidget(row, 4, rm_button)

	def removeChange(self):
		if QMessageBox.Yes != \
				self.questionBox(_translate("MainWindow", "Remove change?")):
			return

		row = self.sender().row_id
		self.ui.changesTable.removeRow(row)

		for i in range(row, self.ui.changesTable.rowCount()):
			self.ui.changesTable.cellWidget(i, 4).row_id -= 1

	def clearChanges(self):
		self.ui.changesTable.setRowCount(0)

	def commitChanges(self):
		if self.ui.changesTable.rowCount() == 0:
			self.warningBox(_translate("MainWindow", "No changes to commit."))
			return

		txt = _translate("MainWindow",
				"Following parameters will be modified:") + "\n\n"

		for i in range(0, self.ui.changesTable.rowCount()):
			txt.append("   * %s\n     (%s)\n\n" \
					% (self.ui.changesTable.item(i,0).text(),
					   self.ui.changesTable.item(i,5).text()))

		txt.append(_translate("MainWindow", "Commit changes?"))

		if QMessageBox.Yes == self.questionBox(txt):
			changes_list = []
			for i in range(0, self.ui.changesTable.rowCount()):
				changes_list.append((
						unicode(self.ui.changesTable.item(i,0).text()),
						unicode(self.ui.changesTable.item(i,5).text()),
						datetime.datetime.strptime(str(self.ui.changesTable.item(i,1).text()), '%Y-%m-%d %H:%M'),
						datetime.datetime.strptime(str(self.ui.changesTable.item(i,2).text()), '%Y-%m-%d %H:%M'),
						float(self.ui.changesTable.item(i,3).text())
						))

			szbw = SzbWriter(changes_list)
			szbp = SzbProgressWin(szbw, parent = self)

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
			self.ui.valueEdit.setText("")
			self.ui.valueEdit.setEnabled(False)
			self.ui.valueEdit.setReadOnly(False)
			self.ui.addButton.setEnabled(False)

			self.setEnabled(False)

	def contextHelp(self):
		QWhatsThis.enterWhatsThisMode()

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
		QDialog.__init__(self, parent)
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

class SzbWriter(QThread):
	jobDone = pyqtSignal()
	paramDone = pyqtSignal(int, str)

	def __init__(self, changes_list):
		QThread.__init__(self)
		self.plist = changes_list

	def run(self):
		i = 1
		for p in self.plist:
			print "Writing param: ", p
			self.paramDone.emit(i, p[0])
			time.sleep(1)
			i += 1

		self.jobDone.emit()

class SzbProgressWin(QDialog):
	def __init__(self, szb_writer, parent=None):
		QDialog.__init__(self, parent)

		self.szb_thread = szb_writer
		self.len = len(szb_writer.plist)
		self.parentWin = parent

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

		self.show()
		self.szb_thread.start()

	def update(self, val, pname):
		self.progressbar.setValue(val)
		progress = "%s / %s" % (val, self.len)
		self.nameLabel.setText(progress)
		self.paramLabel.setText(_translate("MainWindow", "Writing parameter") +
				" " + pname + "...")

	def fin(self):
		self.hide()
		self.parentWin.infoBox(_translate("MainWindow", "Writing to szbase done."))
		self.parentWin.setEnabled(True)

if __name__ == "__main__":
	app = QApplication(sys.argv)

	qt_translator = QTranslator()
	qt_translator.load("qt_" + QLocale.system().name(),
			QLibraryInfo.location(QLibraryInfo.TranslationsPath))
	app.installTranslator(qt_translator)

	filler2_translator = QTranslator()
	filler2_translator.load("filler2_" + QLocale.system().name(),
			"/opt/szarp/resources/locales/qt4")
	app.installTranslator(filler2_translator)

	QIcon.setThemeName("Tango")

	myapp = StartQT4()
	myapp.show()
	sys.exit(app.exec_())

