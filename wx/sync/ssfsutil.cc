/* 
  SZARP: SCADA software 
  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/
#pragma implementation 
#include "config.h"

#include <wx/platform.h>

#ifdef MINGW32
#include <shlwapi.h>
#endif
#include <assert.h>
#include <unistd.h>

#include <sstream>

#include "liblog.h"
#include "ssfsutil.h"

#include "ssstring.h"

FSException::FSException(ssstring what) : Exception(what) {
}

TPath::TPath() {
	m_path = strdup("");
	m_type = TNOTEXISTS;
	m_mtime = -1;
	m_size = 0;
	m_stated = true;
}

TPath::TPath(const char *path) {
	m_path = strdup(path);
	m_type = TUNKNOWN;
	m_mtime = (time_t) -1;
	m_size = 0;
	m_stated = false;
}

void TPath::Stat() const {
	m_stated = true;

	struct stat buf;
#ifndef MINGW32
	if (lstat(m_path, &buf) == 0) 
#else
	if (stat(m_path, &buf) == 0) 
#endif
	{
		m_mtime = buf.st_mtime;
		m_size = buf.st_size;

#ifdef __WXMSW__ 
		if (GetFileAttributesA(m_path) & FILE_ATTRIBUTE_REPARSE_POINT)
			m_type = TLINK;
		else 
#else
		if (S_ISLNK(buf.st_mode))
			m_type = TLINK;
		else
#endif
		if (S_ISDIR(buf.st_mode)) 
			m_type = TDIR;
		else if (S_ISREG(buf.st_mode)) 
			m_type = TFILE;
		else
			m_type = TUNKNOWN;
	} else {
		switch (errno) {
			case ENOENT:
				m_type = TNOTEXISTS;
				break;
			case EACCES:
				m_type = TNACCESS;
				break;
			default:
				m_type = TUNKNOWN;
		}
	}

}

TPath TPath::DirName() const {
	char* tmp = strdup(m_path);
	TPath result(dirname(tmp));
	free(tmp);
	return result;
}

#ifndef MINGW32
TPath TPath::BaseName() const {
	char *tmp = strdup(m_path);
	TPath result(basename(tmp), TNOTEXISTS, -1, 0);
	free(tmp);
	return result;
}
#endif

time_t TPath::GetModTime() const {
	if (!m_stated)
		Stat();
	return m_mtime;
}

uint32_t TPath::GetSize() const {
	if (!m_stated)
		Stat();
	return m_size;
}

bool TPath::IsSzbaseFile() const {
	size_t l = strlen(m_path);
	if (l < 10)
		return false;

	if (m_path[l - 4] != '.'
			|| m_path[l - 3] != 's'
			|| m_path[l - 2] != 'z'
			|| m_path[l - 1] != 'b')
		return false;

	for (size_t i = l - 5; i > l - 11; i--)
		if (m_path[i] < '0' || m_path[i] > '9')
			return false;

	return true;
}

void TPath::CreateDir() {
	if (!m_stated)
		Stat();

	if (m_type == TDIR)
		return;

	if (m_type == TNOTEXISTS) {
		DirName().CreateDir();
#ifndef MINGW32
		int ret = mkdir(GetPath(), 0755);
#else
		int ret = mkdir(GetPath());
#endif
		if (ret != 0) {
			sz_log(2,"Unable to create directory %s", GetPath());
			throw FSException(ssstring(_("Unable to create dir: ")) + csconv(GetPath()));
		}
	} else if (m_type == TFILE) {
		if (unlink(GetPath()) == -1) {
			sz_log(2,"Unable to create directory %s", GetPath());
			throw FSException(ssstring(_("Unable to create dir: ")) + csconv(GetPath()));
		}
		Stat();
		CreateDir();
		return;
	} else if (m_type == TLINK) {
		int ret = 0;
#ifndef MINGW32
		ret = unlink(GetPath());
#else
		ret = remove_symlnk(GetPath());
#endif
		if (ret == -1) {
			sz_log(2,"Unable to create directory %s", GetPath());
			throw FSException(ssstring(_("Unable to create dir: ")) + csconv(GetPath()));
		}
		Stat();
		CreateDir();
		return;
	} else {
		sz_log(2,"Unable to create directory %s", GetPath());
		throw FSException(ssstring(_("Unable to create dir: ")) + csconv(GetPath()));
	}
}

TPath::TPath(const char* path, FILE_TYPE type, time_t modtime, uint32_t size) {
	m_path = strdup(path);
	m_type = type;
	m_mtime = modtime;
	m_size = size;
	m_stated = true;
}

TPath::~TPath() {
	free(m_path);
}

TPath::TPath(const TPath& f) {
	m_type = f.m_type;
	m_path = strdup(f.m_path);
	m_mtime = f.m_mtime;
	m_size = f.m_size;
	m_stated = f.m_stated;
}

TPath& TPath::operator=(const TPath& f) {
	if (this != &f) {
		free(m_path);
		m_path = strdup(f.m_path);
		m_type = f.m_type;
		m_mtime = f.m_mtime;
		m_size = f.m_size;
		m_stated = f.m_stated;
	}
	return *this;
}

const char* TPath::GetPath() const {
	return m_path;
}

const uint16_t TPath::GetType() const {
	if (!m_stated)
		Stat();
	return m_type;
}

TPath TPath::Concat(const TPath& p) const {
	const char sep = 
#ifndef MINGW32
			'/';
#else
			'\\';
#endif

	std::ostringstream oss;

        size_t st = strlen(m_path);
        if ((st > 0 && m_path[st - 1] == sep)
                        || (strlen(p.m_path) && p.m_path[0] == sep))
		oss << m_path << p.m_path;
        else
		oss << m_path << sep << p.m_path;
	TPath result(oss.str().c_str());
	return result;
}

#ifndef MINGW32
char* TPath::Relative(const char* a, const char *b) {
	if (strlen(a) < 1 || a[0] != '/')
		return strdup(a);

	char **tokens1, **tokens2;
	int c1 = 0, c2 = 0;

	tokenize_d(a, &tokens1, &c1, "/");
	tokenize_d(b, &tokens2, &c2, "/");

	int i = 0, j = 0;
	while (i < c2 - 1 && i < c1 - 1 && !strcmp(tokens1[i], tokens2[i])) 
		++i, ++j;

	char *result;
	size_t result_size;
	FILE* stream = open_memstream(&result, &result_size);

	for (;j < c2 - 1; ++j) 
		fprintf(stream, "../");
	for (;i < c1 - 1; ++i)
		fprintf(stream, "%s/", tokens1[i]);
	fprintf(stream, "%s", tokens1[i]);

	fclose(stream);
	tokenize_d(NULL, &tokens1, &c1, "/");
	tokenize_d(NULL, &tokens2, &c2, "/");

	return result;

}
#endif

#ifndef MINGW32
vector<TPath> ScanDir(const TPath& tdir, const FileMatch& filter, bool recursive) {

	vector<TPath> result;

	DIR* dir = opendir(tdir.GetPath());

	if (!dir) {
		sz_log(9, "Unable to open %s", tdir.GetPath());
		return result;
	}

	struct dirent* entry;
	while ((entry = readdir(dir))) { 
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;

		TPath file = tdir.Concat(entry->d_name);
			
		TPath::FILE_TYPE type = file.GetType();
		if ((type == TPath::TFILE 
			|| type == TPath::TLINK 
			|| type == TPath::TDIR)
			&& filter(file))
			result.push_back(file);

		if (type == TPath::TDIR && recursive) {
			vector<TPath> sub = ScanDir(file, filter);
			result.insert(result.end(), sub.begin(), sub.end());
		}
	}
	
	closedir(dir);

	return result;

}
#endif

size_t ReadFully(FILE* f, void *buf, size_t size) {
	assert(f != NULL);
	size_t has_read = 0;
	size_t ret;
	while (has_read < size) {
		ret = fread(((uint8_t*)buf) + has_read, sizeof(uint8_t), size - has_read, f);
		if (ret == 0) {
			if (feof(f))
				break;
			else
				throw FSException(ssstring(_("Error while reading file ")) + csconv(strerror(errno)));
		}
		has_read += ret;
	}

	return has_read;
}

void WriteFully(FILE *f, void* buf, size_t size) {
	size_t written = 0;
	int ret = 0;

	while (written < size) { 
		ret = fwrite((uint8_t*)buf + written, sizeof(uint8_t), size - written, f);
		if (ret == 0) {
			if (ferror(f)) {
				throw FSException(ssstring(_("Error while writing file " )) + csconv(strerror(errno)));
			} else {
				break;
			}
		}
		written += ret;
	}
}

#ifdef MINGW32
char* dirname(const char* param) {
	static char buffer[MAX_PATH * 3];
	buffer[0] = '\0';

	char* tmp = (char*) malloc(sizeof(buffer));

	strcpy(tmp, param);
	PathRemoveBackslashA(tmp);

	strcat(tmp, "\\..");
	PathCanonicalizeA(buffer, tmp);

	free(tmp);

	PathRemoveBackslashA(buffer);
	return buffer;
}
#endif
	
const TPath::FILE_TYPE TPath::TFILE = 0;
const TPath::FILE_TYPE TPath::TDIR = 1;
const TPath::FILE_TYPE TPath::TLINK = 2;
const TPath::FILE_TYPE TPath::TUNKNOWN = 3;
const TPath::FILE_TYPE TPath::TNACCESS = 4;
const TPath::FILE_TYPE TPath::TNOTEXISTS = 5;


#ifdef MINGW32
/*the code below is taken from PostgreSQL (BSD license)*/

/*-------------------------------------------------------------------------
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 *-------------------------------------------------------------------------
 */

/*
 *	symlink support:
 *
 *	This struct is a replacement for REPARSE_DATA_BUFFER which is defined in VC6 winnt.h
 *	but omitted in later SDK functions.
 *	We only need the SymbolicLinkReparseBuffer part of the original struct's union.
 */

#include <windows.h>
#include <winioctl.h>

typedef struct
{
	DWORD		ReparseTag;
	WORD		ReparseDataLength;
	WORD		Reserved;
	/* SymbolicLinkReparseBuffer */
	WORD		SubstituteNameOffset;
	WORD		SubstituteNameLength;
	WORD		PrintNameOffset;
	WORD		PrintNameLength;
	WCHAR		PathBuffer[1];
}	REPARSE_JUNCTION_DATA_BUFFER;

#define REPARSE_JUNCTION_DATA_BUFFER_HEADER_SIZE   \
		FIELD_OFFSET(REPARSE_JUNCTION_DATA_BUFFER, SubstituteNameOffset)


/*
 *	symlink - uses Win32 junction points
 *
 *	For reference:	http://www.codeproject.com/w2k/junctionpoints.asp
 */
int symlink(const char *oldpath, const char *newpath)
{
	HANDLE		dirhandle;
	DWORD		len;
	char		buffer[MAX_PATH * sizeof(WCHAR) + sizeof(REPARSE_JUNCTION_DATA_BUFFER)];
	char		nativeTarget[MAX_PATH];
	char 		*p = nativeTarget;

	REPARSE_JUNCTION_DATA_BUFFER *reparseBuf = (REPARSE_JUNCTION_DATA_BUFFER *) buffer;

	CreateDirectoryA(newpath, 0);
	dirhandle = CreateFileA(newpath, GENERIC_READ | GENERIC_WRITE,
						   0, 0, OPEN_EXISTING,
		   FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, 0);

	if (dirhandle == INVALID_HANDLE_VALUE)
		return -1;

	/* make sure we have an unparsed native win32 path */
	if (memcmp("\\??\\", oldpath, 4))
		sprintf(nativeTarget, "\\??\\%s", oldpath);
	else
		strcpy(nativeTarget, oldpath);

	while ((p = strchr(p, '/')) != 0)
		*p++ = '\\';

	len = strlen(nativeTarget) * sizeof(WCHAR);
	reparseBuf->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
	reparseBuf->ReparseDataLength = len + 12;
	reparseBuf->Reserved = 0;
	reparseBuf->SubstituteNameOffset = 0;
	reparseBuf->SubstituteNameLength = len;
	reparseBuf->PrintNameOffset = len + sizeof(WCHAR);
	reparseBuf->PrintNameLength = 0;
	MultiByteToWideChar(CP_ACP, 0, nativeTarget, -1,
						reparseBuf->PathBuffer, MAX_PATH);

	/*
	 * FSCTL_SET_REPARSE_POINT is coded differently depending on SDK
	 * version; we use our own definition
	 */
	if (!DeviceIoControl(dirhandle,
						 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_ANY_ACCESS),
						 reparseBuf,
						 reparseBuf->ReparseDataLength + REPARSE_JUNCTION_DATA_BUFFER_HEADER_SIZE,
						 0, 0, &len, 0))
	{
		LPSTR		msg;

		errno = 0;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					  NULL, GetLastError(),
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					  (LPSTR) & msg, 0, NULL);
		LocalFree(msg);

		CloseHandle(dirhandle);
		RemoveDirectoryA(newpath);
		return -1;
	}

	CloseHandle(dirhandle);

	return 0;
}

int remove_symlnk(const char *path) {
	HANDLE handle = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
					0,
					0,
					OPEN_EXISTING,
					FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
					0);

	if (handle == INVALID_HANDLE_VALUE)
		return -1;

	DWORD dwBytes;
	REPARSE_JUNCTION_DATA_BUFFER rgdb = { 0 };
	rgdb.ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;

	BOOL bOK = DeviceIoControl(handle,
						FSCTL_DELETE_REPARSE_POINT,
						&rgdb,
						REPARSE_GUID_DATA_BUFFER_HEADER_SIZE,
						NULL,
						0,
						&dwBytes,
						0);
	CloseHandle(handle);
	if (!bOK)
		return -1;
 
	bOK = RemoveDirectoryA(path);
	if (!bOK)
		return -1;

	return 0;


}

#endif
