
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
		self.treeWidget.sortItems(0,QtCore.Qt.AscendingOrder)

		self.args = None
		self.labs = []
		self.ents = []

	def fill( self , plugins ) :
		sections = {}
		for n in plugins.names() :
			s = plugins.get_section(n)
			if s not in sections :
				sections[s] = QtGui.QTreeWidgetItem( self.treeWidget , [s] )

			sections[s].addChild( QtGui.QTreeWidgetItem( [n] ) )

	def selected_name( self ) :
		idxs = self.treeWidget.selectedItems()
		return idxs[0].text(0) if len(idxs) > 0 and idxs[0].parent() != None else None

	def selected_args( self ) :
		return dict( zip( self.args , [ e.text() for e in self.ents ] ) )

	def pluginSelected( self ) :
		name = self.selected_name()

		# section selected
		if name == None : return

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

