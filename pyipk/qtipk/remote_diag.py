
import sip
sip.setapi('QString', 2)

import os , os.path

from PyQt4 import QtGui , QtCore

from .utils import toUtf8

from .ui.remote_diag import Ui_RemoteDialog

class RemoteDialog( QtGui.QDialog , Ui_RemoteDialog ) :
	def __init__( self , parent , szarp_dir ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi( self )

		self.szarp_dir = szarp_dir

	def bboxClicked( self , button ) :
		if self.buttonBox.buttonRole( button ) == QtGui.QDialogButtonBox.AcceptRole :
			self.connect()

	def connect( self ) :
		base = self.lineBase.text()
		serv = self.lineServer.text()

		if serv == '' : serv = base

	def selected_params( self ) :
		return ''



