#ifndef __IPKCONTAINER__H__
#define __IPKCONTAINER__H__

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/filesystem/path.hpp>

#include <unordered_map>

#include "szarp_config.h"

class IPKContainer {
protected:
	static IPKContainer* _object;

public:
	virtual ~IPKContainer() {}

	/* ipk access by global name */
	virtual TParam* GetParam(const std::wstring&, bool = true) = 0;
	virtual TParam* GetParam(const std::basic_string<unsigned char>&, bool = true);

	/**Retrieves config from the container
	 * @return configuration object or NULL if given config is not available*/
	virtual TSzarpConfig* GetConfig(const std::wstring& prefix) = 0;
	virtual TSzarpConfig* GetConfig(const std::basic_string<unsigned char>& prefix);

	/**Loads config into the container
	 * @return loaded config object, NULL if error occured during configuration load*/
	virtual TSzarpConfig *LoadConfig(const std::wstring& prefix) = 0;

	/** Creates IPK and parses params.xml file */
	virtual TSzarpConfig* InitializeIPK(const std::wstring &prefix) = 0;

	static IPKContainer* GetObject();
	static void Destroy();
};

template <typename ST, typename VT, typename HT = std::hash<ST>>
struct Cache {
	using container_type = std::unordered_map<ST, VT*, HT>;
	container_type cached_values;
	bool remove(const ST& pn);
	void insert(const ST& pn, VT* p);
	auto begin() { return cached_values.begin(); }
	auto end() { return cached_values.end(); }
	VT* get(const ST& pn);
};

class ParamsCacher {
	using US = std::basic_string<unsigned char>; // UTF-8 encoded string for LUA lookup
	using SS = std::wstring; // Unicode for other params

	struct USHash {
		// https://en.cppreference.com/w/cpp/utility/hash
		size_t operator() (const US& v) const {
			return std::hash<std::string>{}((const char*) v.c_str());
		}
	};

	Cache<SS, TParam> m_params;
	Cache<US, TParam, USHash> m_utf_params;

public:
	/* access by global name */
	TParam* GetParam(const SS&);
	TParam* GetParam(const US&);

	void AddParam(TParam* param);
	bool RemoveParam(TParam* param);
};

/** Synchronized IPKs container with param cache */
class ParamCachingIPKContainer: public IPKContainer {
protected:
	ParamsCacher cacher;

	mutable boost::shared_mutex m_lock;

	/**Szarp data directory*/
	boost::filesystem::wpath szarp_data_dir;

	struct ConfigAux {
		unsigned _maxParamId;
		std::set<unsigned> _freeIds;
		unsigned _configId;
	};

	using CAUXM = std::unordered_map<std::wstring, ConfigAux>;

	/**Szarp system directory*/
	boost::filesystem::wpath szarp_system_dir;

	/**current language*/
	std::wstring language;

	/**Configs hash table*/
	Cache<std::wstring, TSzarpConfig> configs;

	CAUXM config_aux;

	unsigned max_config_id;

	/** Adds configuration to the the container */
	virtual TSzarpConfig* AddConfig(TSzarpConfig* ipk, const std::wstring& prefix);
	virtual TSzarpConfig* AddConfig(const std::wstring& prefix);

public:
	ParamCachingIPKContainer(const std::wstring& szarp_data_dir,
			const std::wstring& szarp_system_dir,
			const std::wstring& lang);

	~ParamCachingIPKContainer();

	/* Cache both types of strings for faster lookup in LUA scripts */
	TParam* GetParam(const std::wstring&, bool = true) override;
	TParam* GetParam(const std::basic_string<unsigned char>&, bool = true) override;
	template <typename T> TParam* GetParamImpl(const std::basic_string<T>&, bool = true);

	TSzarpConfig* GetConfig(const std::wstring& prefix) override;
	using IPKContainer::GetConfig;

	TSzarpConfig* InitializeIPK(const std::wstring &prefix) override;

	TSzarpConfig* LoadConfig(const std::wstring& prefix) override;

	static void Init(const std::wstring& szarp_data_dir, const std::wstring& szarp_system_dir, const std::wstring& language);
};

#endif
