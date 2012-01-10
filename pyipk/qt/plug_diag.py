
from PyQt4 import QtGui , QtCore

from libipk.plugins import Plugins

from ui.plug_diag import Ui_PluginsDialog

class PluginsDialog( QtGui.QDialog , Ui_PluginsDialog ) :
	def __init__( self , parent , plugins ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi( self )

		self.plugins = plugins
		self.fill( self.plugins )
		self.listView.setSelection( QtCore.QRect(0,0,1,1) , 
				QtGui.QItemSelectionModel.Select )

		self.args = None
		self.lays = []
		self.labs = []
		self.ents = []

	def fill( self , plugins ) :
		self.names = QtCore.QStringList( plugins.names() )
		self.names.sort()
		self.listView.setModel( QtGui.QStringListModel( self.names ) )

	def selected_index( self ) :
		return self.listView.currentIndex().row()

	def selected_name( self ) :
		return unicode(self.names[ self.listView.currentIndex().row() ])

	def selected_args( self ) :
		return dict( zip( self.args , [ unicode(e.text()) for e in self.ents ] ) )

	def pluginSelected( self , index ) :
		# FIXME: Plugins.get creates new plugin object
		#        this may be expensive in future
		self.args = self.plugins.get( unicode(self.names[ index.row() ]) ).get_args()

		for l in self.lays : self.vlay_args.removeItem( l )
		for l in self.labs : l.close()
		for e in self.ents : e.close()

		del self.lays[:]
		del self.labs[:]
		del self.ents[:]
		
		for arg in self.args :
			self.lays.append( QtGui.QHBoxLayout() )
			self.labs.append( QtGui.QLabel( arg ) )
			self.ents.append( QtGui.QLineEdit() )
			self.lays[-1].addWidget( self.labs[-1] )
			self.lays[-1].addWidget( self.ents[-1] )
			self.vlay_args.addLayout( self.lays[-1] )

