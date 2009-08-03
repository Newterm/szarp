#include <stdexcept>

#include "conversion.h"
#include <iconv.h>
#include <errno.h>
#include <assert.h>

#include <boost/thread.hpp>
#include <boost/scoped_array.hpp>

#ifdef MINGW32
#include <windows.h>
#endif

using boost::scoped_array;

namespace {

#ifdef MINGW32
	const char *_locale_string = "CP1250";
#else
	const char *_locale_string = "ISO_8859-2";
#endif

#ifdef MINGW32
#define UTF8 "UTF-8"
#else
#define UTF8 "UTF8"
#endif

	class IconvWrapper {
		private:
			iconv_t m_handle;
			boost::mutex m_mutex;
		public:
			IconvWrapper(const char* tocode, const char* fromcode){
				m_handle = iconv_open(tocode, fromcode);
				if (m_handle  == (iconv_t) -1)
					throw std::runtime_error(std::string("LibSzarp: Error opening iconv: ") + fromcode + " to " + tocode);
			}

			~IconvWrapper() {
				iconv_close(m_handle);
			}

		template<typename T, typename F> std::basic_string<T> convert(const std::basic_string<F> &str) {

			boost::mutex::scoped_lock lock(m_mutex);

			char *input_start = (char*)(str.c_str());
			char *input = input_start;
			size_t input_length = (str.length() + 1) * sizeof(F);
	
			size_t total_length  = sizeof(T) * (str.length() + 1);
			size_t remaining_length = total_length;
			char *output_start = (char*) malloc(total_length);
			char *output = output_start;

			while (iconv(m_handle, 
#ifdef MINGW32
						(const char**)
#endif
							&input, &input_length, &output, &remaining_length) == (size_t) -1) {
				if (errno == E2BIG) {
					char *nb = (char*)realloc(output_start, 2 * total_length);

					output_start = nb;
					output = output_start + (total_length - remaining_length);

					remaining_length += total_length;
					total_length *= 2;
				} else if (errno == EILSEQ) {
					free(output_start);

					throw std::runtime_error("Invalid multibyte sequence encountered in convertsion");
				} else if (errno == EINVAL) {
					free(output_start);

					throw std::runtime_error("Incomplete multibyte sequence encountered in convertsion");

				} else {
					free(output_start);

					throw std::runtime_error("String convertsion failed");
				}
			}

			std::basic_string<T> ret(reinterpret_cast<T*>(output_start));

			free(output_start);

			return ret;
		}
	
	};

}

namespace SC {

#ifndef MINGW32
	IconvWrapper utf2szarp_iw("WCHAR_T", UTF8);
#endif
	std::wstring utf2szarp(const std::basic_string<unsigned char>& c) {
#ifdef MINGW32
		if (c.size() == 0)
			return std::wstring();

		int _size = c.size() + 1;
		scoped_array<wchar_t> buff(new wchar_t[_size]);
		int res = MultiByteToWideChar(CP_UTF8, 0, (CHAR*)c.c_str(), c.size(), buff.get(), c.size() + 1);

		if (res == 0) {
			throw std::runtime_error("Incomplete multibyte sequence encountered in convertsion");
		}
		
		return std::wstring(buff.get(), res);
#else
		return utf2szarp_iw.convert<wchar_t, unsigned char>(c);
#endif
	}

#ifndef MINGW32
	IconvWrapper szarp2utf_iw(UTF8, "WCHAR_T");
#endif
	std::basic_string<unsigned char> szarp2utf(const std::wstring& c) {
#ifdef MINGW32
		if (c.size() == 0)
			return std::basic_string<unsigned char>();

		int size_ = c.size() * sizeof(wchar_t);
		scoped_array<unsigned char> buff(new unsigned char[size_]);
		int res = WideCharToMultiByte(CP_UTF8, 0, (WCHAR*)c.c_str(), c.size(), (CHAR*) buff.get(), size_, NULL, NULL);

		if (res == 0) {
			throw std::runtime_error("Incomplete multibyte sequence encountered in convertsion");
		}
		
		return std::basic_string<unsigned char>(buff.get(), res);
#else
		return szarp2utf_iw.convert<unsigned char, wchar_t>(c);
#endif
	}

#ifndef MINGW32
	IconvWrapper ascii2szarp_iw("WCHAR_T", _locale_string);
#endif
	std::wstring ascii2szarp(const std::basic_string<char>& c) {
#ifdef MINGW32
		if (c.size() == 0)
			return std::wstring();

		scoped_array<wchar_t> buff(new wchar_t[c.size() + 1]);
		int res = MultiByteToWideChar(CP_ACP, 0, c.c_str(), -1, buff.get(), (c.size() + 1) * sizeof(wchar_t));

		if (res == 0) {
			throw std::runtime_error("Incomplete multibyte sequence encountered in convertsion");
		}
		
		return std::wstring(buff.get(), res);
#else
		return ascii2szarp_iw.convert<wchar_t, char>(c);
#endif
	}

#ifndef MINGW32
	IconvWrapper szarp2ascii_iw(_locale_string, "WCHAR_T");
#endif
	std::string szarp2ascii(const std::basic_string<wchar_t>& c) {
#ifdef MINGW32
		if (c.size() == 0)
			return std::string();

		int size_ = c.size() * 2;
		scoped_array<char> buff(new char[size_]);
		int res = WideCharToMultiByte(CP_ACP, 0, c.c_str(), -1, buff.get(), size_, NULL, NULL);

		if (res == 0) {
			throw std::runtime_error("Incomplete multibyte sequence encountered in convertsion");
		}
		
		return std::string(buff.get(), res);
#else
		return szarp2ascii_iw.convert<char, wchar_t>(c);
#endif
	}

	IconvWrapper utf2ascii_iw(_locale_string, UTF8);
	std::string utf2ascii(const std::basic_string<unsigned char>& c) {
		return utf2ascii_iw.convert<char, unsigned char>(c);
	}

	IconvWrapper ascii2utf_iw(UTF8, _locale_string);
	std::basic_string<unsigned char> ascii2utf(const std::basic_string<char>& c) {
		return ascii2utf_iw.convert<unsigned char, char>(c);
	}

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

	std::basic_string<unsigned char> A2U(const std::basic_string<char>& c) {
		return ascii2utf(c);
	}

	std::string U2A(const std::basic_string<unsigned char>& c) {
		return utf2ascii(c);
	}

}
