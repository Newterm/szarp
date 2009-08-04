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
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <time.h>
#include "libpar.h"
#include "ssutil.h"
#include "ssserver.h"
#include "conversion.h"
#include "daemon.h"

#include "latin2.h"

SFileSyncer::Request::Request(SFileSyncer::Request::Type type, uint32_t num, rs_signature_t *sig)
	: m_type(type), m_num(num), m_signature(sig) {
}

SFileSyncer::RequestReceiver::RequestReceiver(std::deque<Request>& request_queue, PacketExchanger* exchanger) 
	: m_request_queue(request_queue), m_exchanger(exchanger), m_state(IDLE) {
	m_exchanger->SetReader(this);
}

bool SFileSyncer::RequestReceiver::WantPacket() {
	return m_request_queue.size() < 100;
}

void SFileSyncer::RequestReceiver::ReadPacket(Packet *p) {

	if (m_state == IDLE) {
		if (p->m_type == Packet::EOF_PACKET) {
			sz_log(6, "No more requests for this base");

			m_exchanger->ReadBackPacket(p);
			m_exchanger->SetReader(NULL);

			m_request_queue.push_back(Request(Request::EOR, 0));

			return;
		}

		uint16_t msg_type = ntohs(*(uint16_t*)p->m_data);
		sz_log(8, "Syncer, got message type: %zu", (size_t) msg_type);
		if (msg_type == MessageType::SEND_COMPLETE_FILE) 
			HandleNewFilePacket(p);
		else if (msg_type == MessageType::SEND_FILE_PATCH)
			HandleSigPacket(p);
		else if (msg_type == MessageType::SEND_LINK)
			HandleNewFilePacket(p, true);
		else {
			sz_log(6, "No more requests for this base");

			m_exchanger->ReadBackPacket(p);
			m_exchanger->SetReader(NULL);

			m_request_queue.push_back(Request(Request::EOR, 0));
		}
	} else { 
		assert(m_state == SIGNATURE_RECEPTION);
		HandleSigPacket(p);
	}

}

void SFileSyncer::RequestReceiver::HandleNewFilePacket(Packet* p, bool link) {
	m_state = IDLE;
	uint32_t file_no = ntohl(*(uint32_t*) (p->m_data + sizeof(uint16_t)));
	Request::Type type; 
	if (link)
		type = Request::LINK;
	else
		type = Request::COMPLETE_FILE;
	m_request_queue.push_back(Request(type, file_no));

	free(p->m_data);
	delete p;

	sz_log(7,"Got query for new file");
}

void SFileSyncer::RequestReceiver::HandleSigPacket(Packet* p) {
	rs_buffers_s iobuf;
	memset(&iobuf, 0, sizeof(iobuf));
	if (m_state == IDLE) {
		sz_log(7, "Begin of signature reception");
		m_sigstate.m_file_no = ntohl(*(uint32_t*) (p->m_data + sizeof(uint16_t)));
		m_sigstate.m_job = rs_loadsig_begin(&m_sigstate.m_signature);

		iobuf.next_in = (char*)p->m_data + sizeof(uint16_t) + sizeof(uint32_t);
		iobuf.avail_in = p->m_size - (sizeof(uint16_t) + sizeof(uint32_t));

	} else {
		iobuf.avail_in = p->m_size;
		iobuf.next_in = (char*)p->m_data;
	}
	iobuf.eof_in = p->m_type == Packet::LAST_PACKET;

	m_state = SIGNATURE_RECEPTION;

	do {
		rs_result res = rs_job_iter(m_sigstate.m_job, &iobuf);

		if (res == RS_DONE) {
			sz_log(7, "End of singature reception");
			rs_job_free(m_sigstate.m_job);

			res = rs_build_hash_table(m_sigstate.m_signature);
			assert(res == RS_DONE);

			Request req = Request(Request::PATCH, m_sigstate.m_file_no, m_sigstate.m_signature);
			m_request_queue.push_back(req);

			m_state = IDLE;

			break;
		}

		assert(res == RS_BLOCKED);
	} while (iobuf.avail_in > 0);

	free(p->m_data);
	delete(p);
}

SFileSyncer::ResponseGenerator::ResponseGenerator(TPath& local_dir, 
	std::vector<TPath>& file_list,
	std::deque<Request>& request_queue,
	PacketExchanger *exchanger) 
	:
	m_local_dir(local_dir),
	m_file_list(file_list),
	m_request_queue(request_queue),
	m_exchanger(exchanger),
	m_state(IDLE) {

	m_exchanger->SetWriter(this);
}

Packet* SFileSyncer::ResponseGenerator::GivePacket() { 
	
	if (m_request_queue.size() == 0) 
		return NULL;

	sz_log(9, "Request queue size %zu", m_request_queue.size());
	Request& request = m_request_queue.front();
	
	switch (request.m_type) {
		case Request::COMPLETE_FILE:
			return RawFilePacket();
		case Request::PATCH:
			return PatchPacket();
		case Request::LINK:
			return LinkPacket();
		case Request::EOR:
			m_request_queue.pop_front();
			m_exchanger->SetWriter(NULL);
			return NULL;
		default:
			assert(false);
	}
};

Packet* SFileSyncer::ResponseGenerator::ErrorPacket() {
	Packet * p = new Packet;
	p->m_size = 0;
	p->m_data = NULL;
	p->m_type = Packet::ERROR_PACKET;

	return p;
}

Packet* SFileSyncer::ResponseGenerator::LinkPacket() {
	TPath path = FilePath();

	char link[PATH_MAX];
	int ret = readlink(path.GetPath(), link, sizeof(link));

	m_request_queue.pop_front();

	if (ret <= 0) {
		return ErrorPacket();
	} else {
		link[ret] = '\0';

		char *relative_path = TPath::Relative(link, path.GetPath());
		xmlChar* path_u = xmlCharStrdup(relative_path);
		size_t link_length = strlen((char*)path_u);

		Packet *p = new Packet;
		p->m_type = Packet::LAST_PACKET;
		p->m_size = link_length + sizeof(uint16_t);
		p->m_data = (uint8_t*)malloc(p->m_size + sizeof(char));

		*(uint16_t*)p->m_data = htons(link_length);
		memcpy((char*)(p->m_data + sizeof(uint16_t)), path_u, link_length);

		free(relative_path);
		xmlFree(path_u);

		return p;
	}

}

TPath SFileSyncer::ResponseGenerator::FilePath() {
	Request& req = m_request_queue.front();
	uint32_t file_no = req.m_num;

	TPath& path = m_file_list.at(file_no);
	TPath local_path = m_local_dir.Concat(path);

	return local_path;
}

Packet *SFileSyncer::ResponseGenerator::RawFilePacket() {
	if (m_state == IDLE) {
		sz_log(6, "Begin sent of raw file %s", FilePath().GetPath());

		TPath path = FilePath();
		m_state_data.m_file = fopen(path.GetPath(), "r");

		if (m_state_data.m_file == NULL) {
			m_request_queue.pop_front();
			return ErrorPacket();
		}
	}
	m_state = SENDING_COMPLETE_FILE;

	Packet *p = new Packet;
	p->m_data = (uint8_t*) malloc(Packet::MAX_LENGTH);
	p->m_type = Packet::OK_PACKET;
	size_t read = ReadFully(m_state_data.m_file, p->m_data, Packet::MAX_LENGTH);
	if (read != Packet::MAX_LENGTH) {
		fclose(m_state_data.m_file);
		p->m_type = Packet::LAST_PACKET;
		m_request_queue.pop_front();
		m_state = IDLE;
		sz_log(6, "End of sent of raw file %zu", m_request_queue.size());
	}
	p->m_size = read;

	return p;
}

Packet* SFileSyncer::ResponseGenerator::PatchPacket() {
	if (m_state == IDLE) {
		memset(&m_state_data, 0, sizeof(m_state_data));

		TPath path = FilePath();
		m_state_data.m_file = fopen(path.GetPath(), "r");

		if (m_state_data.m_file == NULL) {
			m_request_queue.pop_front();
			return ErrorPacket();
		}

		Request req = m_request_queue.front();
		m_state_data.m_job = rs_delta_begin(req.m_signature);
		sz_log(6, "Begin of sent of file patch %zu", m_request_queue.size());
	}

	m_state = SENDING_PATCH;

	Packet *p = new Packet;
	p->m_data = (uint8_t*) malloc(Packet::MAX_LENGTH);
	p->m_type = Packet::OK_PACKET;
		
	p->m_size = 0;
	uint8_t *packet_buf = p->m_data;

	do {
		if (m_state_data.m_buf.avail_in == 0) {
			free(m_state_data.m_buf_start);

			const size_t buf_size = 4 * Packet::MAX_LENGTH;
			m_state_data.m_buf_start = (uint8_t*) malloc(buf_size);
			m_state_data.m_buf.next_in = (char*) m_state_data.m_buf_start;

			size_t r = ReadFully(m_state_data.m_file, m_state_data.m_buf_start, buf_size);

			m_state_data.m_buf.avail_in = r;
			if (r != buf_size) 
				m_state_data.m_buf.eof_in = 1;
		}

		m_state_data.m_buf.next_out = (char*)packet_buf;
		m_state_data.m_buf.avail_out = Packet::MAX_LENGTH - p->m_size;

		rs_result res = rs_job_iter(m_state_data.m_job, &m_state_data.m_buf);

		p->m_size +=  m_state_data.m_buf.next_out - (char*)packet_buf;
		packet_buf = (uint8_t*) m_state_data.m_buf.next_out;

		if (res == RS_DONE) {
			rs_job_free(m_state_data.m_job);
			free(m_state_data.m_buf_start);
			fclose(m_state_data.m_file);

			Request& req = m_request_queue.front();
			rs_free_sumset(req.m_signature);
			m_request_queue.pop_front();

			p->m_type = Packet::LAST_PACKET;

			m_state = IDLE;
			sz_log(6, "End of sent of patch file %zu", m_request_queue.size());
			break;
		}

		assert(res == RS_BLOCKED);

	} while (m_state_data.m_buf.avail_in == 0 && p->m_size != Packet::MAX_LENGTH);

	return p;

}
SFileSyncer::SFileSyncer(TPath &local_path, std::vector<TPath>& file_list, PacketExchanger *exchanger)
		:
		m_request_queue(),
		m_receiver(m_request_queue, exchanger),
		m_generator(local_path, file_list, m_request_queue, exchanger),
		m_exchanger(exchanger) {
}
void SFileSyncer::Sync() {
	m_exchanger->Drive();
}

RegExDirMatcher::RegExDirMatcher(const char *exp) {
	int ret;
	ret = regcomp(&m_exp, exp, REG_EXTENDED | REG_NOSUB);
	if (ret != 0) {
		sz_log(1, "Failed to compile expression %s", exp);
		throw FetchFileListEx(FetchFileListEx::REGEX_COMP_FAILED);
	}
}

bool RegExDirMatcher::operator() (const TPath& path) const {
	if (path.GetType() != TPath::TDIR)
		return false;

	int ret = regexec(&m_exp, path.BaseName().GetPath(), 0, NULL, 0);
	return (ret == 0) ? true : false;
}

RegExDirMatcher::~RegExDirMatcher() {
	regfree(&m_exp);
}

InExFileMatcher::InExFileMatcher(const char *exclude, const char* include) {
	int ret;
	ret = regcomp(&m_expreg, exclude, REG_EXTENDED | REG_NOSUB);
	if (ret != 0) {
		sz_log(1, "Failed to compile expression %s", exclude);
		throw FetchFileListEx(FetchFileListEx::REGEX_COMP_FAILED);
	}

	ret = regcomp(&m_inpreg, include, REG_EXTENDED | REG_NOSUB);
	if (ret != 0) {
		sz_log(1, "Failed to compile expression %s", include);
		regfree(&m_expreg);
		throw FetchFileListEx(FetchFileListEx::REGEX_COMP_FAILED);
	}
}

bool InExFileMatcher::operator() (const TPath& path) const {
	if (path.GetType() != TPath::TFILE && path.GetType() != TPath::TLINK)
		return false;

	int ret = regexec(&m_expreg, path.BaseName().GetPath(), 0, NULL, 0);
	if (ret != 0) 
		return true;

	ret = regexec(&m_inpreg, path.BaseName().GetPath(), 0, NULL, 0);
	if (ret != 0)
		return false;

	return true;
}

InExFileMatcher::~InExFileMatcher() {
	regfree(&m_expreg);
	regfree(&m_inpreg);
}

Server::SynchronizationInfo::SynchronizationInfo(const char *basedir, const char *sync, bool unicode) {
	m_basedir = strdup(basedir);
	m_sync = strdup(sync);
	m_unicode = unicode;
}

Server::SynchronizationInfo::SynchronizationInfo(const SynchronizationInfo& s) {
	m_basedir = strdup(s.GetBaseDir());
	m_sync = strdup(s.GetSync());
	m_unicode = s.m_unicode;
}

Server::SynchronizationInfo& Server::SynchronizationInfo::operator=(const SynchronizationInfo &s) {
	if (this != &s) {
		free(m_basedir);
		free(m_sync);

		m_basedir = strdup(s.GetBaseDir());
		m_sync = strdup(s.GetSync());
		m_unicode = s.m_unicode;
	}
	return *this;
}


const char* Server::SynchronizationInfo::GetBaseDir() const {
	return m_basedir;
}

const char* Server::SynchronizationInfo::GetSync() const {
	return m_sync;
}

Server::SynchronizationInfo::~SynchronizationInfo() {
	free(m_basedir);
	free(m_sync);
}

Server::SynchronizationInfo Server::Authenticate() {

	char *user = NULL, *password = NULL, *key=NULL;
	int ver;
	try {
		MessageReceiver rmsg(m_exchanger);

		ver = rmsg.GetUInt16();
		user = rmsg.GetString();
		password = rmsg.GetString();
	
	
		sz_log(4, "Auth info: user %s, password %s", user, password);
		char *basedir, *sync;
		
		bool auth_ok = m_userdb->FindUser(user, password, &basedir, &sync);
		
		if(ver != MessageType::AUTH) {
			int ssserver;
			char *msg;
			
			key = rmsg.GetString();
			ssserver = m_userdb->CheckUser(ver,user,password,key,&msg);
			MessageSender smsg(m_exchanger);
			sz_log(1,"HWKEY: %s USER: %s",key,user);
			
			if (!auth_ok) {
				smsg.PutUInt16(Message::AUTH_FAILURE);
				smsg.PutUInt16(Message::INVALID_AUTH_INFO);
				smsg.FinishMessage();
				m_exchanger->Disconnect();
				throw AppException(ssstring(_("User authenticaiond failed")));
			}
			
			if (ssserver == Message::AUTH_REDIRECT) {
				smsg.PutUInt16(Message::AUTH_REDIRECT);
				smsg.PutString(msg);
				smsg.FinishMessage();
				m_exchanger->Disconnect();
			} else if (ssserver < 11) {
				smsg.PutUInt16(Message::AUTH_OK);
				smsg.PutUInt16(ssserver);
				smsg.FinishMessage();
			} else {
				smsg.PutUInt16(Message::AUTH_FAILURE);
				smsg.PutUInt16(ssserver);
				smsg.FinishMessage();
				m_exchanger->Disconnect();

				
				if (ssserver == Message::ACCOUNT_EXPIRED) 
					throw  AppException(ssstring(_("Account expired")));
				else if (ssserver == Message::INVALID_HWKEY)
					throw  AppException(ssstring(_("Bad hwkey")));
				else if (ssserver == Message::INVALID_AUTH_INFO)
					throw AppException(ssstring(_("User authenticaiond failed")));
				else
					throw AppException(ssstring(_("Unknown error")));
				
			}				 
			
			
		} 
		
		
		// Backward compatibility for old clients
		
		if (!auth_ok && ver == MessageType::AUTH) {
			MessageSender smsg(m_exchanger);
			smsg.PutUInt32(0);
			smsg.PutUInt32(FetchFileListEx::INVALID_AUTH_INFO);
			smsg.FinishMessage();
			throw AppException(ssstring(_("User authenticaiond failed")));
		}
		
		bool unicode = false;
		if (ver == MessageType::AUTH3) {
			unicode = true;
		}
		SynchronizationInfo result(basedir, sync, unicode);

		free(user);
		free(password);

		return result;

	}
	catch (Exception& e) {
		free(user);
		free(password);
		throw e;
	}
}

std::vector<TPath> Server::SendDirsList(SynchronizationInfo &info) {
	try {
		std::vector<TPath> result;

		TPath basedir(info.GetBaseDir());
		if (basedir.GetType() != TPath::TDIR) {
			sz_log(1, "Not such dir: %s", basedir.GetPath());
			throw FetchFileListEx(FetchFileListEx::NOT_SUCH_DIR);
		}

		RegExDirMatcher match(info.GetSync());

		result = ScanDir(basedir, match, false);

		if (result.size() == 0) {
			sz_log(1, "Configuration error, regexp %s matches no dirs in %s", info.GetSync(), info.GetBaseDir());
			throw FetchFileListEx(FetchFileListEx::NO_FILES_SELECTED);
		}

		MessageSender sender(m_exchanger);
		sender.PutUInt32(result.size());

		for (size_t i = 0; i < result.size() ; ++i) {
			std::string s = result[i].GetPath() + strlen(info.GetBaseDir()) + 1;
			sender.PutString((char*)SC::A2U(s).c_str());
		}

		sender.FinishMessage();

		return result;

	} catch (FetchFileListEx& e) {
		MessageSender sender(m_exchanger);
		sender.PutUInt32(0);
		sender.PutUInt32(e.m_error);
		sender.FinishMessage();
		m_exchanger->Disconnect();
		throw e;
	}
}

void Server::SendFileList(const TPath& dir, const char* exclude, const char *include, std::vector<TPath>& result, SynchronizationInfo& info) {
	try {

		sz_log(9, "Request for dir %s, exclude:'%s', include: '%s'", dir.GetPath(), exclude, include);

		InExFileMatcher match(exclude, include);

		result = ScanDir(dir.GetPath(), match, true);

		MessageSender smsg(m_exchanger);

		smsg.PutUInt32(result.size());

		std::basic_string<unsigned char> previous;

		for (size_t i = 0; i < result.size() ; ++i ) {

			TPath& file = result[i];

			TPath path(file.GetPath() + strlen(info.GetBaseDir()) + 1, 
					file.GetType(),
					file.GetModTime(),
					file.GetSize());

			smsg.PutUInt16(path.GetType());

			if (info.IsUnicode()) {
				std::basic_string<unsigned char> u = SC::A2U(path.GetPath());
				if (i == 0) {
					smsg.PutString((char*)u.c_str());
				} else {
					char *patch = CreateScript((char*)previous.c_str(), (char*)u.c_str());
					smsg.PutString(patch);
					free(patch);
				}
				previous = u;
			} else {
				if (i == 0) {
					char *tmp = strdup(path.GetPath());
					tmp = (char *) fromUTF8((unsigned char*) tmp);
					smsg.PutString(tmp);
					free(tmp);
				} else {
					char *tmp = strdup(result[i - 1].GetPath());
					tmp = (char *) fromUTF8((unsigned char *) tmp);
					char *tmp2 = strdup(path.GetPath());
					tmp2 = (char *) fromUTF8((unsigned char *) tmp2);
					
					char * patch = CreateScript(tmp, tmp2);
					toUTF8(patch);

					smsg.PutString(patch);
					free(patch);
					free(tmp);
					free(tmp2);
				}

			}
				
			smsg.PutUInt32(path.GetSize());
	
			time_t mtime = path.GetModTime();
			struct tm* time = gmtime(&mtime);
			smsg.PutUInt16(time->tm_year);
			smsg.PutByte(time->tm_mon);
			smsg.PutByte(time->tm_mday);
			smsg.PutByte(time->tm_hour);
			smsg.PutByte(time->tm_min);

			result[i] = path;

		}
		smsg.FinishMessage();

		return;

	} catch (FetchFileListEx& e) {
		MessageSender sender(m_exchanger);
		sender.PutUInt32(0);
		sender.PutUInt32(e.m_error);
		sender.FinishMessage();

		throw e;
	}
}

char* Server::CreateScript(const char* a, const char *b) {
#define PUTVAL(STREAM, VAL) 								\
{											\
	assert((VAL) < 126 * 126);							\
	fprintf(STREAM,  "%c", uint8_t(((VAL) / 126) + 1));				\
	fprintf(STREAM, "%c", ((VAL) % 126) + 1);					\
}
	
	char *result;
	size_t result_size;
	FILE* result_stream = open_memstream(&result, &result_size);

	size_t l = min(strlen(a), strlen(b));
	size_t i = 0;
	uint16_t len = 0;

	while (i < l || (i == l && len > 0)) {
		if (i == l || (a[i] == b[i] && len > 0)) {
			fprintf(result_stream, "%c", uint8_t(1));

			uint16_t pos = i - len;
			PUTVAL(result_stream, pos + 1);
			PUTVAL(result_stream, len);

			fprintf(result_stream, "%.*s", len, b + pos);

			len = 0;
		} else if (a[i] != b[i])
			len++;
		i++;
	}
	
	ssize_t d = strlen(b) - strlen(a);

	if (d > 0) {
		fprintf(result_stream, "%c", uint8_t(2));
		fprintf(result_stream, "%s", b + l);
	} else if (d < 0) {
		fprintf(result_stream, "%c", uint8_t(3));
		d = -d;
		PUTVAL(result_stream, d);
	}

	fclose(result_stream);

	return result;

#undef PUTVAL
}

void Server::SynchronizeFiles(std::vector<TPath>& file_list, TPath path) {
	SFileSyncer syncer(path, file_list, m_exchanger);
	syncer.Sync();
	m_exchanger->FlushOutput();
}

Server::Server(int socket, SSL_CTX* ctx, UserDB* db) 
	: m_userdb(db) {
	BIO* bio = BIO_new_socket(socket, BIO_CLOSE);
	SSL* ssl = SSL_new(ctx);
	if (!ssl || !bio) 
                	throw SSLException(ERR_get_error());

	SSL_set_bio(ssl, bio, bio);
	SSL_set_read_ahead(ssl, 1);


	int ret = SSL_accept(ssl);
	if (ret != 1)
		throw SSLException(ERR_get_error());

	int val;
	if ((val = fcntl(socket, F_GETFL, 0)) == -1)
               throw IOException(ssstring(_("Setting non-blocking mode on socket:")) + csconv(strerror(errno)));

	if (!(val & O_NONBLOCK)) {
		val |= O_NONBLOCK;
		if (fcntl(socket, F_SETFL, val) == -1)
			throw IOException(ssstring(_("Setting non-blocking mode on socket:")) + csconv(strerror(errno)));
	}

	m_exchanger = new PacketExchanger(ssl, 1, NULL);

}

void Server::SendBaseStamp(TPath &dir) {
	TPath file = dir.Concat(BASESTAMP_FILENAME);

	sz_log(4, "Sending timestamp of file %s", file.GetPath());

	MessageSender smsg(m_exchanger);

	if (file.GetType() == TPath::TFILE) {
		smsg.PutByte(1);

		time_t mt = file.GetModTime();
		struct tm* time = gmtime(&mt);

		smsg.PutUInt16(time->tm_year);
		smsg.PutByte(time->tm_mon);
		smsg.PutByte(time->tm_mday);
		smsg.PutByte(time->tm_hour);
		smsg.PutByte(time->tm_min);
	} else {
		smsg.PutByte(0);
	}
	smsg.FinishMessage();
}

void Server::Serve() {
	try {
		SynchronizationInfo info = Authenticate();
		std::vector<TPath> synced_dir_list = SendDirsList(info);

		while (true) {
			std::vector<TPath> files_list;

			MessageReceiver recv(m_exchanger);

			uint16_t msg;
			try {
				msg = recv.GetUInt16();
				sz_log(9, "Got message nr %d", msg);
			} catch (ConnectionShutdown &e) {
				//client does not want to talk to us anymore
				m_exchanger->Disconnect();
				sz_log(9, "Client closed connection");
				return;
			}

			if (msg == MessageType::BASE_MODIFICATION_TIME) {
				uint32_t dirno = recv.GetUInt32();
				sz_log(9, "Request for base modfification time for dir %d", dirno);
				SendBaseStamp(synced_dir_list.at(dirno));
			} else if (msg == MessageType::GET_FILE_LIST) {
				char *exclude = NULL, *include = NULL;
				try { 
					uint32_t dirno = recv.GetUInt32();
					sz_log(9, "Request for files list for dir %d", dirno);
					exclude = recv.GetString();
					include = recv.GetString();
					SendFileList(synced_dir_list.at(dirno),
						exclude,
						include,
						files_list,
						info);
					free(exclude);
					free(include);	
				} catch (...) {
					free(exclude);
					free(include);
					throw;
				}
				//and now we are syncing
				SynchronizeFiles(files_list, TPath(info.GetBaseDir()));
			} else {
				sz_log(0, "Recevied unexpected message type: %zu", msg);
				return;
			}

		}
	} catch (FetchFileListEx &e) {
		return;
	}

}

RETSIGTYPE g_sigchild_handler(int sig) {
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0);
}

void LoadUserDatabase() {

	UserDB* userdatabase = XMLUserDB::LoadXML(g_userdatabasefile, prefix, g_rpc);
	if (!userdatabase) {
		sz_log(1, "Cannot open user database file(%s)", g_userdatabasefile);
		if (!g_userdatabase) {
			sz_log(0, "Exiting...");
			exit(1);
		} else {
			sz_log(1, "Using old version of users' database");
		}
	} else {
		g_userdatabase = userdatabase;
	}
}

RETSIGTYPE g_sighup_handler(int sig) {
	sz_log(1, "Received signal %d, reloading configuration", sig);
	LoadUserDatabase();
}

Listener::Listener(int port) : m_port(port) {
}

int Listener::Start() {
	sigset_t sigset;
	sigemptyset(&sigset);

	struct sigaction act;
	act.sa_handler = g_sigchild_handler;
	act.sa_mask = sigset;
	act.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &act, NULL);

	act.sa_handler = g_sighup_handler;
	act.sa_mask = sigset;
	act.sa_flags = 0;
	sigaction(SIGHUP, &act, NULL);

	sigaddset(&sigset, SIGCHLD);
	sigprocmask(SIG_UNBLOCK, &sigset, NULL);
	sigemptyset(&sigset);

	sigaddset(&sigset, SIGHUP);
	sigprocmask(SIG_UNBLOCK, &sigset, NULL);

	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		sz_log(0, "socket %s", strerror(errno));
		exit(1);
	}

	struct sockaddr_in sin;
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(m_port);

	int on = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	if (bind(sock, (struct sockaddr*)&sin, sizeof(sin))) {
		sz_log(0,"bind %s", strerror(errno));
		exit(1);
	}


	if (listen(sock, 1) < 0) {
		sz_log(0, "listen %s", strerror(errno));
		exit(1);
	}

	while (true) {
		struct sockaddr_in caddr;
		socklen_t clen = sizeof(caddr);
		int accepted = accept(sock, (struct sockaddr*) &caddr, &clen);
		if (accepted < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				sz_log(0, "%d accept %s", errno, strerror(errno));
				exit(1);
			}
		}

		sigemptyset(&sigset);
		sigaddset(&sigset, SIGHUP);
		sigaddset(&sigset, SIGCHLD);
		sigprocmask(SIG_BLOCK, &sigset, NULL);

		pid_t child = fork();

		if (child < 0)
			sz_log(0, "fork %s", strerror(errno));

		if (child == 0) {
			close(sock);
			return accepted;
		}

		sigprocmask(SIG_UNBLOCK, &sigset, NULL);
		close(accepted);
	}

}

int main(int argc, char *argv[]) {
	libpar_init();
	if (!InitLogging(&argc, argv, "sss")) {
		fprintf(stderr, "Unable to set up logging\n");
		exit(1);
	}

	char* key_file = libpar_getpar("sss", "key_file", 1);
	char* passphrase = libpar_getpar("sss", "passphrase", 0);
	char* ca_file = libpar_getpar("sss", "ca_file", 1);
	char* port = libpar_getpar("sss", "port", 1);
	char* user = libpar_getpar("sss", "user", 1);
	prefix = libpar_getpar("","config_prefix",1);
	g_rpc = libpar_getpar("sss", "rcp_address", 0);
	if (g_rpc == NULL) {
		g_rpc = "http://localhost:5500/";
	}
	g_userdatabasefile = libpar_getpar("sss", "userdbfile", 1);

	libpar_done();

	go_daemon();

	InitSSL();
	SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
	SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
        if (SSL_CTX_use_certificate_chain_file(ctx, ca_file) == 0) {
		sz_log(0, "Failed to load cert chain file , error %s", ERR_error_string(ERR_get_error(),NULL));
		exit(1);
	}
	if (passphrase != NULL) {
		SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)passphrase);
		SSL_CTX_set_default_passwd_cb(ctx, PassphraseCallback);
	}
	if (!(SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM))) {
		sz_log(0, "Failed to load private key file , error %s", ERR_error_string(ERR_get_error(),NULL));
		exit(1);
	}

	struct passwd* pass = getpwnam(user);

	if (pass == NULL) {
		sz_log(0, "No user %s found, exiting", user);
		exit(1);
	}

	LoadUserDatabase();
	Listener l(atoi(port));
	int connected  = l.Start();
	try {
		if (setegid(pass->pw_gid) != 0
			|| setgid(pass->pw_gid) != 0
			|| setuid(pass->pw_uid) != 0
			|| seteuid(pass->pw_uid) != 0) {
			sz_log(0, "Switch to privileges of user %s failed, exiting", user);
			exit(1);
		}
		Server serv(connected, ctx, g_userdatabase);
		serv.Serve();
	}
	catch (Exception& e) {
		sz_log(0, "error %s", e.What().c_str()); 
		return 1;
	}
	catch (...) {
		sz_log(0, "Uknown exception");
	}

	return 0;
}

