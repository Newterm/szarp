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
#include "config_info.h"
#include "argsmgr.h"


class DaemonConfigInfo {
public:
	virtual std::vector<UnitInfo*> GetUnits() const = 0;
	virtual const IPCInfo& GetIPCInfo() const = 0;
	virtual bool GetSingle() const = 0;
	virtual std::string GetIPKPath() const = 0;
	virtual int GetLineNumber() const = 0;
	virtual size_t GetParamsCount() const = 0;
	virtual size_t GetSendsCount() const = 0;
	virtual size_t GetFirstParamIpcInd() const = 0;
	virtual std::vector<size_t> GetSendIpcInds() const = 0;

	virtual std::string GetPrintableDeviceXMLString() const = 0;
	virtual struct timeval GetDeviceTimeval() const = 0;
};

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
class DaemonConfig: public DaemonConfigInfo {
	std::vector<UnitInfo*> m_units;

public:
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
	// [deprecated]
	virtual void SetUsageHeader(const char *header);
	/** Set string printed at the end of usage info. The default one
	 * is '\n'. Copy of string is made. There is no point to use this 
	 * method after call to Load() - this is checked with assertion.
	 * @param footer usage info footer, use NULL to fall back to default
	 */
	// [deprecated]
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
	virtual int Load(const ArgsManager& args_mgr, TSzarpConfig* sz_cfg = NULL , int force_device_index = -1, void* async_logging_context = NULL);
	// [deprecated]
	virtual int Load(int *argc, char **argv, int libpardone = 1 , TSzarpConfig* sz_cfg = NULL , int force_device_index = -1, void* async_logging_context = NULL);
	/** Returns number of daemon's line. All Get* functions must be called
	 * AFTER successfull call to Load() - otherwise assertion fails. */
	int GetLineNumber() const;
	/** Return pointer to IPK configuration info. */
	TSzarpConfig* GetIPK() const;
	/** Return pointer to device element for daemon. You can use methods
	 * like GetParamsCount(), GetPath(), GetSpeed(), GetStopBits(),
	 * GetOptions(). @see TDevice class. Returns NULL if configuration
	 * was not loaded at user's request or CloseIPK was called.
	 */
	TDevice* GetDevice() const;
	struct timeval GetDeviceTimeval() const { return m_timeval; }

	/** Return XML document containing only this device data
	 * all root and document attributes are copied as well.
	 */
	xmlDocPtr GetDeviceXMLDoc() const;

	/** Return XML string containing only this device data */
	std::string GetDeviceXMLString() const;

	/** Get XML string printable e.g. by SZARP logger */
	std::string GetPrintableDeviceXMLString() const;

	/** Return pointer to whole XML document. */
	xmlDocPtr GetXMLDoc() const;
	/** Return pointer to XML node with daemon configuration. Search for
	 * elements with "http://www.praterm.com.pl/SZARP/ipk-extra"
	 * namespace. Return NULL if configuration is not loaded at user's 
	 * request.
	 */
	xmlNodePtr GetXMLDevice() const;
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

	std::vector<UnitInfo*> GetUnits() const { return m_units; }


	const IPCInfo& GetIPCInfo() const { return ipc_info; }

	const char* GetDevicePath() const;

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

	size_t GetParamsCount() const;
	size_t GetSendsCount() const;

	bool GetSingle() const;
	std::string GetIPKPath() const override;

	size_t GetFirstParamIpcInd() const override { return m_ipk->GetFirstParamIpcInd(*m_device_obj); }
	std::vector<size_t> GetSendIpcInds() const override { return m_ipk->GetSendIpcInds(*m_device_obj); }

protected:
	/** Parses command line parameters. Internal use.
	 * @return 0 on success, 1 on error or if usage info was printed
	 */
	int ParseCommandLine(int argc, char**argv);
	void ParseCommandLine(const ArgsManager&);
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

	void InitUnits(TUnit* unit);
	
private:
	IPCInfo ipc_info;

	std::string m_daemon_name;	/**< name of daemon, used for logging and
				  as libpar section name */
	std::string m_usage_header;	/**< string printed at the begining of
				  usage info */
	std::string m_usage_footer;	/**< string printed at the end of usage info */

	bool m_load_called;	/**< true if Load() method was already called */
	bool m_load_xml_called;	/**< true if LoadXML() method was already called */

	std::string m_linex_path;	/**< path to lineX.cfg file, used for IPC
				  identifiers */
	xmlDocPtr m_ipk_doc;	/**< IPK configuration as XML document */
	xmlNodePtr m_ipk_device;/**< XML 'device' element for daemon */
	TSzarpConfig *m_ipk;	/**< pointer to IPK configuration */ 
	TDevice *m_device_obj;	/**< pointer to IPK device object */

	std::string ipk_path;

	struct timeval m_timeval{10, 0};

	int m_single;		/**< 1 if signle mode*/
	int m_diagno;		/**< 1 if diagno mode*/
	int m_device;		/**< device numer*/
	int m_noconf;		/**< true if configuration is not loaded*/
	mutable std::string m_device_path;	/**< path to device given in command line arguments*/
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

