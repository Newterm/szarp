
import sip
sip.setapi('QString', 2)

import string , random , os.path , subprocess , re

from lxml import etree

from PyQt4 import QtGui , QtCore

from .utils import * 

from .ui.open_with_dialog import Ui_OpenWithDialog

class OpenWithDialog( QtGui.QDialog , Ui_OpenWithDialog ) :
	def __init__( self , parent , default ) :
		QtGui.QDialog.__init__( self , parent )
		self.setupUi( self )

		self.progInput.setText( default )

	def get_binary( self ) :
		return self.progInput.text()

def id_generator(size=6, chars=string.ascii_uppercase + string.digits):
	return ''.join(random.choice(chars) for x in range(size))

class ParamsEditor( QtCore.QObject ) :
	changedSig = QtCore.pyqtSignal()

	def __init__( self , parent , default = '' ) :
		QtCore.QObject.__init__(self)
		self.parent  = parent
		self.default = default
		self.proc = []

	def get_binary( self ) :
		diag = OpenWithDialog( self.parent , self.default )
		if diag.exec_() == QtGui.QDialog.Accepted :
			self.default = diag.get_binary()
			return diag.get_binary()
		return None

	def get_tmp( self ) :
		return os.path.join( QtCore.QDir.tempPath(),id_generator() + '.ipkqt.params.xml' )

	def create_file( self , filename , nodes ) :
		with open(filename,'w') as f :
			for n in nodes :
				f.write( toUtf8( etree.tostring( n , pretty_print = True , encoding='utf8' , method = 'xml' , xml_declaration=False ) ) )

	def read_file( self , filename ) :
		doc = etree.parse( filename )
		return [ doc.getroot() ]

	def on_finish( self , code , stat , filename , nin ) :
		if stat != QtCore.QProcess.NormalExit :
			return 

		nout = self.read_file( filename )

		self.changedSig.emit()

		for i in range(min(len(nin),len(nout))) :
			nin[i].getparent().replace(nin[i],nout[i])

		os.remove(filename)

	def launch( self , prog , filename , nodes ) :
		process = QtCore.QProcess()
		process.finished.connect( lambda c , s : self.on_finish(c,s,filename,nodes) )
		args = [filename]
		if re.search('vim',prog) != None :
			args.append('--nofork') # nasty hax for gvim which likes to fork after start
		process.start( prog , args )

		return process

	def edit( self , nodes ) :
		if len(nodes) > 1 :
			raise NotImplementedError('Too many nodes selected. Currently only one node can be edited.')

		prog = self.get_binary()
		if prog == None : return

		tmp  = self.get_tmp()

		self.create_file( tmp , nodes )

		self.proc.append( self.launch( prog , tmp , nodes ) )

