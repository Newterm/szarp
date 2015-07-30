/* 
  SZARP: SCADA software 

*/
/*
 * IPK

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * XML config for SZARP line daemons.
 * 
 * $Id$
 */

#ifndef __DMNCFG_H__
#define __DMNCFG_H__

#ifndef MINGW32

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/** All extra elements for configuring daemon should have this namespace. */
#define IPKEXTRA_NAMESPACE_STRING "http://www.praterm.com.pl/SZARP/ipk-extra"

#include "szarp_config.h"

/**
 * This class implements configuration loader for SZARP line daemon. It loads
 * configuration from 3 different sources:
 * - szarp.cfg file - general config directives are read from this file, you
 *   can set section name.
 * - command line - only limited set of command line arguments is supported:
 *   '--single', '-s' or 's' (backward compatibility) - run in single mode,
 *   following arguments are parsed;
 *   '--help', '-h' - print usage information, ignore following arguments;
 *   <device number> - index of device in params.xml; following arguments are
 *   ignored;
 *   libpar and liblog command line parameters;
 *   As a result, unless you want single mode or make libpar/liblog variables
 *   initialization, only one argument is parsed.
 *   The default is to print usage info if no arguments are supplied.
 * - params.xml file - standard set of IPK attributes is read, you can access
 *   them directly with IPK API. Aditionally, direct access to XML is
 *   possible, especially for 'device' element, so you can use libxml2 DOM API
 *   to read extra data.
 */
class DaemonConfig {
public:
	/**holds information on unit asscociated with daemon*/
	class UnitInfo {
		/**unit identifier*/
		char m_id;

		/**number of params sent to the unit*/
		int m_send_count;

		/**pointer to next unit on units' list*/
		UnitInfo* m_next;

		long m_sender_msg_type;
	public:
		UnitInfo(TUnit * unit);

		/**@return next unit info on a list*/
		UnitInfo* GetNext();

		/**@return unit identifier*/
		char GetId();

		/**@return number of params sent to this unit*/
		int GetSendParamsCount();
		
		/**sets next object on units' list*/
		void SetNext(UnitInfo *unit);

		long GetSenderMsgType();

		~UnitInfo();
	};

	/** Default constructor. 
	 * @param daemon name, used for logging and as a szarp.cfg section
	 * name, cannot be NULL; string is copied and deleted in 
	 * destructor, so you can free memory. */
	DaemonConfig(const char *);
	/** Destructor. */
	virtual ~DaemonConfig();
	/** Set string printed at the begining of usage info. The default one
	 * is 'SZARP line daemon\n\n'. Copy of string is made. There is no
	 * point to use this method after call to Load() - this is checked
	 * with assertion.
	 * @param header usage info header, use NULL to fall back to default
	 */
	virtual void SetUsageHeader(const char *header);
	/** Set string printed at the end of usage info. The default one
	 * is '\n'. Copy of string is made. There is no point to use this 
	 * method after call to Load() - this is checked with assertion.
	 * @param footer usage info footer, use NULL to fall back to default
	 */
	virtual void SetUsageFooter(const char *footer);
	/** Read configuration - parses command line, loads values
	 * from szarp.cfg, loads IPK configuration, initializes logging.
	 * This method can be called only once (as it modifies command line 
	 * arguments) - there's assertion for that!. After calling this method, 
	 * if no errors occured, you can use Get* methods (assertion again!). 
	 * Rembember to make copies of accessed values if you need them, as 
	 * they are destroyed in DaemonConfig destructor (or don't destroy
	 * DaemonConfig object).
	 * @param argc pointer to main() argc argument
	 * @param argv main() argv argument
	 * @param libpardone should we call libpar_done() ? - use 0 if you
	 * @param sz_cfg pointer to szarp configuration. If NULL, configuration
	 * is read from file
	 * @param force_device_index if >=0 device index is not taken from
	 * command line but set to force_device_index. Works only for LoadNotXML
	 * @param if async logging library is to be used, a proper context
         *  (with current implementation it should be pointer to libevent_base) should
         *  be provided here if no context is provided - synchronous logging will be used
	 *
	 * want to read extra libpar params - you need to call libpar_done
	 * on your own
	 * @return 0 on success, 1 on error (you should exit), 2 if usage
	 * info was printed (you should exit)
	 */
	virtual int Load(int *argc, char **argv, int libpardone = 1 , TSzarpConfig* sz_cfg = NULL , int force_device_index = -1, void* async_logging_context = NULL);
	/** Returns number of daemon's line. All Get* functions must be called
	 * AFTER successfull call to Load() - otherwise assertion fails. */
	int GetLineNumber();
	/** Return pointer to IPK configuration info. */
	TSzarpConfig* GetIPK();
	/** Return pointer to device element for daemon. You can use methods
	 * like GetParamsCount(), GetPath(), GetSpeed(), GetStopBits(),
	 * GetProtocol(), GetFirstRadio(), GetNextRadio(), IsSpecial(),
	 * GetOptions(). @see TDevice class. Returns NULL if configuration
	 * was not loaded at user's request or CloseIPK was called.
	 */
	TDevice* GetDevice();

	/** Return XML document containing only this device data
	 * all root and document attributes are copied as well.
	 */
	xmlDocPtr GetDeviceXMLDoc();

	/** Return XML string containing only this device data */
	std::string GetDeviceXMLString();

	/** Get XML string printable e.g. by SZARP logger */
	std::string GetPrintableDeviceXMLString();

	/** Return pointer to whole XML document. */
	xmlDocPtr GetXMLDoc();
	/** Return pointer to XML node with daemon configuration. Search for
	 * elements with "http://www.praterm.com.pl/SZARP/ipk-extra"
	 * namespace. Return NULL if configuration is not loaded at user's 
	 * request.
	 */
	xmlNodePtr GetXMLDevice();
	/** Return pointer to parcook path (IPC identifiers key). */
	const char* GetParcookPath();
	/** Return pointer to lineX.cfg path (IPC identifiers key). */
	const char* GetLinexPath();
	/** Return 1 in 'single' mode, 0 otherwise. */
	int GetSingle();
	/** Return 1 in 'diagno' mode, 0 otherwise. */
	int GetDiagno();
	/** This method frees loaded IPK document. After calling this method,
	 * GetXMLDoc() and GetXMLDevice() return NULL. Should be called after
	 * loading configuration because, to reduce daemon memory usage.
	 * @param clean_parser if non-zero, xmlCleanupParser() called. You
	 * should set it to 0 if you still want to use libxml2 functions in
	 * your code.
	 */
	void CloseXML(int clean_parser = 1);

	/** This method frees loaded IPK object. After calling this method,
	 * GetDevice() and GetIPK() return NULL. Should be called after
	 * loading configuration to reduce daemon memory usage.
	 */
	void CloseIPK();

	/**@return object holding info on first unit associated with a daemon
	 * (this is also first element of the units info's list)*/
	UnitInfo* GetFirstUnitInfo();

	const char* GetDevicePath();

	std::string& GetIPKPath();

	/** Returns port speed. Returns either value supplied by 
	 * in command line arguments or, if configuraion is loaded,
	 * path specified in the configuration.
	 */
	int GetSpeed();

	/**Returns 1 if user requested to not to load configuration, 0 otherwise*/
	int GetNoConf();

	/**Returns 1 if user requested that all communication is between
	 * daemon and a device should be printed in hex terminal format, 0 otherwise*/
	int GetDumpHex();

	/**Returns 1 if user requested that daemon should work as scanner in that case
	 * id1 and id2 contain start and end of ids range*/
	int GetScanIds(char *id1, char* id2);

	/**Returns 1 if daemon should work as sniffer*/
	int GetSniff();

	/**Returns delay between querying units*/
	int GetAskDelay();

protected:
	/** Parses command line parameters. Internal use.
	 * @return 0 on success, 1 on error or if usage info was printed
	 */
	int ParseCommandLine(int argc, char**argv);
	/** Loads IPK configuration from XML file. Internal use.
	 * @return 0 on success, 1 on error
	 */
	int LoadXML(const char *path);
	/** 
	 * @brief Tries to load DaemonConfig not from XML file.
	 *
	 * After calling this method DaemonConfig will be loaded
	 * as much as possible without reading XML file. Not all
	 * functions will be available after loading in such way
	 * some of them may fail at assert.
	 * 
	 * @param cfg previously loaded szarp configuration
	 * @param device_id id of device that should be loaded
	 * 
	 * @return 0 on success and 1 on error
	 */
	int LoadNotXML( TSzarpConfig* cfg , int device_id );

	/**Inits UnitsInfo. IPK shall be loaded when this method is called*/
	void InitUnits();
	
private:
	UnitInfo* m_units;

	std::string m_daemon_name;	/**< name of daemon, used for logging and
				  as libpar section name */
	std::string m_usage_header;	/**< string printed at the begining of
				  usage info */
	std::string m_usage_footer;	/**< string printed at the end of usage info */

	bool m_load_called;	/**< true if Load() method was already called */
	bool m_load_xml_called;	/**< true if LoadXML() method was already called */

	std::string m_parcook_path;	/**< path to parcook config file, used for IPC
				  identifiers */
	std::string m_linex_path;	/**< path to lineX.cfg file, used for IPC
				  identifiers */
	std::string m_ipk_path; /**< path to params.xml file */
	xmlDocPtr m_ipk_doc;	/**< IPK configuration as XML document */
	xmlNodePtr m_ipk_device;/**< XML 'device' element for daemon */
	TSzarpConfig *m_ipk;	/**< pointer to IPK configuration */ 
	TDevice *m_device_obj;	/**< pointer to IPK device object */

	int m_single;		/**< 1 if signle mode*/
	int m_diagno;		/**< 1 if diagno mode*/
	int m_device;		/**< device numer*/
	int m_noconf;		/**< true if configuration is not loaded*/
	std::string m_device_path;	/**< path to device given in command line arguments*/
	int m_scanner;		/**< true if device should act as scanner*/
	char m_id1,m_id2;	/**< range of ids that driver should scan*/
	int m_sniff;		/**< 1 if daemon should work in sniffer mode*/
	int m_speed;		/**< speed of port(if provided in command line arguments)*/
	int m_askdelay;		/**< minium delay between querying units*/
	int m_dumphex;		/**< 1 if user requested to print data in hex terminal format*/

	// for xmlCopyNode (all work) and xmlCopyDoc (only 0/nonzero distinction)
	static const int SHALLOW_COPY = 0;
	static const int DEEP_COPY = 1;
	static const int COPY_ATTRS = 2;
};

#endif /* MINGW32 */

#endif /* __DMNCFG_H__ */

