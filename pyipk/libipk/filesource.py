
import os , tempfile 

from miscipk import pysftp

class FileSource :
	def filename( self ) :
		return ''

	def sync( self ) :
		passe


class FS_local( FileSource ) :
	def __init__( self , filename ) :
		self.fn = filename

	def filename( self ) :
		return self.fn


class FS_ssh( FileSource ) :
	def __init__( self , serv , path ) :
		self.path = path
		self.con = pysftp.Connection( serv )
		fd , fn  = tempfile.mkstemp('.xml')
		os.close(fd)
		self.fn  = fn
		self.con.get( self.path , self.fn )

	def filename( self ) :
		return self.fn

	def sync( self ) :
		self.con.put( self.fn , self.path )

