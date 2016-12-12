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
/*
 * SZARP IPK header file
 *  2002 Pawe³ Pa³ucha
 * email: pawel@praterm.com.pl
 *
 * $Id$
 */

#ifndef __SZARP_CONFIG_H__
#define __SZARP_CONFIG_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <vector>
#include <map>
#include <tr1/unordered_map>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <string>
#include <set>

#include <stdio.h>
#include <assert.h>
#include <libxml/tree.h>
#ifndef NO_LUA
#include <lua.hpp>
#endif

#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>

#include "szbase/szbdefines.h"
#include <libxml/xmlreader.h>

#define BAD_ORDER -1.0

#define IPK_NAMESPACE_STRING L"http://www.praterm.com.pl/SZARP/ipk"

#define MAX_PARS_IN_FORMULA 160
#define SZARP_NO_DATA -32768

class TDevice;
class TRadio;
class TUnit;
class TParam;
class TSendParam;
class TValue;
class TRaport;
class TDraw;
class TBoiler;
class TAnalysis;
class TAnalysisInterval;
class TAnalysisParam;
class TSSeason;
class TDictionary;
class TTreeNode;
class TDrawdefinable;
class TScript;
class TDefined;
class TBoilers;
class XMLWrapper;
class XMLWrapperException;


class TCheckException : public std::exception {
	virtual const char* what() const throw()
	{ return "Invalid params.xml definition. See log for more informations."; }
};

/** SZARP system configuration */
class TSzarpConfig {
public:
	/** 
	 * @brief constructs empty configuration. 
	 * 
	 * @param logparams if true virtual logging parameters
	 * will be created while loading XML
	 */
	TSzarpConfig( bool logparams = true );
	virtual ~TSzarpConfig(void);

	/**
	 * @return title of configuration.
	 */
	const std::wstring& GetTitle();

	const std::wstring& GetDocumentationBaseURL();

	const std::wstring& GetTranslatedTitle() { return !translatedTitle.empty() ? translatedTitle : title; }

	const std::wstring& GetNativeLanguage() { return nativeLanguage; }

	void SetTranslatedTitle(const std::wstring &t) { translatedTitle = t; }

	/**
	 * @param title set title of configuration.
	 * @param prefix set prefix of configuration.
	 */
	void SetName(const std::wstring& title, const std::wstring& prefix);

	/** 
	 * @brief Specify if loggerparams should be added after XML load
	 * @param _logparams if true, params will be added
	 */
	void SetLogParams( bool _logparams ) { logparams = _logparams; }

	/**
	 * @return prefix of configuration
	 * (NULL if this information is not available)
	 */
	const std::wstring& GetPrefix() const;
	/**
	 * @return read_freq attribute value
	 */
	int GetReadFreq() { return read_freq; }
	/**
	 * @return send_freq attribute value
	 */
	int GetSendFreq() { return send_freq; }
	/**
	 * Returns param with given number (IPC index).
	 * @param ipc_ind index (from 0)
	 * @return pointer to param, NULL when not found
	 */
	TParam *getParamByIPC(int ipc_ind);
	/**
	 * Return param with given name (searches in all params, including
	 * defined and 'draw definable').
	 * @param name name of param to find
	 * @return pointer to param, NULL if not found
	 */
	TParam *getParamByName(const std::wstring& name);
	/**
	 * Return param with given base index (searches in all params, including
	 * defined and 'draw definable').
	 * @param base_ind base index of param to find
	 * @return pointer to param, NULL if not found
	 */
	TParam *getParamByBaseInd(int base_ind);
	/**
	 * Loads config information from XML file.
	 * @param path to input file
	 * @param prefix prefix name of configuration
	 * @return 0 on success, 1 on error
	 */
	int loadXML(const std::wstring& path, const std::wstring& prefix = std::wstring());
	/**
	 * Loads config information from XML file - uses XML DOM.
	 * @param path to input file
	 * @param prefix prefix name of configuration
	 * @return 0 on success, 1 on error
	 */
	int loadXMLDOM(const std::wstring& path, const std::wstring& prefix = std::wstring());
	/**
	 * Loads config information from XML file - uses xmlTextReaderPtr.
	 * @param path to input file
	 * @param prefix prefix name ofconfiguration
	 * @return 0 on success, 1 on error
	 */
	int loadXMLReader(const std::wstring& path, const std::wstring& prefix = std::wstring());
	/**
	 * Load configuration from already parsed XML document.
	 * @param doc parsed XML document
	 * @return 0 on success, 1 on error (call xmlLineNumbersDefault(1) to
	 * see line numbers in error messages)
	 */
	int parseXML(xmlDocPtr doc);
	/** Parses XML node (current set in reader).
	 * @param reader XML reader set on params
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlTextReaderPtr reader);
	/**
	 * Initalize definable parameters.
	 * Creates formula cache, checks for N function and const params.
	 */
	int PrepareDrawDefinable();
	/**
	 * Generate XML version of configuration file. Before using this method
	 * XML library should be initilized.
	 * @return pointer to created XML document, NULL on error
	 */
	xmlDocPtr generateXML(void);
	/**
	 * Saves XML version of configuration to a file.
	 * @param path file to create
	 * @return number of bytes written or -1 on error
	 */
	int saveXML(const std::wstring& path);
	/**
	 * Returns pointer to first param in config.
	 * @return pointer to first param, NULL if there are no params
	 */
	TParam* GetFirstParam();
	/**
	 * Return pointer to next param after given. Iterates all params in all
	 * devices and also defined.
	 * @param p current param
	 * @return pointer to param after current, NULL if current is last
	 */
	TParam* GetNextParam(TParam* p);

	/** @return pointer to first defined param, NULL if no one exists */
	TParam* GetFirstDefined() { return defined; }

	/** @return pointer to first drawdefinable param, NULL if no one exists */
	TParam* GetFirstDrawDefinable() { return drawdefinable; }

	/**
	 * @param id identifier of line (is equal to line number + 1 - starts
	 * from 1)
	 * @return pointer to device with given id.
	 */
	TDevice* DeviceById(int id);

	/** @return number of all defined params (all with not null 'formula'
	 * attribute and defined) */
	int GetAllDefinedCount();

	/** @return the biggest base index value among all params, -1 if no
	 * params with base index exist; base index is always non-negative,
	 * index of first param is 0 */
	int GetMaxBaseInd();

	/** @return number of devices */
	int GetDevicesCount();

	/** @return number of all params in devices (without defined and
	 * 'draw definable') */
	int GetParamsCount();

	/** @return number of defined params */
	int GetDefinedCount();

	/** @return number of 'draw definable' params */
	int GetDrawDefinableCount();

	/** @return first element of devices list, NULL if no one exists */
	TDevice* GetFirstDevice() { return devices; }

	/** adds new device at the end of devices list */
	TDevice* AddDevice(TDevice *d);
	/** @return next list element after given, NULL if 'd' is last on list
	 */
	TDevice* GetNextDevice(TDevice* d);
	/** @return title of first raport */
	std::wstring GetFirstRaportTitle();
	/** @return title of next raport */
	std::wstring GetNextRaportTitle(const std::wstring& cur);
	/** @return first raport item with given title */
	TRaport* GetFirstRaportItem(const std::wstring& title);
	/** @return next raport item with title same as that of current item */
	TRaport* GetNextRaportItem(TRaport* cur);

	/** @return first boiler on a boilers' list */
	TBoiler* GetFirstBoiler();

	/** adds new boiler at the end of boilers list */
	/*@param boiler boiler to be added
	 *@return added boiler*/
	TBoiler* AddBoiler(TBoiler* boiler);

	/** Adds new defined param. */
	void AddDefined(TParam *p);

	/** Removes defined param. */
	void RemoveDrawDefinable(TParam *p);

	/** Adds new draw-definable param. */
	void AddDrawDefinable(TParam *p);

	/**
	 * Substitues wildcards in param name.
	 * @param name name with possible wildcards
	 * @param ref reference name
	 * @return absolute name
	 */
	std::wstring absoluteName(const std::wstring& name, const std::wstring& ref);

	/**
	 * Check if configuration is valid. Basicly calls all check* functions
	 */
	bool checkConfiguration();

	/**
	 * Check if formulas are well defined
	 */
	bool checkFormulas();

	/**
	 * Check if formulas are correct in mean of lua syntax
	 */
	bool checkLuaSyntax(TParam *);
	bool compileLuaFormula(lua_State *, const char *, const char *);

	/**
	 * Check if formulas are correct in mean of SZARP optimalization
	 */
	bool optimizeLuaParam(TParam *);

	/** Checks if repetitions of parameters names in params.xml occurred
	 * @param quiet if equal 1 do not print anything to standard output
	 * @return 0 if there is no repetitions or 1 if there are some
	 */
	bool checkRepetitions(int quiet = 0);

	/**
	 * Check if sends are ok -- this means if every send has valid param
	 */
	bool checkSend();

	/** Configrures default summer seasons limit (if seasons are not already configured*/
	void ConfigureSeasonsLimits();

	/** @return object representing seasons limit's
	 * NULL if no seasons configuration is present*/
	const TSSeason* GetSeasons() const;

	/** TODO: comment */
	const std::wstring& GetPSAdress() { return ps_address; }

	/** TODO: comment */
	const std::wstring& GetPSPort() { return ps_port; }

	void CreateLogParams();

	unsigned GetConfigId() const { return config_id; }

	void SetConfigId(unsigned _config_id) { config_id = _config_id; }

	TDevice* createDevice(const std::wstring& _daemon = std::wstring(), const std::wstring& _path = std::wstring());

	TRadio* createRadio(TDevice* parent, wchar_t _id = 0, TUnit* _units = NULL);

	TUnit* createUnit(TRadio* parent, wchar_t _id = 0, int _type = 0, int _subtype = 0);

	void UseNamesCache();

	void AddParamToNamesCache(TParam* _param);

protected:
	/**
	 * Adds new defined parameter. Adds parameter with given formula at
	 * and of defined params table.
	 * @param formula formula in IPK format, only pointer is passed (string
	 * is not copied)
	 * @return number of parameter (from 0)
	 */
	int AddDefined(const std::wstring &formula);

	unsigned config_id;
			/**< Configuration unique numbe id */
	int read_freq;	/**< Param reading frequency (in seconds) */
	int send_freq;
			/**< Param send frequency (in seconds) */
	TDevice *devices;
			/**< List of devices info */
	TParam *defined;
			/**< List of defined parameters */
	TParam *drawdefinable;
			/**< List of DRAW 2.1 'definable' parameters */
	std::wstring title;	/**< Configuration title */

	std::wstring translatedTitle;

	std::wstring prefix;	/**< configuration prefix*/

	std::wstring nativeLanguage;

	std::wstring documentation_base_url;

	TBoiler *boilers;/**< List of boilers for analysis*/

	TSSeason *seasons;/**< Summer seasons configuration*/

	std::wstring ps_address; /**< Parameter Setting server address */

	std::wstring ps_port;	/**< Parameter Setting server port*/

	bool logparams; /**< specify if logging parameters should be created */
	
	size_t device_counter; /**< numer of created TDevice objects */

	size_t radio_counter; /**< numer of created TRadio objects */

	size_t unit_counter; /**< numer of created TUnit objects */

	bool use_names_cache; /**< flag, if set std::map will be used for fast searching params by name */

	std::map<std::wstring, TParam *> params_map; /**< map for params search by name */
};


/** Device description */
class TDevice {
public:
	TDevice(size_t _number, TSzarpConfig *parent, const std::wstring& _daemon = std::wstring(), const std::wstring& _path = std::wstring(),
			int _speed = -1, int _stop = -1, int _protocol = -1,
			const std::wstring& _options = std::wstring(), bool _parcookDevice = true) :
		number(_number),
		parentSzarpConfig(parent),
		daemon(_daemon), path(_path), speed(_speed),
		stop(_stop),
		protocol(_protocol), special(0), special_value(0), options(_options),
		radios(NULL), next(NULL), parcookDevice(_parcookDevice)
	{ }
	/** Destroy whole list. */
	~TDevice();
	/**
	 * Generates XML node with device info.
	 * @return created XML node, NULL on error
	 */
	xmlNodePtr generateXMLNode(void);
	/**
	 * Searches for unit with given identifier within line.
	 * @param id unit identifier
	 * @param uniq if not 0, method checks if there's only one unit with
	 * given id
	 * @return unit found, NULL if unit is not found or uniq is not 0 and
	 * there are more units with given id
	 */
	TUnit* searchUnitById(const wchar_t id, int uniq);
	/** @return index of device, from 0 */
	int GetNum();
	/** @return number of params in device */
	int GetParamsCount();
	/** @return path to device daemon */
	const std::wstring& GetDaemon()
	{
		return daemon;
	}
	/** @return path to device */
	const std::wstring& GetPath() {
		return path;
	}
	/** @return port speed (in bauds) */
	int GetSpeed() {
		return speed;
	}
	/** @return number of stop bits (1 or 2), -1 if attribute is not
	 * defined */
	int GetStopBits()
	{
		return stop;
	}
	/** @return protocol version id (0 or 1), -1 if attribute is not
	 * defined */
	int GetProtocol()
	{
		return protocol;
	}
	/** @return pointer to first radio object, after proper initialization
	 * should not be NULL */
	TRadio* GetFirstRadio()
	{
		return radios;
	}
	/** Add radio to device */
	TRadio* AddRadio(TRadio* r);
	/** @return next list element, NULL if current is last */
	TDevice* GetNext() {
		return next;
	}
	/**
	 * @param radio pointer to current radio object
	 * @return pointer to next radio object after current, NULL if no more
	 * are available
	 */
	TRadio* GetNextRadio(TRadio *radio);
	/** @return number of radios in device (after proper initializations
	 * should be grater then 0) */
	int GetRadiosCount();
	/** @return pointer to parent TSzarpConfig object */
	TSzarpConfig* GetSzarpConfig() const
	{
		return parentSzarpConfig;
	}
	/** @return true 1 if it's simplified RS-232, without units
	 * addresing or some other device, which need special
	 * value in line<x>.cfg. This value can be accessed by GetSpecial()
	 * method. */
	bool IsSpecial() {
		return special;
	}
	/** @return special value
	 * @see IsSpecial */
	int GetSpecial()
	{
		return special_value;
	}
	/** @return options attribute may be empty */
	const std::wstring& GetOptions() {
		return options;
	}
	/**
	 * Appends new device at end of list.
	 * @param d new element to append to list
	 * @return pointer to appended element (d)
	 */
	TDevice* Append(TDevice* d);
	/**
	 * Loads information parsed from XML file.
	 * @param node XML node with device configuration info
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlNodePtr node);
	/** Parses XML node (current set in reader).
	 * @param reader XML reader set on device
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlTextReaderPtr reader);
	/** Returns true if the device sends data to parcook via
         * shared memory */
	bool isParcookDevice() const { return parcookDevice; }
protected:
	/**
	 * Returns num'th param from device line.
	 * @param num number of param (from 0)
	 * @return pointer to param, NULL if not found
	 */
	TParam *getParamByNum(int num);

	size_t number;

	TSzarpConfig *parentSzarpConfig;
			/**< Pointer to SzarpConfig object. */
	std::wstring daemon;	/**< Path to daemon responsible for the line, if NULL
			  default should be used */
	std::wstring path;	/**< Path to Unix device, if NULL default should be
			  used (usually /dev/ttyX[n], when [n] is the device
			  number */
	int speed;	/**< Speed (in bauds), if <= 0, default should be used*/
	int stop;	/**< Stop bits, if < 0, default should be used */
	int protocol;	/**< Protocol, if < 0 then default should be used */
	int special;	/**< 1 if it's simplified RS-232, without units
			  addresing or some other device, which need special
			  value in line<x>.cfg. This value is holded in
			  special_value attribute. */
	int special_value;
			/**< Special, non-standard value from first line of
			 * line<x>.cfg. */
	std::wstring options;	/**< Aditional parameters for line daemon, not interpreted. */
	TRadio *radios;	/**< List of radio modems. If there are no radio
			  modems, array contains one dummy TRadio object, which
			  holds comm. units description. */
	TDevice *next;
			/**< Next list element */
	bool parcookDevice; /**< Do parcook need to send params from this device to meaner4. */
};

/**
 * Description of radio line
 */
class TRadio {
public:
	TRadio(size_t _number, TDevice *parent, wchar_t _id = 0, TUnit* _units = NULL) :
		number(_number), parentDevice(parent), id(_id), units(_units), next(NULL)
	{ }
	/** Destroys whole list. */
	~TRadio();
	/**
	 * Generates XML node with radio modem info.
	 * @return created XML node, NULL on error
	 */
	xmlNodePtr generateXMLNode(void);
	/**
	 * Searches for unit with given identifier.
	 * @param c unit's id
	 * @return pointer to unit found, NULL if not found
	 */
	TUnit* unitById(wchar_t c);
	/** @return id attribute - identifier of radio line, NULL if it's not a
	 * really radio modem, just a dummy object - container for
	 * communication units */
	wchar_t GetId()
	{
		return id;
	}
	/** @return next list element, NULL current is last */
	TRadio* GetNext()
	{
		return next;
	}
	/** @return pointer to parent TDevice object */
	TDevice* GetDevice() const
	{
		return parentDevice;
	}
	/** numer of units within radio line (or device in case of dummy radio
	 * object */
	int GetUnitsCount();
	/** @return pointer to first unit in radio line, after proper
	 * initialization should not be NULL */
	TUnit* GetFirstUnit()
	{
		return units;
	}
	/** @param current current radio line
	 * @return pointer to next radio line after current, NULL if current is
	 * last */
	TUnit* GetNextUnit(TUnit* current);
	/** Add new unit */
	TUnit* AddUnit(TUnit* unit);
	/** Appends new element to the end of list.
	 * @param unit new element to append
	 * @return pointer to appended element
	 */
	TRadio* Append(TRadio* unit);
	/**
	 * Loads information parsed from XML file.
	 * @param node XML node with device configuration info
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlNodePtr node);
	/** Parses XML node (current set in reader)
	 * @param reader XML reader set on  "radio"
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlTextReaderPtr reader);
protected:
	size_t number;
	TDevice *parentDevice;
			/**< Pointer to parent device object. */
	wchar_t id;	/**< Identifier used by radio modem, NULL if it's
			  a dummy object (@see TDevice) */
	TUnit *units;	/**< List of communications units */
	TRadio* next;	/**< Next list element */
};

/**
 * Description of communication unit.
 */
class TUnit {
public:
	TUnit(size_t _number, TRadio *parent, wchar_t _id = 0, int _t = 0, int _st = 0,
			int _bs = 0, const std::wstring& _n = std::wstring()) :
		number(_number), parentRadio(parent), id(_id), type(_t), subtype(_st),
		bufsize(_bs), name(_n), params(NULL), sendParams(NULL), next(NULL)
	{ }
	/** Deletes whole list */
	~TUnit();
	/**
	 * Generates XML node with communication unit info.
	 * @return created XML node, NULL on error
	 */
	xmlNodePtr generateXMLNode(void);
	/** @return id of unit (one ASCII character) */
	wchar_t GetId()
	{
		return id;
	}
	/** @return unit's raport type */
	int GetType()
	{
		return type;
	}
	/** @return unit's raport subtype */
	int GetSubType()
	{
		return subtype;
	}
	/** @return pointer to next list element, NULL if current is last */
	TUnit* GetNext() {
		return next;
	}
	/** @return number of params in unit */
	int GetParamsCount();
	/** @return number of send params in unit */
	int GetSendParamsCount();
	/** @return size of av. buffer */
	int GetBufSize()
	{
		return bufsize;
	}
	/** @return pointer to first send param in unit, may be NULL */
	TSendParam* GetFirstSendParam()
	{
		return sendParams;
	}
	/** @param current current send param
	 * @return pointer to next send param after current, NULL if current is
	 * last or NULL */
	TSendParam* GetNextSendParam(TSendParam* current);
	/**
	 * Adds param to unit (at last position).
	 * @param p param to add
	 */
	void AddParam(TParam* p);
	/**
	 * Adds send param to unit (at last position).
	 * @param sp param to add
	 */
	void AddParam(TSendParam* sp);
	/** @return pointer to containing device object */
	TDevice* GetDevice() const;
	/** @return pointer to main TSzarpConfig object */
	TSzarpConfig* GetSzarpConfig() const;
	/**
	 * Appends unit at end of units list.
	 * @param unit unit to append
	 * @return pointer to appended element (u)
	 */
	TUnit* Append(TUnit* unit);
	/** @returns pointer to first param in unit, NULL if no one exists */
	TParam* GetFirstParam()
	{
		return params;
	}
	/** @return pointer to next param after given, NULL if given is last */
	TParam* GetNextParam(TParam* param);
	/**
	 * Loads information parsed from XML file.
	 * @param node XML node with device configuration info
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlNodePtr node);
	/** Parses XML node (current set in reader)
	 * @param reader XML reader set on "unit"
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlTextReaderPtr node);
	/** @return pointer to parent radio object */
	TRadio* GetRadio()
	{
		return parentRadio;
	}
	/** @return additional name associated with the unit*/
	const std::wstring& GetUnitName();

	const std::wstring& GetTranslatedUnitName();

	void SetTranslatedUnitName(const std::wstring& s);

	long GetSenderMsgType()
	{
		return number + 256L;
	}
protected:
	size_t number;

	TRadio *parentRadio;
			/**< Pointer to parent TRadio object. */
	wchar_t id;/**< Ascii character - line identifier */
	int type;	/**< Raport type */
	int subtype;	/**< Raport subtype */
	int bufsize;	/**< Size of buffer for averages counting */
	std::wstring name;     /**< Name of the unit(optional)*/
	std::wstring translated_name; /**< Translated name of the unit(optional)*/
	TParam *params;
			/**< List of input params */
	TSendParam *sendParams;
			/**< List of output parameters */
	TUnit *next;
};

/**
 * Single input parameter description
 */

#ifndef NO_LUA

#if LUA_PARAM_OPTIMISE

namespace LuaExec {
	class Param;
};

#endif

#endif
class TParam {
public:
	typedef enum { NONE, RPN, DEFINABLE
#ifndef NO_LUA
		, LUA_VA, LUA_AV, LUA_IPC
#endif
		} FormulaType;
	typedef enum { P_REAL, P_COMBINED, P_DEFINABLE
#ifndef NO_LUA
		, P_LUA
#endif
		} ParamType;

	typedef enum { SHORT = 0, INT, FLOAT, DOUBLE, LAST_DATA_TYPE = DOUBLE} DataType;

	typedef enum { SECOND = 0, NANOSECOND, LAST_TIME_TYPE  = NANOSECOND } TimeType; 

	typedef enum { 	SZ4_NONE,
			SZ4_REAL,
			SZ4_COMBINED,
			SZ4_DEFINABLE,
			SZ4_LUA,
			SZ4_LUA_OPTIMIZED } Sz4ParamType;
	
	TParam(TUnit *parent,
		TSzarpConfig *parentSC = NULL,
		const std::wstring& formula = std::wstring(),
		FormulaType ftype = NONE,
		ParamType ptype = P_REAL) :
	    _parentUnit(parent),
	    _parentSzarpConfig(parentSC),
	    _name(),
	    _shortName(),
	    _drawName(),
	    _unit(),
	    _values(NULL),
	    _prec(0),
		_has_forbidden(false),
		_forbidden_val(SZARP_NO_DATA),
	    _baseInd(-1),
	    _inbase(0),
	    _formula(formula),
	    _ftype(ftype),
	    _param_type(ptype),
	    _parsed_formula(),
	    _prepared(false),
	    _f_const(false),
	    _f_N(false),
	    _is_new_def(false),
	    _raports(NULL),
	    _draws(NULL),
	    _analysis(NULL),
	    _next(NULL),
	    _szbase_name(),
	    _psc(false),
	    _period(SZBASE_DATA_SPAN),
#ifndef NO_LUA
	    _script(NULL),
	    _lua_function_reference(LUA_NOREF),
            _lua_start_date_time(-1),
	    _lua_start_offset(0),
	    _lua_end_offset(0),
#if LUA_PARAM_OPTIMISE
	    _opt_lua_param(NULL),
#endif
#endif
	    _sum_unit(),
	    _meter_par(false),
	    _sum_divisor(6.),
	    _dataType(SHORT),
	    _timeType(SECOND),
	    _sz4ParamType(SZ4_NONE)
	{ }

	/** Deletes whole list. */
	~TParam();

	/**
	 * Configures param attributes.
	 */
	void Configure(const std::wstring name, const std::wstring shortName, const std::wstring drawName, const std::wstring unit,
			TValue *values, int prec = 0, int baseInd = -1,
			int inbase = -1,
			short forbidden_val = SZARP_NO_DATA);

	/**
	 * Generates XML node with param info.
	 * @return created XML node, NULL on error
	 */
	xmlNodePtr generateXMLNode(void);

	/** @return name attribute, always non NULL */
	const std::wstring& GetName() {	return _name; }

	std::wstring GetGlobalName() const;

	const std::wstring& GetTranslatedName() { return !_translatedName.empty() ? _translatedName : _name; }

	void SetTranslatedName(const std::wstring& tn) { _translatedName = tn; }

	/** @return name of parameter converted to szbase string, always non
	 * NULL */
	const std::wstring& GetSzbaseName();

	/** @param name params short name should not be NULL */
	void SetShortName(const std::wstring& name);

	void SetTranslatedShortName(const std::wstring& name) { _translatedShortName =  name; }

	const std::wstring& GetTranslatedShortName() { return !_translatedShortName.empty() ? _translatedShortName : _shortName; }

	/** @return param short name, after proper initialization
	 * should be empty */
	const std::wstring& GetShortName() { return _shortName; }

	/** @return name of params unit*/
	const std::wstring& GetUnit() {	return _unit; }

	/** @param name params draw name */
	void SetDrawName(const std::wstring& name);

	void SetTranslatedDrawName(const std::wstring& name) { _translatedDrawName = name; }

	const std::wstring& GetTranslatedDrawName() { return !_translatedDrawName.empty() ? _translatedDrawName : _drawName; }

	/** @return name of params draw name, can be NULL */
	const std::wstring& GetDrawName() { return _drawName; }

	/** @return pointer to parent unit object, NULL for defined params */
	TUnit* GetParentUnit() { return _parentUnit; }

	/** @return pointer to param's formula (may be NULL) */
	const std::wstring& GetFormula() { return _formula; }

	const std::wstring& GetSumUnit() { return _sum_unit; }

	bool IsMeterParam() const { return _meter_par; }

	const double& GetSumDivisor() { return _sum_divisor; }

	/**
	 * Sets params formula to given string. Old formula is deleted.
	 * @param f new formula
	 * @param type type of new formula
	 */
	void SetFormula(const std::wstring& f, FormulaType type = RPN);

	/** @return pointer to string with formula in definable.cfg format
	 * do not free it - its handled iternaly by TParam object.
	 * (may be NULL) */
	const std::wstring&  GetDrawFormula() throw(TCheckException);
	/**
	 * Returns parameter's RPN formula in parcook.cfg format (with comment).
	 * @return newly allocated string with formula in parcook format
	 */
	std::wstring GetParcookFormula() throw(TCheckException);

	/** @return type of formula (RPN, DEFINABLE or NONE) */
	FormulaType GetFormulaType() { return _ftype; }

        /** Sets type of formula (RPN, DEFINABLE or NONE) */
        void SetFormulaType(FormulaType type);

	/** @return type of parameter */
	ParamType GetType() { return _param_type; }

	/** @return cache of params used in formula */
	TParam ** GetFormulaCache() { return &(_f_cache[0]); }

	/** @return number of params used in formula */
	int GetNumParsInFormula() { return _f_cache.size(); }

	/** @return 1 if N fuction in formula */
	bool IsNUsed() { return _f_N; }

	bool IsConst() { return (TParam::P_REAL != _param_type) && _f_const; };

	double GetConstValue() { return _f_const_value; };

	/** @return 1 if parameter is definable */
	int IsDefinable() { return TParam::P_REAL != _param_type
#ifndef NO_LUA
		       	&& TParam::P_LUA != _param_type
#endif
				; }

	/** @return 1 if definable param should be calculated in new way */
	bool IsNewDefinable() { return _is_new_def; }

	/** Set param as new defnable. Takes effect only for definable parameters */
	void SetNewDefinable(bool is_new_def);

	/** @return param's ipc index (is equal to index among all params, including
	 * defined and drawdefinable) */
	unsigned int GetIpcInd();

	/** @return param's index in base (non-negative, starting with 0),
	 * negative if param in not writen to codebase base */
	int GetBaseInd() { return _baseInd; }

	/** @return for param with real base index it returns base index, for others it
	 * returns IPC index
	 */
	unsigned int GetVirtualBaseInd();

	/** @return 1 if parameter is in base, 0 if not; for 'auto' base
	 * indexes of new SzarpBase this method returns 1 and GetBaseInd()
	 * returns -1. */
	int IsInBase() { return _inbase; }

	/** @return 1 if param is written to base and has 'auto' base index. */
	int IsAutoBase() { return ((_baseInd < 0) && (_inbase)); }

	/** Sets base index to 'auto'. */
	void SetAutoBase();

	/** @return 1 if param can be read from base, 0 if not; param can be
	 * directly in base or can be defined */
	int IsReadable();

	/** Sets base index for param. No checking for double indexes.
	 * @param index new index for param, if it's less then 0, param is
	 * marked as not written to base. If you wan't to mark param as writen
	 * to base with auto index, @see SetAutoBase().
	 */
	void SetBaseInd(int index);


	/** @return representation presition (number of digits after comma) for
	 * integer params (non-negative), 0 for non-integer values */
	int GetPrec() {	return _prec; }

	/** @return whether has limits */
	short GetForbidden() const { return _forbidden_val; }

	/** @return whether has a forbidden value */
	bool HasForbidden() const { return _has_forbidden; }

	/** @return sets presition (number of digits after comma) for
	 * integer params (non-negative), 0 for non-integer values */
	void SetPrec(int prec) { _prec = prec; }

	/** @return pointer to the first element of possible param values list,
	 * NULL for integer parameters */
	TValue* GetFirstValue()	{ return _values; }

	/** @return pointer to the first element of possible analysis elements list*/
	TAnalysis* GetFirstAnalysis() {	return _analysis; }

	/** Print to string value converted according to parameter precision
	 * and values information. String value or float number or special
	 * string for empty values is printed.
	 * @param buffer allocated output buffer
	 * @param buffer_size size of buffer, output will be truncated to this
	 * size (snprintf is used)
	 * @param value SZBASE_TYPE type value
	 * @param no_data_str string printed when value is equal to
	 * SZB_NODATA
	 */
	void PrintValue(wchar_t *buffer, size_t buffer_size, SZBASE_TYPE value,
			const std::wstring& no_data_str, int prec = 0);

	/** The same as previous function but returns std::wstring*/
	std::wstring PrintValue(SZBASE_TYPE value, const std::wstring& no_data_str, int prec = 0);

	/** The same as PrintValue, but value is assumed to be a raw IPC
	 * value.
	 * @see PrintValue
	 */
	void PrintIPCValue(wchar_t *buffer, size_t buffer_size, SZB_FILE_TYPE value,
			const wchar_t *no_data_str);

	/** Convert SZBASE_TYPE (double) value to SZB_FILE_TYPE (short int)
	 * value according to paramater precision. */
	SZB_FILE_TYPE ToIPCValue(SZBASE_TYPE value);

	/** If params formula is NULL, set it to "null " string. Used for
	 * defined params, to simplify null formulas handling.
	 * TODO: if (formula == NULL) formula = strdup("null ") */
	void CheckForNullFormula();

	/*
	 * Initializes param not defined in PTT.act (without name).
	 * @param id unknown param identifier, must be uniq
	 */
	void ConfigureUnknown(int id);

	/**
	 * Returns n-th element of params list.
	 * @param n number of param to search, if 0 current is returned
	 * @param global if true, global search is activated (through all
	 * devices, radios, lines and defined params)
	 * @return pointer to param found, NULL if not found
	 */
	TParam* GetNthParam(int n, bool global = false);

	/** Append param to the end of params list.
	 * @param p param to append
	 * @return pointer to appended element */
	TParam* Append(TParam* p);

	/** Directly sets given param as next in the params list*/
	void SetNext(TParam *p);

	/** @return pointer to containing radio object, NULL for defined
	 * parameters */
	TRadio* GetRadio();

	/** @return pointer to containing device object, NULL for defined
	 * parameters */
	TDevice* GetDevice() const;

	/** @return pointer to main TSzarpConfig object */
	TSzarpConfig* GetSzarpConfig() const;

	/**
	 * Loads information parsed from XML file.
	 * @param node XML node with device configuration info
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlNodePtr node);
	/** Parses XML node (current set in reader)
	 * @param reader XML reader set on "param"
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlTextReaderPtr node);

	/**
	 * Get next param in list.
	 * @param global if true, global search is activated (through all
	 * devices, radios, lines and defined params)
	 * @return next params list element, NULL if current element is last
	 */
	TParam* GetNext(bool global = false);

	/**
	 * Adds raport info to param.
	 * @param title title of raport
	 * @param descr description of param, if NULL last part of param name
	 * is used
	 * @param order priority of param within raport, default is -1.0
	 */
	void AddRaport(const std::wstring& title, const std::wstring& descr = std::wstring(), double order = -1.0 );

	/**
	 * Returns list of raports for param.
	 * @return param's raports (may be NULL)
	 */
	TRaport* GetRaports() {	return _raports; }

	/**
	 * Returns list of draws for param.
	 * @return pointer to first element of draws' list for param, NULL if
	 * param has no draws
	 */
	TDraw* GetDraws() { return _draws; }

	/*
	 * Adds draw info to param. Draw is appended to the end of draws' list.
	 * @param draw initialized info about param
	 * @return pointer to added draw element
	 */
	TDraw* AddDraw(TDraw* draw);

	/**
	 * Adds analysis item to param. Item is appended to the end of items' list.
	 * @param analysis analysis param description
	 * @return pointer to added analysis item
	 */
	TAnalysis* AddAnalysis(TAnalysis* analysis);

	/** Prepares data for definable calculation. */
	void PrepareDefinable() throw(TCheckException);

#ifndef NO_LUA
	const unsigned char* GetLuaScript() const;

	void SetLuaParamRef(int ref);

	int GetLuaParamReference() const;

	void SetLuaStartDateTime(time_t start_time_t) { _lua_start_date_time = start_time_t; }

	time_t GetLuaStartDateTime() const { return _lua_start_date_time; }

	time_t GetLuaStartOffset() const { return _lua_start_offset; }

	time_t GetLuaEndOffset() const { return _lua_end_offset; }

	void SetLuaScript(const unsigned char* script);

#if LUA_PARAM_OPTIMISE
	LuaExec::Param* GetLuaExecParam();

	void SetLuaExecParam(LuaExec::Param *param);
#endif 
#endif
	void SetName(const std::wstring& name) { assert (_name == std::wstring()); _name = name; }
	/** Get parameter writting period. */
	time_t GetPeriod() { return  _period; }
	/** Set parameter writting period. */
	void SetPeriod(time_t period) { _period = period <= 0 ? SZBASE_DATA_SPAN : period; }
	bool GetPSC() { return _psc; }
	void SetPSC(bool psc) { _psc = psc; }

	void SetParentSzarpConfig(TSzarpConfig *parentSzarpConfig) { _parentSzarpConfig = parentSzarpConfig; }

	DataType GetDataType() const { return _dataType; }

	void SetDataType(DataType dataType) { _dataType = dataType; }

	TimeType GetTimeType() const { return _timeType; }

	void SetTimeType(TimeType timeType) { _timeType = timeType; }

	Sz4ParamType GetSz4Type() const { return _sz4ParamType; }

	void SetSz4Type(Sz4ParamType sz4ParamType) { _sz4ParamType = sz4ParamType; }

	unsigned GetParamId() const { return _paramId; }

	void SetParamId(unsigned paramId) { _paramId = paramId; }

	unsigned GetConfigId() const { return _configId; }

	void SetConfigId(unsigned configId) { _configId = configId; }

	static bool IsHourSumUnit(const std::wstring& unit);
protected:
	TUnit * _parentUnit;  /**< Pointer to parent TUnit object (NULL for defined). */

	TSzarpConfig * _parentSzarpConfig;
			/**< Pointer to main TSzarpConfig object (needed for
			 * definable parameters, which don't have parentUnit.*/

	std::wstring _name;	    /**< Full name */
	std::wstring _translatedName;	    /**< Full name */
	std::wstring _shortName;  /**< Short name */
	std::wstring _translatedShortName;  /**< Short name */
	std::wstring _drawName;   /**< Name for use in DRAW program */
	std::wstring _translatedDrawName;   /**< Name for use in DRAW program */
	std::wstring _unit;	    /**< Symbol of unit */

	TValue * _values;   /**< List of possible values defintions, NULL if it's
			      an integer parameter */

	int _prec;	/**< Precision for integer parameters, for others should be 0 */

	bool _has_forbidden;
	short _forbidden_val; // value to treat as NO_DATA

	int _baseInd;	/**< Index of parameter in data base. Must be uniq among
			  all params. For params not saved in data base should
			 be < 0 (-<index in PTT.act>). */

	int _inbase;	/**< 0 for parameters that are not saved to base, otherwise
			  1; inbase > 0 and baseInd < 0 it means 'auto' base index
			  (only available for SzarpBase base format) */

	unsigned _paramId;

	unsigned _configId;

	std::wstring _formula;    /**< NULL if it's an oridinary parameter, formula
			      for defined parameters. */

	FormulaType _ftype;  /**< Type of formula, NONE, RPN or DEFINABLE */

	ParamType _param_type;	/**< Type of parameter: P_REAL, P_COMBINED, P_DEFINABLE */

	std::wstring	_parsed_formula;    /**< parsed formula for definable calculating */
	std::vector<TParam *> _f_cache;	    /**< formula cache */

	double _f_const_value; /**< const value if formula is const */

	bool _prepared; /**< flag - is definalbe param prepared */
	bool _f_const; /**< flag - is formula const */
	bool _f_N;	    /**< flag - if 1 - N function i formula */
	bool _is_new_def;    /**< flag - if 1 param should be calculated in new way */

	TRaport	* _raports;	/**< List of raports for param */
	TDraw * _draws;		/**< List of draws for param */
	TAnalysis * _analysis;	/**< List of analysis items for param*/

	TParam* _next;	/**< Next list element */

	std::wstring _szbase_name;	/**< Name of parameter converted to szbase format.
				  May be empty - will get converted on next call to
				  GetSzbaseName(). */

	/** Calculate Value of const param */
	double calculateConst(const std::wstring& _formula);
	bool _psc; /**< marks if parameter can be set by psc */

	/** Period of writting parameter to the database */
	time_t _period;

#ifndef NO_LUA
	unsigned char* _script;		/**<lua script describing param*/

	int _lua_function_reference;
				/**<lua param function reference*/

	time_t _lua_start_date_time;
				/**<lua param end time offset*/
	time_t _lua_start_offset;
				/**<lua param start time offset*/
	time_t _lua_end_offset;
				/**<lua param end time offset*/

#if LUA_PARAM_OPTIMISE
	LuaExec::Param* _opt_lua_param;
#endif

#endif
	/** unit that shall be used to display summaried values of this params*/
	std::wstring _sum_unit;

	/** parameter is taken from a meter (always take it's latest value) */
	bool _meter_par;

	/** summaried values of this param shall be divided by this factor*/
	double _sum_divisor;

	/** this parameter data type*/
	DataType _dataType;

	/** this parameter time type, i.e. resolution*/
	TimeType _timeType;

	/** parameter type as classified by Sz4 code*/
	Sz4ParamType _sz4ParamType;
};

/**
 * Info about single node script.
 */
class TScript {
public:
	/** Parses XML node (current set in reader on script) and returns pointer to script.
	 * @param reader XML reader set on "script"
	 * @return script on success or NULL if empty
	 */
	static unsigned char* parseXML(xmlTextReaderPtr reader);
};

/**
 * Info about single node drawdefinable.
 */
class TDrawdefinable {
public:
	/** Parses XML node (current set in reader) and returns pointer to new TParam object.
	 * @param reader XML reader set on "drawdefinable"
	 * @param tszarp pointer to actual TSzarpConfig
	 * @return NULL on error, pointer to newly created TParam object on
	 * success.
	 */
	static TParam* parseXML(xmlTextReaderPtr reader,TSzarpConfig *tszarp);
};

/**
 * Info about single node boilers.
 */
class TBoilers {
public:
	/** Parses XML node (current set in reader) and returns pointer to new TBoiler object.
	 * @param reader XML reader set on "boilers"
	 * @param tszarp pointer to actual TSzarpConfig
	 * @return NULL on error, pointer to newly created TBoiler object on
	 * success.
	 */
	static TBoiler* parseXML(xmlTextReaderPtr reader, TSzarpConfig *tszarp);
};

/**
 * Info about single node defined.
 */
class TDefined {
public:
	/** Parses XML node (current set in reader) and returns pointer to new TParam object.
	 * @param reader XML reader set on "defined" node
	 * @param tszarp pointer to actual TSzarpConfig
	 * @return NULL on error, pointer to newly created TParam object on
	 * success.
	 */
	static TParam* parseXML(xmlTextReaderPtr reader,TSzarpConfig *tszarp);
};

/**
 * Info about single param draw.
 */
class TDraw {
public:
	/** Type definition for special draw attributes (SZARP 2.1) */
	typedef enum { NONE, PIEDRAW, HOURSUM, VALVE, REL, DIFF } SPECIAL_TYPES;
	TDraw(const std::wstring _color, const std::wstring& _window, double _prior, double _order,
			double _minVal, double _maxVal,
			int _scaleProc, double _scaleMin, double _scaleMax,
			SPECIAL_TYPES _special = NONE) :
		color(_color), window(_window), prior(_prior), order(_order),
		minVal(_minVal), maxVal(_maxVal), scaleProc(_scaleProc),
		scaleMin(_scaleMin), scaleMax(_scaleMax),
		special(_special), next(NULL)
	{ }
	/** Delete whole list. */
	~TDraw();
	/** Parses XML node and returns pointer to new TDraw object.
	 * @param node XML node with draw description
	 * @return NULL on error, pointer to newly created TDraw object on
	 * success.
	 */
	static TDraw* parseXML(xmlNodePtr node);
	/** Parses XML node (current set in reader) and returns pointer to new TDraw object.
	 * @param reader XML reader set on "draw"
	 * @return 0 on success, 1 on error
	 */
	static TDraw* parseXML(xmlTextReaderPtr reader);
	/**
	 * Return special attribute for draw.
	 */
	SPECIAL_TYPES GetSpecial()
	{
		return special;
	}
	/** Sets special attribute for draw. */
	void SetSpecial(SPECIAL_TYPES spec);
	/**
	 * Append element at the end of draws' list.
	 * @param d element to append
	 * @return pointer to appened element
	 */
	TDraw* Append(TDraw* d);
	/**
	 * Returns next list element.
	 * @return next element of draws' list, NULL if current is the last one
	 */
	TDraw* GetNext()
	{
		return next;
	}
	/**
	 * Generates XML node with draw info.
	 * @return created XML node, NULL on error
	 */
	xmlNodePtr GenerateXMLNode();
	/**
	 * Return pointer to color descritpion for draw.
	 */
	const std::wstring& GetColor()
	{
		return color;
	}
	/**
	 * Return pointer to title of draw's window.
	 */
	const std::wstring& GetWindow()
	{
		return window;
	}

	const std::wstring& GetTranslatedWindow() {
		return translatedWindow.empty() ? window : translatedWindow;
	}

	void SetTranslatedWindow(const std::wstring& tw) {
		translatedWindow = tw;
	}

	void SetWindow(const std::wstring& _window)
	{
		window = _window;
	}


	/**
	 * Return priority of draw's window.
	 */
	double GetPrior()
	{
		return prior;
	}
	/**
	 * Return order of draw in window.
	 */
	double GetOrder()
	{
		return order;
	}
	/**
	 * Return true if param had its order given, false otherwise.
	 */
	bool isBadOrder() const
	{
		return order == BAD_ORDER;
	}
	/**
	 * Return minimal value for draw.
	 */
	double GetMin()
	{
		return minVal;
	}
	/**
	 * Set minimal value for draw.
	 */
	void SetMin(double val)
	{
		minVal = val;
	}
	/**
	 * Return maximum value for draw.
	 */
	double GetMax()
	{
		return maxVal;
	}
	/**
	 * Set maximum value for draw.
	 */
	void SetMax(double val)
	{
		maxVal = val;
	}
	/**
	 * Return rescalling factor for draw.
	 */
	int GetScale()
	{
		return scaleProc;
	}
	/**
	 * Return start of rescalled sector. Makes sense only if GetScale()
	 * return number grater then 0.
	 */
	double GetScaleMin()
	{
		return scaleMin;
	}
	/**
	 * Return end of rescalled sector. Makes sense only if GetScale()
	 * return number grater then 0.
	 */
	double GetScaleMax()
	{
		return scaleMax;
	}
	/**
	 * @return number of draws in list.
	 */
	int GetCount();

	/**
	* @return parent tree nodes of this draw
	*/
	std::vector<TTreeNode*>& GetTreeNode() { return _tree_nodev; }
protected:
	std::wstring color;	/**< Color of draw, may be a name (e.g. "Red") or an
			 * RGB value (e.g. "#AA99FF"), NULL means default (auto)
			 */
	std::wstring window;	/**< Name of draw window must not be empty */
	std::wstring translatedWindow;	/**< Name of translated draw window */
	double prior;	/**< Priority of window (order), less or equal 0 means
			 * default */
	double order;	/**< Priority of draw within window (order), less or equal
			 * 0 means default */
	double minVal;	/**< Minimal param value */
	double maxVal;	/**< Maximum param value, less or equal minVal
			 means that both minVal and maxVal should be
			 automatically detected, it is not supported for
			 SZARP 2.1 */
	int scaleProc; /**< Scaling factor (if 0, no rescaling is made) */
	double scaleMin;	/**< Start of rescaling sector */
	double scaleMax;	/**< End of rescaling sector */
	SPECIAL_TYPES special;
			/**< Special SZARP 2.1 draw attribute (may affect all
			 * the window). */
	TDraw* next;	/**< Next list element */

	std::vector<TTreeNode*> _tree_nodev; /**< Parent tree nodes for this draw*/

};

/** Class representing a node in draws tree hierarchy*/
class TTreeNode {
	TTreeNode* _parent;
	double _prior;
	double _draw_prior;
	std::wstring _name;
public:
	TTreeNode();
	xmlNodePtr generateXML();
	int parseXML(xmlNodePtr node);
	/** Parses XML node (current set in reader)
	 * @param reader XML reader set on "treenode" node
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlTextReaderPtr reader);
	double GetPrior() { return _prior; }
	double GetDrawPrior() { return _draw_prior; }
	std::wstring GetName() { return _name; }
	TTreeNode* GetParent() { return _parent; }
	~TTreeNode();
};

/**
 * Different types of available data
 */
typedef enum {PROBE, MIN, MIN10, HOUR, DAY} TProbeType;

/**
 * Single output parameter descritpion.
 */
class TSendParam {
public:
	TSendParam(TUnit *parent) :
		parentUnit(parent), configured(0),
		paramName(L""), value(0), repeat(0), type(PROBE),
		sendNoData(0), next(NULL)
	{ }
	/** Delete whole list. */
	~TSendParam();
	/**
	 * Initializes param with data. Sets 'configured' attribute to 1.
	 * @param paramName name of param to send (NULL if constant will be
	 * send)
	 * @param value value to send (if name is NULL)
	 * @param repeat repeat rate
	 * @param type probe type
	 * @param sendNoData 1 if SZARP_NO_DATA has to be sent, 0 if no
	 */
	void Configure(const std::wstring& paramName, int value, int repeat, TProbeType type,
			int sendNoData);
	/**
	 * Generates XML node with param info.
	 * @return created XML node, NULL on error
	 */
	xmlNodePtr generateXMLNode(void);
	/**
	 * @returns n-th param's list element after 'this'.
	 */
	TSendParam* GetNthParam(int n);
	/** @return true if param is configured for sending, false if not */
	bool IsConfigured()
	{
		return configured;
	}
	/** @return name of param to send, empty string if constant value should be
	 * sent */
	const std::wstring& GetParamName()
	{
		return paramName;
	}
	/** @return constant value to send
	 * @see GetParamName */
	int GetValue()
	{
		return value;
	}
	/** @return repeat rate for param */
	int GetRepeatRate()
	{
		return repeat;
	}
	/** @return type of data to send (probe, min etc.) */
	TProbeType GetProbeType()
	{
		return type;
	}
	/** @return 1 if SZARP_NO_DATA value should be send, 0 if not */
	int GetSendNoData()
	{
		return sendNoData;
	}
	/** @return next list element, NULL if current is last */
	TSendParam* GetNext()
	{
		return next;
	}
	/** Append param to the end of params list.
	 * @param p param to append
	 * @return pointer to appended element */
	TSendParam* Append(TSendParam* p);
	/**
	 * Loads information parsed from XML file.
	 * @param node XML node with device configuration info
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlNodePtr node);
	/**
	 * Loads information parsed from XML file.
	 * @param reader XML reader set on "send"
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlTextReaderPtr reader);
protected:
	TUnit *parentUnit;
			/**< Pointer to parent TUnit object. */
	int configured;	/**< 1 if param is configured in sender.cfg file,
			  0 if it's empty */
	std::wstring paramName;
			/**< Name of input param. empty for constant values. */
	int value;	/**< Value to send if param attribute is NULL. */
	int repeat;	/**< Repeat rate for param. */
	TProbeType type;
			/**< Type of data to send. */
	int sendNoData; /**< Should we send SZARP_NO_DATA if param value is
			  not available? */
	TSendParam* next;
			/**< Next list element. */
};

/**
 * Values defintions for input params.
 */
class TValue {
public:
	/**
	 * @param i numerical value
	 * @param n name of value, must be dynamicaly allocated, will be freed
	 * in destructor
	 * @param _next next TValue object
	 */
	TValue(int i = 0, const std::wstring& n = std::wstring(), TValue *_next = NULL) :
		intValue(i), name(n), next(_next)
	{ }
	/** Destroys all list. */
	~TValue();
	/**
	 * Creates and returns newly created TValue list based on given
	 * SZARP constant.
	 * @param prec SZARP constant - 5 is "Tak" / "Nie", 6 is "Pochmurno" /
	 * "Zmiennie", "S³onecznie", 7 is "Plus" / "Minus".
	 * @return created TValue list
	 */
	static TValue *createValue(int prec);

	xmlNodePtr generateXMLNode(void);
	/**
	 * Appends value to at end of list.
	 * @param v value to append
	 * @return pointer to appended value (v)
	 */
	TValue* Append(TValue* v);
	/** @return pointer to next value in list, NULL if current is last */
	TValue* GetNext()
	{
		return next;
	}
	/** @return string for value */
	const std::wstring& GetString()
	{
		return name;
	}

	const std::wstring& GetTranslatedString() {
		return !translatedName.empty() ? translatedName : name;
	}

	void SetTranslatedString(const std::wstring& tn) {
		translatedName = tn;
	}

	/** @return integer value */
	int GetInt()
	{
		return intValue;
	}
	/**
	 * Search in list for value with given integer value.
	 * @param val integer value to search for
	 * @return corresponding TValue object or NULL if value not found
	 */
	TValue* SearchValue(int val);
protected:
	int intValue;	/**< Integer value */
	std::wstring name;	/**< String reprezentation of value */
	std::wstring translatedName;
	TValue *next;	/**< Next list element */
};

/**
 * Info about param in raport.
 */
class TRaport {
public:
	TRaport(TParam * p, const std::wstring& t, const std::wstring& d = std::wstring(), double o = -1.0) :
		param(p), title(t), descr(d), order(o), next(NULL)
	{
		if (!descr.empty() && descr == p->GetName())
			descr = std::wstring();
	}
	/** Delete whole list. */
	~TRaport();
	/** Appends new raport at the end of list */
	void Append(TRaport* rap);
	/** @returns next list element */
	TRaport* GetNext()
	{
		return next;
	}
	/** @return XML node representing raport item */
	xmlNodePtr GenerateXMLNode();
	/** @return title of raport */
	const std::wstring& GetTitle()
	{
		return title;
	}
	const std::wstring& GetTranslatedTitle()
	{
		return !translatedTitle.empty() ? translatedTitle : title;
	}

	void SetTranslatedTitle(const std::wstring &tt)
	{
		translatedTitle = tt;
	}
	/** @return priority of param within raport */
	double GetOrder()
	{
		return order;
	}
	/** @return param description */
	std::wstring GetDescr();
	/** @return raports file name
	 * @see filename */
	std::wstring GetFileName();
	/** Check if filename is default (can be ommited).
	 * @return 1 if is default, 0 if no
	 */
	int IsFileNameDefault();
	/** Check if param description is default (can be ommited).
	 * @return 1 if is default, 0 if no
	 */
	int IsDescrDefault();
	/** @return pointer to parent param object */
	TParam* GetParam()
	{
		return param;
	}
protected:
	TParam* param;	/**< Containing param. */
	std::wstring title;	/**< Title of raport (raport ID). */
	std::wstring translatedTitle;	/**< Title of raport (raport ID). */
	std::wstring descr;	/**< Description of param in raport (if empty, last
			  part of param's name should be used. */
	double order;	/**< Priority of param within raport. Ignored if
			  less then 0. Greater number means later in raport.
			  */
	TRaport* next;	/**< Next list element */
};


/**
 * Info about analysis element
 */
class TAnalysis {

public:
	/**posible values of analysis param types*/
	enum AnalysisParam {
		INVALID,
		ANALYSIS_ENABLE,
		REGULATOR_WORK_TIME,
		COAL_VOLUME,
		ENERGY_COAL_VOLUME_RATIO,
		LEFT_GRATE_SPEED,
		RIGHT_GRATE_SPEED,
		MAX_AIR_STREAM,
		MIN_AIR_STREAM,
		LEFT_COAL_GATE_HEIGHT,
		RIGHT_COAL_GATE_HEIGHT,
		AIR_STREAM,
		ANALYSIS_STATUS,
		AIR_STREAM_RESULT
	};
	TAnalysis(int _boiler_no = -1, AnalysisParam _param = INVALID)
		:
		boiler_no(_boiler_no),
		param(_param),
		next(NULL)
	{};

	/**Appends analysis object at end of list
	 * @return appended obejct*/
	TAnalysis* Append(TAnalysis *analysis);

	/**Converts xml node to TAanalis object
	 * @param node with analysis element
	 * @return pointer to TAnalysis object, NULL if error ocurred*/
	static TAnalysis* parseXML(xmlNodePtr node);

	/**Converts xml node to TAanalis object
	 * @param reader set on "analysis" element
	 * @return pointer to TAnalysis object, NULL if error ocurred*/
	static TAnalysis* parseXML(xmlTextReaderPtr reader);

	/**Creates xml node describing this object
	 * @return pointer to a node*/
	xmlNodePtr generateXMLNode(void);

	 /** @return next element on a list, NULL is this is the last*/
	TAnalysis* GetNext() {
		return next;
	}

	/**@return number of a boiler associated with this element*/
	int GetBoilerNo() {
		return boiler_no;
	}

	/**@return type of analysis param*/
	AnalysisParam GetParam() {
		return param;
	}

	/**Return a string desribing a type of param in xml document
	 * @param type - param type
	 * @return string with param type name*/
	static const std::wstring& GetNameForParam(AnalysisParam type);


	/**Return a enum value for param type for given params.xml name
	 * @param name string param name
	 * @return type of param*/
	static AnalysisParam GetTypeForParamName(const std::wstring& name);
	~TAnalysis();
private:
	/**Maps AnlisysParm values to a corresponding names in params.xml*/
	struct ParamName {
		AnalysisParam name_id;		/**<id of param*/
		const std::wstring name;		/**<name of a param*/
	};
	/**list of mappings, @see param_name*/
	static const struct ParamName ParamsNames[];

	int boiler_no;		/**<number of boiler*/
	AnalysisParam param;	/**<param type*/
	TAnalysis *next;	/**<next element on the list*/

};

/**
 * info about analysis interval
 */
class TAnalysisInterval {
	int grate_speed_lower; 	/**<lower bound for grate speed in this interval*/
	int grate_speed_upper;	/**<upper bound for grate speed in this interval*/
	int duration;		/**<interval duration*/
	TAnalysisInterval *next;/**<next element on intervals list*/
	public:
	TAnalysisInterval(int _grate_speed_lower,
			int _grate_speed_upper,
			int _duration) :
			grate_speed_lower(_grate_speed_lower),
			grate_speed_upper(_grate_speed_upper),
			duration(_duration),
			next(NULL)
	{};
	/*Sets lower bound for grate speed in interval
	 * @param _grate_speed bound value*/
	void SetGrateSpeedLower(int grate_speed) {
		grate_speed_lower = grate_speed;
	}

	/**@return lower bound for grate speed in interval*/
	int GetGrateSpeedLower() {
		return grate_speed_lower;
	}

	/*Sets upper bound for grate speed in interval
	 * @param _grate_speed bound value*/
	void SetGrateSpeedUpper(int grate_speed) {
		grate_speed_upper = grate_speed;
	}

	/**@return upper bound for grate speed in interval*/
	int GetGrateSpeedUpper() {
		return grate_speed_upper;
	}

	/*Sets interval duration
	 * @param grate_speed bound value*/
	void SetDuration(int _duration) {
		duration = _duration;
	}
	/*@return interaval duration*/
	int GetDuration() const {
		return duration;
	}

	/**Appends interval object at end of intervals list
	 * @return appended obejct*/
	TAnalysisInterval* Append(TAnalysisInterval* interval);

	/** @return next element on intervals list, NULL is this is last*/
	TAnalysisInterval* GetNext() {
		return next;
	}

	/**Converts xml node to TAnalysisInterval object
	 * @param node with interval element
	 * @return pointer to TAnalysisInterval object, NULL if error ocurred*/
	static TAnalysisInterval* parseXML(xmlNodePtr node);

	/**Converts xml node to TAnalysisInterval object
	 * @param reader set on "interval" element
	 * @return pointer to TAnalysisInterval object, NULL if error ocurred*/
	static TAnalysisInterval* parseXML(xmlTextReaderPtr reader);

	/**Creates xml node describing this object
	 * @return pointer to a node*/
	xmlNodePtr generateXMLNode();

	~TAnalysisInterval();

};

/**
 * info on boiler for analysis
 */
class TBoiler {
public:
	/**boilers list*/
	enum BoilerType {
		INVALID,
		WR1_25,
		WR2_5,
		WR5,
		WR10,
		WR25
	};

	TBoiler(int _boiler_no = -1,
		float _grate_speed = -1,
		float _coal_gate_height = -1,
		BoilerType _boiler_type = INVALID,
		TSzarpConfig* _parent = NULL,
		TBoiler* _next = NULL)
		:
		boiler_no(_boiler_no),
	    	grate_speed(_grate_speed),
	    	coal_gate_height(_coal_gate_height),
	    	boiler_type(_boiler_type),
		intervals(NULL),
	    	parent(_parent),
	    	next(_next)
	{
	};

	/** @return next element on boilers list, NULL is this is last*/
	TBoiler* GetNext() {
		return next;
	}

	/** sets containg TSzarpConfig object
	 * @param _parent pointer to TSzarpConfig*/
	void SetParent(TSzarpConfig *_parent) {
		parent = _parent;
	}

	/** @return containing TSzarpConfig object*/
	TSzarpConfig* GetParent() {
		return parent;
	}

	/**Retrieves TParam object associated with this boiler
	 * @param param_type type of param
	 * @return pointer to a TParam object, NULL if not found*/
	TParam* GetParam(TAnalysis::AnalysisParam param_type);

	/**@return head of intervals list*/
	TAnalysisInterval* GetFirstInterval() {
		return intervals;
	}

	/**appends interval at the end of intervals
	 * @return added interval*/
	TAnalysisInterval* AddInterval(TAnalysisInterval* interval);

	/**Sets boiler number
	 * param _boiler_no number of boiler*/
	void SetBoilerNo(int _boiler_no) {
		boiler_no = _boiler_no;
	}

	/**@return boiler number*/
	int GetBoilerNo() {
		return boiler_no;
	}

	/**@return maximum coal gate change*/
	float GetCoalGateHeight() {
		return coal_gate_height;
	}

	/**@return type of a boiler*/
	BoilerType GetBoilerType() {
		return boiler_type;
	}

	/**Sets boiler type
	 * @param type type of boiler*/
	void SetBoilerType(BoilerType type) {
		boiler_type = type;
	}

	/**Sets max coal gate height change
	 * @param _coal_gate_height maximum coal gate height*/
	void SetCoalGateHeight(float _coal_gate_height) {
		coal_gate_height = _coal_gate_height;
	}

	/**@return max grate speed change*/
	float GetGrateSpeed() {
		return grate_speed;
	}

	/**Sets max grate speed change
	 * @param _grate_speed maximum grate speed*/
	void SetGrateSpeed(float _grate_speed) {
		grate_speed = _grate_speed;
	}

	/**Appends boiler to the end of boielr's list
	 * @param boiler element to be added
	 * @return appended element*/
	TBoiler* Append(TBoiler* boiler);

	/**Converts xml node to TBoiler object
	 * @param reader set on "boiler" element
	 * @return pointer to TABoiler object, NULL if error ocurred*/
	static TBoiler* parseXML(xmlTextReaderPtr reader);

	/**Converts xml node to TBoiler object
	 * @param node with boiler element
	 * @return pointer to TABoiler object, NULL if error ocurred*/
	static TBoiler* parseXML(xmlNodePtr node);

	/**Creates xml node describing this object
	 * @return pointer to a node*/
	xmlNodePtr generateXMLNode();

	/**Return a string describing a type of boiler in xml docuement
	 * @param type boiler type
	 * @return string with boiler name*/
	static const std::wstring& GetNameForBoilerType(BoilerType type);


	/**Return a enum value for boiler type for given params.xml name
	 * @param name string with boiler name
	 * @return type of boiler*/
	static BoilerType GetTypeForBoilerName(const std::wstring& name);
	~TBoiler();
private:
	/**Maps BoilerType values to a corresponding names in params.xml*/
	struct BoilerTypeName {
		BoilerType 	type_id; 	/**<type of boiler*/
		const std::wstring name;		/**<name of boiler in params.xml*/
	};

	/**list of mappings, @see BoilerTypeName*/
	static const struct BoilerTypeName BoilerTypesNames[];

	int boiler_no;
			/**<boiler's number*/
	float grate_speed;
			/**<maximum grate speed change in a analysis period*/
	float coal_gate_height;
			/**<maximum coal gate height change in a analysis period*/
	BoilerType boiler_type;
			/**<type of a boiler*/
	TAnalysisInterval* intervals;
			/**<list of analysis intervals*/
	TSzarpConfig *parent;
			/**<pointer to containing TSzarpConfig*/
	TBoiler* next;	/**<next list's element*/

};

/**Class describes summer seasons*/
class TSSeason {
public:
	/**struct defines a summer season*/
	struct Season {
		Season(int _ms = 4, int _ds = 15, int _me = 10 , int _de = 15) :
			month_start(_ms), day_start(_ds), month_end(_me), day_end(_de) {}
		int month_start;		/*<month of season start*/
		int day_start;		/*<day of season start*/
		int month_end;		/*<month of season end*/
		int day_end;		/*<day of season end*/
	};
	TSSeason(Season _defs = Season()) : defs(_defs) {};

	/**return true if given date falls within summer season*/
	bool IsSummerSeason(time_t time) const;

	/**return true if given date falls within summer season*/
	bool IsSummerSeason(int year, int month, int day) const;

	/**
	 * @param year
	 * @return season definiton */
	const Season& GetSeason(int year) const;

	/**parses XML node describing seasons
	@return 0 if node was successfully parsed, 0 otherwise*/
	int parseXML(xmlNodePtr p);
	/**parses XML node describing seasons
	@return 0 if node was successfully parsed, 0 otherwise*/
	int parseXML(xmlTextReaderPtr reader);

	/**generates XML node describing seasons
	 * @return xml node ptr*/
	xmlNodePtr generateXMLNode() const;

	bool CheckSeason(const Season& season, int month, int day) const;

private:
	typedef std::tr1::unordered_map<int, Season> tSeasons;
	/**list of seasons*/
	tSeasons seasons;
	/**default season definition used if no season is explicitly given for given year*/
	Season defs;

};

/**Synchronized IPKs container*/
class IPKContainer {
	class UnsignedStringHash {
		std::tr1::hash<std::string> m_hasher;
		public:
		size_t operator() (const std::basic_string<unsigned char>& v) const {
			return m_hasher((const char*) v.c_str());
		}
	};

	/** Maps global parameters names encoding in utf-8 to corresponding szb_buffer_t* and TParam* objects. 
	 UTF-8 encoded param names are used by LUA formulas*/
	typedef std::tr1::unordered_map<std::basic_string<unsigned char>, TParam*, UnsignedStringHash > utf_hash_type;
	/*Maps global parameters encoded in wchar_t. Intention of having two separate maps 
	is to avoid frequent conversions between two encodings*/
	typedef std::tr1::unordered_map<std::wstring, TParam* > hash_type;

	hash_type m_params;

	utf_hash_type m_utf_params;

	boost::shared_mutex m_lock;

	/**Szarp data directory*/
	boost::filesystem::wpath szarp_data_dir;

	typedef std::tr1::unordered_map<std::wstring, TSzarpConfig*> CM;

	struct ConfigAux {
		unsigned _maxParamId;
		std::set<unsigned> _freeIds;
		unsigned _configId;
	};

	typedef std::tr1::unordered_map<std::wstring, ConfigAux> CAUXM;

	/**Szarp system directory*/
	boost::filesystem::wpath szarp_system_dir;

	/**current language*/
	std::wstring language;

	/**Configs hash table*/
	CM configs;

	/**Preload configurations*/
	CM configs_ready_for_load;

	CAUXM config_aux;

	unsigned max_config_id;

	static IPKContainer* _object;

	std::map<std::wstring, std::vector<std::shared_ptr<TParam>>> m_extra_params;

	/**Adds configuration to the the container
	 * @param prefix configuration prefix
	 * @param file path to the file with the configuration
	 * @param logparams defines if logging params should be generated
	 */
	TSzarpConfig* AddConfig(const std::wstring& prefix, const std::wstring& file = std::wstring() , bool logparams = true );

	TParam* GetParamFromHash(const std::basic_string<unsigned char>& global_param_name);

	TParam* GetParamFromHash(const std::wstring& global_param_name);

	void AddParamToHash(TParam* p);

	void RemoveParamFromHash(TParam* p);

	void AddExtraParamImpl(const std::wstring& prefix, TParam *n);

	void RemoveExtraParamImpl(const std::wstring& prefix, TParam *p);
public:
	IPKContainer(const std::wstring& szarp_data_dir,
			const std::wstring& szarp_system_dir,
			const std::wstring& lang);
			

	~IPKContainer();

	void RemoveExtraParam(const std::wstring& prefix, TParam *param);

	void RemoveExtraParam(const std::wstring& prefix, const std::wstring &name);

	bool ReadyConfigurationForLoad(const std::wstring &prefix, bool logparams = true );

	template<class T> TParam* GetParam(const std::basic_string<T>& global_param_name, bool add_config_if_not_present = true);

	void AddExtraParam(const std::wstring& prefix, TParam *param);

	/**Retrieves config from the container
	 * @return configuration object or NULL if given config is not available*/
	TSzarpConfig *GetConfig(const std::wstring& prefix);
	
	TSzarpConfig* GetConfig(const std::basic_string<unsigned char>& prefix);
	/**Loads config into the container
	 * @return loaded config object, NULL if error occured during configuration load*/
	TSzarpConfig *LoadConfig(const std::wstring& prefix, const std::wstring& file = std::wstring(), bool logparams = true );

	std::map<std::wstring, std::vector<std::shared_ptr<TParam>>> GetExtraParams();

	/**@return the container object*/
	static IPKContainer* GetObject();
	/**Inits the container
	 * @param szarp_dir path to main szarp directory*/
	static void Init(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring& language);

	/**Destroys the container*/
	static void Destroy();
};

class TDictionary {
private:
	typedef std::pair<std::wstring, std::wstring> ENTRY;
	typedef std::map<std::wstring, std::vector<ENTRY> > DT;
	DT m_dictionary;

	std::wstring m_dictionary_path;

	void LoadDictionary(const std::wstring &from_lang, const std::wstring& to_lang);
	std::wstring TranslateEntry(const std::wstring &section, const std::wstring &wstring);

	void TranslateParamName(TParam *p);
	void TranslateParam(TParam *p);
public:
	TDictionary(const std::wstring& szarp_dir);
	void TranslateIPK(TSzarpConfig *ipk, const std::wstring& tolang);

};

/** xmlTextReaderPtr wrapper */
class XMLWrapper {
private:
	/** reference to wrapped reader */
	xmlTextReaderPtr &r;
	/** current tag name */
	const xmlChar* name;
	/** current attribute name */
	const xmlChar* attr_name;
	/** value of current attribute */
	const xmlChar* attr;
	/** list of ignored tags */
	std::set<std::string> ignoredTags;
	/** list of ignored xml trees */
	std::set<std::string> ignoredTrees;
	/** list of needed attributes */
	std::set<std::string> neededAttr;
	/** flag - use local namespace */
	bool isLocal;

public:
	/**
	 * Create XMLWrapper and load tag name
	 * @param _r reference to xmlTextReaderPtr
	 * @param  local flag - use local names in tags
	 */
	XMLWrapper(xmlTextReaderPtr &_r, bool local = false);
	/**
	 * Set list of ignored tags.
	 * @param i_list pointer to list of ignored tags. The last position of list have to be 0.
	 */
	void SetIgnoredTags(const char *i_list[]);
	/**
	 * Set list of ignored trees.
	 * @param i_list pointer to list of ignored xml trees. The last position of list have to be 0.
	 */
	void SetIgnoredTrees(const char *i_list[]);
	/**
	 * Check are exist all needed attributes in current tag
	 * @param attr_list pointer to list of needed attributes. The last position of list have to be 0.
	 * @return return true if exists all needed attributes, otherwise return false
	 */
	bool AreValidAttr(const char* attr_list[]);
	/**
	 * Take a next tag in param. Omit all tags listed in 'ingnoredTags', and 'ignoredTrees'.
	 * @return return true if exists next tag, otherwise return false
	 */
	bool NextTag()
#ifndef MINGW32
__wur
#endif
;
	/**
	 * @param n name of tag
	 * @return true if current tag is equel n, otherwise return false
	 */
	bool IsTag(const char* n);
	/**
	 * @param a name of attribute
	 * @return true if current attribute name is equal a, otherwise return false
	 */
	bool IsAttr(const char* a);
	/**
	 * If a tag has the first attribute than set 'attr_name' and 'attr' value.
	 * @return true if tag has attribute, otherwise return false
	 */
	bool IsFirstAttr();
	/**
	 * If a tag has attributes than move to the next one and set 'attr_name' and 'attr' value. IMPORTANT: use IsFirstAttr before.
	 * @return true if moved to next attribute, otherwise return false
	 */
	bool IsNextAttr();
	/**
	 * @return true if current tag is begin tag, otherwise return false
	 */
	bool IsBeginTag();
	/**
	 * @return true if current tag is end tag, otherwise return false
	 */
	bool IsEndTag();
	/**
	 * @return true if current tag is empty tag, otherwise return false
	 */
	bool IsEmptyTag();
	/**
	 * @return true if current tag has attribute/s, otherwise return false
	 */
	bool HasAttr();
	/**
	 * 
	 * @return value of current attribute
	 */
	const xmlChar* GetAttr();
	/**
	 * @return name of current attribute
	 */
	const xmlChar* GetAttrName();
	/**
	 * @return current tag name
	 */
	const xmlChar* GetTagName();
	/**
	 * write a message error into log file and throw XMLWrapperException
	 * @param text message error
	 * @param prior priority of log message (like in sz_log)
	 */
	void XMLError(const char *text, int prior = 1);
	/**
	 * write a message error into a log file that a attribute was not found in a tag; throw XMLWrapperException
	 * @param tag_name name of tag
	 * @param attr_name name of attribute (which was not found)
	 */
	void XMLErrorAttr(const xmlChar* tag_name, const char* attr_name);
	/**
	 * write a message error into a log file that was found not known tag inside current tag tree; throw XMLWrapperException
	 * @param current_tag name of a current tag tree where the error occurred
	 */
	void XMLErrorNotKnownTag(const char* current_tag);
	/**
	 * write a message error into a log file that a attribute 'attr_name' has wrong value; throw XMLWrapperException
	 */
	void XMLErrorWrongAttrValue();
	/**
	 * write a message into a log file that 'attr_name' is not known in current tag 'name'
	 */
	void XMLWarningNotKnownAttr();
};

/** An exception class for XMLWrapper */
class XMLWrapperException {};

#endif /* __SZARP_CONFIG_H__ */

