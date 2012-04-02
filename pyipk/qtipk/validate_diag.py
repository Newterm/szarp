
import sip
sip.setapi('QString', 2)

import os , os.path

from PyQt4 import QtGui , QtCore

from lxml import etree

from .utils import toUtf8

from .ui.validate_diag import Ui_ValidateDialog 

class ValidateDialog( QtGui.QDialog , Ui_ValidateDialog ) :
	def __init__( self , parent , default_relax ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi( self )

		self.setFile( default_relax )

		self.buttonBox.accepted.connect( self.validate )

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
			self.setFile( fn[0] )

	def exec_( self , params ) :
		self.params = params
		QtGui.QDialog.exec_( self )

	def validate( self ) :
		if self.params == None or self.relax_file == None :
			return

		relaxng_doc = etree.parse(self.relax_file)
		relaxng = etree.RelaxNG(relaxng_doc)
		if not relaxng.validate( self.params.root.node ) :
			self.errorText.setPlainText( repr( relaxng.error_log ) )
		else :
			self.errorText.setPlainText( 'OK' )

