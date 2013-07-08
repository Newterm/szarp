
import sip
sip.setapi('QString', 2)

import os , os.path

from PyQt4 import QtGui , QtCore

from lxml import etree

from .utils import toUtf8
from .config import Configurable

from .ui.validate_diag import Ui_ValidateDialog 

class ValidateDialog( QtGui.QDialog , Ui_ValidateDialog , Configurable ) :
	def __init__( self , parent ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi( self )
		Configurable.__init__( self , parent )

		self.cfg.on_edited( 'path:relax_ng' , self.setFile )

		self.setFile( self.cfg['path:relax_ng'] )

	def setFile( self , filename ) :
		self.relax_file = filename
		self.lineFile.setText( filename )

	def bboxClicked( self , button ) :
		if self.buttonBox.buttonRole( button ) == QtGui.QDialogButtonBox.ApplyRole :
			self.validate()

	def selectFile( self ) :
		fn = QtGui.QFileDialog.getOpenFileNameAndFilter( self , 
				'Open file' , os.path.dirname(self.relax_file) ,
				'Relax NG Files (*.rng);;All (*)' )
		if fn[0] != '' :
			self.cfg['path:relax_ng'] = fn[0]

	def exec_( self , params ) :
		self.params = params
		QtGui.QDialog.exec_( self )

	def validate( self ) :
		if self.params == None or self.relax_file == None :
			return

		if not self.params.validate( self.relax_file ) :
			self.errorText.setPlainText( self.params.validate_log() )
		else :
			self.errorText.setPlainText( 'OK' )

