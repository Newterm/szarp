
import os , tempfile 

try :
	from urlparse import urlparse
except ImportError :
	from urllib.parse import urlparse

from miscipk import pysftp

class FileSource :
	def read( self ) :
		return ''

	def write( self , data ) :
		pass

	def filename( self ) :
		return ''

	def url( self ) :
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

	def url( self ) :
		return 'file://%s' % self.fn

	def __str__( self ) :
		return self.fn

from miscipk.pysftp import ConnectionError
from miscipk.pysftp import AuthenticationError
from miscipk.pysftp import HostNotFoundError

class FS_ssh( FS_local ) :
	def __init__( self , serv , path , username = None , port = None , *l , **m ) :
		self.fn = None
		self.serv = serv
		self.path = path
		self.con = pysftp.Connection( serv , username , port = port , *l , **m )
		fd , fn  = tempfile.mkstemp('.xml','params-')
		os.close(fd)
		FS_local.__init__( self , fn )
		self.refresh()
		self._url = self.join_url( serv , path , username , port )

	def refresh( self ) :
		self.con.get( self.path , self.fn )

	def read( self ) :
		return FS_local.read( self )

	def write( self , data ) :
		FS_local.write( self , data )
		self.con.put( self.fn , self.path )

	def filename( self ) :
		return self.fn

	def join_url( self , host , path , user = None , port = None ) :
		netloc = host   if port == None else '%s:%s'%(host,port)
		addr   = netloc if user == None else '%s@%s'%(user,netloc)
		return 'ssh://%s%s' % (addr,path)

	def url( self ) :
		return self._url

	def __del__( self ) :
		FS_local.__del__( self )
		if self.fn != None : os.remove(self.fn)

	def __str__( self ) :
		return '%s:%s' % (self.serv,self.path)

def FS_url( url ) :
	o = urlparse( url )
	if o.scheme == 'file' :
		return FS_local( o.path )
	elif o.scheme == 'ssh' :
		return FS_ssh( o.hostname , o.path , username = o.username , password = o.password , port = o.port if o.port != None else 22 )

	raise ValueError("Url protocol not supported")

