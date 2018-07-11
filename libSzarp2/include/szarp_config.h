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
#include <unordered_map>
#include <tr1/unordered_map>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

#include <string>
#include <set>

#include <stdio.h>
#include <stdexcept>
#include <assert.h>
#include <libxml/tree.h>
#ifndef NO_LUA
#include <lua.hpp>
#endif

#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/any.hpp>

#include "szbase/szbdefines.h"
#include <libxml/xmlreader.h>
#include "config_info.h"

#define BAD_ORDER -1.0

#define IPK_NAMESPACE_STRING L"http://www.praterm.com.pl/SZARP/ipk"

#define MAX_PARS_IN_FORMULA 160
#define SZARP_NO_DATA -32768

class TDevice;
class TUnit;
class IPCParamInfo;
class TParam;
class TSendParam;
class TValue;
class TRaport;
class TDraw;
class TSSeason;
class TDictionary;
class TTreeNode;
class TDrawdefinable;
class TScript;
class TDefined;
class XMLWrapper;
class XMLWrapperException;

struct ParseErrors {
	bool continueOnParseError;
};

extern ParseErrors errorStruct;

class TCheckException : public std::exception {
	virtual const char* what() const throw()
	{ return "Invalid params.xml definition. See log for more informations."; }
};

namespace szarp {

/**
 * Substitues wildcards in param name.
 * @param name name with possible wildcards
 * @param ref reference name
 * @return absolute name
 */
std::wstring substituteWildcards(const std::wstring& name, const std::wstring& ref);

} // namespace szarp

class SzarpConfigInfo: public TAttribHolder {
public:
	virtual const std::wstring& GetPrefix() const = 0;
	virtual IPCParamInfo* getIPCParamByName(const std::wstring& name) const = 0;
	virtual TParam* GetFirstParam() const = 0;
	virtual TDevice* GetFirstDevice() const = 0;
	virtual TDevice* DeviceById(int id) const = 0;
	virtual unsigned GetConfigId() const = 0;
	virtual void SetConfigId(unsigned) = 0;
	virtual size_t GetFirstParamIpcInd(const TDevice&) const = 0;
	virtual const std::vector<size_t> GetSendIpcInds(const TDevice&) const = 0;
};

/** SZARP system configuration */
class TSzarpConfig: public SzarpConfigInfo {
public:
	/** 
	 * @brief constructs empty configuration. 
	 */
	TSzarpConfig();
	virtual ~TSzarpConfig();

	/**
	 * @return title of configuration.
	 */
	const std::wstring& GetTitle() const;
	const std::wstring& GetTranslatedTitle() { return !translatedTitle.empty() ? translatedTitle : title; }
	void SetTranslatedTitle(const std::wstring &t) { translatedTitle = t; }

	/**
	 * @return prefix of configuration
	 * (NULL if this information is not available)
	 */
	const std::wstring& GetPrefix() const override;

	/**
	 * Return param with given name (searches in all params, including
	 * defined and 'draw definable').
	 * @param name name of param to find
	 * @return pointer to param, NULL if not found
	 */
	TParam *getParamByName(const std::wstring& name) const;
	IPCParamInfo* getIPCParamByName(const std::wstring& name) const override;

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
	 * Returns pointer to first param in config.
	 * @return pointer to first param, NULL if there are no params
	 */
	TParam * GetFirstParam() const override;

	/** @return pointer to first defined param, NULL if no one exists */
	TParam* GetFirstDefined() { return defined; }

	/** @return pointer to first drawdefinable param, NULL if no one exists */
	TParam* GetFirstDrawDefinable() { return drawdefinable; }

	/**
	 * @param id identifier of line (is equal to line number + 1 - starts
	 * from 1)
	 * @return pointer to device with given id.
	 */
	TDevice* DeviceById(int id) const override;

	/** @return number of devices */
	int GetDevicesCount();

	/** @return number of all params in devices (without defined and
	 * 'draw definable') */
	int GetParamsCount() const;

	/** @return number of defined params */
	int GetDefinedCount();

	/** @return number of 'draw definable' params */
	int GetDrawDefinableCount();

	/** @return first element of devices list, NULL if no one exists */
	TDevice* GetFirstDevice() const { return devices; }

	/** @return title of first raport */
	std::wstring GetFirstRaportTitle();
	/** @return title of next raport */
	std::wstring GetNextRaportTitle(const std::wstring& cur);
	/** @return first raport item with given title */
	TRaport* GetFirstRaportItem(const std::wstring& title);

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

	/** Checks if repetitions of parameters names in params.xml occurred
	 * @param quiet if equal 1 do not print anything to standard output
	 * @return 0 if there is no repetitions or 1 if there are some
	 */
	bool checkRepetitions(int quiet = 0);

	/**
	 * Check if sends are ok -- this means if every send has valid param
	 */
	bool checkSend();

	/** @return object representing seasons limit's
	 * NULL if no seasons configuration is present*/
	const TSSeason* GetSeasons() const;

	unsigned GetConfigId() const { return config_id; }

	void SetConfigId(unsigned _config_id) { config_id = _config_id; }

	void SetPrefix(const std::wstring& _prefix) {
		prefix = _prefix;
	}

	TUnit* createUnit(TDevice* parent);
	TParam* createParam(TUnit* parent);

	void UseNamesCache();

	void AddParamToNamesCache(TParam* _param);

	size_t GetFirstParamIpcInd(const TDevice &d) const;

	const std::vector<size_t> GetSendIpcInds(const TDevice &d) const;

protected:
	unsigned config_id;
			/**< Configuration unique numbe id */
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

	TSSeason *seasons;/**< Summer seasons configuration*/

	size_t unit_counter; /**< numer of created TUnit objects */

	bool use_names_cache; /**< flag, if set std::map will be used for fast searching params by name */

	std::unordered_map<std::wstring, TParam *> params_map; /**< map for params search by name */
};

class DeviceInfo: public TAttribHolder {};

/** Device description */
class TDevice: public DeviceInfo, public TNodeList<TDevice> {
public:
	TDevice(TSzarpConfig *parent);
	/** Destroy whole list. */
	~TDevice();

	/** @return number of params in device */
	int GetParamsCount();

	/** numer of units within line (or device in case of dummy radio
	 * object */
	int GetUnitsCount();
	/** @return pointer to first unit in radio line, after proper
	 * initialization should not be NULL */
	TUnit* GetFirstUnit() const
	{
		return units;
	}

	/** Add new unit */
	TUnit* AddUnit(TUnit* unit);

	/** @return pointer to parent TSzarpConfig object */
	SzarpConfigInfo* GetSzarpConfig() const
	{
		return parentSzarpConfig;
	}
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

protected:
	/**
	 * Returns num'th param from device line.
	 * @param num number of param (from 0)
	 * @return pointer to param, NULL if not found
	 */
	TParam *getParamByNum(int num);

	TSzarpConfig *parentSzarpConfig;
			/**< Pointer to SzarpConfig object. */
	TUnit *units;
};

class Sz4ParamInfo: public TAttribHolder {
public:
	typedef enum { SHORT = 0, INT, FLOAT, DOUBLE, UINT, USHORT, LAST_DATA_TYPE = USHORT} DataType;
	virtual DataType GetDataType() const { return _data_type; }
	virtual void SetDataType(DataType data_type) { _data_type = data_type; }

	typedef enum { SECOND = 0, NANOSECOND, LAST_TIME_TYPE  = NANOSECOND } TimeType; 
	virtual TimeType GetTimeType() const { return _time_type; }
	virtual void SetTimeType(TimeType time_type) { _time_type = time_type; }

protected:

	DataType _data_type{ SHORT };
	TimeType _time_type{ SECOND };
};

class IPCParamInfo: public Sz4ParamInfo {
public:
	virtual ~IPCParamInfo() {}

	virtual const std::wstring& GetName() const { return _name; }
	virtual void SetName(const std::wstring& name) { _name = name; }

	virtual int GetPrec() const { return getAttribute<int>("prec", 0); }
	virtual void SetPrec(int prec) { storeAttribute("prec", std::to_string(prec)); }

	virtual unsigned int GetIpcInd() const = 0;

protected:
	std::wstring _name;
};


class SendParamInfo: public TAttribHolder {
public:
	virtual IPCParamInfo* GetParamToSend() const = 0;

	/* constant value to be sent instead of real param value */
	virtual int GetValue() const { return getAttribute<int>("value", SZARP_NO_DATA); }
	virtual int GetPrec() const { return getAttribute<int>("extra:prec", GetParamToSend()? GetParamToSend()->GetPrec() : 0); }

	virtual const std::wstring& GetParamName() const = 0;
};


class UnitInfo: public TAttribHolder {
public:
	virtual ~UnitInfo() {}
	virtual size_t GetSenderMsgType() const = 0;
	virtual size_t GetParamsCount() const = 0;
	virtual size_t GetSendParamsCount(bool ignore_non_ipc = false) const = 0;

	virtual std::vector<IPCParamInfo*> GetParams() const = 0;
	virtual std::vector<SendParamInfo*> GetSendParams() const = 0;

	virtual SzarpConfigInfo* GetSzarpConfig() const = 0;

	virtual wchar_t GetId() const = 0;
	virtual int GetUnitNo() const = 0;
};


/**
 * Description of communication unit.
 */
class TUnit: public UnitInfo, public TNodeList<TUnit> {
	static constexpr auto SHM_MSG_OFFSET = 256L;

public:
	TUnit(size_t unit_no, TDevice *parent, TSzarpConfig* ipk): parentSzarpConfig(ipk), send_msg_type(unit_no + TUnit::SHM_MSG_OFFSET), parentDevice(parent), params(NULL), sendParams(NULL) { }
	~TUnit();

	// implementation of UnitInfo
	/** @return number of params in unit */
	size_t GetParamsCount() const override;
	/** @return number of send params in unit */
	size_t GetSendParamsCount(bool ignore_non_ipc = false) const override;
	size_t GetSenderMsgType() const override {
		return send_msg_type;
	}

	int GetUnitNo() const override { return send_msg_type - TUnit::SHM_MSG_OFFSET; }

	/** @return id of unit (one ASCII character) */
	// [deprecated]
	wchar_t GetId() const override;

	/** @return pointer to first send param in unit, may be NULL */
	TSendParam* GetFirstSendParam() const
	{
		return sendParams;
	}

	std::vector<IPCParamInfo*> GetParams() const override;

	std::vector<SendParamInfo*> GetSendParams() const override;

	/** @returns pointer to first param in unit, NULL if no one exists */
	/**
	 * Adds param to unit (at last position).
	 * @param p param to add
	 */
	void AddParam(TParam* p);
	void AddParam(TSendParam* sp);

	/** @return pointer to containing device object */
	TDevice* GetDevice() const;
	/** @return pointer to main TSzarpConfig object */
	SzarpConfigInfo* GetSzarpConfig() const override;
	TParam* GetFirstParam() const
	{
		return params;
	}

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

protected:
	TSzarpConfig* parentSzarpConfig;
	size_t send_msg_type;

	TDevice *parentDevice;
	TParam *params;
	TSendParam *sendParams;
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

enum class FormulaType {
	NONE, RPN, DEFINABLE
#ifndef NO_LUA
	, LUA_VA, LUA_AV, LUA_IPC
#endif
};

enum class ParamType {
	REAL, COMBINED, DEFINABLE
#ifndef NO_LUA
	, LUA
#endif
};


// TODO: add visitors for LUA/RPN/DEFINABLE formulas and make then separate classes with normal interfaces
class ParamFormulaInfo: public IPCParamInfo {
public:
	ParamFormulaInfo(TUnit *parent, TSzarpConfig *parentSC = NULL, const std::wstring& formula = std::wstring(), FormulaType ftype = FormulaType::NONE, ParamType ptype = ParamType::REAL):
	    _parentUnit(parent), _parentSzarpConfig(parentSC), _formula(formula), _ftype(ftype), _param_type(ptype) {}
	virtual ~ParamFormulaInfo();

public:
	void PrepareDefinable();
	unsigned int GetIpcInd() const;

	/** @return pointer to parent unit object, NULL for defined params */
	TUnit* GetParentUnit() { return _parentUnit; }

	void SetParentSzarpConfig(TSzarpConfig *parentSzarpConfig) { _parentSzarpConfig = parentSzarpConfig; }

	TSzarpConfig *GetSzarpConfig() const;

	const std::wstring& GetFormula() { return _formula; }
	double calculateConst(const std::wstring& _formula);

	void SetFormula(const std::wstring& f, FormulaType type = FormulaType::RPN);
	const std::wstring&  GetDrawFormula();
	std::wstring GetParcookFormula(bool ignoreIndexes = false, std::vector<std::wstring>* ret_params_list = nullptr);

	ParamType GetType() { return _param_type; }
	FormulaType GetFormulaType() const { return _ftype; }
	void SetFormulaType(FormulaType type);

	TParam ** GetFormulaCache() { return &(_f_cache[0]); }
	int GetNumParsInFormula() { return _f_cache.size(); }
	bool IsNUsed() { return _f_N; }
	bool IsConst() { return (ParamType::REAL != _param_type) && _f_const; };
	double GetConstValue() { return _f_const_value; };
	int IsDefinable() { return ParamType::REAL != _param_type
#ifndef NO_LUA
		       	&& ParamType::LUA != _param_type
#endif
				; }
	void CheckForNullFormula();

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

// Set by TParam
protected:
	TUnit* _parentUnit;
	TSzarpConfig* _parentSzarpConfig;

	std::wstring _formula;
	FormulaType _ftype;
	ParamType _param_type{ ParamType::REAL };

	std::wstring	_parsed_formula;    /**< parsed formula for definable calculating */
	std::vector<TParam *> _f_cache;	    /**< formula cache */

	double _f_const_value; /**< const value if formula is const */

	bool _prepared = false; /**< flag - is definalbe param prepared */
	bool _f_const = false; /**< flag - is formula const */
	bool _f_N = false; /**< flag - if 1 - N function i formula */

#ifndef NO_LUA
	unsigned char* _script{ NULL };

	int _lua_function_reference{ LUA_NOREF };

	time_t _lua_start_date_time { -1 };
	time_t _lua_start_offset{ 0 };
	time_t _lua_end_offset { 0 };

#if LUA_PARAM_OPTIMISE
	LuaExec::Param* _opt_lua_param{ nullptr };
#endif
#endif

};

enum class Sz4ParamType {
	NONE,
	REAL,
	COMBINED,
	DEFINABLE,
	LUA,
	LUA_OPTIMIZED
};


class TParam: public ParamFormulaInfo, public TNodeList<TParam> {
public:

	TParam(TUnit *parent,
		TSzarpConfig *parentSC = NULL,
		const std::wstring& formula = std::wstring(),
		FormulaType ftype = FormulaType::NONE,
		ParamType ptype = ParamType::REAL) :
		ParamFormulaInfo(parent, parentSC, formula, ftype, ptype),
	    _shortName(),
	    _drawName(),
	    _unit(),
	    _values(NULL),
	    _baseInd(-1),
	    _inbase(0),
	    _raports(NULL),
	    _draws(NULL),
	    _szbase_name()
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

	// [deprecated]
	std::wstring GetSumUnit() { return L""; }

	// [deprecated]
	bool IsMeterParam() const { return getAttribute("is_meter", false); }

	// [deprecated]
	const double GetSumDivisor() const { return 6.; }

	/** @return for param with real base index it returns base index, for others it
	 * returns IPC index
	 */
	unsigned int GetVirtualBaseInd() const;

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

	/** @return whether has limits */
	short GetForbidden() const { return getAttribute<short>("forbidden", SZARP_NO_DATA); }

	/** @return whether has a forbidden value */
	bool HasForbidden() const { return hasAttribute("forbidden"); }

	/** @return pointer to the first element of possible param values list,
	 * NULL for integer parameters */
	TValue* GetFirstValue()	{ return _values; }

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

	/**
	 * Returns n-th element of params list.
	 * @param n number of param to search, if 0 current is returned
	 * @param global if true, global search is activated (through all
	 * devices, units and defined params)
	 * @return pointer to param found, NULL if not found
	 */
	TParam* GetNthParam(int n, bool global = false);

	/** @return pointer to containing device object, NULL for defined
	 * parameters */
	TDevice* GetDevice() const;

	int parseXML(xmlNodePtr node) override;
	int parseXML(xmlTextReaderPtr node) override;

	int processAttributes() override;

	/**
	 * Get next param in list.
	 * @param global if true, global search is activated (through all
	 * devices, radios, lines and defined params)
	 * @return next params list element, NULL if current element is last
	 */
	TParam* GetNextGlobal() const;

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

	Sz4ParamType GetSz4Type() const { return _sz4ParamType; }

	void SetSz4Type(Sz4ParamType sz4ParamType) { _sz4ParamType = sz4ParamType; }

	unsigned GetParamId() const { return _paramId; }

	void SetParamId(unsigned paramId) { _paramId = paramId; }

	unsigned GetConfigId() const { return _configId; }

	void SetConfigId(unsigned configId) { _configId = configId; }

	static bool IsHourSumUnit(const std::wstring& unit);
protected:

	std::wstring _translatedName;	    /**< Full name */
	std::wstring _shortName;  /**< Short name */
	std::wstring _translatedShortName;  /**< Short name */
	std::wstring _drawName;   /**< Name for use in DRAW program */
	std::wstring _translatedDrawName;   /**< Name for use in DRAW program */
	std::wstring _unit;	    /**< Symbol of unit */

	TValue * _values;   /**< List of possible values defintions, NULL if it's
			      an integer parameter */

	int _baseInd;	/**< Index of parameter in data base. Must be uniq among
			  all params. For params not saved in data base should
			 be < 0 (-<index in PTT.act>). */

	int _inbase;	/**< 0 for parameters that are not saved to base, otherwise
			  1; inbase > 0 and baseInd < 0 it means 'auto' base index
			  (only available for SzarpBase base format) */

	unsigned _paramId;

	unsigned _configId;

	TRaport	* _raports;	/**< List of raports for param */
	TDraw * _draws;		/**< List of draws for param */

	std::wstring _szbase_name;	/**< Name of parameter converted to szbase format.
				  May be empty - will get converted on next call to
				  GetSzbaseName(). */

	Sz4ParamType _sz4ParamType{ Sz4ParamType::NONE };
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
class TSendParam: public SendParamInfo, public TNodeList<TSendParam> {
public:
	TSendParam(TUnit *parent) :
		parentUnit(parent), configured(0),
		paramName(L""), type(PROBE)
	{ }
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
	const std::wstring& GetParamName() const override
	{
		return paramName;
	}

	/** @return repeat rate for param */
	int GetRepeatRate()
	{
		return getAttribute<int>("repeat", 1);
	}
	/** @return type of data to send (probe, min etc.) */
	TProbeType GetProbeType()
	{
		return type;
	}
	/** @return 1 if SZARP_NO_DATA value should be send, 0 if not */
	int GetSendNoData()
	{
		return hasAttribute("send_no_data")? 1 : 0;
	}

	IPCParamInfo* GetParamToSend() const override {
		if (!hasAttribute("param")) return nullptr;
		if (paramName.empty() || parentUnit == nullptr) return nullptr;
		return parentUnit->GetSzarpConfig()->getIPCParamByName(paramName);
	}

	/**
	 * Loads information parsed from XML file.
	 * @param node XML node with device configuration info
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlNodePtr node) override;
	/**
	 * Loads information parsed from XML file.
	 * @param reader XML reader set on "send"
	 * @return 0 on success, 1 on error
	 */
	int parseXML(xmlTextReaderPtr reader) override;
	int processAttributes() override;
protected:
	TUnit *parentUnit;
			/**< Pointer to parent TUnit object. */
	int configured;	/**< 1 if param is configured in sender.cfg file,
			  0 if it's empty */
	std::wstring paramName;
			/**< Name of input param. empty for constant values. */
	TProbeType type;
			/**< Type of data to send. */
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
	TRaport* GetNext();
	/** @return XML node representing raport item */
	xmlNodePtr GenerateXMLNode();
	/** @return title of raport */
	const std::wstring& GetTitle() const
	{
		return title;
	}
	const std::wstring& GetTranslatedTitle() const
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

	bool CheckSeason(const Season& season, int month, int day) const;

private:
	typedef std::unordered_map<int, Season> tSeasons;
	/**list of seasons*/
	tSeasons seasons;
	/**default season definition used if no season is explicitly given for given year*/
	Season defs;

};

/**Synchronized IPKs container*/
class IPKContainer {
	class UnsignedStringHash {
		std::hash<std::string> m_hasher;
		public:
		size_t operator() (const std::basic_string<unsigned char>& v) const {
			return m_hasher((const char*) v.c_str());
		}
	};

	/** Maps global parameters names encoding in utf-8 to corresponding szb_buffer_t* and TParam* objects. 
	 UTF-8 encoded param names are used by LUA formulas*/
	typedef std::unordered_map<std::basic_string<unsigned char>, TParam*, UnsignedStringHash > utf_hash_type;
	/*Maps global parameters encoded in wchar_t. Intention of having two separate maps 
	is to avoid frequent conversions between two encodings*/
	typedef std::unordered_map<std::wstring, TParam* > hash_type;

	hash_type m_params;

	utf_hash_type m_utf_params;

	mutable boost::shared_mutex m_lock;

	/**Szarp data directory*/
	boost::filesystem::wpath szarp_data_dir;

	typedef std::unordered_map<std::wstring, TSzarpConfig*> CM;

	struct ConfigAux {
		unsigned _maxParamId;
		std::set<unsigned> _freeIds;
		unsigned _configId;
	};

	typedef std::unordered_map<std::wstring, ConfigAux> CAUXM;

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

	std::unordered_map<std::wstring, std::vector<std::shared_ptr<TParam>>> m_extra_params;

	/**Adds configuration to the the container
	 * @param prefix configuration prefix
	 * @param file path to the file with the configuration
	 */
	TSzarpConfig* AddConfig(const std::wstring& prefix, const std::wstring& file = std::wstring());

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

	bool ReadyConfigurationForLoad(const std::wstring &prefix);

	template<class T> TParam* GetParam(const std::basic_string<T>& global_param_name, bool add_config_if_not_present = true);

	void AddExtraParam(const std::wstring& prefix, TParam *param);

	/**Retrieves config from the container
	 * @return configuration object or NULL if given config is not available*/
	TSzarpConfig *GetConfig(const std::wstring& prefix);
	
	TSzarpConfig* GetConfig(const std::basic_string<unsigned char>& prefix);
	/**Loads config into the container
	 * @return loaded config object, NULL if error occured during configuration load*/
	TSzarpConfig *LoadConfig(const std::wstring& prefix, const std::wstring& file = std::wstring());

	std::unordered_map<std::wstring, std::vector<std::shared_ptr<TParam>>> GetExtraParams() const;

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
	typedef std::unordered_map<std::wstring, std::vector<ENTRY> > DT;
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

