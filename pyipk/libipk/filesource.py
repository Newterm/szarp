
import os , tempfile 

from miscipk import pysftp

class FileSource :
	def read( self ) :
		return ''

	def write( self , data ) :
		passe

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

class FS_ssh( FS_local ) :
	def __init__( self , serv , path , port = 22 ) :
		self.path = path
		self.con = pysftp.Connection( serv , port = port )
		fd , fn  = tempfile.mkstemp('.xml','params-')
		os.close(fd)

		FS_local.__init__( self , fn )

	def read( self ) :
		self.con.get( self.path , self.fn )
		return FS_local.read( self )

	def write( self , data ) :
		FS_local.write( self , data )
		self.con.put( self.fn , self.path )

	def __del__( self ) :
		FS_local.__del__( self )
		os.remove(self.fn)

