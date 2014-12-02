#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
from PyQt4.QtCore import *
from PyQt4.QtGui import *

from filler2 import Ui_MainWindow
from ipkparser import IPKParser

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

