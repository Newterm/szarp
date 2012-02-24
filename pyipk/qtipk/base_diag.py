
import sip
sip.setapi('QString', 2)

import os , os.path

from PyQt4 import QtGui , QtCore

from .utils import toUtf8

from .ui.base_diag import Ui_BaseDialog

class BaseDialog( QtGui.QDialog , Ui_BaseDialog ) :
	def __init__( self , parent , szarp_dir ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi( self )

		self.szarp_dir = szarp_dir

		self.bases = [ b for b in os.listdir(szarp_dir) if os.path.exists(self.params_path(b)) ]

		self.bases.sort()

		self.listView.setModel( QtGui.QStringListModel( self.bases ) )

	def params_path( self, base ) :
		return os.path.join(self.szarp_dir,base,'config','params.xml')

	def selected_index( self ) :
		return self.listView.currentIndex().row()

	def selected_base( self ) :
		return self.bases[ self.listView.currentIndex().row() ]

	def selected_params( self ) :
		return self.params_path(self.selected_base())


