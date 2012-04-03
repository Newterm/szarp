
import os , tempfile 

from miscipk import pysftp

class FileSource :
	def read( self ) :
		return ''

	def write( self , data ) :
		passe

	def filename( self ) :
		return ''

	def __del__( self ) :
		pass


class FS_local( FileSource ) :
	def __init__( self , filename ) :
		self.fn = filename

	def read( self ) :
		with open(self.fn,'r') as f :
			return f.read()

	def write( self , data ) :
		with open(self.fn,'w') as f :
			f.write(data)

	def filename( self ) :
		return self.fn

from miscipk.pysftp import ConnectionError
from miscipk.pysftp import AuthenticationError
from miscipk.pysftp import HostNotFoundError

class FS_ssh( FS_local ) :
	def __init__( self , serv , path , *l , **m ) :
		self.fn = None
		self.path = path
		self.con = pysftp.Connection( serv , *l , **m )
		fd , fn  = tempfile.mkstemp('.xml','params-')
		os.close(fd)
		FS_local.__init__( self , fn )
		self.refresh()

	def refresh( self ) :
		self.con.get( self.path , self.fn )

	def read( self ) :
		return FS_local.read( self )

	def write( self , data ) :
		FS_local.write( self , data )
		self.con.put( self.fn , self.path )

	def filename( self ) :
		return self.fn

	def __del__( self ) :
		FS_local.__del__( self )
		if self.fn != None : os.remove(self.fn)

