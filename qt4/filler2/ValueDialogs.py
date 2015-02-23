# -*- coding: utf-8 -*-

# imports
from PyQt4.QtCore import *
from PyQt4.QtGui import *

from ipkparser import SZB_LIMIT, SZB_LIMIT_COM, NO_DATA, NO_DATA_COM

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
			desc = _translate("ValueDialogs", td.desc)
			lst.append((d, icon, desc))
		return lst


class ValueDialogs:
	class ValueDialog(QDialog):
		"""Base class for ValueDialogs."""
		qicon_path = None   # path to plot type icon
		desc = None         # plot type name
		remark = False		# add a remark for a change?
		comment = None		# optional user comment to remark

		def __init__(self, prec, lswmsw, parent=None):
			"""Define and initialize dialog.

			Arguments:
				prec - parameter's precision.
				lswmsw - whether parameter is a combined one.
			"""
			QDialog.__init__(self, parent)

		def generate(self, dates):
			"""Generate probes for given dates.

			Arguments:
				dates - a list of probes' dates.

			Returns:
				dvals - a list of tuples (date, probe).
			"""
			return []

		def get_value_desc(self):
			"""Return value description string.

			Returns:
				desc_str - plot description string.
			"""
			return ""

		def addRemarkWidget(self, layout):
			"""Add widget for adding remark/comment to dialog. Should be
			called in dialog's constructor.

			Arguments:
				layout - main dialog layout.
			"""
			self.remarkCheck = QCheckBox(_translate("ValueDialogs", "Leave a remark in Draw3"))
			layout.addWidget(self.remarkCheck)

			self.commentEdit = QLineEdit()
			self.commentEdit.setAutoFillBackground(True)
			self.commentEdit.setMaxLength(140)
			self.commentEdit.setToolTip(_translate("ValueDialogs", "Enter an optional user comment."))
			self.commentEdit.setPlaceholderText(_translate("ValueDialogs", "Optional user comment"))
			self.commentEdit.hide()
			layout.addWidget(self.commentEdit)

			QObject.connect(self.remarkCheck, SIGNAL("stateChanged(int)"), self.toggleCommentEdit)

		def saveRemark(self):
			"""Save remark state and optional user's comment. Should be
			called in accept().
			"""
			if self.remarkCheck.checkState() == Qt.Checked:
				self.remark = True
				self.comment = unicode(self.commentEdit.text().toUtf8(), 'utf-8')
			else:
				self.remark = False
				self.comment = None

		def toggleCommentEdit(self, state):
			"""Slot for signal stateChanged(int) from remarkCheck  (QCheckBox).
			Shows/hides commentEdit (QLineEdit) when checkbox is toggled."""
			if state == Qt.Checked:
				self.commentEdit.show()
			else:
				self.commentEdit.hide()
			self.adjustSize()

		def get_remark(self):
			"""Return remark string or None if not set."""
			if self.remark:
				rmrk_str = u"Zmiana wprowadzona programem Filler 2 w dniu %s przez użytkownika %s"
				if len(self.comment) != 0:
					rmrk_str += u": " + self.comment
				rmrk_str += u"."
				return rmrk_str
			else:
				return None

	class BlankDialog():
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-blank.png"
		desc = _translate("ValueDialogs", "Choose a type of plot")

	class ConstDialog(ValueDialog):
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-const.png"
		desc = _translate("ValueDialogs", "Constant value")

		def __init__(self, prec, lswmsw, parent=None):
			ValueDialogs.ValueDialog.__init__(self, prec, lswmsw, parent)

			self.parent = parent
			self.prec = prec
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
				qdv.setRange(SZB_LIMIT_COM * -1, SZB_LIMIT_COM, prec)
			else:
				qdv.setRange(SZB_LIMIT * -1, SZB_LIMIT, prec)
			self.valueEdit.setValidator(qdv)

			self.valueEdit.setText("")
			self.mainLayout.addWidget(self.valueEdit)

			self.addRemarkWidget(self.mainLayout)

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

			if (self.lswmsw and (val > SZB_LIMIT_COM or val < SZB_LIMIT_COM * -1)) \
				or ((not self.lswmsw) and (val > SZB_LIMIT or val < SZB_LIMIT * -1)):
					self.parent.warningBox(_translate("ValueDialogs",
						"Parameter's value is out of range."))
			else:
				self.val = val
				self.saveRemark()
				QDialog.accept(self)

		def generate(self, dates):
			dvals = [(d, self.val) for d in dates]
			return dvals

		def get_value_desc(self):
			return "x = %s" % self.val

	class NoDataDialog(ValueDialog):
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-nodata.png"
		desc = _translate("ValueDialogs", "No data")

		def __init__(self, prec, lswmsw, parent=None):
			ValueDialogs.ValueDialog.__init__(self, prec, lswmsw, parent)

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

			self.addRemarkWidget(self.mainLayout)

			self.buttonBox = QDialogButtonBox()
			self.buttonBox.setOrientation(Qt.Horizontal)
			self.buttonBox.setStandardButtons(QDialogButtonBox.Cancel|QDialogButtonBox.Ok)
			self.mainLayout.addWidget(self.buttonBox)

			QObject.connect(self.buttonBox, SIGNAL("accepted()"), self.accept)
			QObject.connect(self.buttonBox, SIGNAL("rejected()"), self.reject)
			QMetaObject.connectSlotsByName(self)

		def accept(self):
			if self.lswmsw:
				self.val = NO_DATA_COM
			else:
				self.val = NO_DATA
			self.saveRemark()
			QDialog.accept(self)

		def generate(self, dates):
			dvals = [(d, self.val) for d in dates]
			return dvals

		def get_value_desc(self):
			return "x = NO_DATA"

	class LinearIncDialog(ValueDialog):
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-inc.png"
		desc = _translate("ValueDialogs", "Linear increasing")

		def __init__(self, prec, lswmsw, parent=None):
			ValueDialogs.ValueDialog.__init__(self, prec, lswmsw, parent)

			self.parent = parent
			self.prec = prec
			self.lswmsw = lswmsw

			self.setWindowIcon(QIcon(self.qicon_path))
			self.setModal(True)
			self.setWindowTitle(_translate("ValueDialogs",
								"Define linear increasing plot"))
			self.setToolTip(_translate("ValueDialogs",
				"This is a dialog for defining linear increasing values of parameter."))

			self.mainLayout = QVBoxLayout()
			self.setLayout(self.mainLayout)

			self.label_a = QLabel()
			self.label_a.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.label_a.setText(_translate("ValueDialogs",
				"Enter parameter's value in starting point:"))
			self.mainLayout.addWidget(self.label_a)

			self.valueEdit_a = QLineEdit()
			self.valueEdit_a.setAutoFillBackground(True)
			self.valueEdit_a.setMaxLength(12)
			self.valueEdit_a.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.valueEdit_a.setToolTip(_translate("ValueDialogs",
				"Parameter\'s value in starting point (a)."))
			self.valueEdit_a.setPlaceholderText("0.0")
			qdv_a = QDoubleValidator()
			qdv_a.setNotation(0)
			if lswmsw:
				qdv_a.setRange(SZB_LIMIT_COM * -1, SZB_LIMIT_COM, prec)
			else:
				qdv_a.setRange(SZB_LIMIT * -1, SZB_LIMIT, prec)
			self.valueEdit_a.setValidator(qdv_a)

			self.valueEdit_a.setText("")
			self.mainLayout.addWidget(self.valueEdit_a)

			self.label_b = QLabel()
			self.label_b.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.label_b.setText(_translate("ValueDialogs",
				"Enter parameter's value in ending point:"))
			self.mainLayout.addWidget(self.label_b)

			self.valueEdit_b = QLineEdit()
			self.valueEdit_b.setAutoFillBackground(True)
			self.valueEdit_b.setMaxLength(12)
			self.valueEdit_b.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.valueEdit_b.setToolTip(_translate("ValueDialogs",
				"Parameter\'s value in ending point (b)."))
			self.valueEdit_b.setPlaceholderText("0.0")
			qdv_b = QDoubleValidator()
			qdv_b.setNotation(0)
			if lswmsw:
				qdv_b.setRange(SZB_LIMIT_COM * -1, SZB_LIMIT_COM, prec)
			else:
				qdv_b.setRange(SZB_LIMIT * -1, SZB_LIMIT, prec)
			self.valueEdit_b.setValidator(qdv_b)

			self.valueEdit_b.setText("")
			self.mainLayout.addWidget(self.valueEdit_b)

			self.addRemarkWidget(self.mainLayout)

			self.buttonBox = QDialogButtonBox()
			self.buttonBox.setOrientation(Qt.Horizontal)
			self.buttonBox.setStandardButtons(QDialogButtonBox.Cancel|QDialogButtonBox.Ok)
			self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
			self.mainLayout.addWidget(self.buttonBox)

			QObject.connect(self.buttonBox, SIGNAL("accepted()"), self.accept)
			QObject.connect(self.buttonBox, SIGNAL("rejected()"), self.reject)
			QObject.connect(self.valueEdit_a, SIGNAL("lostFocus()"), self.onValueChanged_a)
			QObject.connect(self.valueEdit_a, SIGNAL("returnPressed()"), self.onValueChanged_a)
			QObject.connect(self.valueEdit_a, SIGNAL("textChanged(QString)"), self.validateInput)
			QObject.connect(self.valueEdit_b, SIGNAL("lostFocus()"), self.onValueChanged_b)
			QObject.connect(self.valueEdit_b, SIGNAL("returnPressed()"), self.onValueChanged_b)
			QObject.connect(self.valueEdit_b, SIGNAL("textChanged(QString)"), self.validateInput)
			QMetaObject.connectSlotsByName(self)

		def onValueChanged_a(self):
			new_value = self.valueEdit_a.text()
			try:
				self.valueEdit_a.setText(str(float(new_value)))
			except ValueError:
				self.valueEdit_a.setText("")
			self.validateInput()

		def onValueChanged_b(self):
			new_value = self.valueEdit_b.text()
			try:
				self.valueEdit_b.setText(str(float(new_value)))
			except ValueError:
				self.valueEdit_b.setText("")
			self.validateInput()

		def validateInput(self):
			if len(self.valueEdit_a.text()) > 0 and len(self.valueEdit_b.text()) > 0:
				self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)
			else:
				self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

		def accept(self):
			val_a = float(self.valueEdit_a.text())
			val_b = float(self.valueEdit_b.text())

			if (self.lswmsw and (val_a > SZB_LIMIT_COM or val_a < SZB_LIMIT_COM * -1)) \
				or ((not self.lswmsw) and (val_a > SZB_LIMIT or val_a < SZB_LIMIT * -1)):
					self.parent.warningBox(_translate("ValueDialogs",
						"Parameter's value in starting point is out of range."))
			elif (self.lswmsw and (val_a > SZB_LIMIT_COM or val_a < SZB_LIMIT_COM * -1)) \
				or ((not self.lswmsw) and (val_a > SZB_LIMIT or val_a < SZB_LIMIT * -1)):
					self.parent.warningBox(_translate("ValueDialogs",
						"Parameter's value in ending point is out of range."))
			elif val_a > val_b:
					self.parent.warningBox(_translate("ValueDialogs",
						"Starting point is greater than ending point. "
						"If that's what you want, choose linear decreasing plot."))
			else:
				self.val_a = val_a
				self.val_b = val_b
				self.saveRemark()
				QDialog.accept(self)

		def generate(self, dates):
			np = len(dates)
			delta = (self.val_b - self.val_a) / float(np-1)
			vals = [ self.val_a ]
			dsum = 0
			while np > 2:
				dsum += delta
				vals.append(round(self.val_a+dsum, self.prec))
				np -= 1
			vals.append(self.val_b)

			return zip(dates, vals)

		def get_value_desc(self):
			return "a = %s, b = %s" % (self.val_a, self.val_b)

	class LinearDecDialog(ValueDialog):
		qicon_path = "/opt/szarp/resources/qt4/icons/plot-dec.png"
		desc = _translate("ValueDialogs", "Linear decreasing")

		def __init__(self, prec, lswmsw, parent=None):
			ValueDialogs.ValueDialog.__init__(self, prec, lswmsw, parent)

			self.parent = parent
			self.prec = prec
			self.lswmsw = lswmsw

			self.setWindowIcon(QIcon(self.qicon_path))
			self.setModal(True)
			self.setWindowTitle(_translate("ValueDialogs",
								"Define linear decreasing plot"))
			self.setToolTip(_translate("ValueDialogs",
				"This is a dialog for defining linear decreasing values of parameter."))

			self.mainLayout = QVBoxLayout()
			self.setLayout(self.mainLayout)

			self.label_a = QLabel()
			self.label_a.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.label_a.setText(_translate("ValueDialogs",
				"Enter parameter's value in starting point:"))
			self.mainLayout.addWidget(self.label_a)

			self.valueEdit_a = QLineEdit()
			self.valueEdit_a.setAutoFillBackground(True)
			self.valueEdit_a.setMaxLength(12)
			self.valueEdit_a.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.valueEdit_a.setToolTip(_translate("ValueDialogs",
				"Parameter\'s value in starting point (a)."))
			self.valueEdit_a.setPlaceholderText("0.0")
			qdv_a = QDoubleValidator()
			qdv_a.setNotation(0)
			if lswmsw:
				qdv_a.setRange(SZB_LIMIT_COM * -1, SZB_LIMIT_COM, prec)
			else:
				qdv_a.setRange(SZB_LIMIT * -1, SZB_LIMIT, prec)
			self.valueEdit_a.setValidator(qdv_a)

			self.valueEdit_a.setText("")
			self.mainLayout.addWidget(self.valueEdit_a)

			self.label_b = QLabel()
			self.label_b.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.label_b.setText(_translate("ValueDialogs",
				"Enter parameter's value in ending point:"))
			self.mainLayout.addWidget(self.label_b)

			self.valueEdit_b = QLineEdit()
			self.valueEdit_b.setAutoFillBackground(True)
			self.valueEdit_b.setMaxLength(12)
			self.valueEdit_b.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
			self.valueEdit_b.setToolTip(_translate("ValueDialogs",
				"Parameter\'s value in ending point (b)."))
			self.valueEdit_b.setPlaceholderText("0.0")
			qdv_b = QDoubleValidator()
			qdv_b.setNotation(0)
			if lswmsw:
				qdv_b.setRange(SZB_LIMIT_COM * -1, SZB_LIMIT_COM, prec)
			else:
				qdv_b.setRange(SZB_LIMIT * -1, SZB_LIMIT, prec)
			self.valueEdit_b.setValidator(qdv_b)

			self.valueEdit_b.setText("")
			self.mainLayout.addWidget(self.valueEdit_b)

			self.addRemarkWidget(self.mainLayout)

			self.buttonBox = QDialogButtonBox()
			self.buttonBox.setOrientation(Qt.Horizontal)
			self.buttonBox.setStandardButtons(QDialogButtonBox.Cancel|QDialogButtonBox.Ok)
			self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)
			self.mainLayout.addWidget(self.buttonBox)

			QObject.connect(self.buttonBox, SIGNAL("accepted()"), self.accept)
			QObject.connect(self.buttonBox, SIGNAL("rejected()"), self.reject)
			QObject.connect(self.valueEdit_a, SIGNAL("lostFocus()"), self.onValueChanged_a)
			QObject.connect(self.valueEdit_a, SIGNAL("returnPressed()"), self.onValueChanged_a)
			QObject.connect(self.valueEdit_a, SIGNAL("textChanged(QString)"), self.validateInput)
			QObject.connect(self.valueEdit_b, SIGNAL("lostFocus()"), self.onValueChanged_b)
			QObject.connect(self.valueEdit_b, SIGNAL("returnPressed()"), self.onValueChanged_b)
			QObject.connect(self.valueEdit_b, SIGNAL("textChanged(QString)"), self.validateInput)
			QMetaObject.connectSlotsByName(self)

		def onValueChanged_a(self):
			new_value = self.valueEdit_a.text()
			try:
				self.valueEdit_a.setText(str(float(new_value)))
			except ValueError:
				self.valueEdit_a.setText("")
			self.validateInput()

		def onValueChanged_b(self):
			new_value = self.valueEdit_b.text()
			try:
				self.valueEdit_b.setText(str(float(new_value)))
			except ValueError:
				self.valueEdit_b.setText("")
			self.validateInput()

		def validateInput(self):
			if len(self.valueEdit_a.text()) > 0 and len(self.valueEdit_b.text()) > 0:
				self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(True)
			else:
				self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)

		def accept(self):
			val_a = float(self.valueEdit_a.text())
			val_b = float(self.valueEdit_b.text())

			if (self.lswmsw and (val_a > SZB_LIMIT_COM or val_a < SZB_LIMIT_COM * -1)) \
				or ((not self.lswmsw) and (val_a > SZB_LIMIT or val_a < SZB_LIMIT * -1)):
					self.parent.warningBox(_translate("ValueDialogs",
						"Parameter's value in starting point is out of range."))
			elif (self.lswmsw and (val_a > SZB_LIMIT_COM or val_a < SZB_LIMIT_COM * -1)) \
				or ((not self.lswmsw) and (val_a > SZB_LIMIT or val_a < SZB_LIMIT * -1)):
					self.parent.warningBox(_translate("ValueDialogs",
						"Parameter's value in ending point is out of range."))
			elif val_a < val_b:
					self.parent.warningBox(_translate("ValueDialogs",
						"Starting point is lesser than ending point. "
						"If that's what you want, choose linear increasing plot."))
			else:
				self.val_a = val_a
				self.val_b = val_b
				self.saveRemark()
				QDialog.accept(self)

		def generate(self, dates):
			np = len(dates)
			delta = (self.val_b - self.val_a) / float(np-1)
			vals = [ self.val_a ]
			dsum = 0
			while np > 2:
				dsum += delta
				vals.append(round(self.val_a+dsum, self.prec))
				np -= 1
			vals.append(self.val_b)

			return zip(dates, vals)

		def get_value_desc(self):
			return "a = %s, b = %s" % (self.val_a, self.val_b)

