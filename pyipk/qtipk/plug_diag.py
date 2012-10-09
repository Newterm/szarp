
import sip
sip.setapi('QString', 2)

from PyQt4 import QtGui , QtCore

from libipk.plugins import Plugins

from .utils import toUtf8

from .ui.plug_diag import Ui_PluginsDialog

from .config import Configurable

class PluginsDialog( QtGui.QDialog , Ui_PluginsDialog , Configurable ) :
	def __init__( self , parent , plugins ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi( self )
		Configurable.__init__( self , parent )

		self.args = None
		self.labs = []
		self.ents = []

		self.light_color = QtGui.QColor("grey")
		self.default_color = QtGui.QColor("black")

		self.plugins = plugins
		self.fill( self.plugins )
		self.treeWidget.sortItems(0,QtCore.Qt.AscendingOrder)
		self.select_last()

	def fill( self , plugins ) :
		sections = {}
		for n in plugins.names() :
			s = plugins.get_section(n)
			if s not in sections :
				sections[s] = QtGui.QTreeWidgetItem( self.treeWidget , [s] )

			item = QtGui.QTreeWidgetItem( [n] )
			sections[s].addChild( item )

			if n == self.cfg['gui:plugin_selected'] :
				self.treeWidget.setCurrentItem( item )

	def select_last( self ) :
		pass

	def selected_name( self ) :
		idxs = self.treeWidget.selectedItems()
		return idxs[0].text(0) if len(idxs) > 0 and idxs[0].parent() != None else None

	def selected_args( self ) :
		return dict( zip( self.args , [ e.text() for e in self.ents ] ) )

	def pluginSelected( self ) :
		name = self.selected_name()

		self.cfg['gui:plugin_selected'] = name

		# section selected
		if name == None : return

		self.args = self.plugins.get_args( name )
		defaults  = self.plugins.get_defaults( name )
		self.textDoc.setText( self.plugins.help( name ) )

		for l in self.labs : l.close()
		for e in self.ents : e.close()

		del self.labs[:]
		del self.ents[:]

		row = 0
		for arg in self.args :
			self.labs.append( QtGui.QLabel( arg ) )
			le = QtGui.QLineEdit()
			if arg in defaults :
				le.setText( defaults[arg] )
			self.ents.append( le )
			self.glay_args.addWidget( self.labs[-1] , row , 0 )
			self.glay_args.addWidget( self.ents[-1] , row , 1 )
			row += 1

		pal = QtGui.QPalette()
		pal.setColor( QtGui.QPalette.Text , self.light_color )
		for le in self.ents : le.setPalette(pal)

		map( lambda e : e.textEdited.connect( lambda : self.set_color(e,self.default_color) ) , self.ents )

		if len(self.ents) > 0 :
			QtGui.QWidget.setTabOrder( self.treeWidget , self.ents[0] )
			for i in range(1,len(self.ents)) :
				QtGui.QWidget.setTabOrder( self.ents[i-1] , self.ents[i] )
			QtGui.QWidget.setTabOrder( self.ents[-1] , self.buttonBox )

	def set_color( self , widget , color ) :
		pal = QtGui.QPalette()
		pal.setColor( QtGui.QPalette.Text , color )
		widget.setPalette(pal)

