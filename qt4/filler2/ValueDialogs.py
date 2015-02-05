# -*- coding: utf-8 -*-

# imports
import datetime
from PyQt4.QtCore import *
from PyQt4.QtGui import *

# constants
SZLIMIT = 32767.0
SZLIMIT_COM = 2147483647.0
NODATA = -32768.0
NODATA_COM = -2147483648.0

# translation function for QTranslator
try:
	_encoding = QApplication.UnicodeUTF8
	def _translate(context, text):
		return QApplication.translate(context, text, None, _encoding)
except AttributeError:
	def _translate(context, text):
		return QApplication.translate(context, text, None)

class ValueDialogFactory:
	def __init__(self):
		self.availableDialogs = [ "BlankDialog",
				"NoDataDialog", "ConstDialog",
				"LinearIncDialog", "LinearDecDialog"
				]

	def construct(self, dialogName, prec, lswmsw, parent=None):
		targetDialog = getattr(ValueDialogs, dialogName)
		dialog = targetDialog(prec, lswmsw, parent)
		return dialog

	def get_dialogs(self):
		lst = []
		for d in self.availableDialogs:
			td = getattr(ValueDialogs, d)
			icon = QIcon(td.qicon_path)
			desc = td.desc
			lst.append((d, icon, desc))
		return lst


class ValueDialogs:
	class ValueDialogInterface(QDialog):
		"""Sample class with ValueDialog's interface."""
		qicon_path = None   # path to plot type icon
		desc = None         # plot type name

		def __init__(self, prec, lswmsw, parent=None):
			"""Define and initialize dialog."""
			pass

		def generate(self, dates):
			"""Generate probes for given dates."""
			return [(None, None)]

		def get_value_desc(self):
			"""Return value description string."""
			return ""

	class BlankDialog():
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-blank.png"
		desc = _translate("ValueDialogs", "Choose a type of plot")

	class ConstDialog(QDialog):
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-const.png"
		desc = _translate("ValueDialogs", "Constant value")

		def __init__(self, prec, lswmsw, parent=None):
			QDialog.__init__(self, parent)
			self.parent = parent
			self.prec = int(prec)
			self.lswmsw = lswmsw

			self.setWindowIcon(QIcon(self.qicon_path))
			self.setModal(True)
			self.setWindowTitle(_translate("ValueDialogs",
								"Enter a constant value"))
			self.setToolTip(_translate("ValueDialogs",
				"This is a dialog for inputing parameter\'s constant value."))

			self.mainLayout = QVBoxLayout()
			self.setLayout(self.mainLayout)

			self.label = QLabel()
			self.label.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.label.setText(_translate("ValueDialogs",
				"Enter parametr\'s value:"))
			self.mainLayout.addWidget(self.label)

			self.valueEdit = QLineEdit()
			self.valueEdit.setAutoFillBackground(True)
			self.valueEdit.setMaxLength(12)
			self.valueEdit.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.valueEdit.setToolTip(_translate("ValueDialogs",
				"Parameter\'s value"))
			self.valueEdit.setPlaceholderText("0.0")
			qdv = QDoubleValidator()
			qdv.setNotation(0)
			if lswmsw:
				qdv.setRange(SZLIMIT_COM * -1, SZLIMIT_COM, prec)
			else:
				qdv.setRange(SZLIMIT * -1, SZLIMIT, prec)
			self.valueEdit.setValidator(qdv)

			self.valueEdit.setText("")
			self.mainLayout.addWidget(self.valueEdit)

			self.buttonBox = QDialogButtonBox()
			self.buttonBox.setOrientation(Qt.Horizontal)
			self.buttonBox.setStandardButtons(QDialogButtonBox.Cancel|QDialogButtonBox.Ok)
			self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
			self.mainLayout.addWidget(self.buttonBox)

			QObject.connect(self.buttonBox, SIGNAL("accepted()"), self.accept)
			QObject.connect(self.buttonBox, SIGNAL("rejected()"), self.reject)
			QObject.connect(self.valueEdit, SIGNAL("lostFocus()"), self.onValueChanged)
			QObject.connect(self.valueEdit, SIGNAL("returnPressed()"), self.onValueChanged)
			QObject.connect(self.valueEdit, SIGNAL("textChanged(QString)"), self.validateInput)
			QMetaObject.connectSlotsByName(self)

		def onValueChanged(self):
			new_value = self.valueEdit.text()
			try:
				self.valueEdit.setText(str(float(new_value)))
			except ValueError:
				self.valueEdit.setText("")
			self.validateInput()

		def validateInput(self):
			if len(self.valueEdit.text()) > 0:
				self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)
			else:
				self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

		def accept(self):
			val = float(self.valueEdit.text())

			if (self.lswmsw and (val > SZLIMIT_COM or val < SZLIMIT_COM * -1)) \
				or ((not self.lswmsw) and (val > SZLIMIT or val < SZLIMIT * -1)):
					self.parent.warningBox(_translate("ValueDialogs",
						"Parameter's value is out of range."))
			else:
				self.val = val
				QDialog.accept(self)

		def generate(self, dates):
			dvals = [(d, self.val) for d in dates]
			return dvals

		def get_value_desc(self):
			return "x = %s" % self.val

	class NoDataDialog(QDialog):
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-nodata.png"
		desc = "No data"

		def __init__(self, prec, lswmsw, parent=None):
			QDialog.__init__(self, parent)
			self.parent = parent
			self.lswmsw = lswmsw

			self.setWindowIcon(QIcon(self.qicon_path))
			self.setModal(True)
			self.setWindowTitle(_translate("ValueDialogs",
				"Confirm NO_DATA insertion"))
			self.setToolTip(_translate("ValueDialogs", "This is a confirmation"
				" dialog for inserting NO_DATA values (i.e. deleting probes)."))

			self.mainLayout = QVBoxLayout()
			self.setLayout(self.mainLayout)

			self.label = QLabel()
			self.label.setWordWrap(True)
			self.label.setAlignment(Qt.AlignLeft|Qt.AlignVCenter)
			self.label.setMargin(10)
			self.label.setText(_translate("ValueDialogs", "This parameter "
				"probes in given time interval will be deleted."
				"\n\nAre you sure you want to do this?"))
			self.mainLayout.addWidget(self.label)

			self.buttonBox = QDialogButtonBox()
			self.buttonBox.setOrientation(Qt.Horizontal)
			self.buttonBox.setStandardButtons(QDialogButtonBox.Cancel|QDialogButtonBox.Ok)
			self.mainLayout.addWidget(self.buttonBox)

			QObject.connect(self.buttonBox, SIGNAL("accepted()"), self.accept)
			QObject.connect(self.buttonBox, SIGNAL("rejected()"), self.reject)
			QMetaObject.connectSlotsByName(self)

		def accept(self):
			if self.lswmsw:
				self.val = NODATA_COM
			else:
				self.val = NODATA
			QDialog.accept(self)

		def generate(self, dates):
			dvals = [(d, self.val) for d in dates]
			return dvals

		def get_value_desc(self):
			return "x = NO_DATA"

	class LinearIncDialog(QDialog):
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-inc.png"
		desc = "Linear increasing"

		def __init__(self, prec, lswmsw, parent=None):
			QDialog.__init__(self, parent)

		def generate(self):
			pass

	class LinearDecDialog(QDialog):
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-dec.png"
		desc = "Linear decreasing"

		def __init__(self, prec, lswmsw, parent=None):
			QDialog.__init__(self, parent)

		def generate(self):
			pass

