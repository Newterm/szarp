#include <stdexcept>
#include "szarpbase64.h"
namespace base64 {

namespace {
const char* base64_alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

std::basic_string<unsigned char> encode(const std::basic_string<unsigned char>& input) {
	std::basic_string<unsigned char> ret;

	unsigned char pv = 0;
	int state = 0;

	for (size_t i = 0; i < input.size(); i++) {
		unsigned char c = input[i];
		switch (state) {
			case 0:
				ret += (unsigned char) base64_alphabet[c >> 2];
				pv = (c & 0x3) << 4;
				state = 1;
				break;
			case 1:
				ret += (unsigned char) base64_alphabet[pv | (c >> 4)];
				pv = (c & 0xf) << 2;
				state = 2;
				break;
			case 2:
				ret += (unsigned char) base64_alphabet[pv | (c >> 6)];
				ret += (unsigned char) base64_alphabet[c & 0x3f];
				state = 0;
				break;
		}
	}

	switch (state) {
		case 0:
			break;
		case 1:
			ret += (unsigned char) base64_alphabet[pv];
			ret += '=';
			ret += '=';
			break;
		case 2:
			ret += (unsigned char) base64_alphabet[pv];
			ret += '=';
			break;
	}

	return ret;
}

namespace {

unsigned char base64_char_to_val(unsigned char v) {
	switch (v) {
		case 'A'...'Z':
			return v - 'A';
		case 'a'...'z':
			return v - 'a' + 26;
		case '0'...'9':
			return v - '0' + 52;
		case '+':
			return 62;
		case '/':
			return 63;
		case '=':
			return 0;
		default:
			throw std::range_error("encutered invalid character");

	}
	//NOTREACHED
	return 0;
}

}

std::basic_string<unsigned char> decode(const std::basic_string<unsigned char>& input) {
	std::basic_string<unsigned char> ret;

	unsigned char pv = 0;
	unsigned char cv;
	int state = 0;

	try {
		for (size_t i = 0; i < input.size(); i++) {
			unsigned char c = input[i];
			if (c == '=')
				break;

			switch (state) {
				case 0:
					pv = base64_char_to_val(c) << 2;
					state = 1;
					break;
				case 1:
					cv = base64_char_to_val(c);
					ret += (unsigned char) (pv | (cv >> 4));
					pv = (cv & 0xf) << 4;
					state = 2;
					break;
				case 2:
					cv = base64_char_to_val(c);
					ret += (unsigned char) (pv | (cv >> 2));
					pv = (cv & 0x3) << 6;
					state = 3;
					break;
				case 3:
					cv = base64_char_to_val(c);
					ret += (unsigned char) (pv | cv);
					state = 0;
					break;
			}
		}

		if (state != 0)
			ret += pv;

		return ret;

	} catch (std::range_error) {
		return std::basic_string<unsigned char>();
	}

}

}
