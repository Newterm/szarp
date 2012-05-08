
import sip
sip.setapi('QString', 2)

import os , os.path

from PyQt4 import QtGui , QtCore

from .utils import *

from .ui.remote_diag import Ui_RemoteDialog

import libipk.filesource as fs

class RemoteDialog( QtGui.QDialog , Ui_RemoteDialog ) :
	def __init__( self , parent , szarp_dir ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi( self )

		self.szarp_dir = szarp_dir

		self.labelError.hide()

	def connect( self ) :
		base = fromUtf8( self.lineBase.text() )
		serv = fromUtf8( self.lineServer.text() )
		user = fromUtf8( self.lineUser.text() )
		pswd = fromUtf8( self.linePass.text() )
		port = fromUtf8( self.linePort.text() )
		path = '%s/%s/config/params.xml' % (self.szarp_dir,base)

		if serv == '' : serv = base
		if user == '' : user = None
		if pswd == '' : pswd = None
		port = 22 if port == '' else int(port)

		try :
			self.fs = fs.FS_ssh( serv , path , username = user , password = pswd , port = port )
		except fs.AuthenticationError :
			self.labelError.setText('Authentication error')
			self.labelError.show()
		except fs.ConnectionError :
			self.labelError.setText('Connection error')
			self.labelError.show()
		except fs.HostNotFoundError :
			self.labelError.setText('Host not found: ssh://%s:%d' % (serv,port) )
			self.labelError.show()
		except IOError :
			self.labelError.setText('File not found: ssh://%s:%d%s' % (serv,port,path) )
			self.labelError.show()
		else :
			self.labelError.setText('')
			self.labelError.hide()
			self.repaint()
			self.accept()

	def get_fs( self ) :
		return self.fs

