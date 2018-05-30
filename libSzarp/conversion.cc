#include <stdexcept>

#include "conversion.h"
#include <iconv.h>
#include <errno.h>
#include <assert.h>

#include <boost/thread.hpp>
#include <boost/scoped_array.hpp>
#include <boost/locale.hpp>

#ifdef MINGW32
#include <windows.h>
#endif


/* IconvWrapper - wrapper class for iconv library conversions */
class IconvWrapper {
	public:
		IconvWrapper (const char* tocode, const char* fromcode) {
			/* create iconv handler */
			m_handle = iconv_open(tocode, fromcode);
			if ( (iconv_t) -1 == m_handle) {
				throw std::runtime_error(std::string("libSzarp: Error opening iconv: ")
						+ fromcode + " to " + tocode);
			}
		}

		~IconvWrapper() {
			iconv_close(m_handle);
		}

		static const char* system_enc (void) {
			return m_sysenc._system_encoding.c_str();
		}

		static bool isutf8 (void) {
			return m_sysenc._utf8;
		}

		template<typename T, typename F>
		std::basic_string<T> convert (const std::basic_string<F> &str, bool omit_inv = false) {
			boost::mutex::scoped_lock lock(m_mutex);	// iconv() is not thread-safe

			/* allocate buffers */
			char* input_start = (char*) str.c_str();
			char* input = input_start;
			size_t input_length = (str.length() + 1) * sizeof(F);

			size_t total_length  = sizeof(T) * (str.length() + 1);
			size_t remaining_length = total_length;
			char* output_start = (char*) malloc(total_length);
			char* output = output_start;

			/* convert string */
			while ( (size_t) -1 == iconv(m_handle, &input,  &input_length,
						&output, &remaining_length))
			{
				if (errno == E2BIG) {
					char* nb = (char*) realloc(output_start, 2 * total_length);

					output_start = nb;
					output = output_start + (total_length - remaining_length);

					remaining_length += total_length;
					total_length *= 2;

				} else if (errno == EILSEQ) {
					if (!omit_inv) {
						free(output_start);
						throw std::runtime_error("Invalid multibyte sequence encountered in conversion");
					}

				} else if (errno == EINVAL) {
					free(output_start);
					throw std::runtime_error("Incomplete multibyte sequence encountered in conversion");

				} else {
					free(output_start);
					throw std::runtime_error("String conversion failed");
				}
			}

			std::basic_string<T> ret(reinterpret_cast<T*>(output_start));
			free(output_start);

			return ret;
		}

	private:
		iconv_t m_handle;
		boost::mutex m_mutex;

		struct SystemEncoding {
			std::string _system_encoding;
			bool _utf8;

			SystemEncoding (void) {
				/* get system locale and extract encoding */
				boost::locale::generator gen;
				std::locale loc = gen("");
				_system_encoding =
					std::use_facet<boost::locale::info>(loc).encoding();
				_utf8 = std::use_facet<boost::locale::info>(loc).utf8();
			}
		};
		static SystemEncoding m_sysenc;
};

IconvWrapper::SystemEncoding IconvWrapper::m_sysenc = IconvWrapper::SystemEncoding();


/* Szarp Conversion objects and helper functions */

/* UTF-8 -> SZARP */
#ifndef MINGW32
IconvWrapper utf2szarp_iw("WCHAR_T", "UTF-8");

std::wstring utf2szarp(const std::basic_string<unsigned char>& c) {
	return utf2szarp_iw.convert<wchar_t, unsigned char>(c);
}
#else
std::wstring utf2szarp(const std::basic_string<unsigned char>& c) {
	if (c.size() == 0)
		return std::wstring();

	int _size = c.size();
	boost::scoped_array<wchar_t> buff(new wchar_t[_size]);
	int res = MultiByteToWideChar(CP_UTF8, 0, (CHAR*)c.c_str(), c.size(), buff.get(), c.size());

	if (res == 0) {
		throw std::runtime_error("Incomplete multibyte sequence encountered in convertsion");
	}

	return std::wstring(buff.get(), res);
}
#endif

/* SZARP -> UTF-8 */
#ifndef MINGW32
IconvWrapper szarp2utf_iw("UTF-8", "WCHAR_T");

std::basic_string<unsigned char> szarp2utf(const std::wstring& c) {
	return szarp2utf_iw.convert<unsigned char, wchar_t>(c);
}
#else
std::basic_string<unsigned char> szarp2utf(const std::wstring& c) {
	if (c.size() == 0)
		return std::basic_string<unsigned char>();

	int size_ = c.size() * sizeof(wchar_t);
	boost::scoped_array<unsigned char> buff(new unsigned char[size_]);
	int res = WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)c.c_str(), c.size(), (CHAR*) buff.get(), size_, NULL, NULL);

	if (res == 0) {
		throw std::runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return std::basic_string<unsigned char>(buff.get(), res);
}
#endif

/* LOCAL -> SZARP */
#ifndef MINGW32
IconvWrapper local2szarp_iw("WCHAR_T", IconvWrapper::system_enc());

std::wstring local2szarp(const std::basic_string<char>& c) {
	return local2szarp_iw.convert<wchar_t, char>(c);
}
#else
std::wstring local2szarp(const std::basic_string<char>& c) {
	if (c.size() == 0)
		return std::wstring();

	boost::scoped_array<wchar_t> buff(new wchar_t[c.size()]);
	int res = MultiByteToWideChar(CP_ACP, 0, c.c_str(), c.size(), buff.get(), c.size() * sizeof(wchar_t));

	if (res == 0) {
		throw std::runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return std::wstring(buff.get(), res);
}
#endif

/* LOCAL -> SZARP (omit invalid characters) */
#ifndef MINGW32
IconvWrapper local2szarp_ign_iw("WCHAR_T//IGNORE", IconvWrapper::system_enc());

std::wstring local2szarp_ign(const std::basic_string<char>& c) {
	return local2szarp_ign_iw.convert<wchar_t, char>(c, true);
}
#endif

/* SZARP -> LOCAL */
#ifndef MINGW32
IconvWrapper szarp2local_iw(IconvWrapper::system_enc(), "WCHAR_T");

std::string szarp2local(const std::basic_string<wchar_t>& c) {
	return szarp2local_iw.convert<char, wchar_t>(c);
}
#else
std::string szarp2local(const std::basic_string<wchar_t>& c) {
	if (c.size() == 0)
		return std::string();

	int size_ = c.size() * 2;
	boost::scoped_array<char> buff(new char[size_]);
	int res = WideCharToMultiByte(CP_ACP, 0, c.c_str(), -1, buff.get(), size_, NULL, NULL);

	if (res == 0) {
		throw std::runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return std::string(buff.get(), res - 1);
}
#endif

/* SZARP -> ASCII */
#ifndef MINGW32
IconvWrapper szarp2ascii_iw("ASCII//TRANSLIT", "WCHAR_T");

std::string szarp2ascii(const std::basic_string<wchar_t>& c) {
	return szarp2ascii_iw.convert<char, wchar_t>(c);
}
#else
std::string szarp2ascii(const std::basic_string<wchar_t>& c) {
	// FIXME: to be honest, it's szarp2local
	if (c.size() == 0)
		return std::string();

	int size_ = c.size() * 2;
	boost::scoped_array<char> buff(new char[size_]);
	int res = WideCharToMultiByte(CP_ACP, 0, c.c_str(), -1, buff.get(), size_, NULL, NULL);

	if (res == 0) {
		throw std::runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return std::string(buff.get(), res - 1);
}
#endif

/* ASCII -> SZARP */
#ifndef MINGW32
IconvWrapper ascii2szarp_iw("WCHAR_T", "ASCII");

std::wstring ascii2szarp(const std::basic_string<char>& c) {
	return ascii2szarp_iw.convert<wchar_t, char>(c);
}
#else
std::wstring ascii2szarp(const std::basic_string<char>& c) {
	if (c.size() == 0)
		return std::wstring();

	boost::scoped_array<wchar_t> buff(new wchar_t[c.size() + 1]);
	int res = MultiByteToWideChar(CP_ACP, 0, c.c_str(), -1, buff.get(), (c.size() + 1) * sizeof(wchar_t));

	if (res == 0) {
		throw std::runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return std::wstring(buff.get(), res);
}
#endif

/* UTF-8 -> ASCII */
IconvWrapper utf2ascii_iw("ASCII//TRANSLIT", "UTF-8");
std::string utf2ascii(const std::basic_string<unsigned char>& c) {
	return utf2ascii_iw.convert<char, unsigned char>(c);
}

/* ASCII -> UTF-8 */
IconvWrapper ascii2utf_iw("UTF-8", "ASCII");
std::basic_string<unsigned char> ascii2utf(const std::basic_string<char>& c) {
	return ascii2utf_iw.convert<unsigned char, char>(c);
}

/* ASCII -> UTF-8 (omit invalid characters) */
IconvWrapper ascii2utf_ign_iw("UTF-8//IGNORE", "ASCII");
std::basic_string<unsigned char> ascii2utf_ign(const std::basic_string<char>& c) {
	return ascii2utf_ign_iw.convert<unsigned char, char>(c, true);
}

/* UTF-8 -> LOCAL */
IconvWrapper utf2local_iw(IconvWrapper::system_enc(), "UTF-8");
std::string utf2local(const std::basic_string<unsigned char>& c) {
	if (IconvWrapper::isutf8())
		return std::string(reinterpret_cast<const char *>(c.c_str()));
	else
		return utf2local_iw.convert<char, unsigned char>(c);
}

/* LOCAL -> UTF-8 */
IconvWrapper local2utf_iw("UTF-8", IconvWrapper::system_enc());
std::basic_string<unsigned char> local2utf(const std::basic_string<char>& c) {
	if (IconvWrapper::isutf8())
		return std::basic_string<unsigned char>(reinterpret_cast<const unsigned char *>(c.c_str()));
	else
		return local2utf_iw.convert<unsigned char, char>(c);
}

/* LOCAL -> UTF-8 (omit invalid characters) */
IconvWrapper local2utf_ign_iw("UTF-8//IGNORE", IconvWrapper::system_enc());
std::basic_string<unsigned char> local2utf_ign(const std::basic_string<char>& c) {
	if (IconvWrapper::isutf8())
		return std::basic_string<unsigned char>(reinterpret_cast<const unsigned char *>(c.c_str()));
	else
		return local2utf_ign_iw.convert<unsigned char, char>(c, true);
}


namespace SC {	/* Szarp Conversions */

/* Descriptions of names:
 *   A means ASCII
 *   L means LOCAL (system encoding)
 *   S means SZARP (internal)
 *   U means UTF-8
 */
	std::wstring U2S(const std::basic_string<unsigned char>& c) {
		return utf2szarp(c);
	}

	std::basic_string<unsigned char> S2U(const std::wstring& c) {
		return szarp2utf(c);
	}

	std::wstring A2S(const std::basic_string<char>& c) {
		return ascii2szarp(c);
	}

	std::string S2A(const std::basic_string<wchar_t>& c) {
		return szarp2ascii(c);
	}

	std::wstring L2S(const std::basic_string<char>& c, bool fallback) {
#ifndef MINGW32
		try {
			return local2szarp(c);
		} catch (const std::runtime_error& ex) {
			if (!fallback)
				throw;
		}
		return local2szarp_ign(c);
#else
		return local2szarp(c);
#endif
	}

	std::string S2L(const std::basic_string<wchar_t>& c) {
		return szarp2local(c);
	}

	std::basic_string<unsigned char> A2U(const std::basic_string<char>& c, bool fallback) {
		try {
			return ascii2utf(c);
		} catch (const std::runtime_error& ex) {
			if (!fallback)
				throw;
		}

		return ascii2utf_ign(c);
	}

	std::string U2A(const std::basic_string<unsigned char>& c) {
		return utf2ascii(c);
	}

	std::basic_string<unsigned char> L2U(const std::basic_string<char>& c, bool fallback) {
		try {
			return local2utf(c);
		} catch (const std::runtime_error& ex) {
			if (!fallback)
				throw;
		}

		return local2utf_ign(c);
	}

	std::string U2L(const std::basic_string<unsigned char>& c) {
		return utf2local(c);
	}

	std::wstring lua_error2szarp(const char* lua_error)
	{

		try {
			return SC::U2S((const unsigned char*)lua_error);

		} catch(const std::runtime_error& ex) { }

		// try to omit the []-bracketed abbreviated error string,
		// which may contain a split in half unicode 2-byte
		// and retain the rest of information
		std::string source(lua_error);
		size_t right_bracket_pos = source.rfind("]");
		std::string target = "[<unparseable string>]";

		if (right_bracket_pos != std::string::npos) {
			size_t first_char_pos = right_bracket_pos + 1;
			if (first_char_pos < source.size()) {
				target += source.substr(first_char_pos);
			}
		}

		try {
			return SC::U2S((const unsigned char*)target.c_str());

		} catch(const std::runtime_error& ex) { }

		return SC::A2S("[<unparseable string>]");
	}

	std::string printable_string(std::string s)
	{
		s.erase(
				std::remove_if(s.begin(), s.end(),
					[]( char ch ) {
						return std::isspace<char>(ch, std::locale::classic()) && !(ch == ' ');
					}),
			s.end());
		return s;
	}
}

namespace std {

ostream& operator<<(ostream& os, const std::basic_string<unsigned char> &us) {
	return os << std::string(us.begin(), us.end());
}

}
