#include <string>

namespace base64 {

std::basic_string<unsigned char> encode(const std::basic_string<unsigned char>& input);

std::basic_string<unsigned char> decode(const std::basic_string<unsigned char>& input);

}
