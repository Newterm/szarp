#include "tmmapparam.h"

#include <algorithm>

#include <sys/mman.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#include "liblog.h"
#include "conversion.h"
#include "szbase/szbbase.h"

TMMapParam::TMMapParam( const std::wstring& dir , const std::wstring& name , int year , int month , time_t probe_length )
	: fd(-1) , filemap(NULL) , length(0) , probe_length( probe_length )
{
	path = dir + L"/" + szb_createfilename_ne(name, year, month, probe_length == SZBASE_DATA_SPAN ? L".szb" : L".szc");

	const unsigned int probes_in_hour = 3600 / probe_length;
	const unsigned int max_file_size = 31 * 24 * probes_in_hour;

	file_size = max_file_size;

	// create directory if it doesn't exist
	if( szb_cc_parent(path) )
		throw failure("Cannot create directory for file "+SC::S2A(path));

	// open file to write-only
	fd = open(SC::S2A(path).c_str(), O_RDWR | O_CREAT | O_CLOEXEC, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
	if( fd == -1 )
		throw failure("Cannot open file "+SC::S2A(path)+" to write: "+strerror(errno));

	// save end of old data
	int err;
	if( (err=lseek(fd,0,SEEK_END)) == (off_t)-1 )
		throw failure("Cannot seek trough file "+SC::S2A(path)+": "+strerror(errno));
	begin = (unsigned int)err;
	sz_log(10,"Last file index: %d",begin);
	begin /= sizeof(short);

	// seek to last short in the file
	if( lseek(fd,sizeof(short)*max_file_size-1,SEEK_SET) == (off_t)-1 )
		throw failure("Cannot seek trough file "+SC::S2A(path)+": "+strerror(errno));

	// write something to the file, to resize the file
	if( ::write(fd,"\0",1) == -1 )
		throw failure("Cannot write to file "+SC::S2A(path)+": %s"+strerror(errno));

	// force os to resize the file
	if( fdatasync(fd) == -1 ) 
		throw failure("Cannot sync data in file "+SC::S2A(path)+": "+strerror(errno));

	// map whole file to memory
	filemap = (short*)mmap(NULL,sizeof(short)*file_size,PROT_WRITE,MAP_SHARED,fd,0);
	if( filemap == MAP_FAILED )
		throw failure("Cannot map file "+SC::S2A(path)+" to memory: "+strerror(errno));

	sz_log(10,"Clearing data from %d to %d",begin,file_size);
	for( unsigned int i = begin ; i<file_size ; ++i )
		filemap[i] = SZB_FILE_NODATA;
	// FIXME: why memset doesn't work?
//        memset( filemap , (int)SZB_FILE_NODATA , sizeof(short)*file_size );
}

TMMapParam::~TMMapParam()
{
	try {
		close();
	} catch( const failure& f ) {
		sz_log(1,"Cannot truncate file %s\n",SC::S2A(path).c_str());
		fprintf(stderr,"Cannot truncate file %s\n",SC::S2A(path).c_str());
		::close( fd );
	}
}

int TMMapParam::close()
{
	munmap(filemap,sizeof(short)*file_size);
	if( ftruncate(fd,sizeof(short)*std::max(begin,length)) == -1 )
		throw failure("Cannot truncate file "+SC::S2A(path)+": "+strerror(errno));
	::close( fd );

	return 0;
}

int TMMapParam::write( time_t t , short*probes , unsigned int len ) 
{
	unsigned int index = szb_probeind(t, probe_length);

	sz_log(10,"Writing %d probes at index: %d",len,index);

	assert( index >=0 );
	assert( index+len <= file_size );

	memcpy( filemap+index , probes , sizeof(short)*len );

	length = index+len;

	return 0;
}

