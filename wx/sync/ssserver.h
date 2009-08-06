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
#pragma interface 
#include "config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <regex.h>
#include <librsync.h>
#include <deque>

#include "liblog.h"
#include "sspackexchange.h"
#include "ssfsutil.h"
#include "ssuserdb.h"
#include "ssutil.h"

using std::min;

/**Class resposible for synchronizing files with client*/
class SFileSyncer {
	/**Represents request from a client*/
	struct Request {
		/**Type of request*/
		enum Type { 
			COMPLETE_FILE, 	/**<Request for a complete file*/
			PATCH, 		/**<Request for a patch*/
			LINK, 		/**<Request for a link*/
			EOR,		/**<No more request from server*/
			} m_type;
		uint32_t m_num;		/**<Nubmer of file on file list this request refers to*/
		/**If request if of type PATCH, this variable points to client's file signature*/
		rs_signature_t* m_signature;
		Request(Type type, uint32_t num, rs_signature_t *sig = NULL);
	};

	/**Reads requests from client and puts them in request queue*/
	class RequestReceiver : PacketReader {
		std::deque<Request>& m_request_queue;	/**<Requests queue shared with @see ResponseGenerator*/
		PacketExchanger* m_exchanger;		/**<Packets exchanger object that we use to read request
							  from user*/
		enum { 	IDLE, 				/**<No request is currently read*/
			SIGNATURE_RECEPTION 		/**<We are receving signature form client*/
		} m_state;				/**<Holds information on state of the class
							  between subsequent calls to @see Iterate
							  method*/
		struct {
			rs_job_t *m_job;		/**<librsyc job object*/
			rs_signature_t *m_signature;	/**<Resultant signature*/
			uint32_t m_file_no;		/**<Number of file on file list*/
		} m_sigstate;				/**<Holds data asscociated with currently read
							  signature*/
	public:
		/**Constructor
		 * @param request_queue queue to which incoming requests are added
		 * @param exchanger PacketExchanger used for packet reading*/
		RequestReceiver(std::deque<Request>& request_queue, PacketExchanger* exchanger);

		virtual bool WantPacket();

		/**Reads and queues request from client*/
		void ReadPacket(Packet *p);

		/**Handles data from a packet requesting new file, enqueues request
		 * @param p packet to read data from
		 * @param link true is it is request for a link*/
		void HandleNewFilePacket(Packet* p, bool link = false);

		/**Handles packet holding signature that requests patch for an existing file
		 * enqueues request
		 * @param p packet to read data from*/
		void HandleSigPacket(Packet* p);

	};
				
	class ResponseGenerator : public PacketWriter {
		/**Direcotry to which all paths from @see m_file_list are relative*/
		TPath& m_local_dir;
		/**Synchronized files list*/
		std::vector<TPath>& m_file_list;
		/**Requests queue*/
		std::deque<Request>& m_request_queue;
		/**Object that is used for sending responses*/
		PacketExchanger* m_exchanger;
		enum { IDLE, 			/**<No request is processed*/
			SENDING_COMPLETE_FILE,	/**<We are in process of sending new files*/ 
			SENDING_PATCH } 	/**<We are in process of sending patch*/
		m_state;		/**<Holds info on internal object's state between subsequent
					  calls to @see Iterate method*/
		struct {
			FILE *m_file;		/**<File that is currently processed*/
			rs_job *m_job;		/**<Singature generating job*/
			rs_buffers_s m_buf;	/**<librsync's data buffers*/
			uint8_t* m_buf_start;	/**<pointer to file's data buffer*/
		} m_state_data;			/**<data associacted with current state*/
	public:
		ResponseGenerator(TPath& local_dir, 
				std::vector<TPath>& file_list,
				std::deque<Request>& request_queue,
				PacketExchanger *exchanger);

		/**Dequeses request from @see m_request_queue, generates respones and sends them 
		 * via @see m_exchanger. Returns only if request queue is empty
		 * @param finish indicates that all requests for this database has been read
		 * @return false if all requests are answered true otherwise*/
		Packet* GivePacket();

		/**Terminates response for currently processed response with error packet*/
		Packet *ErrorPacket();

		/**Sends packets holdling response for link request*/
		Packet *LinkPacket();

		/**Sends packet holding response for request for a complete file*/
		Packet* RawFilePacket();

		/**Sends packet holding response for request for a patch for a file*/
		Packet* PatchPacket();

		/**@return @see TPath object representing file that we are currently working on*/
		TPath FilePath();


	};
	/**Requests queue shared by @see RequestReceiver and @see ResponseGenerator*/
	std::deque<Request> m_request_queue;
	/**@see RequestReceiver*/
	RequestReceiver m_receiver;
	/**@see ResponseGenerator*/
	ResponseGenerator m_generator;

	PacketExchanger *m_exchanger;
public:
	SFileSyncer(TPath &local_path, std::vector<TPath>& file_list, PacketExchanger *exchanger);

	/**Performs files synchronization*/
	void Sync();
};

/**Class that performs dir names matching against
 * regular expression*/
class RegExDirMatcher : public FileMatch {
	/**Regexp that we are matching*/
	regex_t m_exp;
public:
	RegExDirMatcher(const char *exp);

	/**Matching operator
	 * @param path path against which match is performed
	 * @return true is path is a dir and it's name matches
	 * against @see m_exp*/
	virtual bool operator() (const TPath& path) const;

	virtual ~RegExDirMatcher();
};

/**Class that performs match of two reg expression against file, dir or link
 * name. */
class InExFileMatcher : public FileMatch {
	/**Exclude regular expression*/
	regex_t m_expreg;
	/**Include regular expression*/
	regex_t m_inpreg;
public:
	InExFileMatcher(const char *exclude, const char* include);

	/**Actual matching operator. 
	 * @param path path we are perforoming match against
	 * @return true if path doest not match @see m_expreg
	 * or path matches both @see m_expreg and @see m_inpreg,
	 * in other case returns false*/
	virtual bool operator() (const TPath& path) const;

	virtual ~InExFileMatcher();

};
	
/**Main class of a server*/
class Server {
	/**Holds info on data that client is allowed to sync*/
	class SynchronizationInfo {
		/**Base directory for @see m_sync regex*/
		char* m_basedir;
		/**Regular expression describing directores from @see m_basedir
		 * that client is allowed to sync*/
		char* m_sync;
		/**Create script in unicode */
		bool m_unicode;

	public:
		SynchronizationInfo(const char* basedir, const char* sync, bool unicode);

		SynchronizationInfo(const SynchronizationInfo& s);

		SynchronizationInfo& operator=(const SynchronizationInfo& s);

		/**@return @see m_basedir*/
		const char* GetBaseDir() const ;

		/**@return @see m_sync*/
		const char* GetSync() const;

		/**@return @see m_unicode*/
		bool IsUnicode(){ return m_unicode; }

		~SynchronizationInfo();
	};

	/**@see object for accessing network connection*/
	PacketExchanger* m_exchanger;
	/**users' database*/
	UserDB* m_userdb;


	/**Authenticates user
	 * @return @see SynchronizationInfo if authentication suceeded, 
	 * otherwise throws an exception*/
	SynchronizationInfo Authenticate();

	/**Sends list of dirs for given @SynchronizationInfo
	 * @param info info on synced dirs
	 * @return list of sent directories*/
	std::vector<TPath> SendDirsList(SynchronizationInfo& info);

	/**Reads a request for a file list from a client, sends this lists
	 * and returns it in its result param
	 * @param dir  dir we are acting upon
	 * @param exclude  exclude regexp
	 * @param include  include regexp
	 * @param result list of selected files 
	 * @param info @see SynchronizationInfo
	 */
	void SendFileList(const TPath& dir, const char* exclude, const char *include, std::vector<TPath>& result, SynchronizationInfo &info);

	/** Synchornizes given file list
	 * @param file_list file list to synchronize with client
	 * @param path path to which all entries of @see file_list is relative*/
	void SynchronizeFiles(std::vector<TPath>& file_list, TPath path);

	/**Given two strings a and b creates script that applied to 
	 * string a reconstructs string b. Of course idea behind this is that
	 * script is (hopefully) much shorter than string b. These procedure
	 * does not create Shortest Editing Script, but is trivial
	 * and seems to have quite reasonable efficiency.
	 * Script is also a string. Script "language" is descibed by following 
	 * regexp: (1 POS LEN CHARACTER{LEN})*((2 CHARACTERS)|(3 VAL))?0
	 * 1,2,3,0 - these are commands represented by char values (char type) 
	 * command 1:
	 * replace LEN characters at position POS in base string with
	 * following LEN chars
	 * command 2:
	 * add following characters to the end of base string
	 * command 3:
	 * cut VAL characters from end of a base string
	 * @param a base string used for resonstruction
	 * @param b string to be resconstructed by a script
	 * @return editing script, shall be freed by a caller*/
	static char* CreateScript(const char* a, const char *b);

	/**Sends a message with timestamp of BASESTAMP_FILENAME file located
	 * in given dir*/
	void SendBaseStamp(TPath &dir);

public:
	Server(int socket, SSL_CTX* ctx, UserDB* db);

	/**Handles connection with client*/
	void Serve();

};

/**signal handler for SIGCHLD*/
RETSIGTYPE g_sigchild_handler(int sig);

/**users' database*/
UserDB *g_userdatabase = NULL;

/**users' database file*/
const char *g_userdatabasefile = NULL;

/**configuration prefix*/
const char *prefix = NULL;

/**load users database*/
void LoadUserDatabase();

/**handles SIGHUP signal*/
RETSIGTYPE g_sighup_handler(int sig);

/**listens for connection from clients*/
class Listener {
	/**port to listen at*/
	int m_port;
	public: 
	Listener(int port);
	/*Starts listening for a connections.
	 * Creates new process for each established connection
	 * and return this connection socket (the newly created 
	 * process returns from this function)
	 * @return established connection socket*/
	int Start();
};
