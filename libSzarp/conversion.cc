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

using boost::scoped_array;
using namespace std;

/* IconvWrapper - wrapper class for iconv library conversions */
class IconvWrapper {

	public:
		IconvWrapper (const char* tocode, const char* fromcode) {
			/* create iconv handler */
			m_handle = iconv_open(tocode, fromcode);
			if ( (iconv_t) -1 == m_handle) {
				throw runtime_error(string("libSzarp: Error opening iconv: ")
						+ fromcode + " to " + tocode);
			}
		}

		~IconvWrapper() {
			iconv_close(m_handle);
		}

		static const char* system_enc (void) {
			return m_sysenc._system_encoding.c_str();
		}

		template<typename T, typename F>
		basic_string<T> convert (const basic_string<F> &str) {
			/* strange things going here... */
			boost::mutex::scoped_lock lock(m_mutex);

			char* input_start = (char*) str.c_str();
			char* input = input_start;
			size_t input_length = (str.length() + 1) * sizeof(F);

			size_t total_length  = sizeof(T) * (str.length() + 1);
			size_t remaining_length = total_length;
			char* output_start = (char*) malloc(total_length);
			char* output = output_start;

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
					free(output_start);
					throw runtime_error("Invalid multibyte sequence encountered in conversion");

				} else if (errno == EINVAL) {
					free(output_start);
					throw runtime_error("Incomplete multibyte sequence encountered in conversion");

				} else {
					free(output_start);
					throw runtime_error("String conversion failed");
				}
			}

			basic_string<T> ret(reinterpret_cast<T*>(output_start));
			free(output_start);

			return ret;
		}

	private:
		iconv_t m_handle;
		boost::mutex m_mutex;

		struct SystemEncoding {
			string _system_encoding;

			SystemEncoding (void) {
				/* get system locale and extract encoding */
				boost::locale::generator gen;
				locale loc = gen("");
				_system_encoding =
					use_facet<boost::locale::info>(loc).encoding();
			}
		};
		static SystemEncoding m_sysenc;
};

IconvWrapper::SystemEncoding IconvWrapper::m_sysenc = IconvWrapper::SystemEncoding();


/* Szarp Conversion objects and helper functions */

/* UTF-8 -> SZARP */
#ifndef MINGW32
IconvWrapper utf2szarp_iw("WCHAR_T", "UTF-8");

wstring utf2szarp(const basic_string<unsigned char>& c) {
	return utf2szarp_iw.convert<wchar_t, unsigned char>(c);
}
#else
wstring utf2szarp(const basic_string<unsigned char>& c) {
	if (c.size() == 0)
		return wstring();

	int _size = c.size() + 1;
	scoped_array<wchar_t> buff(new wchar_t[_size]);
	int res = MultiByteToWideChar(CP_UTF8, 0, (CHAR*)c.c_str(), c.size(), buff.get(), c.size() + 1);

	if (res == 0) {
		throw runtime_error("Incomplete multibyte sequence encountered in convertsion");
	}

	return wstring(buff.get(), res);
}
#endif

/* SZARP -> UTF-8 */
#ifndef MINGW32
IconvWrapper szarp2utf_iw("UTF-8", "WCHAR_T");

basic_string<unsigned char> szarp2utf(const wstring& c) {
	return szarp2utf_iw.convert<unsigned char, wchar_t>(c);
}
#else
basic_string<unsigned char> szarp2utf(const wstring& c) {
	if (c.size() == 0)
		return basic_string<unsigned char>();

	int size_ = c.size() * sizeof(wchar_t);
	scoped_array<unsigned char> buff(new unsigned char[size_]);
	int res = WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)c.c_str(), c.size(), (CHAR*) buff.get(), size_, NULL, NULL);

	if (res == 0) {
		throw runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return basic_string<unsigned char>(buff.get(), res);
}
#endif

/* LOCAL -> SZARP */
#ifndef MINGW32
IconvWrapper local2szarp_iw("WCHAR_T", IconvWrapper::system_enc());

wstring local2szarp(const basic_string<char>& c) {
	return local2szarp_iw.convert<wchar_t, char>(c);
}
#else
wstring local2szarp(const basic_string<char>& c) {
	if (c.size() == 0)
		return wstring();

	scoped_array<wchar_t> buff(new wchar_t[c.size() + 1]);
	int res = MultiByteToWideChar(CP_ACP, 0, c.c_str(), -1, buff.get(), (c.size() + 1) * sizeof(wchar_t));

	if (res == 0) {
		throw runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return wstring(buff.get(), res);
}
#endif

/* SZARP -> LOCAL */
#ifndef MINGW32
IconvWrapper szarp2local_iw(IconvWrapper::system_enc(), "WCHAR_T");

string szarp2local(const basic_string<wchar_t>& c) {
	return szarp2local_iw.convert<char, wchar_t>(c);
}
#else
string szarp2local(const basic_string<wchar_t>& c) {
	if (c.size() == 0)
		return string();

	int size_ = c.size() * 2;
	scoped_array<char> buff(new char[size_]);
	int res = WideCharToMultiByte(CP_ACP, 0, c.c_str(), -1, buff.get(), size_, NULL, NULL);

	if (res == 0) {
		throw runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return string(buff.get(), res - 1);
}
#endif

/* SZARP -> ASCII */
#ifndef MINGW32
IconvWrapper szarp2ascii_iw("ASCII//TRANSLIT", "WCHAR_T");

string szarp2ascii(const basic_string<wchar_t>& c) {
	return szarp2ascii_iw.convert<char, wchar_t>(c);
}
#else
string szarp2ascii(const basic_string<wchar_t>& c) {
	// TODO: to be honest, it's szarp2local
	if (c.size() == 0)
		return string();

	int size_ = c.size() * 2;
	scoped_array<char> buff(new char[size_]);
	int res = WideCharToMultiByte(CP_ACP, 0, c.c_str(), -1, buff.get(), size_, NULL, NULL);

	if (res == 0) {
		throw runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return string(buff.get(), res - 1);
}
#endif

/* ASCII -> SZARP */
#ifndef MINGW32
IconvWrapper ascii2szarp_iw("WCHAR_T", "ASCII");

wstring ascii2szarp(const basic_string<char>& c) {
	return ascii2szarp_iw.convert<wchar_t, char>(c);
}
#else
wstring ascii2szarp(const basic_string<char>& c) {
	if (c.size() == 0)
		return wstring();

	scoped_array<wchar_t> buff(new wchar_t[c.size() + 1]);
	int res = MultiByteToWideChar(CP_ACP, 0, c.c_str(), -1, buff.get(), (c.size() + 1) * sizeof(wchar_t));

	if (res == 0) {
		throw runtime_error("Incomplete multibyte sequence encountered in conversion");
	}

	return wstring(buff.get(), res);
}
#endif

/* UTF-8 -> ASCII */
IconvWrapper utf2ascii_iw("ASCII//TRANSLIT", "UTF-8");
string utf2ascii(const basic_string<unsigned char>& c) {
	return utf2ascii_iw.convert<char, unsigned char>(c);
}

/* ASCII -> UTF-8 */
IconvWrapper ascii2utf_iw("UTF-8", "ASCII");
basic_string<unsigned char> ascii2utf(const basic_string<char>& c) {
	return ascii2utf_iw.convert<unsigned char, char>(c);
}

/* UTF-8 -> LOCAL */
IconvWrapper utf2local_iw(IconvWrapper::system_enc(), "UTF-8");
string utf2local(const basic_string<unsigned char>& c) {
	return utf2local_iw.convert<char, unsigned char>(c);
}

/* LOCAL -> UTF-8 */
IconvWrapper local2utf_iw("UTF-8", IconvWrapper::system_enc());
basic_string<unsigned char> local2utf(const basic_string<char>& c) {
	return local2utf_iw.convert<unsigned char, char>(c);
}


namespace SC {	/* Szarp Conversions */

/* Descriptions of names:
 *   A means ASCII
 *   L means LOCAL (system encoding)
 *   S means SZARP (internal)
 *   U means UTF-8
 */
	wstring U2S(const basic_string<unsigned char>& c) {
		return utf2szarp(c);
	}

	basic_string<unsigned char> S2U(const wstring& c) {
		return szarp2utf(c);
	}

	wstring A2S(const basic_string<char>& c) {
		return ascii2szarp(c);
	}

	string S2A(const basic_string<wchar_t>& c) {
		return szarp2ascii(c);
	}

	wstring L2S(const basic_string<char>& c) {
		return local2szarp(c);
	}

	string S2L(const basic_string<wchar_t>& c) {
		return szarp2local(c);
	}

	basic_string<unsigned char> A2U(const basic_string<char>& c) {
		return ascii2utf(c);
	}

	string U2A(const basic_string<unsigned char>& c) {
		return utf2ascii(c);
	}

	basic_string<unsigned char> L2U(const basic_string<char>& c) {
		return local2utf(c);
	}

	string U2L(const basic_string<unsigned char>& c) {
		return utf2local(c);
	}

	wstring lua_error2szarp(const char* lua_error)
	{

		try {
			return SC::U2S((const unsigned char*)lua_error);

		} catch(const runtime_error& ex) { }

		// try to omit the []-bracketed abbreviated error string,
		// which may contain a split in half unicode 2-byte
		// and retain the rest of information
		string source(lua_error);
		size_t right_bracket_pos = source.rfind("]");
		string target = "[<unparseable string>]";

		if (right_bracket_pos != string::npos) {
			size_t first_char_pos = right_bracket_pos + 1;
			if (first_char_pos < source.size()) {
				target += source.substr(first_char_pos);
			}
		}

		try {
			return SC::U2S((const unsigned char*)target.c_str());

		} catch(const runtime_error& ex) { }

		return SC::A2S("[<unparseable string>]");
	}
}

