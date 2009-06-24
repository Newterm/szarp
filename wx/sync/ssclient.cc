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
#include "ssclient.h"
#include "ssutil.h"
#include "libpar.h"
#include "ipc.h"
#include "md5.h"
#include "../../resources/wx/icons/ssc16.xpm"

#ifdef MINGW32
#undef int16_t
typedef short int               int16_t;
#endif


#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

#include <functional>
#include <set>

#include <ares.h>
#include <ares_dns.h>

namespace fs = boost::filesystem;

#ifndef MINGW32
#include "signal.h"
#endif

IMPLEMENT_APP(SSCApp);
CFileSyncer::Request::Request(CFileSyncer::Request::Type type, uint32_t num) : 
	m_type(type), m_num(num) {
}

std::set<wxString> get_servers_from_config() {
	std::set<wxString> ret;
	wxString tmp;

	wxConfigBase* config = wxConfigBase::Get(true);

	if (config->Read(_T("SERVERS"), &tmp)) {

		wxStringTokenizer tkz(tmp, _T(";"), wxTOKEN_STRTOK);
		while (tkz.HasMoreTokens()) {
			wxString token = tkz.GetNextToken();
			ret.insert(token);
		}
	}

	return ret;
}


std::map<wxString, std::set<wxString> > get_servers_bases_from_config() {
	std::map<wxString, std::set<wxString> > ret;
	wxConfigBase* config = wxConfigBase::Get(true);

	std::set<wxString> servers = get_servers_from_config();

	for (std::set<wxString>::iterator i = servers.begin();
			i != servers.end();
			i++) {
		std::set<wxString> bs;
		wxString bases;
		config->Read(*i + _T("/BASES"), &bases);

		wxStringTokenizer tkz(bases, _T(";"), wxTOKEN_STRTOK);
		while (tkz.HasMoreTokens())
			bs.insert(tkz.GetNextToken());

		ret[*i] = bs;

	}

	return ret;

}

void store_server_bases_to_config(std::map<wxString, std::set<wxString> > bs) {
	wxConfigBase* config = wxConfigBase::Get(true);

	for (std::map<wxString, std::set<wxString> >::iterator i = bs.begin();
			i != bs.end();
			i++) {
		bool first = true;
		wxString bstring;
		for (std::set<wxString>::iterator j = i->second.begin();
				j != i->second.end();
				j++) {
			if (!first)
				bstring += _T(";");
			bstring += *j;
			first = false;
		}
		config->Write(i->first + _T("/BASES"), bstring);
	}
	config->Flush();
}


std::map<wxString, std::pair<wxString, wxString> > get_servers_creditenials_config() {
	std::set<wxString> servers = get_servers_from_config();

	std::map<wxString, std::pair<wxString, wxString> > ret;

	wxConfigBase* config = wxConfigBase::Get(true);

	for (std::set<wxString>::iterator i = servers.begin();
			i != servers.end();
			i++) {

		wxString username, password;
		config->Read(*i + _T("/USERNAME"), &username);
		config->Read(*i + _T("/PASSWORD"), &password);

		ret[*i] = std::make_pair(username, password);
	}

	return ret;
}

void store_servers_creditenials_config(std::map<wxString, std::pair<wxString, wxString> > servers_creds) {
	bool first = true;
	wxString servers_string;

	wxConfigBase* config = wxConfigBase::Get(true);

	for (std::map<wxString, std::pair<wxString, wxString> >::iterator i = servers_creds.begin();
			i != servers_creds.end();
			i++) {

		if (!first)
			servers_string += _T(";");
		servers_string += i->first;

		config->Write(i->first + _T("/USERNAME"), i->second.first);
		config->Write(i->first + _T("/PASSWORD"), i->second.second);
		
		first = false;

	}

	config->Write(_T("SERVERS"), servers_string);
	config->Flush();

}


#ifdef __WXMSW__

void CFileSyncer::RequestGenerator::MswFileIterator::ProceedToLink() {
	while (m_iterator != m_map.end() &&
		m_iterator->second.GetType() != TPath::TLINK) 
	m_iterator++;
}

void CFileSyncer::RequestGenerator::MswFileIterator::ProceedToNonLink() {
	while (m_iterator != m_map.end() &&
			m_iterator->second.GetType() == TPath::TLINK) 
		m_iterator++;
}

CFileSyncer::RequestGenerator::MswFileIterator::MswFileIterator(MAP& map) :
	m_map(map),
	m_iterator(map.begin()),
	m_link(false) {
}

void CFileSyncer::RequestGenerator::MswFileIterator::operator++(int) {
	m_iterator++;
	if (m_link) 
		ProceedToLink();
	else {
		ProceedToNonLink();
		if (m_iterator == m_map.end()) {
			m_link = true;
			m_iterator = m_map.begin();
			ProceedToLink();
		}
	}
}

bool CFileSyncer::RequestGenerator::MswFileIterator::operator==(const MAPI& iterator) const {
	return m_iterator == iterator;
}

CFileSyncer::RequestGenerator::MswFileIterator::MAP::value_type* CFileSyncer::RequestGenerator::MswFileIterator::operator->(){
	return &*m_iterator;
}

void CFileSyncer::RequestGenerator::MswFileIterator::operator=(const MAPI& iterator) {
	m_iterator = iterator;
}
#endif

CFileSyncer::RequestGenerator::RequestGenerator(const TPath& local_dir, 
	std::map<uint32_t, TPath>& file_list, 
	std::vector<TPath>& files, 
	std::deque<Request>& requests,
	PacketExchanger *exch,
	Progress& progress)
	: 
	m_local_dir(local_dir),
	m_file_list(file_list),
#ifdef __WXMSW__
	m_current_file(file_list),
#endif
	m_requests(requests),
	m_exchanger(exch),
	m_state(IDLE),
	m_processing_extra_requests(false) {

	memset(&m_sigstate, 0, sizeof(m_sigstate));

	m_sigstate.m_file_buffer = (uint8_t*) malloc(Packet::MAX_LENGTH);

	for (uint32_t i = 0; i < files.size(); ++i) {
		//check for termination request sometime
		if (!(i % 500))
			progress.Check();

		TPath& path = files[i];
		TPath localpath(local_dir.Concat(path).GetPath());
		if (path.GetType() == TPath::TFILE 
			&& localpath.GetType() == TPath::TFILE
			&& path.GetSize() == localpath.GetSize()
			&& path.GetModTime() == localpath.GetModTime()) 
			continue; 

		sz_log(7, "File %s is to be synced", path.GetPath());
		/*wxLogWarning(_("File %s is to be synced %d %d %d %d"), csconv(path.GetPath()).c_str(),
				path.GetType(), localpath.GetType(), path.GetSize(), localpath.GetSize());*/

		m_file_list[i] = path;
	}

	//it's not needed anymore
	files.resize(0);

	m_current_file = m_file_list.begin();

	m_exchanger->SetWriter(this);

}

CFileSyncer::RequestGenerator::~RequestGenerator() {
	free(m_sigstate.m_file_buffer);
	if (m_sigstate.m_signed_file)
		fclose(m_sigstate.m_signed_file);
	if (m_sigstate.m_job)
		rs_job_free(m_sigstate.m_job);
}

bool CFileSyncer::RequestGenerator::Finished() {
	if (m_processing_extra_requests)
		return m_extra_requests_iterator == m_extra_requests.end();
	else
		return (m_current_file == m_file_list.end()) && m_extra_requests.size() == 0;
	
}

void CFileSyncer::RequestGenerator::MoveForward() {
	if (!m_processing_extra_requests)
		m_current_file++;

	if (!(m_current_file == m_file_list.end()))
		return;

	if (m_processing_extra_requests)
		m_extra_requests_iterator++;
	else {
		m_processing_extra_requests = true;
		m_extra_requests_iterator = m_extra_requests.begin();
	}

}

Packet* CFileSyncer::RequestGenerator::GivePacket() {
	Packet *p = NULL;

	do {
		if (m_state == SIGNATUE_GENERATION) 
			 p = PatchReqPacket();
		else if (Finished()) {
			sz_log(7, "All request send");

			Packet *p = new Packet;
			p->m_type = Packet::EOS_PACKET;
			p->m_size = 0;
			p->m_data = NULL;
			m_exchanger->ReadBackPacket(p);

			m_exchanger->SetWriter(NULL);
			return NULL;
		} else 
			p = StartRequest();
		
	} while (p == NULL);

	return p;
		
}

void CFileSyncer::RequestGenerator::AddExtraRequest(uint32_t file_no) {

	if (m_processing_extra_requests && 
			m_extra_requests_iterator == m_extra_requests.end()) {

			m_extra_requests.clear();
			m_extra_requests.push_back(file_no);

			m_extra_requests_iterator = m_extra_requests.begin();

			m_exchanger->SetWriter(this);
	} else
		m_extra_requests.push_back(file_no);

}

TPath& CFileSyncer::RequestGenerator::FilePath() {
	if (!m_processing_extra_requests)
		return m_current_file->second;
	else
		return m_file_list[*m_extra_requests_iterator];
}

uint32_t CFileSyncer::RequestGenerator::FileNo() {
	if (!m_processing_extra_requests)
		return m_current_file->first;
	else
		return *m_extra_requests_iterator;
}

Packet *CFileSyncer::RequestGenerator::StartRequest() {
	TPath& path = FilePath();
	TPath local_path = m_local_dir.Concat(path);

	if (local_path.GetType() == TPath::TNOTEXISTS) {
		sz_log(7, "Requesting new file %s", local_path.GetPath());
		return NewFileRequest(path);
	}

	if (local_path.GetType() == TPath::TFILE 
		&& path.GetType() == TPath::TFILE) {
		sz_log(7, "Requesting patch for file %s", local_path.GetPath());
		return PatchReqPacket();
	} 

	return NewFileRequest(path);
}

Packet* CFileSyncer::RequestGenerator::NewFileRequest(TPath& path) {
	uint16_t msg_type;
	if (path.GetType() == TPath::TLINK) {
		Request req(Request::LINK, FileNo());
		m_requests.push_back(req);

		msg_type = MessageType::SEND_LINK;
		sz_log(7, "Sent request for link file %s ", path.GetPath());
	} else {
		Request req(Request::COMPLETE_FILE, FileNo());
		m_requests.push_back(req);

		msg_type = MessageType::SEND_COMPLETE_FILE;
		sz_log(7, "Sent request for complete file %s ", path.GetPath());
	}

	Packet* p = new Packet;
	p->m_type = Packet::LAST_PACKET;
	size_t s = sizeof(uint16_t) +  sizeof(uint32_t);
	p->m_data = (uint8_t*) malloc(s);
	*(uint16_t*)p->m_data = htons(msg_type);
	*(uint32_t*)(p->m_data + sizeof(uint16_t)) = htonl(FileNo());
	p->m_size = s;

	MoveForward();

	return p;
}

Packet* CFileSyncer::RequestGenerator::PatchReqPacket() {
	
	if (m_state == IDLE) {
		TPath& path = FilePath();
		TPath local_path = m_local_dir.Concat(path);

		memset(&m_sigstate.m_iobuf, 0, sizeof(m_sigstate.m_iobuf));
		m_sigstate.m_signed_file = fopen(local_path.GetPath(), "rb");

		if (m_sigstate.m_signed_file == NULL) {
			MoveForward();
			return NULL;
		}

		m_sigstate.m_job = rs_sig_begin(700, 2);
		sz_log(8, "Begin generation of signature of file %s", local_path.GetPath());
	}

	Packet *p = NULL;
	uint8_t* out_buffer = NULL;
	uint32_t out_left = 0;

	do {
		if (p == NULL) {
			p = new Packet;
			p->m_type = Packet::OK_PACKET;
			p->m_data = (uint8_t*) malloc(Packet::MAX_LENGTH);
			if (m_state == IDLE) {
				*(uint16_t*)p->m_data = htons(MessageType::SEND_FILE_PATCH);
				*(uint32_t*)(p->m_data + sizeof(uint16_t)) = htonl(FileNo());

				const size_t msg_info_size = sizeof(uint16_t) + sizeof(uint32_t);
				out_buffer = p->m_data + msg_info_size;
				out_left = Packet::MAX_LENGTH - msg_info_size;
				p->m_size = msg_info_size; 
			} else {
				out_buffer = p->m_data;
				out_left = Packet::MAX_LENGTH;
				p->m_size = 0;
			}
		}
		m_sigstate.m_iobuf.next_out = (char*)out_buffer;
		m_sigstate.m_iobuf.avail_out = out_left;

		if (!m_sigstate.m_iobuf.avail_in) {
			size_t r = ReadFully(m_sigstate.m_signed_file, 
				m_sigstate.m_file_buffer, 
				Packet::MAX_LENGTH);
			m_sigstate.m_iobuf.next_in = (char*)m_sigstate.m_file_buffer;
			m_sigstate.m_iobuf.avail_in = r;
			if (r != Packet::MAX_LENGTH) 
				m_sigstate.m_iobuf.eof_in = 1;
		}

		rs_result res = rs_job_iter(m_sigstate.m_job, &m_sigstate.m_iobuf);

		uint32_t added = ((uint8_t*)m_sigstate.m_iobuf.next_out - out_buffer);

		if (res != RS_BLOCKED && res != RS_DONE)
			assert(false);

		p->m_size += added ;
		out_buffer += added;
		out_left -= added;

		if (res == RS_DONE) {
			m_requests.push_back(Request(Request::PATCH, FileNo()));
			sz_log(8, "Finish of sending signature");
			p->m_type = Packet::LAST_PACKET;

			fclose(m_sigstate.m_signed_file);
			m_sigstate.m_signed_file = NULL;
			rs_job_free(m_sigstate.m_job);
			m_sigstate.m_job = NULL;
			m_state = IDLE;

			MoveForward();

			p->m_data = (uint8_t*)realloc(p->m_data, p->m_size);
			return p;
		}

		if (m_sigstate.m_iobuf.avail_in) {
			m_state = SIGNATUE_GENERATION;
			return p;
		}

	} while (true);
}
					
CFileSyncer::ResponseReceiver::ResponseReceiver(const TPath& local_dir, 
	std::map<uint32_t, TPath>& file_list, 
	std::deque<Request>& requests,
	PacketExchanger* exchanger,
	Progress& progress)
	:
	m_local_dir(local_dir),
	m_file_list(file_list),
	m_requests(requests),
	m_exchanger(exchanger),
	m_state(IDLE),
	m_dest(NULL),
	m_base(NULL),
	m_patch_job(NULL),
	m_progress(progress),
	m_synced_count(0),
	m_all_requests_sent(false) {
	
	m_exchanger->SetReader(this);
}

CFileSyncer::ResponseReceiver::~ResponseReceiver() {
	if (m_requests.size() == 0)
		return;

	RemoveTmpFile();
	CloseBaseFile();
}

TPath CFileSyncer::ResponseReceiver::TempFileName() {
	TPath& path = m_file_list[FileNo()];
	TPath local_path = m_local_dir.Concat(path);
	TPath dir = local_path.DirName();
	return dir.Concat("ssc.tmp"); 
}

uint32_t CFileSyncer::ResponseReceiver::FileNo() {
	Request& req = m_requests.front();
	return req.m_num;
}


TPath CFileSyncer::ResponseReceiver::LocalFileName() {
	TPath& path = m_file_list[FileNo()];
	TPath local_path = m_local_dir.Concat(path);
	return local_path;
}

TPath CFileSyncer::ResponseReceiver::FileName() {
	TPath& path = m_file_list[FileNo()];
	return path;
}

void CFileSyncer::ResponseReceiver::OpenTmpFile() {
	TPath path = TempFileName();
	path.DirName().CreateDir();
	assert (m_dest == NULL);
	m_dest = fopen(path.GetPath(), "wb");
	if (!m_dest) 
		sz_log(6, "Failed to open temp file %s", path.GetPath());
}

void CFileSyncer::ResponseReceiver::OpenBaseFile() {
	assert (m_base == NULL);
	TPath path = LocalFileName();
	m_base = fopen(path.GetPath(), "rb");
}

void CFileSyncer::ResponseReceiver::CloseBaseFile() {
	if (m_base)
		fclose(m_base);
	m_base = NULL;
}

void CFileSyncer::ResponseReceiver::CloseTmpFile() {
	if (m_dest)
		fclose(m_dest);
	m_dest = NULL;
}

void CFileSyncer::ResponseReceiver::SetFileModTime() {
	TPath& p = m_file_list[FileNo()];
	struct utimbuf buf;
	buf.actime = buf.modtime = p.GetModTime();

	int ret = utime(LocalFileName().GetPath() , &buf);
	if (ret != 0)
		sz_log(2, "Unable to set mod and access time for file %s(%s)", p.GetPath(), strerror(errno));
}

void CFileSyncer::ResponseReceiver::MoveTmpFile() {
	CloseTmpFile();
#ifdef __WXMSW__
	unlink(LocalFileName().GetPath());
#endif
	int ret = rename(TempFileName().GetPath(), LocalFileName().GetPath());
	if (ret != 0) {
		sz_log(2, "Failed to rename %s %s", TempFileName().GetPath(), LocalFileName().GetPath());
		RemoveTmpFile();
	} 
	SetFileModTime();
}

void CFileSyncer::ResponseReceiver::RemoveTmpFile() {
	CloseTmpFile();
	unlink(TempFileName().GetPath());
}

void CFileSyncer::ResponseReceiver::ReadPacket(Packet *p) {

	sz_log( 8, "Requests queue size %zu", m_requests.size());

	if (p->m_type == Packet::ERROR_PACKET) 
		HandleErrorPacket(p);
	else if (p->m_type == Packet::EOS_PACKET) {
		m_all_requests_sent = true;
		delete p;
	} else {
		assert (m_requests.size() > 0);
		Request& req = m_requests.front();
		switch (m_state)  {
			case COMPLETE_RECEPTION:
				HandleNewFilePacket(p);
				break;
			case PATCH_RECEPTION:
				HandlePatchPacket(p);
				break;
			case IDLE:
				switch (req.m_type) {
					case Request::COMPLETE_FILE:
						HandleNewFilePacket(p);
						break;
					case Request::PATCH:
						HandlePatchPacket(p);
						break;
					case Request::LINK:
						HandleLinkPacket(p);
						break;
					default:
						assert(false);
				}
				break;
			default:
				assert(false);
		}
	}

	if (m_state == IDLE && m_requests.size() == 0 && m_all_requests_sent)
		m_exchanger->SetReader(NULL);

}

void CFileSyncer::ResponseReceiver::HandleLinkPacket(Packet *p) {
	TPath local_path = LocalFileName();

	local_path.DirName().CreateDir();

	sz_log(8, "Link packet received");
	uint16_t len = ntohs(*(uint16_t*)p->m_data);
	assert (p->m_size == len + sizeof(uint16_t));

	char *path = (char*) malloc(len + 1);
	strncpy(path, (char*) (p->m_data + sizeof(uint16_t)), len);
	path[len] = '\0';
	free(p->m_data);
	delete p;

	sz_log(7, "Creating link %s",path);

	unlink(local_path.GetPath());
#ifndef MINGW32
	symlink(path, local_path.GetPath());
#else
	for (char *c = path; *c; ++c)
		if (*c == '/')
			*c = '\\';
	
	//reparse point must point to an absolute path so we have to do some
	//extra "processing"
	//
	char destination_path[MAX_PATH] = "";

	char *dir = dirname(local_path.GetPath());
	PathCombineA(destination_path, dir, path);

	size_t dlen = strlen(destination_path);
	if (dlen > 0 && destination_path[dlen - 1] == '\\')
		destination_path[dlen - 1] = '\0';

	symlink(destination_path, local_path.GetPath());
#endif

	free(path);

	SetFileModTime();

	m_requests.pop_front();
	m_synced_count++;
}

void CFileSyncer::ResponseReceiver::HandleErrorPacket(Packet* p) {
	sz_log(8, "Error packet received");
	if (m_state != IDLE) {
		RemoveTmpFile();
		if (m_patch_job) {
			rs_job_free(m_patch_job);
			m_patch_job = NULL;
		}
	}

	free(p->m_data);
	delete p;

	m_requests.pop_front();
	m_synced_count++;
	m_state = IDLE;
}

ssstring CFileSyncer::ResponseReceiver::DataBaseName() {
	TPath& file_name = m_file_list[FileNo()];
	char *path = strdup(file_name.GetPath());

#ifdef MINGW32
	char *c = strchr(path, '\\');
#else
	char *c = strchr(path, '/');
#endif
	
	ssstring result;

	if (c != NULL) {
		*c = '\0';
		result = csconv(path);
	}
	free(path);

	return result;
}


void CFileSyncer::ResponseReceiver::HandleNewFilePacket(Packet *p) {
	if (m_state == IDLE) {
		sz_log(8, "Begin reception of raw file");
		m_progress.SetProgress(Progress::SYNCING, 
				m_synced_count * 100 / m_file_list.size(),
				DataBaseName());
		OpenTmpFile();
		m_state = COMPLETE_RECEPTION;
	}

	if (m_dest)  
		WriteFully(m_dest, p->m_data, p->m_size);

	if (p->m_type == Packet::LAST_PACKET) {
		MoveTmpFile();
		m_state = IDLE;
		sz_log(8, "End reception of raw file");
		m_requests.pop_front();
		m_synced_count++;
	}

	free(p->m_data);
	delete p;
}

void CFileSyncer::ResponseReceiver::HandlePatchPacket(Packet *p) {
	if (m_state == IDLE) {
		OpenTmpFile();
		OpenBaseFile();
		if (m_base == NULL) { /*base file disappered - this may happen when link is converted to a directory 
				      - schedule refetch of this file*/
			m_generator->AddExtraRequest(FileNo());
			m_all_requests_sent = false;
		}
		m_patch_job = rs_patch_begin(rs_file_copy_cb, m_base);
		sz_log(8, "Begin reception of patch for file %s", LocalFileName().GetPath());
		m_progress.SetProgress(Progress::SYNCING, 
				m_synced_count * 100 / m_file_list.size(),
				DataBaseName());
		m_state = PATCH_RECEPTION;
	}

	if (!m_dest || !m_base) { 
		if (p->m_type == Packet::LAST_PACKET) {
			CloseBaseFile();
			CloseTmpFile();
			RemoveTmpFile();
			m_state = IDLE;
			m_requests.pop_front();
			m_synced_count++;
		}

		free(p->m_data);
		delete p;

		return;
	}

	rs_buffers_s rsbuf;
	rsbuf.next_in = (char*)p->m_data;
	rsbuf.avail_in = p->m_size;
	rsbuf.eof_in = p->m_type == Packet::LAST_PACKET;

	do {
		uint8_t buffer[4 * Packet::MAX_LENGTH];	//zupelnie arbitralnie wybrana wartosc 
		rsbuf.next_out = (char*)buffer;
		rsbuf.avail_out = sizeof(buffer);

		rs_result res = rs_job_iter(m_patch_job, &rsbuf);

		WriteFully(m_dest, buffer, (uint8_t*)rsbuf.next_out - buffer);
		if (res == RS_DONE) {
			rs_job_free(m_patch_job);
			m_patch_job = NULL;

			CloseTmpFile();
			CloseBaseFile();
			MoveTmpFile();

			m_state = IDLE;
			sz_log(8, "End reception of patch");
			m_requests.pop_front();
			m_synced_count++;
			break;
		}

		if (res != RS_BLOCKED) {
			CloseBaseFile();
			CloseTmpFile();
			RemoveTmpFile();

			rs_job_free(m_patch_job);
			m_patch_job = NULL;

			if (p->m_type == Packet::LAST_PACKET) {
				m_state = IDLE;
				m_requests.pop_front();
				m_synced_count++;
			}

			break;
		}

	} while (rsbuf.avail_in > 0);

	free(p->m_data);
	delete p;
}

void CFileSyncer::ResponseReceiver::SetRequestGenerator(RequestGenerator *generator) {
	m_generator = generator;
}

CFileSyncer::CFileSyncer (const TPath &local_dir, const TPath &sync_dir,
	std::vector<TPath> &server_list, 
	PacketExchanger* exchanger, 
	Progress& progress,
	bool delete_option) 
	:
	m_sync_file_list(server_list),
	m_request_queue(),	
	m_generator(local_dir, m_file_list, server_list, m_request_queue, exchanger, progress),
	m_receiver(local_dir, m_file_list, m_request_queue, exchanger, progress),
	m_progress(progress),
	m_exchanger(exchanger),
	m_sync_dir(sync_dir),
	m_delete_option(delete_option)
{
	m_receiver.SetRequestGenerator(&m_generator);
}

std::string stringToLower(std::string strToLower) {
	const int length = strToLower.length();
	for(int i=0; i!=length; ++i) {
		strToLower[i] = std::tolower(strToLower[i]);
	}
	return strToLower;
}

void CFileSyncer::Sync() {
	sz_log(7, "Start syncing");

	m_progress.SetProgress(Progress::SYNCING, 0);

	m_exchanger->Drive();

#if 0
	wxLogDebug(_("synchronization completed"));
#endif
	
	if (m_delete_option) {
		
#if 0
		wxLogDebug(_("deleting files not present on the server - start"));
#endif
		
		bool ok = true;
		
		fs::path dirpath(m_sync_dir.GetPath());
		fs::path tmppath = dirpath;
	
		tmppath.remove_leaf();
	
		std::vector<fs::path> presentFiles(m_sync_file_list.size());
			
		try {
			for(unsigned int i = 0; i < m_sync_file_list.size(); i++) {
				presentFiles[i] = tmppath / fs::path(m_sync_file_list[i].GetPath());
#ifdef MINGW32
				std::string tmp = stringToLower(presentFiles[i].string());
				presentFiles[i] = fs::path(tmp);
#endif
			}
		} catch (fs::filesystem_error &e) {
#if 0
			wxLogError(wxString(_("error while preparing list of files from the server: ") + SC::A2S(e.what())));
#endif
			ok = false;
		}
		
		if (ok) {
			try {
				std::sort(presentFiles.begin(), presentFiles.end());
			
				for (fs::recursive_directory_iterator  itr( dirpath ); itr != fs::recursive_directory_iterator(); ++itr ) {
					fs::path tmp = itr->path();
		
					TPath tmppath(tmp.string().c_str());
		
					if( tmppath.GetType() == TPath::TLINK) {
						itr.no_push();
						continue;
					}
		
					if (fs::is_regular(itr->status())) {
			
						fs::path tmp_path = itr->path();
#ifdef MINGW32
						std::string tmpstring = stringToLower(itr->path().string());
						tmp_path = fs::path(tmpstring);
#endif
	
						try{
							if(!std::binary_search(presentFiles.begin(), presentFiles.end(), tmp_path)) {
								fs::remove(itr->path());
							}
						} catch(fs::filesystem_error &e) {
#if 0
								wxLogError(wxString(SC::A2S(itr->path().string().c_str())) 
										+ _(" error while deleting file: ") + SC::A2S(e.what()));
#endif
								ok = false;
						}
					}
					
					if(!ok)
						break;
				}
				
			} catch(fs::filesystem_error &e) {
#if 0
					wxLogError(wxString(_("error while iterating through directories: ") + SC::A2S(e.what())));
#endif
					ok = false;
			}
		}
		
#if 0
		wxLogDebug(_("deleting files not present on the server - end"));
#endif
	}

	m_progress.SetProgress(Progress::SYNCING, 100);

	sz_log(7, "Done syncing");
}

void Client::Init()  {

	m_work_mutex = new wxMutex();

	m_work_condition = new wxCondition(*m_work_mutex);

	Create();
	Run();

}

void Client::BeginSync(bool delete_option, wxArrayString dirList) {
	m_dirList.clear();
	for (size_t i = 0; i < dirList.GetCount(); i++) {
		m_dirList.push_back((char*)SC::S2U(dirList[i]).c_str());
	}
	m_delete_option = delete_option;
	m_work_condition->Signal();
}

void *Client::Entry() {
#ifdef MINGW32
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	WSAStartup(wVersionRequested, &wsaData);
#endif

	InitSSL();

	while (true) {
		m_work_condition->Wait();

		m_dir_server_map.clear();

		m_exchanger = NULL;
		try {
			bool ok = Synchronize();
			if (ok)
				m_progress.SetProgress(Progress::FINISHED, 100, _(""), &m_dir_server_map);
			else
				m_progress.SetProgress(Progress::FAILED, 100);
		} catch (TerminationRequest &e) {
			m_progress.SetProgress(Progress::SYNC_TERMINATED, 0);
		}

	}
	return NULL;
}


Client::Client(Progress &progress) 
		: 
		m_ctx(NULL),
		m_addresses(NULL),
		m_port(0),
		m_users(NULL),
		m_passwords(NULL),
		m_servers_count(0),
		m_local_dir(),
		m_progress(progress),
		m_exchanger(NULL) {

}

void Client::SetOptions(char **addresses,
		char **users,
		char **passwords,
		int servers_count,
		char *local_dir,
		int port) {

	for (int i = 0; i < m_servers_count; i++)
		free(m_addresses[i]);
	free(m_addresses);
	m_addresses = addresses;

	for (int i = 0; i < m_servers_count; i++)
		free(m_users[i]);
	free(m_users);
	m_users = users;

	for (int i = 0; i < m_servers_count; i++)
		free(m_passwords[i]);
	free(m_passwords);
	m_passwords = passwords;
	
	m_servers_count = servers_count;

	m_local_dir = local_dir;
	free(local_dir);

	m_port = port;

}


Client::~Client() {
	delete m_exchanger;
}

bool Client::Synchronize()  {
	bool ok = true;

	for (int i = 0; i < m_servers_count; i++) {
		m_current_server = i;
		m_current_address = m_addresses[i];

		bool was_redirected = false;
		try {
			bool redirected = false;
			do {
				Connect();

				Auth(redirected);

				if (!was_redirected)
					was_redirected = redirected;

			} while (redirected);

			SyncFiles();
		
		} catch (TerminationRequest &e) {
			if (was_redirected)
				free(m_current_address);

			delete m_exchanger;
			m_exchanger = NULL;
			throw;
		} catch (Exception &e) {
			m_progress.SetProgress(Progress::MESSAGE, 0, e.What() + _(" (") + csconv(m_addresses[m_current_server]) + _(")"));
			ok = false;
		}

		if (was_redirected)
			free(m_current_address);

		delete m_exchanger;
		m_exchanger = NULL;

	}

	return ok;
}

namespace {
void ares_cb(void *arg, int status, int timeouts, struct hostent *host) {
	std::pair<bool, uint32_t> *r = reinterpret_cast<std::pair<bool, uint32_t>*>(arg);
	if (status != ARES_SUCCESS) {
		r->first = false;
	} else {
		assert(host->h_length == 4);
		r->second = *((uint32_t*)host->h_addr_list[0]);
		r->first = true;
	}

}

}


uint32_t Client::ResolveAddress() {

	ares_channel channel;
	int status = ares_init(&channel);
	if (status != ARES_SUCCESS)
		throw Exception(_("unable to connect to server "));

	std::pair<bool, uint32_t> result;
	ares_gethostbyname(channel, m_current_address, AF_INET, ares_cb, &result);

	try {
		while (true) {
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 500000;

			fd_set read_fds, write_fds
#ifdef MINGW32
			       , err_fds
#endif
					;

			FD_ZERO(&read_fds);
			FD_ZERO(&write_fds);
#ifdef MINGW32
			FD_ZERO(&err_fds);
#endif
			m_progress.Check();

			int nfds = ares_fds(channel, &read_fds, &write_fds);
			if (nfds == 0)
				break;

			bool any_in = false;
			bool any_out = false;
			for (int i = 0; i < nfds + 1; i++) {
				if (FD_ISSET(i, &read_fds)) {
					any_in = true;
#ifdef MINGW32
					FD_SET(i, &err_fds);
#endif
				}
				if (FD_ISSET(i, &write_fds)) {
					any_out = true;
#ifdef MINGW32
					FD_SET(i, &err_fds);
#endif
				}
			}

			select(nfds + 1, any_in ? &read_fds : NULL, any_out ? &write_fds : NULL, 
#ifdef MINGW32
					&err_fds, 
#else
					NULL,
#endif
					&tv);

#ifdef MINGW32
			for (int i = 0; i < nfds + 1; i++)
				if (FD_ISSET(i, &err_fds))
					throw Exception(_("unable to connect to server "));
#endif
			ares_process(channel, &read_fds, &write_fds);

		}
	} catch (...) {
		ares_destroy(channel);
		throw;
	}

	if (result.first == false)
		throw Exception(_("unable to connect to server "));

	return result.second;
}

void Client::Connect() {
	m_progress.SetProgress(Progress::CONNECTING, 1, csconv(m_current_address));

	uint32_t address = ResolveAddress();

	BIO* sock_bio = BIO_new(BIO_s_connect());
	BIO_set_conn_ip(sock_bio, &address);
	BIO_set_conn_int_port(sock_bio, &m_port);
	//we are closing socket on our own
	BIO_set_close(sock_bio, 0);

	BIO_set_nbio(sock_bio, 1);

	if (m_ctx == NULL) {
		m_ctx = SSL_CTX_new(SSLv3_client_method());
		SSL_CTX_set_mode(m_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
	}

	SSL *ssl = SSL_new(m_ctx);
		
	SSL_set_bio(ssl, sock_bio, sock_bio);
	SSL_set_read_ahead(ssl, 1);

	bool write_blocked = false;

	int sret;
	try {
		while ((sret = SSL_connect(ssl)) != 1) {
			switch (SSL_get_error(ssl, sret)) {
				case SSL_ERROR_WANT_WRITE:
				case SSL_ERROR_WANT_CONNECT:
					write_blocked = true;
					break;	
				case SSL_ERROR_WANT_READ:
					write_blocked = false;
					break;
				case SSL_ERROR_ZERO_RETURN:
					throw ConnectionShutdown();
				case SSL_ERROR_SYSCALL:
#ifdef MINGW32
					if (GetLastError() != 0)
						throw IOException(GetLastError());
#else
					throw IOException(errno);
#endif
				default:
					throw SSLException(ERR_get_error());
				}
again:
			int sock = BIO_get_fd(sock_bio, NULL);
			fd_set set;
			FD_ZERO(&set);
			FD_SET(sock, &set);

			fd_set exset;
			FD_ZERO(&exset);
#ifdef __WXMSW__
			FD_SET(sock, &exset);
#endif

			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 500000;

			int ret;
			if (write_blocked)
				ret = select(sock + 1, NULL, &set, &exset, &tv);
			else
				ret = select(sock + 1, &set , NULL, &exset, &tv);

			m_progress.Check();

			if (ret == -1) {
				if (errno == EINTR)
					goto again;
				else
					throw IOException(errno);
			}

			if (ret == 0)
				goto again;

#ifdef __WXMSW__
			if (FD_ISSET(sock, &exset))
				throw Exception(_("unable to connect to server "));
#endif
		}
	} catch (...) {
		SSL_free(ssl);
		throw;
	}

	m_exchanger = new PacketExchanger(ssl, 1, &m_progress);

}

void Client::SyncFiles() {
	std::map<TPath, int, TPath::less> dir_map = GetDirList();

#ifdef MINGW32
	std::map<TPath, int, TPath::less> aggregated_map;
#endif

	for (std::map<TPath, int>::iterator i = dir_map.begin(); i != dir_map.end() ; ++i) {
#ifdef MINGW32
		if (i->first.GetPath()[strlen(i->first.GetPath()) - 1] == 'X') {
			aggregated_map[i->first] = i->second;
			continue;
		}
#endif
		std::vector<TPath> file_list = GetFileList(i->first, i->second, m_delete_option);
		CFileSyncer syncer(m_local_dir,
				i->first,
				file_list,
				m_exchanger,
				m_progress,
				m_delete_option);
		syncer.Sync();
	}

#ifdef MINGW32
	for (std::map<TPath, int>::iterator i = aggregated_map.begin(); i != aggregated_map.end() ; ++i) {

		std::vector<TPath> file_list = GetFileList(i->first, i->second, m_delete_option);
		CFileSyncer syncer(m_local_dir,
				i->first,
				file_list,
				m_exchanger,
				m_progress,
				m_delete_option);
		syncer.Sync();
	}
#endif
	m_exchanger->Disconnect();

}

char* Client::ExecuteScript(const char* b, const char* s) {
	uint16_t pos, len;
	char c;
	size_t i = 0, j;
	
	char* result = strdup(b);
	result = (char*) realloc(result, strlen(b) + strlen(s) + 1);

#define GETVAL(VAL)						\
{								\
	VAL = 0;						\
	uint8_t t = (s[i++] - 1) * 126;				\
	t = s[i++] - 1;						\
	VAL += t;						\
}
	
	while ((c = s[i++]) == 1) {
		GETVAL(pos);
		pos = pos - 1;
		GETVAL(len);
		for (j = pos; j < pos + len; ++j) 
			result[j] = s[i++];
	}

	switch (c) {
		case 0:
			break;
		case 2:
			j = strlen(result);
			while ((c = s[i++])) 
				result[j++] = c;
			result[j] = '\0';
			break;
		case 3:
			GETVAL(len);
			result[strlen(b) - len] = '\0';
			break;
		default:
			assert(false);
			break;
	}
	
	return (char*) realloc(result, strlen(result) + 1);
#undef GETVAL
}

void Client::Auth(bool &redirected) {
	redirected = false;

	m_progress.SetProgress(Progress::AUTHORIZATION, 0, csconv(m_current_address));
	MessageSender smsg(m_exchanger);
	smsg.PutUInt16(MessageType::AUTH3);
	smsg.PutString(m_users[m_current_server]);
	smsg.PutString(m_passwords[m_current_server]);
	smsg.PutString(key.GetKey());
	smsg.FinishMessage();
	
	MessageReceiver rmsg(m_exchanger);
	uint16_t status = rmsg.GetUInt16();
	uint16_t msg = Message::NULL_MESSAGE;
	
	if (status == Message::AUTH_REDIRECT) {
		char * message = rmsg.GetString();
		sz_log(9, "Redirected to %s", message);
		m_current_address = message;
		m_exchanger->Disconnect();
		delete m_exchanger;
		redirected = true;
		return;
	} else if (status == Message::AUTH_FAILURE) {
		uint16_t error = rmsg.GetUInt16();
		m_exchanger->Disconnect();
		if(error == Message::INVALID_HWKEY)
			throw Exception(_("SSC discover an attemption to run synchronization on invalid computer. Please contact with us."));
		else if (error == Message::ACCOUNT_EXPIRED)
			throw Exception(_("Your account expired, contact with us."));
		else if (error == Message::INVALID_AUTH_INFO)
			throw Exception(_("Incorrect user name or/and password."));

	} else if (status == Message::AUTH_OK) {
		msg = rmsg.GetUInt16();
	}
	
	if (msg == Message::NEAR_ACCOUNT_EXPIRED) 
		m_progress.SetProgress(Progress::MESSAGE, 0,_("Your account is almost expired. Please contact with us."));
	else if (msg == Message::OLD_VERSION)
		m_progress.SetProgress(Progress::MESSAGE, 0,_("You are using old version of ssc/sss protocol. Please update to the newest version your ssc"));
	
	sz_log(5, "Auth ok");
}	
	
std::map<TPath, int, TPath::less> Client::GetDirList() {
	std::map<TPath, int, TPath::less> result;
	MessageReceiver rmsg(m_exchanger);
	
	uint32_t count = rmsg.GetUInt32();
		
	if (count == 0) {
		m_exchanger->Disconnect();
		throw Exception(_("Not able to obtain file list - server configuration error"));
	}
	
	size_t dirCount = m_dirList.size();
	for (uint32_t i = 0; i < count; ++i)  {
		const char *dir_name = rmsg.GetString();
		sz_log(9, "got dir %s", dir_name);
		if (dirCount == 0) {
			result[m_local_dir.Concat(dir_name)] = i;
		} else {
			for (size_t j = 0; j < dirCount; j++) {
				if (!strcmp(dir_name, m_dirList[j].c_str()))
				{
					result[m_local_dir.Concat(dir_name)] = i;
					break;
				}
			}
		}

		m_dir_server_map[dir_name] = m_addresses[m_current_server];

	}
	return result;
}

void Client::GetExInEpxression(TPath& dir, uint32_t dir_no, char*& exclude, char*& include, bool &force_delete) {
	TPath program_uruchomiony = dir.Concat(
#ifndef MINGW32
						"szbase/Status/Meaner3/program_uruchomiony"
#else
						"szbase\\Status\\Meaner3\\program_uruchomiony"
#endif
					);

	const int loop_guard = 12;
	int i = 0;
	char *_include = NULL;
	bool found = false;
	wxDateTime now = wxDateTime::Now();
	long year = now.GetYear();
	long month = now.GetMonth() + 1;
	uint8_t present;

	MessageReceiver rmsg(m_exchanger);
	MessageSender smsg(m_exchanger);

	if (program_uruchomiony.GetType() != TPath::TDIR)
		goto none;

	smsg.PutUInt16(MessageType::BASE_MODIFICATION_TIME);
	smsg.PutUInt32(dir_no);
	smsg.FinishMessage();

	present = rmsg.GetByte();
	if (present) {
		sz_log(9, "Base stamp present on server");
		struct tm t;
		memset(&t, 0, sizeof(t));

		t.tm_year = rmsg.GetUInt16();
		t.tm_mon = rmsg.GetByte();
		t.tm_mday = rmsg.GetByte();
		t.tm_hour = rmsg.GetByte();
		t.tm_min = rmsg.GetByte();

		time_t modtime = timegm(&t);

		TPath szbase_stamp = dir.Concat(BASESTAMP_FILENAME);
		if (szbase_stamp.GetType() != TPath::TFILE ||
				szbase_stamp.GetModTime() != modtime) {

			sz_log(9, "Base stamp mismatch(or file not present locally), requesint all files info");
			goto none;
		}
	} else
		sz_log(9, "Base stamp not present on server");

	do {
		char *current, *tmp;
		asprintf(&current, "%.4lu%.2lu.szb", year, month);
		if (_include == NULL)
			asprintf(&tmp, "%s", current);
		else
			asprintf(&tmp, "%s|%s", _include, current);
		free(_include);
		_include = tmp;

		if (program_uruchomiony.Concat(current).GetType() != TPath::TNOTEXISTS) {
			if (found) {
				free(current);
				break;
			}
			found = true;
		}
		free(current);
		
		if (i++ > loop_guard) {
			free(_include);
			goto none;
		}

		if (--month == 0) {
			month = 12;
			year--;
		}

	} while (true);

	asprintf(&exclude, "(.*\\.szb|%s)", BASESTAMP_FILENAME);
	asprintf(&include, "(%s)", _include);
	free(_include);

	return;

none:
	exclude = strdup("");
	include = strdup("");
	force_delete = true;

}
	

std::vector<TPath> Client::GetFileList(const TPath &dir, uint32_t dir_no, bool &delete_option) {
	sz_log(9, "Starting grab of file list");

	m_progress.SetProgress(Progress::FETCHING_FILE_LIST, 1, 
			csconv(strrchr(dir.GetPath(), 
#ifndef MINGW32
							'/'
#else
							'\\'
#endif
							) + 1));

	TPath current_dir = dir;

	char *exclude, *include;
	if (delete_option) {
		exclude = strdup("");
		include = strdup("");
	} else
		GetExInEpxression(current_dir, dir_no, exclude, include, delete_option);
	sz_log(9, "Sending expressions exclude: '%s', include '%s' for dir %s", exclude, include, current_dir.GetPath());

	MessageSender smsg(m_exchanger);
	smsg.PutUInt16(MessageType::GET_FILE_LIST);
	smsg.PutUInt32(dir_no);
	smsg.PutString(exclude);
	smsg.PutString(include);
	smsg.FinishMessage();

	free(exclude);
	free(include);

	MessageReceiver rmsg(m_exchanger);

	uint32_t count = rmsg.GetUInt32();

	std::vector<TPath> result;

	char *previous_path = NULL;
	for (uint32_t i = 0 ; i < count; i++) {
		uint16_t type = rmsg.GetUInt16();

		char *path;
		if (previous_path == NULL) {
			path = rmsg.GetString();
			previous_path = path;
		} else {
			char *patch = rmsg.GetString();
			path = ExecuteScript(previous_path, patch);
			free(patch);
			free(previous_path);
			previous_path = path;
		}
#ifdef MINGW32
		for (char *p = path; *p; ++p)
			if (*p == '/') 
				*p = '\\';
#endif
		
		uint32_t size = rmsg.GetUInt32();
		
		struct tm t;	
		memset(&t, 0, sizeof(t));

		t.tm_year = rmsg.GetUInt16();
		t.tm_mon = rmsg.GetByte();
		t.tm_mday = rmsg.GetByte();
		t.tm_hour = rmsg.GetByte();
		t.tm_min = rmsg.GetByte();

		time_t modtime = timegm(&t);

		result.push_back(TPath(path, type, modtime, size));
		sz_log(9, "Got file on list %s", path);

		int progress = std::min(99, (int) (i * 100 / count) );
		m_progress.SetProgress(Progress::FETCHING_FILE_LIST, progress, 
			csconv(strrchr(dir.GetPath(), 
#ifndef MINGW32
							'/'
#else
							'\\'
#endif
							) + 1));
	}

	free(previous_path);

	sz_log(9, "The file is list is here");

	return result;

}

ProgressEvent::ProgressEvent(int value, Progress::Action action, ssstring description, std::map<std::string, std::string>* dir_map)
	: 
	wxCommandEvent(PROGRESS_EVENT, ID_PROGRESS_EVNT_ORIG), 
	m_value(value), 
	m_action(action),
	m_description(wcsdup(description.c_str()))
{
	if (dir_map) {
		m_dir_server_map = new std::map<char*, char*>();
		for (std::map<std::string, std::string>::iterator i = dir_map->begin();
				i != dir_map->end();
				i++)
			(*m_dir_server_map)[strdup(i->first.c_str())] = strdup(i->second.c_str());
	} else
		m_dir_server_map = NULL;
}

int ProgressEvent::GetValue() {
	return m_value;
}

Progress::Action ProgressEvent::GetAction() {
	return m_action;
}

wxChar* ProgressEvent::GetDescription() {
	return m_description;
}

wxEvent* ProgressEvent::Clone() const {
	ProgressEvent *ev = new ProgressEvent(m_value, m_action, wcsdup(m_description));
	if (m_dir_server_map) {
		ev->m_dir_server_map = new std::map<char*, char*>();
		for (std::map<char*, char*>::iterator i = m_dir_server_map->begin();
				i != m_dir_server_map->end();
				i++)
			(*ev->m_dir_server_map)[strdup(i->first)] = strdup(i->second);
	}

	return ev;
}

std::map<char*, char*>* ProgressEvent::GetDirServerMap() {
	return m_dir_server_map;
}

ProgressEvent::~ProgressEvent() {
	free(m_description);

	if (m_dir_server_map) {
		std::vector<char*> dirs;
		for (std::map<char*, char*>::iterator i = m_dir_server_map->begin();
				i != m_dir_server_map->end();
				i++) {
			free(i->second);
			dirs.push_back(i->first);
		}

		for (std::vector<char*>::iterator i = dirs.begin();
				i != dirs.end();
				i++)
			free(*i);

		delete m_dir_server_map;
	}
}
		
Progress::Progress()
	:
	m_progress_handler(NULL),
	m_last_update(time(NULL)),
	m_last_term_check(time(NULL)),
	m_term_request(false) 
{}

void Progress::SetEventHandler(wxEvtHandler *progress_handler) {
	m_progress_handler = progress_handler;
}

void Progress::SetProgress(Action action, int progress, wxString extra_info, std::map<std::string, std::string>* dir_server_map) {

	Check();

	if (m_last_action == action 
			&& m_last_progress == progress
			&& m_last_extra_info == extra_info)
		return;

	m_last_action = action;
	m_last_progress = progress;
	m_last_extra_info = extra_info;

	ProgressEvent event(progress, action , extra_info, dir_server_map);
	wxPostEvent(m_progress_handler, event);
}

void Progress::Check() {
	time_t t = time(NULL);

	if (t <= m_last_term_check + 2)
		return;

	wxCriticalSectionLocker c(m_term_request_lock);

	if (m_term_request) {
		m_term_request = false;
		throw TerminationRequest();
	}

}

void Progress::SetTerminationRequest(bool value) {
	wxCriticalSectionLocker c(m_term_request_lock);
	m_term_request = value;
}


DEFINE_EVENT_TYPE(PROGRESS_EVENT)

#ifdef __WXMSW__
ProgressFrame::ProgressFrame(Progress *progress, BallonTaskBar *ballon) 
#else
ProgressFrame::ProgressFrame(Progress *progress) 
#endif
					: wxDialog(NULL, 
					wxID_ANY, 
					_("SSC - synchronization"),
					wxDefaultPosition,
					wxDefaultSize,
					wxFRAME_NO_TASKBAR | wxDEFAULT_DIALOG_STYLE | wxMINIMIZE_BOX
					) 
{

	m_status_text = new wxStaticText(this, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	m_progress_bar = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(250,25));
	m_cancel_button = new wxButton(this, ID_CANCEL_BUTTON, _("Cancel"));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_status_text, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(m_progress_bar, 0, wxALIGN_CENTER | wxALL, 5);
	sizer->Add(m_cancel_button, 0, wxALIGN_CENTER | wxALL, 5);

	m_progress = progress;

	m_client = new Client(*m_progress);
	m_client->Init();

	SetSize(300, 140);

	SetSizer(sizer);
			
	m_sync_in_progress = false;
	Layout();

#ifdef __WXMSW__
	m_ballon = ballon;
#endif
}

void ProgressFrame::StartSync(bool show, wxArrayString dirList, bool delete_option) {

	if (m_sync_in_progress) {
		if (show) {
			Iconize(false);
			Show(true);
			Raise();
		}
		return;
	}


	std::map<wxString, std::set<wxString> > sb = get_servers_bases_from_config();

	bool all_servers = false;
	std::set<wxString> servers;

	if (dirList.GetCount()) for (unsigned int i = 0; i < dirList.GetCount(); i++) {
		bool found = false;
		for (std::map<wxString, std::set<wxString> >::iterator j = sb.begin();
				j != sb.end();
				j++)
			if (j->second.find(dirList[i]) != j->second.end()) {
				servers.insert(j->first);
				found = true;
				break;
			}
		if (found == false) {
			all_servers = true;
			break;
		}
	} else
		all_servers = true;

	std::map<wxString, std::pair<wxString, wxString> > sc = get_servers_creditenials_config();

	if (all_servers) for (std::map<wxString, std::pair<wxString, wxString> >::iterator i = sc.begin();
				i != sc.end();
				i++)
			servers.insert(i->first);

	size_t count = servers.size();
	char **addresses = (char**) malloc(count * sizeof(char*));
	char **users = (char**) malloc(count * sizeof(char*));
	char **passwords = (char**) malloc(count * sizeof(char*));

	int n = 0;
	std::set<wxString>::iterator i = servers.begin();
	for (; i != servers.end(); i++, n++) {
		addresses[n] = strdup(SC::S2A(*i).c_str());
		users[n] = strdup(SC::S2A(sc[*i].first).c_str());
		passwords[n] = strdup(SC::S2A(sc[*i].second).c_str());
	}

	wxString _local_dir = dynamic_cast<szApp*>(wxTheApp)->GetSzarpDataDir();
	char *local_dir = strdup(SC::S2A(_local_dir).c_str());

	m_client->SetOptions(addresses,
			users,
			passwords,
			n,
			local_dir,
			23);

	m_progress->SetTerminationRequest(false);

	m_sync_in_progress = true;
	m_sync_start_time = wxDateTime::Now();
	m_client->BeginSync(delete_option, dirList);

	if (show) {
		Iconize(false);
		Show(true);
	}

}

bool ProgressFrame::ShowIfRunning() {
	if (m_sync_in_progress) {
		Iconize(false);
		Show(true);
		Raise();
		return true;
	}
	return false;
}

void ProgressFrame::OnIconize(wxIconizeEvent& event) {
	if (event.Iconized()) 
		Show(false); 
	else {
		Show(true);
		Raise();
	}
}

void ProgressFrame::SendReloadToSCC() {
	// send "reload menu" to SCC
	SSCClient* SSC_client = new SSCClient();
	SSC_client->SendReload();
	delete SSC_client;
}

void ProgressFrame::UpdateServerBaseMap(std::map<char*, char*>& dir_map) {
	std::map<wxString, std::set<wxString> > sb = get_servers_bases_from_config();

	for (std::map<char*, char*>::iterator i = dir_map.begin();
			i != dir_map.end();
			i++) {
		wxString base = SC::U2S((unsigned char*)i->first);
		wxString server = SC::A2S(i->second);

		for (std::map<wxString, std::set<wxString> >::iterator j = sb.begin();
				j != sb.end();
				j++)
			if (j->first != server)
				j->second.erase(base);

		sb[server].insert(base);
	}

	store_server_bases_to_config(sb);

}

void ProgressFrame::OnUpdate(ProgressEvent& event) {

	Progress::Action action = event.GetAction();

	wxString action_string;
	
	switch (action) {
		case Progress::CONNECTING:
			action_string = wxString(_("Connecting "));
			break;
		case Progress::AUTHORIZATION:
			action_string = wxString(_("Authorization "));
			break;
		case Progress::MESSAGE:
			wxMessageBox(event.GetDescription(),_("SSC"));
			break;
		case Progress::FETCHING_FILE_LIST:
			action_string = _("Fetching file list ");
			break;
		case Progress::SYNCING:
			action_string = _("Synchronization ");
			break;
		case Progress::FINISHED:
			m_sync_in_progress = false;
#ifdef __WXMSW__
			m_ballon->ShowBalloon(_("SSC"), wxString(_("Sync finished")),
					BallonTaskBar::ICON_INFO);
#endif
			if (IsShown()) {
				wxTimeSpan elapsed = wxDateTime::Now() - m_sync_start_time;
				long e = elapsed.GetSeconds().ToLong();

				int seconds = e % 60;
				e/=60;
				int minutes = e % 60;
				e/=24;
				int hours = e;

				wxString msg = wxString::Format(_("Sychronization complete (elapsed %.2d:%.2d:%.2d)"), 
						hours,
						minutes,
						seconds);	
						
				wxMessageBox(msg, _("SSC"), wxICON_INFORMATION);
			}
			m_status_text->SetLabel(_T(""));
			m_progress_bar->SetValue(0);
			UpdateServerBaseMap(*event.GetDirServerMap());
			Hide();
			SendReloadToSCC();
			return;
		case Progress::SYNC_TERMINATED:
			m_sync_in_progress = false;
			Hide();
			Enable();
			return;
		case Progress::FAILED:
			m_sync_in_progress = false;
#ifdef __WXMSW__
			m_ballon->ShowBalloon(_("SSC"), wxString(_("Sync failed ")) + event.GetDescription(),
					BallonTaskBar::ICON_ERROR);
#endif
#if 0
			if (IsShown()) 
				wxMessageBox(wxString(_("Synchronization failed ")) + event.GetDescription(), 
						_("Synchronization failed "), 
						wxICON_EXCLAMATION);
#endif
			Hide();
			Enable();
			return;
		/*case Progress::THREAD_EXIT:
			wxExit();
			return;*/
	}
	m_status_text->SetLabel(action_string + _T(" ") + event.GetDescription());
	m_progress_bar->SetValue(event.GetValue());

	Layout();
}	


void ProgressFrame::OnCancel(wxCommandEvent& event) {
	TerminateSynchronization();
}

void ProgressFrame::TerminateSynchronization() {
	int result = wxMessageBox(_("Do you really want to terminate synchronization?"), 
			_T(""), 
			wxYES|wxNO|wxICON_QUESTION, 
			this); 

	if (result == wxYES && m_sync_in_progress) {
		m_progress->SetTerminationRequest(true); 
		Disable();
	}	
}

void ProgressFrame::OnClose(wxCloseEvent &event) {
	if (event.CanVeto()) {
		event.Veto();
		TerminateSynchronization();
	}
}


BEGIN_EVENT_TABLE(ProgressFrame, wxDialog)
	EVT_PROGRESS(ID_PROGRESS_EVNT_ORIG, ProgressFrame::OnUpdate)	
	EVT_ICONIZE(ProgressFrame::OnIconize)
	EVT_CLOSE(ProgressFrame::OnClose)
	EVT_BUTTON(ID_CANCEL_BUTTON, ProgressFrame::OnCancel)
END_EVENT_TABLE()

SSCConfigFrame::SSCConfigFrame() : wxDialog(NULL, wxID_ANY, _("SZARP File synchronizer - configuration"), 
	wxDefaultPosition, wxSize(300,500), wxTAB_TRAVERSAL | wxFRAME_NO_TASKBAR | wxRESIZE_BORDER | wxCAPTION)
{

	wxBoxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);

	main_sizer->Add(new wxStaticText(this, wxID_ANY, _("Configuration")), 0, wxALIGN_CENTER | wxALL, 20);
	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	main_sizer->Add(new wxStaticText(this, 
				wxID_ANY, 
				_("Servers:"), 
				wxDefaultPosition, 
				wxDefaultSize, 
				wxALIGN_CENTER),
			0, 
			wxALIGN_CENTER | wxALL,
			10);
	main_sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxALL);

	wxBoxSizer *servers_data_sizer = new wxBoxSizer(wxHORIZONTAL);

	m_servers_box = new wxListBox(this, ID_CFG_FRAME_SERVERS_BOX);
	servers_data_sizer->Add(m_servers_box, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* data_sizer = new wxBoxSizer(wxVERTICAL);

	m_server_text = new wxStaticText(this, wxID_ANY, _T("!"));
	data_sizer->Add(m_server_text, 0, wxALIGN_CENTER | wxALL, 5);

	wxBoxSizer *user_sizer = new wxBoxSizer(wxHORIZONTAL);
	user_sizer->Add(new wxStaticText(this, 
				wxID_ANY, 
				_("User name:"), 
				wxDefaultPosition, 
				wxDefaultSize, 
				wxALIGN_CENTER),
			1, 
			wxALIGN_CENTER | wxALL);
	m_user_ctrl = new wxTextCtrl(this, ID_CFG_FRAME_USER_CTRL, _T(""), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	user_sizer->Add(m_user_ctrl, 1, wxALIGN_CENTER | wxALL, 5);
	data_sizer->Add(user_sizer, 0, wxEXPAND);

	wxBoxSizer *password_sizer = new wxBoxSizer(wxHORIZONTAL);
	password_sizer->Add(new wxStaticText(this, 
				wxID_ANY, 
				_("Password:"), 
				wxDefaultPosition, 
				wxDefaultSize, 
				wxALIGN_CENTER),
			1, 
			wxALIGN_CENTER | wxALL);
	password_sizer->Add(new wxButton(this, ID_CFG_FRAME_CHANGE_PASSWORD_BUTTON, _("Change")), 1, wxALIGN_CENTER | wxALL, 5);

	data_sizer->Add(password_sizer, 0, wxEXPAND);
	servers_data_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	servers_data_sizer->Add(data_sizer, 1, wxALIGN_CENTER_VERTICAL | wxALL, 10);

	main_sizer->Add(servers_data_sizer, 1, wxEXPAND);

	main_sizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND);

	wxSizer *servers_buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
	servers_buttons_sizer->Add(new wxButton(this, ID_CFG_FRAME_SERVER_ADD_BUTTON, _("Add server")), 1, wxALIGN_CENTER_VERTICAL | wxALL, 10);
	servers_buttons_sizer->Add(new wxButton(this, ID_CFG_FRAME_SERVER_CHANGE_ADDRESS_BUTTON, _("Change address")), 1, wxALIGN_CENTER_VERTICAL | wxALL, 10);
	servers_buttons_sizer->Add(new wxButton(this, ID_CFG_FRAME_SERVER_REMOVE_BUTTON, _("Remove server")), 1, wxALIGN_CENTER_VERTICAL | wxALL, 10);

	main_sizer->Add(servers_buttons_sizer, 0, wxEXPAND);

	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);
	main_sizer->Add(new wxStaticText(this, wxID_ANY, _("Synchronization setup:")), 0, wxALIGN_CENTER | wxALL, 10);
	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	m_autosync_box = new wxCheckBox(this,
					ID_CFG_FRAME_AUTOSYNC_CTRL,
					_("Automatic synchronization"),
					wxDefaultPosition,
					wxDefaultSize,
					wxALIGN_LEFT);
	main_sizer->Add(m_autosync_box, 0, wxALIGN_CENTER | wxALL, 10);

	wxBoxSizer* autosync_sizer = new wxBoxSizer(wxHORIZONTAL);

	autosync_sizer->Add(new wxStaticText(this, wxID_ANY, _("Data actualization period(minutes):"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT), 1, wxALIGN_CENTER | wxALL, 10);

	const wxChar* vals[] = { _T("10") , _T("30"), _T("60")};
	wxArrayString sync_intervals(3, vals);

	m_syncinterval_combo = new wxComboBox(this,
					ID_CFG_FRAME_SYNCINTERVAL_CTRL,
					sync_intervals[0],
					wxDefaultPosition,
					wxDefaultSize,
					sync_intervals,
					wxCB_READONLY
				);

	autosync_sizer->Add(m_syncinterval_combo, 0, wxALIGN_CENTER | wxALL, 15);

	main_sizer->Add(autosync_sizer, 0, wxEXPAND);
#ifdef __WXMSW__
	m_show_notification_box = new wxCheckBox(this,
					wxID_ANY,
					_("Show notification upon sync completion"),
					wxDefaultPosition,
					wxDefaultSize,
					wxALIGN_LEFT);
	main_sizer->Add(m_show_notification_box, 0, wxALIGN_CENTER | wxALL, 10);
#endif

	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	wxButton* button = new wxButton(this, ID_CFG_FRAME_OK_BUTTON, _("OK"), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	button_sizer->Add(button, 0, wxALL, 10);

	button = new wxButton(this, ID_CFG_FRAME_CANCEL_BUTTON, _("Cancel"), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	button_sizer->Add(button, 0, wxALL, 10);

	main_sizer->Add(button_sizer, 0, wxALL | wxALIGN_CENTER);

	SetSizer(main_sizer);

	main_sizer->SetSizeHints(this);

}

void SSCConfigFrame::LoadConfiguration() {

	wxConfigBase* config = wxConfigBase::Get(true); 

	m_servers_box->Clear();

	m_servers_creditenials_map = get_servers_creditenials_config();

	for (std::map<wxString, std::pair<wxString, wxString> >::iterator i = m_servers_creditenials_map.begin();
			i != m_servers_creditenials_map.end();
			i++)
		m_servers_box->Append(i->first);

	bool auto_sync = (config->Read(_T("AUTO_SYNC"), 0L) == 1);

	m_autosync_box->SetValue(auto_sync);

	long freq = config->Read(_T("SYNC_FREQ"), 10L);

	m_syncinterval_combo->SetValue(wxString::Format(_T("%ld"), freq)); 

#ifdef __WXMSW__
	m_show_notification_box->SetValue(config->Read(_T("SHOW_NOTIFICATION"), 1L) == 1);
#endif

	m_servers_box->SetSelection(0);

	ServerSelected(false);

}

void SSCConfigFrame::StoreConfiguration() {

	store_servers_creditenials_config(m_servers_creditenials_map);

	wxConfigBase* config = wxConfigBase::Get(true); 

	config->Write(_T("AUTO_SYNC"), m_autosync_box->GetValue());

	long  freq;
	if (!m_syncinterval_combo->GetValue().ToLong(&freq, 10))
		freq = 10;
	config->Write(_T("SYNC_FREQ"), freq);

#ifdef __WXMSW__
	config->Write(_T("SHOW_NOTIFICATION"), m_show_notification_box->GetValue());
#endif

	config->Flush();

}

void SSCConfigFrame::OnAutoSyncCheckBox(wxCommandEvent& WXUNUSED(event)) {
	if (m_autosync_box->IsChecked())
		m_syncinterval_combo->Enable();
	else
		m_syncinterval_combo->Disable();
}

void SSCConfigFrame::OnPasswordChange(wxCommandEvent& event) {
	wxString password = wxGetPasswordFromUser(
			wxString::Format(_("Enter password for server %s:"), m_current_server.c_str()), 
			_("New password"),
			_T(""),
			this);

	if (password.IsEmpty())
		return;

	wxString hashed_password = sz_md5(password);
	m_servers_creditenials_map[m_current_server].second = hashed_password;
				
	return;
}

void SSCConfigFrame::OnOKButton(wxCommandEvent& event) {
	m_servers_creditenials_map[m_current_server].first = m_user_ctrl->GetValue();

	StoreConfiguration();
	Hide();
}

void SSCConfigFrame::OnCancelButton(wxCommandEvent& event) {
	m_servers_creditenials_map.clear();
	Hide();
}

void SSCConfigFrame::OnAddServerButton(wxCommandEvent& event) {

	wxString address = wxGetTextFromUser(
		_("Enter server address:"),
		_("New server"),
		_T(""),
		this);

	if (address.IsEmpty())
		return;

	if (m_servers_creditenials_map.find(address) != m_servers_creditenials_map.end()) {
		wxMessageBox(_("Server with that address already exists."), _("Server already exists"), wxICON_HAND);
		return;
	}


	wxString username = wxGetTextFromUser(
		wxString::Format(_("Enter user name for server: %s"), address.c_str()),
		_("Username"),
		_T(""),
		this);

	if (username.IsEmpty())
		return;

	wxString password = wxGetPasswordFromUser(
		wxString::Format(_("Enter password for server: %s"), address.c_str()),
		_("Password"),
		_T(""),
		this);

	if (password.IsEmpty())
		return;


	m_servers_creditenials_map[address] = std::make_pair(username, sz_md5(password));
	m_servers_box->Append(address);

}

void SSCConfigFrame::OnRemoveServerButton(wxCommandEvent& event) {
	if (m_servers_box->GetCount() == 1) {
		wxMessageBox(_("At least one server must be present."), _("Error"), wxICON_HAND);
		return;
	}

	if (wxYES == wxMessageBox(
				wxString::Format(_("Do you wish to remove server: %s ?"), m_current_server.c_str()),
				_("Server removal"),
				wxYES_NO | wxICON_QUESTION)) {

		assert(m_servers_creditenials_map.erase(m_current_server) == 1);

		m_servers_box->Delete(m_servers_box->GetSelection());
		m_servers_box->SetSelection(0);
		ServerSelected(false);

	}

}

void SSCConfigFrame::OnChangeServerNameButton(wxCommandEvent& event) {
	wxString address = wxGetTextFromUser(
		wxString::Format(_("Enter new address of server: %s"), m_current_server.c_str()),
		_("Server address"),
		m_current_server,
		this);

	if (address.IsEmpty())
		return;

	if (m_current_server == address)
		return;

	if (m_servers_creditenials_map.find(address) != m_servers_creditenials_map.end()) {
		wxMessageBox(_("Server with that address already exists."), _("Server already exists"), wxICON_HAND);
		return;
	}

	m_servers_creditenials_map[address] = m_servers_creditenials_map[m_current_server];

	assert(m_servers_creditenials_map.erase(m_current_server) == 1);

	m_servers_box->SetString(m_servers_box->GetSelection(), address);

	ServerSelected(false);

}

void SSCConfigFrame::OnServerSelected(wxCommandEvent& event) {
	//if this event is triggered and current server is not present
	//we should mess with server's list 
	if (m_servers_creditenials_map.find(m_current_server) == m_servers_creditenials_map.end())
		return;
	ServerSelected(true);
}

void SSCConfigFrame::ServerSelected(bool grab_username) {
	if (grab_username)
		m_servers_creditenials_map[m_current_server].first = m_user_ctrl->GetValue();

	m_current_server = m_servers_box->GetStringSelection();

	m_user_ctrl->SetValue(m_servers_creditenials_map[m_current_server].first);

	m_server_text->SetLabel(wxString(_("Server ")) + m_current_server + _T(":"));

}

BEGIN_EVENT_TABLE(SSCConfigFrame, wxDialog)
	EVT_BUTTON(ID_CFG_FRAME_OK_BUTTON, SSCConfigFrame::OnOKButton)
	EVT_BUTTON(ID_CFG_FRAME_CANCEL_BUTTON, SSCConfigFrame::OnCancelButton)
	EVT_BUTTON(ID_CFG_FRAME_AUTOSYNC_CTRL, SSCConfigFrame::OnAutoSyncCheckBox)
	EVT_BUTTON(ID_CFG_FRAME_CHANGE_PASSWORD_BUTTON, SSCConfigFrame::OnPasswordChange)
	EVT_BUTTON(ID_CFG_FRAME_SERVER_CHANGE_ADDRESS_BUTTON, SSCConfigFrame::OnChangeServerNameButton)
	EVT_BUTTON(ID_CFG_FRAME_SERVER_ADD_BUTTON, SSCConfigFrame::OnAddServerButton)
	EVT_BUTTON(ID_CFG_FRAME_SERVER_REMOVE_BUTTON, SSCConfigFrame::OnRemoveServerButton)
	EVT_LISTBOX(ID_CFG_FRAME_SERVERS_BOX, SSCConfigFrame::OnServerSelected)
END_EVENT_TABLE()

SSCSelectionFrame::SSCSelectionFrame() : wxDialog(NULL, wxID_ANY, _("SZARP Select databases"), 
	wxDefaultPosition, wxSize(300,500), wxTAB_TRAVERSAL | wxFRAME_NO_TASKBAR | wxRESIZE_BORDER | wxCAPTION)
{
	wxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);

	main_sizer->Add(new wxStaticText(this, wxID_ANY, _("Select databases to synchronize")), 0, wxALIGN_CENTER | wxALL, 20);
	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	LoadDatabases();
	LoadConfiguration();

	wxArrayString bases;
	for (unsigned int i = 0; i < m_databases.GetCount(); i++) {
		wxString base;
		base = m_config_titles[m_databases[i]];

		std::map<wxString, wxString>::iterator j = m_database_server_map.find(m_databases[i]);
		if (j != m_database_server_map.end())
			base += _T(" (") + j->second + _T(")");

		bases.Add(base);
	}

	m_databases_list_box = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, bases);

	for (unsigned int x = 0; x < m_selected_databases.GetCount(); x++) {
		for (unsigned int i = 0; i < m_databases.GetCount(); i++) {
			if (!m_selected_databases[x].Cmp(m_databases[i]))
			{
				m_databases_list_box->Check(i);
				break;
			}
		}		
	}

	main_sizer->Add(m_databases_list_box, 1, wxEXPAND);

	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	m_delete_box = new wxCheckBox(this, wxID_ANY, _("Delete files not present on the server"));
	main_sizer->Add(m_delete_box, 0, wxEXPAND);

	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);

	wxButton* button = new wxButton(this, ID_SELECTION_FRAME_OK_BUTTON, _("OK"), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	button_sizer->Add(button, 0, wxALL, 10);

	button = new wxButton(this, ID_SELECTION_FRAME_CANCEL_BUTTON, _("Cancel"), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	button_sizer->Add(button, 0, wxALL, 10);

	main_sizer->Add(button_sizer, 0, wxALL | wxALIGN_CENTER);

	SetSizer(main_sizer);

}

void SSCSelectionFrame::StoreConfiguration() {

	wxConfigBase* config = wxConfigBase::Get(true);

	wxString storestr;
	bool first = true;

	m_selected_databases.Clear();

	for (unsigned int i = 0; i < m_databases.GetCount(); i++)
	{
		if (m_databases_list_box->IsChecked(i))
			m_selected_databases.Add(m_databases[i]);
	}

	for (unsigned int i = 0; i < m_selected_databases.GetCount(); i++)
	{
		if (!first)
			storestr.Append(_T(","));
		first = false;
		storestr += (m_selected_databases[i]);
	}
	config->Write(_T("SELECTED_DATABASES"), storestr);
	config->Flush();
}

void SSCSelectionFrame::LoadConfiguration() {
	wxString tmp;
	wxConfigBase* config = wxConfigBase::Get(true);
	if (config->Read(_T("SELECTED_DATABASES"), &tmp))
	{
		wxStringTokenizer tkz(tmp, _T(","), wxTOKEN_STRTOK);
		while (tkz.HasMoreTokens())
		{
			wxString token = tkz.GetNextToken();
			token.Trim();
			if (!token.IsEmpty())
				m_selected_databases.Add(token);
		}
	}

	std::map<wxString, std::set<wxString> > servers_bases = get_servers_bases_from_config();
	for (std::map<wxString, std::set<wxString> >::iterator i = servers_bases.begin();
			i != servers_bases.end();
			i++)
		for (std::set<wxString>::iterator j = i->second.begin();
				j != i->second.end();
				j++) 
			m_database_server_map[*j] = i->first;
}

void SSCSelectionFrame::LoadDatabases() {
	m_config_titles = GetConfigTitles(dynamic_cast<szApp*>(wxTheApp)->GetSzarpDataDir());
	for (ConfigNameHash::iterator i = m_config_titles.begin(); i != m_config_titles.end(); i++)
		m_databases.Add(i->first);

	m_databases.Sort();
}

wxArrayString SSCSelectionFrame::GetDirs() {
	return wxArrayString(m_selected_databases);
}

void SSCSelectionFrame::OnOKButton(wxCommandEvent& event) {
	StoreConfiguration();
	EndModal(wxID_OK);
}

void SSCSelectionFrame::OnCancelButton(wxCommandEvent& event) {
	EndModal(wxID_CANCEL);
}

BEGIN_EVENT_TABLE(SSCSelectionFrame, wxDialog)
	EVT_BUTTON(ID_SELECTION_FRAME_OK_BUTTON, SSCSelectionFrame::OnOKButton)
	EVT_BUTTON(ID_SELECTION_FRAME_CANCEL_BUTTON, SSCSelectionFrame::OnCancelButton)
END_EVENT_TABLE()

SSCTaskBarItem::SSCTaskBarItem(wxString szarp_dir) {

	m_log = new wxLogWindow(NULL, _("Log dialog"), false);;
	
	m_szarp_dir = szarp_dir;

	m_help = new szHelpController;

	wxIcon icon(ssc16_xpm);
	SetIcon(icon);

	m_cfg_frame = new SSCConfigFrame();

	m_progress = new Progress();

#ifdef __WXMSW__
	m_progress_frame = new ProgressFrame(m_progress, this);
#else
	m_progress_frame = new ProgressFrame(m_progress);
#endif

	m_progress->SetEventHandler(m_progress_frame);

	m_timer = new wxTimer(this, ID_TIMER);

	m_timer->Start(1000);
}

wxMenu* SSCTaskBarItem::CreateMenu() {
	wxMenu *menu = new wxMenu();

	menu->Append(new wxMenuItem(menu, ID_START_SYNC_MENU, _("Begin synchronization"), wxEmptyString, wxITEM_NORMAL));
	menu->Append(new wxMenuItem(menu, ID_SYNC_SELECTED_MENU, _("Synchronize selected databeses"), wxEmptyString, wxITEM_NORMAL));
	menu->AppendSeparator();
	menu->Append(new wxMenuItem(menu, ID_CONFIGURATION_MENU, _("Configuration"), wxEmptyString, wxITEM_NORMAL));
	menu->Append(new wxMenuItem(menu, ID_HELP_MENU, _("Help"), wxEmptyString, wxITEM_NORMAL));
	menu->Append(new wxMenuItem(menu, ID_ABOUT_MENU, _("About"), wxEmptyString, wxITEM_NORMAL));
	menu->Append(new wxMenuItem(menu, ID_LOG_MENU, _("Log"), wxEmptyString, wxITEM_NORMAL));

	menu->AppendSeparator();
	menu->Append(new wxMenuItem(menu, ID_CLOSE_APP_MENU, _("Close application"), wxEmptyString, wxITEM_NORMAL));

	return menu;
}

void SSCTaskBarItem::OnCloseMenuEvent(wxCommandEvent &event) {
	m_cfg_frame->Close(true);
#ifdef MINGW32
	RemoveIcon();
#endif 
	wxExit();
}

void SSCTaskBarItem::OnConfiguration(wxCommandEvent &event) {
	m_cfg_frame->LoadConfiguration();
	m_cfg_frame->Refresh();
	m_cfg_frame->Raise();
	m_cfg_frame->Show(true);
}

void SSCTaskBarItem::OnHelp(wxCommandEvent &event) {
	m_help->AddBook(m_szarp_dir + _T("resources/documentation/new/ssc/html/ssc.hhp"));
	m_help->DisplayContents();
}

void SSCTaskBarItem::OnAbout(wxCommandEvent &event) {
	wxGetApp().ShowAbout();
}

void SSCTaskBarItem::OnLog(wxCommandEvent &event) {
	m_log->Show();
}

void SSCTaskBarItem::OnBeginSync(wxCommandEvent &event) {
	m_last_sync_time = wxDateTime::Now();
	m_progress_frame->StartSync(true);
}

void SSCTaskBarItem::OnSyncSelected(wxCommandEvent &event) {
	/*if running, only rise window*/
	if(!m_progress_frame->ShowIfRunning()) {
		SSCSelectionFrame *m_sel_frame = new SSCSelectionFrame();
		m_sel_frame->Centre();
		if(m_sel_frame->ShowModal() == wxID_OK && m_sel_frame->GetDirs().GetCount() > 0) {
			m_progress_frame->StartSync(true, m_sel_frame->GetDirs(), m_sel_frame->GetDeleteOption());
		}
		m_sel_frame->Destroy();
	}
}

void SSCTaskBarItem::OnTimer(wxTimerEvent& WXUNUSED(event)) {

	wxConfigBase* config = wxConfigBase::Get(true);

	if (config->Read(_T("AUTO_SYNC"), 0L) == 0)
		return;

	long sync_freq = config->Read(_T("SYNC_FREQUENCY"), 10);

	wxDateTime now = wxDateTime::Now();

	if (m_last_sync_time == wxInvalidDateTime 
			|| (now - m_last_sync_time).GetSeconds().ToLong() >= sync_freq * 60) {
		m_last_sync_time = now;
		m_progress_frame->StartSync(false);
	}

}

void SSCTaskBarItem::OnMouseDown(wxTaskBarIconEvent &event) {
	wxMenu *menu = CreateMenu();
	PopupMenu(menu);
	delete menu;
}


#ifdef __WXMSW__
BEGIN_EVENT_TABLE(SSCTaskBarItem, BallonTaskBar) 
#else
BEGIN_EVENT_TABLE(SSCTaskBarItem, szTaskBarItem) 
#endif
	EVT_MENU(ID_CLOSE_APP_MENU, SSCTaskBarItem::OnCloseMenuEvent)
	EVT_MENU(ID_CONFIGURATION_MENU, SSCTaskBarItem::OnConfiguration)
	EVT_MENU(ID_START_SYNC_MENU, SSCTaskBarItem::OnBeginSync)
	EVT_MENU(ID_SYNC_SELECTED_MENU, SSCTaskBarItem::OnSyncSelected)
	EVT_MENU(ID_HELP_MENU, SSCTaskBarItem::OnHelp)
	EVT_MENU(ID_ABOUT_MENU, SSCTaskBarItem::OnAbout)
	EVT_MENU(ID_LOG_MENU, SSCTaskBarItem::OnLog)
	EVT_TIMER(ID_TIMER, SSCTaskBarItem::OnTimer)
	EVT_TASKBAR_LEFT_DOWN(SSCTaskBarItem::OnMouseDown)
	EVT_TASKBAR_RIGHT_DOWN(SSCTaskBarItem::OnMouseDown)
END_EVENT_TABLE()

SSCWizardFrameCancelImpl::SSCWizardFrameCancelImpl(wxWizard *wizard) : wxWizardPageSimple(wizard)
{}

SSCWizardFrameCancelImpl::~SSCWizardFrameCancelImpl() {}

void SSCWizardFrameCancelImpl::OnWizardCancel(wxWizardEvent &event) {
	if (wxMessageBox(_("Do you really want to cancel configuration and terminate the application"), _("Question"),
				wxICON_QUESTION | wxYES_NO, this) != wxYES)
		event.Veto();
}

BEGIN_EVENT_TABLE(SSCWizardFrameCancelImpl, wxWizardPageSimple)
	EVT_WIZARD_CANCEL(wxID_ANY, SSCWizardFrameCancelImpl::OnWizardCancel)
END_EVENT_TABLE();
		
SSCFirstWizardFrame::SSCFirstWizardFrame(wxWizard *parent) : SSCWizardFrameCancelImpl(parent) {
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* text1 = new wxStaticText(this, wxID_ANY,
			_(
			"\nWelcome to SZARP Synchronizer Client\n\n\n"
			),
			wxDefaultPosition,
			wxDefaultSize,
			wxALIGN_CENTER);

	sizer->Add(text1, 0, wxALIGN_CENTER);

	sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	wxStaticText* text2 = new wxStaticText(this, wxID_ANY,
			_("\n\n\n\n\n\n"
			"This utility guides you through process of initial configuration.\n\n"
			"of SSC application. You can always leave the utlility by clicking \n\n"
			"Cancel button.\n\nClick Next button to proceed"
			),
			wxDefaultPosition,
			wxDefaultSize,
			wxALIGN_CENTER);
	
	sizer->Add(text2, 0, wxALIGN_CENTER | wxALL);
	SetSizer(sizer);
	sizer->Fit(this);
}

SSCSecondWizardFrame::SSCSecondWizardFrame(wxWizard *parent) : SSCWizardFrameCancelImpl(parent) {
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* text1 = new wxStaticText(this, wxID_ANY,
			_(
			"\n\nBefore you can start using the application you need to\n"
			"enter your username and password so that program would be able\n"
			"to fetch data from server\n"
			),
			wxDefaultPosition,
			wxDefaultSize,
			wxALIGN_CENTER);
	main_sizer->Add(text1, 1, wxALIGN_CENTER | wxALL);

	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	wxBoxSizer *user_sizer = new wxBoxSizer(wxHORIZONTAL);
	user_sizer->Add(new wxStaticText(this, 
				wxID_ANY, 
				_("User name:"), 
				wxDefaultPosition, 
				wxDefaultSize, 
				wxALIGN_RIGHT),
			1, 
			wxALIGN_CENTER | wxRIGHT,
			10);
	m_user_ctrl = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	user_sizer->Add(m_user_ctrl, 1, wxALIGN_CENTER | wxRIGHT, 50);
	main_sizer->Add(user_sizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 20);

	wxBoxSizer *password_sizer = new wxBoxSizer(wxHORIZONTAL);
	password_sizer->Add(new wxStaticText(this, 
				wxID_ANY, 
				_("Password:"), 
				wxDefaultPosition, 
				wxDefaultSize, 
				wxALIGN_RIGHT),
			1, 
			wxALIGN_CENTER | wxRIGHT,
			10);
	m_password_ctrl = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, 
			wxTE_PASSWORD | wxTAB_TRAVERSAL);
	password_sizer->Add(m_password_ctrl, 1, wxALIGN_CENTER | wxRIGHT, 50);
	main_sizer->Add(password_sizer, 0, wxEXPAND | wxBOTTOM | wxTOP, 20);

	wxBoxSizer *server_sizer = new wxBoxSizer(wxHORIZONTAL);
	server_sizer->Add(new wxStaticText(this,
				wxID_ANY,
				_("Server address:"),
				wxDefaultPosition,
				wxDefaultSize,
				wxALIGN_RIGHT),
			1,
			wxALIGN_CENTER | wxRIGHT,
			10);
	m_server_ctrl = new wxTextCtrl(this, wxID_ANY, _T(""),  wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	server_sizer->Add(m_server_ctrl, 1, wxALIGN_CENTER | wxRIGHT, 50);
	main_sizer->Add(server_sizer, 0, wxEXPAND | wxBOTTOM | wxTOP, 20);

	main_sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize), 0, wxEXPAND);

	wxStaticText* text2 = new wxStaticText(this, wxID_ANY,
			_(
			"If you don't know your username or password please server administrator.\n"
			"You may click Cancel button to stop configuration process and close the\n"
			"application.\n"
			),
			wxDefaultPosition,
			wxDefaultSize,
			wxALIGN_CENTER);
	main_sizer->Add(text2, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

	SetSizer(main_sizer);
}

BEGIN_EVENT_TABLE(SSCSecondWizardFrame, SSCWizardFrameCancelImpl)
	EVT_WIZARD_PAGE_CHANGING(wxID_ANY, SSCSecondWizardFrame::OnWizardPageChanging)
END_EVENT_TABLE()

void SSCSecondWizardFrame::OnWizardPageChanging(wxWizardEvent& event) {
	if (event.GetDirection()) { 
		if (m_password_ctrl->GetValue().Length() == 0 || m_user_ctrl->GetValue().Length() == 0
				|| m_server_ctrl->GetValue().length() == 0) {

			wxMessageBox(_("You must fill in username, password and server address fields"), _("SSC"), wxICON_HAND);
			event.Veto();
		} else {
			wxString password = sz_md5(m_password_ctrl->GetValue());

			wxConfigBase* config = wxConfigBase::Get(true);
			config->Write(_T("USERNAME"), m_user_ctrl->GetValue());
			config->Write(_T("PASSWORD"), password);
			config->Write(_T("SERVER_ADDRESS"), m_server_ctrl->GetValue());
			config->Flush();
		}
	}
}
	

SSCThirdWizardFrame::SSCThirdWizardFrame(wxWizard *parent) : SSCWizardFrameCancelImpl(parent) {
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	wxStaticText* text = new wxStaticText(this, wxID_ANY,
			_("The configuration process is now over.\n\n\n\n"
			"You can now start using the application. Choose \"Synchronize\"\n"
			"item from application menu (which is displayed after clicking on application\n"
			"icon located on your TaskBar).\n"
			"You can further customize you application by choosing \"Configuration\"\n"
			"item from menu (it may also by useful if you misspelled your\n"
			"username or password).\n\n\n"
			"Click Finish button to start application.\n"),
			wxDefaultPosition,
			wxDefaultSize,
			wxALIGN_CENTER);

	sizer->Add(text, 1, wxEXPAND);
	
	SetSizer(sizer);
	sizer->Fit(this);
}

bool SSCApp::ParseCommandLine() {
	wxCmdLineParser parser;
	parser.AddOption(_("d"), _("debug"), _("debug level (0-10)"), wxCMD_LINE_VAL_NUMBER);
	parser.AddSwitch(_("h"), _("help"), _("print usage info"));

	parser.SetCmdLine(argc, argv);

	if (parser.Parse(false) || parser.Found(_("h"))) {
		parser.Usage();
		return false;
	}

	long loglevel;
	if (parser.Found(_("d"), &loglevel)) {
		if (loglevel < 0 || loglevel > 10) {
			parser.Usage();
			return false;
		}
		sz_loginit(loglevel, NULL);
	}
	return true;
}

void SSCApp::InitLocale() {
#ifndef __WXMSW__
	locale.Init();
#else //__WXMSW__
	locale.Init(wxLANGUAGE_POLISH);
#endif //__WXMSW__

	locale.AddCatalogLookupPathPrefix(GetSzarpDir() + _T("/resources/locales"));

	locale.AddCatalog(_T("wx"));
	locale.AddCatalog(_T("common"));
	locale.AddCatalog(_T("ssc"));

}

void SSCApp::ConvertConfigToMultipleServers() {
	wxConfigBase* config = wxConfigBase::Get(true); 

	if (config->Read(_T("SERVER_ADDRESS"), _T("")) == _T(""))
		return;

	wxString server = config->Read(_T("SERVER_ADDRESS"), _T(""));
	wxString password = config->Read(_T("PASSWORD"), _T(""));
	wxString username = config->Read(_T("USERNAME"), _T(""));

	config->Write(_T("SERVERS"), server);
	config->Write(server + _T("/PASSWORD"), password);
	config->Write(server + _T("/USERNAME"), username);

	ConfigNameHash ct = GetConfigTitles(dynamic_cast<szApp*>(wxTheApp)->GetSzarpDataDir());

	wxString bases;
	bool first = true;
	for (ConfigNameHash::iterator i = ct.begin(); i != ct.end(); i++) {
		if (!first)
			bases += _T(";");
		bases += i->first;
		first = false;
	}

	config->Write(server + _T("/BASES"), bases);

	config->DeleteEntry(_T("SERVER_ADDRESS"));
	config->DeleteEntry(_T("PASSWORD"));
	config->DeleteEntry(_T("USERNAME"));

	config->Flush();

}

bool SSCApp::OnInit() {
	szApp::OnInit();
	this->SetProgName(_("Sync Client"));

	wxLog *logger = new wxLogStderr();
	wxLog::SetActiveTarget(logger);

	if (GetSzarpDir() == wxEmptyString) {
		wxMessageBox(_("ERROR!"), _("Cannot determine main SZARP directory.\n"
			"Application is not able to function"),
			wxICON_HAND);
		return false;
	}

	scc_ipc_messages::init_ipc();

	wxInitAllImageHandlers();

	InitLocale();

	if (!ParseCommandLine())
		return false;

	m_single_instance_check = new wxSingleInstanceChecker(_T(".ssc_lock"));

	if (m_single_instance_check->IsAnotherRunning()) {
		return false;
	}
	wxConfigBase* config = wxConfigBase::Get(true);
	if (config->Read(_T("FIRST_TIME_RUN"), 1L) == 1) { 
		if (!RunWizard()) {
			delete m_single_instance_check;
			return false;
		} else {
			config->Write(_T("FIRST_TIME_RUN"), 0L);
			config->Flush();
		}
	}
	ConvertConfigToMultipleServers();

#ifndef MINGW32
	libpar_init();
#else
        libpar_init_with_filename(SC::S2A((GetSzarpDir() + _T("resources/szarp.cfg"))).c_str(), 0);
#endif

	libpar_done();
	m_taskbar = new SSCTaskBarItem(GetSzarpDir());

#ifndef MINGW32
	signal(SIGPIPE, SIG_IGN);
#endif

	return true;

}

bool SSCApp::RunWizard() {
	wxWizard* wizard = new wxWizard(NULL, wxID_ANY,
			_("SSC - intial configuration"),
			wxBitmap(ssc16_xpm),
			wxDefaultPosition,
			wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

	wxWizardPageSimple* page1 = new SSCFirstWizardFrame(wizard);
	wxWizardPageSimple* page2 = new SSCSecondWizardFrame(wizard);
	wxWizardPageSimple* page3 = new SSCThirdWizardFrame(wizard);
	wxWizardPageSimple::Chain(page1, page2);
	wxWizardPageSimple::Chain(page2, page3);

	wizard->GetPageAreaSizer()->Add(page1);

	bool result = wizard->RunWizard(page1);
	wizard->Destroy();
	return result;

}

int SSCApp::OnExit() {
	delete m_single_instance_check;

	return 1;
}


wxChar* SSCConnection::SendMsg(const wxChar* msg)
{
	return Request(msg);
}

int SSCClient::SendReload()
{
	int result=0;
	connection = Connect(); // connect
	
	// send if connected
	if(connection != NULL)
		connection->SendMsg(scc_ipc_messages::reload_menu_msg);
	else
		result=1; // not connected

	delete connection;
	return result;
}

SSCConnection* SSCClient::Connect()
{
	return (SSCConnection*) this->MakeConnection(scc_ipc_messages::hostname, scc_ipc_messages::scc_service, scc_ipc_messages::ipc_topic);
}


