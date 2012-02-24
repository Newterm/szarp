
import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui , QtCore

from libipk.plugins import Plugins

from .utils import toUtf8

from .ui.plug_diag import Ui_PluginsDialog

class PluginsDialog( QtGui.QDialog , Ui_PluginsDialog ) :
	def __init__( self , parent , plugins ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi( self )

		self.plugins = plugins
		self.fill( self.plugins )
		self.listView.setSelection( QtCore.QRect(0,0,1,1) , 
				QtGui.QItemSelectionModel.Select )

		self.args = None
		self.labs = []
		self.ents = []

	def fill( self , plugins ) :
		self.names = list( plugins.names() )
		self.names.sort()
		self.listView.setModel( QtGui.QStringListModel( self.names ) )

	def selected_index( self ) :
		return self.listView.currentIndex().row()

	def selected_name( self ) :
		return self.names[ self.listView.currentIndex().row() ]

	def selected_args( self ) :
		return dict( zip( self.args , [ e.text() for e in self.ents ] ) )

	def pluginSelected( self , index ) :
		name = self.names[ index.row() ]

		self.args = self.plugins.get_args( name )
		self.textDoc.setText( self.plugins.help( name ) )

		for l in self.labs : l.close()
		for e in self.ents : e.close()

		del self.labs[:]
		del self.ents[:]
		
		row = 0
		for arg in self.args :
			self.labs.append( QtGui.QLabel( arg ) )
			self.ents.append( QtGui.QLineEdit() )
			self.glay_args.addWidget( self.labs[-1] , row , 0 )
			self.glay_args.addWidget( self.ents[-1] , row , 1 )
			row += 1

