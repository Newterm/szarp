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
#ifndef _SSCLIENT_H_
#define _SSCLIENT_H_

#ifdef __WXMSW__
#define _WIN32_IE 0x600
#endif

#include "config.h"
#include "version.h"
#include <wx/platform.h>

#include <openssl/ssl.h>
#include <openssl/md5.h>
#include <math.h>

#ifndef MINGW32
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <sys/types.h>
#include <shlwapi.h>
#endif
#include <utime.h>

#include <librsync.h>

#include <deque>
#include <map>
#include <set>
#include <list>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/wxprec.h>
#include <wx/progdlg.h>
#include <wx/cmdline.h>
#include <wx/datetime.h>
#include <wx/statline.h>
#include <wx/config.h>
#include <wx/thread.h>
#include <wx/taskbar.h>
#include <wx/wizard.h>
#include <wx/image.h>
#include <wx/ipc.h>
#include <wx/tokenzr.h>
#include <wx/dir.h>
#include <wx/xml/xml.h>
#include <wx/log.h>
#else
#include <wx/wx.h>
#endif

#include "sspackexchange.h"
#include "ssfsutil.h"
#include "ssutil.h"
#include "szapp.h"
#include "singleinstance.h"
#include "szhlpctrl.h"
#include "cfgnames.h"
#include "hwkey.h"
#include "sztaskbaritem.h"

#ifdef MINGW32
#include "mingw32_missing.h"
#endif

#ifdef __WXMSW__
#include "balloontaskbaritem.h"
#endif

class Client;

/**Class representing progress of synchronization process*/
class Progress : public TermChecker {

	/**Handler of PROGRESS_EVENT*/
	wxEvtHandler *m_progress_handler;

	/**Last progress update time*/
	time_t m_last_update;

	/**Latest time we checked for a termination request*/
	time_t m_last_term_check;

	/**Latest synchronization action*/
	int m_last_action;

	/**Latest value of progress for an action*/
	int m_last_progress;

	/**Latest extra info for a progress*/
	ssstring m_last_extra_info;

	/**true if user requested termination of sync process*/
	bool m_term_request;

	/**synchronizes access to @see m_term_request*/
	wxCriticalSection m_term_request_lock;

public:
	/**Action - action that is being or have been performed by synchronization
	 * thread*/
	enum Action {
		CONNECTING,		/**<thread is in phase of connecting to server*/
		AUTHORIZATION,		/**<currently authorization is peformed*/
		FETCHING_FILE_LIST,	/**<files list is fetched*/
		SYNCING,		/**<files are being synchronized*/
		FINISHED,		/**<sync process has finished*/
		SYNC_TERMINATED,	/**<sync process was terminated at user's request*/
		MESSAGE,		/**<show message for user*/
		//THREAD_EXIT,
		FAILED			/**<synchronization failed*/
	};

	Progress(); 

	/**Sets handler for PROGRESS_EVENT*/
	void SetEventHandler(wxEvtHandler *progress_handler);

	/**Sets progress values, called by thread that performs synchronizastion*/
	virtual void SetProgress(Action action, int progress, wxString extra_info = _T(""), std::map<std::string, std::string>* dir_server_map = NULL);

	/**Sets values of termination request. 
	 * @param value if true request for sync termination is set, if false request flag is cleared*/
	void SetTerminationRequest(bool value);

	/**Checks if termination request flag is set, called by synchronizing thread*/
	void Check();

};

/** Class responsible for files synchronization*/
class CFileSyncer {
	/**Describes request*/
	struct Request {
		enum Type { COMPLETE_FILE, 	/**<request for a complete file*/
			PATCH, 			/**<request for a patch	*/
			LINK, 			/**<request for a link*/ 
			FILE_REST		/**<request for a file remainder*/	
		} m_type;
		/**number of file this request refers to*/
		uint32_t m_num;
		
		Request(Type type, uint32_t num);
	};

	/**Generates and sends requests for a sever*/
	class RequestGenerator : public PacketWriter {
		/**Our main dir, all path are relative to this directory*/
		const TPath& m_local_dir;
		/**Dictionary of synchronized files 
		 * key - file number 
		 * value - @see TPath object describing path*/
		std::map<uint32_t, TPath>& m_file_list;
#ifdef __WXMSW__
		/**This is object which iterates over a map
		 * @see m_file_list
		 * It iterates over all regular files and then all
		 * symlinks are traversed. This is only required for 
		 * MSW, because NTFS does not allow a "junction point"
		 * to point to non-existing path. So we are
		 * synchronizing files first. */
		class MswFileIterator {
			/**Type of object we are iterating over*/
			typedef std::map<uint32_t, TPath> MAP;
			/**"Native" iterator for a @see MAP*/
			typedef MAP::iterator MAPI;
			/**Object we are itervating over*/
			MAP& m_map;
			/**Actual iterator*/
			MAPI m_iterator;
			/** True if we are iterateing over links
			 * at the moment, false otherwise*/
			bool m_link;

			/** Advences @see m_iteartor to first link
			 * element in @see m_map*/
			void ProceedToLink();

			/** Advences @see m_iteartor to first non-link
			 * element in @see m_map*/
			void ProceedToNonLink();

		public:
			MswFileIterator(MAP& map);

			/** Advanes the iterator*/
			void operator++(int);

			/** Returns true if given iterator
			 * points to the same element as this object*/
			bool operator==(const MAPI& iterator) const;

			MAP::value_type* operator->();

			void operator=(const MAPI& iterator);
		};
		/**Object used for traversing files' list*/
		MswFileIterator m_current_file;
#else
		/**Object used for traversing files' list*/
		std::map<uint32_t, TPath>::iterator m_current_file;

#endif
		/**List of requests*/
		std::deque<Request>& m_requests;
		/**@see PacketExachnger that is used for sending requests*/
		PacketExchanger *m_exchanger;

		/**Object state between subsequent call to @see Iterate*/
		enum { IDLE, 			/**< nothing is currently being done*/
			SIGNATUE_GENERATION 	/**< object is in process of signature
						generation*/
		} m_state;

		struct {
			FILE *m_signed_file;	/**<file that is currently signed*/
			rs_job* m_job;		/**<librsync job object, signes the files*/
			rs_buffers_s m_iobuf;	/**<rs buffers containg signed file data*/
			uint8_t *m_file_buffer; /**<buffer holds data from signed file*/
		} m_sigstate;

		bool m_processing_extra_requests;

		bool m_request_new_file_data;

		std::list<uint32_t> m_extra_requests;

		std::list<uint32_t>::iterator m_extra_requests_iterator;

		uint32_t FileNo();

		TPath& FilePath();

		void MoveForward();

		bool Finished();
	public:
		RequestGenerator(const TPath& local_dir, 
				std::map<uint32_t, TPath>& file_list, 
				std::vector<TPath>& files, 
				std::deque<Request>& requests,
				PacketExchanger *exch,
				bool request_new_data,
				Progress &progress);


		~RequestGenerator();

		/**Sends requests until responses from 
		 * arrive. 
		 * @return false is all requests has been sent, */
		Packet *GivePacket();

		/**Determines what type of request should be sent*/
		Packet *StartRequest();

		/**Generates request for a new file*/
		Packet* NewFileRequest(TPath& path);

		/**Generates packets holding existing file signature*/
		Packet* PatchReqPacket();

		Packet* RestReqPacket(TPath& path);
					
		void AddExtraRequest(uint32_t file_no);
	};

	/**Class receives and handles responses from a server*/
	class ResponseReceiver : public PacketReader {
		/**local directory*/
		const TPath& m_local_dir;
		/**list of synchronized files*/
		std::map<uint32_t, TPath>& m_file_list;
		/**requests queue*/
		std::deque<Request>& m_requests;
		/**@see PacketExchander used for packets reception*/
		PacketExchanger* m_exchanger;
		/**Object state between subsequent calls to @see Iterate method*/
		enum { IDLE, 			/**<no file is synchronized at this moment*/
			COMPLETE_RECEPTION, 	/**<object is in process of reception of new file*/
			PATCH_RECEPTION, 	/**<object is in process of reception of patch for a file*/
			LINK_RECEPTION,		/**<object is in process of reception of link data*/
			REST_RECEPTION		/**<object is in process of reception of new file data*/
		} m_state;

		/**destination(temporary) file*/
		FILE* m_dest;
		/**base file*/
		FILE* m_base;
		/**librsync object, performs file patching*/
		rs_job_t *m_patch_job;
		/**progress indicator*/
		Progress& m_progress;
		/**number of already synchronized files*/
		uint32_t m_synced_count;
		/**indicates if sender has sent all requests*/
		bool m_all_requests_sent;

		RequestGenerator *m_generator;
	public:
		ResponseReceiver(const TPath& local_dir, 
			std::map<uint32_t, TPath>& file_list, 
			std::deque<Request>& requests,
			PacketExchanger* exchanger,
			Progress& progress);

		~ResponseReceiver();

		/**Returns @see TPath representing temporary file*/
		TPath TempFileName();

		/**Returns number of currently synchornized file*/
		uint32_t FileNo();

		/**Return @see TPath object reporesenting a local path of
		 * currently synchronized file*/
		TPath LocalFileName();

		/**Return @see TPath object reporesenting a path of
		 * currently synchronized file*/
		TPath FileName();

		/** Opens temporary file*/
		void OpenTmpFile();

		/** Opens base file i.e. file to which patch is applied*/
		void OpenBaseFile();

		/*Closes base file*/
		void CloseBaseFile();

		/**Closes temporary file*/
		void CloseTmpFile();

		/**Sets modification time of destination file*/
		void SetFileModTime();

		/**Moves temporary file to destination file*/
		void MoveTmpFile();

		/**Removes temporary file*/
		void RemoveTmpFile();

		/**Attemts to perform one step of synchronization process
		 * i.e. receive a packet and handle it appropriately
		 * @return true - if request queue is empty and 
		 * requests generation is complete i.e. done_generatrion 
		 * is true, this indicates that synchronization process
		 * is finished*/
		void ReadPacket(Packet *p);

		/**Handles a packet with data of a link
		 * (creates link)*/
		void HandleLinkPacket(Packet *p);

		/**Handles an error packet
		 * (removes temp file and advances to next file*/
		void HandleErrorPacket(Packet* p);

		/**Currently synchornized database name*/
		ssstring DataBaseName();

		/**Appends data from a packet to a new file*/
		void HandleNewFilePacket(Packet *p);

		/**Applies patch to a file*/
		void HandlePatchPacket(Packet *p);

		void HandleRestFilePacket(Packet *p);

		void SetRequestGenerator(RequestGenerator *gen);

	};

	/**Collection of files to synchronize*/
	std::vector<TPath> m_sync_file_list;

	/**List of requests shared by @see RequestGenerator and ResponseReceiver*/
	std::deque<Request> m_request_queue;
	/**Collection of synchronized files*/
	std::map<uint32_t, TPath> m_file_list;
	/** @see RequestGenerator*/
	RequestGenerator m_generator;
	/** @see ResponseReceiver*/
	ResponseReceiver m_receiver;
	/** progress indicator*/
	Progress& m_progress;
	/** packet exchanger*/
	PacketExchanger* m_exchanger;
	/** synchronization dir*/
	const TPath &m_sync_dir;
	/** delete files not present on the server */
	bool m_delete_option;


	public:
	CFileSyncer(const TPath &local_dir, const TPath &sync_dir, std::vector<TPath> &server_list, PacketExchanger* exchanger, Progress& progress, bool delete_option);

	/**Synchronizes files*/
	void Sync();

};


/*Main client class, runs as separate thread*/
class Client : public wxThread {
	/**SSL_CTX objects*/
	SSL_CTX *m_ctx;
	/**address of a server*/
	char **m_addresses;
	/**port to connect to*/
	int m_port;
	/**our username*/
	char **m_users;
	/**our password*/
	char **m_passwords;

	char *m_current_address;

	int m_servers_count;

	int m_current_server;
	/**local dir*/
	TPath m_local_dir;
	/**process idicator class*/
	Progress& m_progress;
	/**object that is used for communitaion over network*/
	PacketExchanger* m_exchanger;
	/**mutex utilized by @see m_work_condition*/
	wxMutex* m_work_mutex;
	/**signaling this condition causes client to start synchronization*/
	wxCondition* m_work_condition;
	/**hardware key*/
	HardwareKey key;
	/**list of directories to synchronize*/
	std::vector<std::string> m_dirList;
	/**delete option*/
	bool m_delete_option;

	std::map<std::string, std::string> m_dir_server_map;

	public:
	Client(Progress& progress);

	/**initializes thread*/
	void Init();

	/**sets connection options. must not be called when synchronization is in progress
	 * @param addresses of servers addres
	 * @param users's names for corresponding servers
	 * @param passwords passwords to servers
	 * @param server_count number of servers to connect to
	 * @param local_dir directory where synchronized files are saved
	 * @param port to connect to*/
	void SetOptions(char** addresses,
		char **users,
		char **passwords,
		int servers_count,
		char *local_dir,
		int port);
	/**Starts synchronization process, connection parameters must be set prior to calling this function
	 * @param dirList list of directories to synchronize, empty list means synchronize all available*/
	void BeginSync(bool delete_option, wxArrayString dirList = wxArrayString());

private:
	/**thread entry point
	 * @return NULL*/
	void *Entry();
	/**Starts synchronization*/
	void Start();
	/**Synchronizes all data*/
	bool Synchronize();
	/**Connects client to server*/
	void Connect();
	/**Synchronizes files*/
	void SyncFiles();

	uint32_t ResolveAddress();

	/**Recives from server list of directories that shall be synchronized*/
	std::map<TPath, int, TPath::less> GetDirList();
	/**Authenticate user with password, hardware key and protocol version*/
	void Auth(bool& redirect);

	/**Executes script generated by @see Server::GenerateScript
	 * @param b base string
	 * @param s script to be executed
	 * @return resultant string, shall be freed by caller*/
	static char* ExecuteScript(const char* b, const char* s);

	/**Receives list of files for synchronization for give directory*/
	std::vector<TPath> GetFileList(const TPath &dir, uint32_t dir_no, bool &delete_option);

	/**For given catalog returns two regular expression exclude and include.
	 * These expressions declare files that shall be put on file list by server,
	 * depend on existence and content of "szbase/Status/program_uruchomiony".
	 * @param dir direcotry to be considered
	 * @param exclude output param, exclude regular expression string, shall be freed by caller
	 * @param include output param, exclude regular expression string, shall be freed by caller*/
	void GetExInEpxression(TPath& dir, uint32_t dir_no, char*& exclude, char*& include, bool &force_delete);
		
	~Client();
};

/**Progress event class*/
class ProgressEvent : public wxCommandEvent
{
public:
	ProgressEvent(int value, Progress::Action action, ssstring extra_info, std::map<std::string, std::string> *dir_server_map = NULL);
	/**
	 * @return current action*/
	Progress::Action GetAction(); 
	/**
	 * @return progress of current action, value ranging from 0 to 100*/
	int GetValue();
	/**
	 * @return extra descrption associated with this synchronization state*/
	wxChar* GetDescription();

	/**
	 * @return mapping of bases to servers*/
	std::map<char*, char*>* GetDirServerMap();
	/**
	 * @return copied event*/
	virtual wxEvent* Clone() const;

	virtual ~ProgressEvent();
private:
	/**current progress value*/
	int m_value;
	/**current action*/
	Progress::Action m_action;
	/**extra description*/
	wxChar* m_description;
	/**dir server map*/
	std::map<char*, char*> *m_dir_server_map;
};


DECLARE_EVENT_TYPE(PROGRESS_EVENT, -1)

typedef void (wxEvtHandler::*ProgressEventFuntion)(ProgressEvent&);


#define EVT_PROGRESS(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( PROGRESS_EVENT, id, -1, \
    (wxObjectEventFunction) (wxEventFunction) (wxCommandEventFunction) (wxNotifyEventFunction) \
    wxStaticCastEvent( ProgressEventFuntion, & fn ), (wxObject *) NULL ),

#if 0
/*Allow user to choose bases that he wishes to configure*/
class ChooseBasesWindow : public wxDialog {
	DECLARE_EVENT_TABLE();

	wxCheckListCtrl *m_check_list;
	wxRadioButton *m_sync_only_choosen;
	wxRadioButton *m_sync_all;
};
#endif


/**Displays current synchronization progress*/
class ProgressFrame : public wxDialog {
	DECLARE_EVENT_TABLE();
	/**status text*/
	wxStaticText* m_status_text;
	/**current progress value*/
	wxGauge* m_progress_bar;
	/**user clicks this button to cancel synchronization*/
	wxButton* m_cancel_button;
	/**pointer to @see Progress class*/
	Progress *m_progress;
	/**true is sync in is progress*/
	bool m_sync_in_progress;
	/**object performing synchronization*/
	Client* m_client;
	/**synchronization started at this moment*/
	wxDateTime m_sync_start_time;

#ifdef __WXMSW__
	/**Pointer to taskbar. Used for balloons displaying*/
	BalloonTaskBar* m_ballon;
#endif
	/**progress event handler*/
	void OnUpdate(ProgressEvent& event); 
	/**cancel button click handler*/
	void OnCancel(wxCommandEvent& event);
	/**close event handler*/
	void OnClose(wxCloseEvent& event);
	/**requests @see Client thread to terminate
	 * synchronization*/
	void TerminateSynchronization();
	/**Iconize event handler. Shows/hides the frame*/
	void OnIconize(wxIconizeEvent& event);

	void SendReloadToSCC();

	void UpdateServerBaseMap(std::map<char*, char*>& dir_map);

	public:
#ifdef __WXMSW__
	ProgressFrame(Progress *progress, BalloonTaskBar *ballon);
#else
	ProgressFrame(Progress *progress);
#endif
	void StartSync(bool show, wxArrayString dirList = wxArrayString(), bool delete_option = false);

	bool ShowIfRunning();
};

class SSCTaskBarItem;
/**This class lets user to change configuration.*/
class SSCConfigFrame : public wxDialog
{
	DECLARE_EVENT_TABLE();
	/**wxTextCtrl holding user name*/
	wxTextCtrl* m_user_ctrl;
	/**wxTextCtrl holding server address*/
	wxListBox* m_servers_box;
	/**wxCheckBox that turns on/off automatic 
	 * synchronization*/
	wxCheckBox* m_autosync_box;
#ifdef __WXMSW__
	/**wxCheckBox that turns on/off baloon tooltips
	 * displaying upon synchronization finish*/
	wxCheckBox* m_show_notification_box;
#endif
	/**Lets users choose time interval between automatic
	 * synchronizations*/
	wxStaticText *m_server_text;

	wxComboBox* m_syncinterval_combo;

	std::map<wxString, std::pair<wxString, wxString> > m_servers_creditenials_map;

	wxString m_current_server;

	/**Stores current synchronization*/
	void StoreConfiguration();
	/**Handler called when user want to change password*/
	void OnPasswordChange(wxCommandEvent& event);
	/**OK button press handler. Saves current configuration.
	 * Iconizes the frame.*/
	void OnOKButton(wxCommandEvent& event);
	/**Cancel button press handler. Iconizes the frame*/
	void OnCancelButton(wxCommandEvent& event);
	/**Metod called when state of autosync checkbox is changed*/
	void OnAutoSyncCheckBox(wxCommandEvent& event);

	void OnAddServerButton(wxCommandEvent& event);

	void OnRemoveServerButton(wxCommandEvent& event);

	void OnChangeServerNameButton(wxCommandEvent& event);

	void OnServerSelected(wxCommandEvent& event);

	void ServerSelected(bool grab_username);
public:

	SSCConfigFrame();

	/**Loads configration into the frame*/
	void LoadConfiguration();
};

/**This class lets user to synchronize only selected databases.*/
class SSCSelectionFrame : public wxDialog
{
	DECLARE_EVENT_TABLE();

	/**OK button press handler. Saves current selection of databases*/
	void OnOKButton(wxCommandEvent& event);
	/**Cancel button press handler. Hides dialog*/
	void OnCancelButton(wxCommandEvent& event);
	/**CheckListBox for selecting database to synchronize*/
	wxCheckListBox* m_databases_list_box;
	/**CheckBox for delete_option*/ 
	wxCheckBox* m_delete_box;
	/**List of selected databases*/
	wxArrayString m_selected_databases;
	/**List of local databases*/
	wxArrayString m_databases;
	/**List of local databases*/
	std::map<wxString, wxString> m_database_server_map;
	/**Config titles*/
	ConfigNameHash m_config_titles;
	/**Loads selected databases from configuration*/
	void LoadConfiguration();
	/**Saves selected databases in configuration*/
	void StoreConfiguration();
	/**Gets list of local databases*/
	void LoadDatabases();
public:
	/**Gets list of selected directories*/
	wxArrayString GetDirs();
	/**Delete option*/
	bool GetDeleteOption() {return m_delete_box->GetValue();};
	/**Default constructor*/
	SSCSelectionFrame();
};

/**class displaying icon on a taskbar
 * and context menu upon mouse click*/
#ifdef __WXMSW__
class SSCTaskBarItem : public BalloonTaskBar {
#else
class SSCTaskBarItem : public szTaskBarItem {
#endif
	DECLARE_EVENT_TABLE();

	/**pointer to @see SSCConfigFrame*/
	SSCConfigFrame *m_cfg_frame;
	/**client class @see Client*/
	Client* m_client;
	/**frame displaying synchronization progress*/
	ProgressFrame *m_progress_frame;
	/**@see Progress*/
	Progress *m_progress;
	/**Timer triggering synchronization process*/
	wxTimer* m_timer;
	/**Latest synchronization time*/
	wxDateTime m_last_sync_time;
	/**SZARP root directory*/
	wxString m_szarp_dir;
	/**Context help controller*/
	szHelpController* m_help;
	/**Log Dialog*/
	wxLogWindow* m_log;
	/**Creates context menu*/
	wxMenu* CreateMenu();
	/**Handles event generated by "Close application" menu item*/
	void OnCloseMenuEvent(wxCommandEvent &event);
	/**Handles event generated by "Start synchronization" menu item,
	 * Starts synchronization*/
	void OnBeginSync(wxCommandEvent &event);
	/**Handles event generated by "Synchronize selected databases" menu item,
	 * shows dialog for databases selection and starts sybchronization*/
	void OnSyncSelected(wxCommandEvent &event);
	/**Handles event generated by "Configuration" menu item*/
	void OnConfiguration(wxCommandEvent &event);
	/**Handles event generated by "Configuration" menu item*/
	void OnHelp(wxCommandEvent &event);
	/**Handles event generated by "About" menu item*/
	void OnAbout(wxCommandEvent &event);
	/**Handles event generated by "Log" menu item*/
	void OnLog(wxCommandEvent &event);
	/**timer event handler.*/
	void OnTimer(wxTimerEvent& event);
	/**handler for left mouse button click event.
	 * Show context menu*/
	void OnMouseDown(wxTaskBarIconEvent &event);
public:
	SSCTaskBarItem(wxString szarp_dir);
};

/**Implements EVT_WIZARD_CANCEL event handler*/
class SSCWizardFrameCancelImpl : public wxWizardPageSimple {
public:
	SSCWizardFrameCancelImpl(wxWizard *wizard);
	/**handler for EVT_WIZARD_CANCEL, asks user 
	 * for confirmation*/
	void OnWizardCancel(wxWizardEvent &event);
	virtual ~SSCWizardFrameCancelImpl();

	DECLARE_EVENT_TABLE()
};

/**configuration wizard, first frame,
 * just displays welcome message*/
class SSCFirstWizardFrame : public SSCWizardFrameCancelImpl {
public:
	SSCFirstWizardFrame(wxWizard *parent);
};

/**configuration wizard, second frame
 * asks for username and password*/
class SSCSecondWizardFrame : public SSCWizardFrameCancelImpl {
public:
	SSCSecondWizardFrame(wxWizard *parent);
	/**only allows to go forward if @see m_user_ctrl,
	 * @m_password_ctrl and @m_server_ctrl widgets contain some text*/
	void OnWizardPageChanging(wxWizardEvent& event);
private:
	wxTextCtrl* m_user_ctrl;
	wxTextCtrl* m_password_ctrl;
	wxTextCtrl* m_server_ctrl;
	DECLARE_EVENT_TABLE()
};

/**configuration wizard, third frame
 * displays "you can now start using application.." text*/
class SSCThirdWizardFrame : public SSCWizardFrameCancelImpl {
public:
	SSCThirdWizardFrame(wxWizard *parent);
};

/**
 * main application class*/
class SSCApp : public szApp {

	/**locale handling object*/
	wxLocale locale;
	/**checks if other instances of the application are running*/
	szSingleInstanceChecker* m_single_instance_check;
	/**pointer to @see SSCTaskBarItem*/
	SSCTaskBarItem* m_taskbar;
	/**parses command line parameters. Returns true if 
	 * parse was suncessfull*/
	bool ParseCommandLine();
	/**Initalizes locale*/
	void InitLocale();
	/**method called upon application start*/
	bool OnInit();
	/**runs configuration wizard
	 * @return true if aplication was configured by a user false otherwise*/
	bool RunWizard();
	/**method called upon application exit*/
	int OnExit();

	void ConvertConfigToMultipleServers();

};

/**
 * Class responsible for IPC connections
 */ 
class SSCConnection : public wxConnection 
{
	public:
		/**
		 * Sends message to wxSerwer */
		wxChar* SendMsg(const wxChar* msg);
};


/**
 * IPC client class, responsible for connections with scc */
class SSCClient : public wxClient
{
	public:
		/**
		 * Sends "reload menu" request to scc */
		int SendReload();
	private:
		/**
		 * Current connection */
		SSCConnection* connection;
		/**
		 * Connects with scc wxServer */
		SSCConnection* Connect();
};

enum {
	ID_CANCEL_BUTTON = wxID_HIGHEST,
	ID_CFG_FRAME_SERVERS_BOX,
	ID_CFG_FRAME_ADDRESS_CTRL,
	ID_CFG_FRAME_USER_CTRL,
	ID_CFG_FRAME_AUTOSYNC_CTRL,
	ID_CFG_FRAME_SYNCINTERVAL_CTRL,
	ID_CFG_FRAME_SERVER_ADD_BUTTON,
	ID_CFG_FRAME_SERVER_REMOVE_BUTTON,
	ID_CFG_FRAME_SERVER_CHANGE_ADDRESS_BUTTON,
	ID_CFG_FRAME_CHANGE_PASSWORD_BUTTON,
	ID_CFG_FRAME_OK_BUTTON,
	ID_CFG_FRAME_CANCEL_BUTTON,
	ID_SELECTION_FRAME_OK_BUTTON,
	ID_SELECTION_FRAME_CANCEL_BUTTON,
	ID_START_SYNC_MENU,
	ID_SYNC_SELECTED_MENU,
	ID_CONFIGURATION_MENU,
	ID_HELP_MENU,
	ID_ABOUT_MENU,
	ID_LOG_MENU,
	ID_CLOSE_APP_MENU,
	ID_CONFIG_OK_BUTTON,
	ID_PASSWORD_CTRL,
	ID_PROGRESS_EVNT_ORIG,
	ID_TIMER
};

#define DEFAULT_SERVER_ADDRESS _T("bazy.szarp.com.pl")

#endif /* __SSCLIENT_H__ */
