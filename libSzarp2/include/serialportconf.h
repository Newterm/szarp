#ifndef __SERIALPORTCONF_H_
#define __SERIALPORTCONF_H_

#include <string>

/**self-descriptive struct holding all aspects of serial port conifguration in one place*/
class SerialPortConfiguration {
public:
	SerialPortConfiguration()
	:
	path(""), parity(NONE), stop_bits(1), speed(0), char_size(CS_8)
	{}

	std::string path;
	enum PARITY {
		NONE,
		ODD,
		EVEN,
		MARK,
		SPACE
	} parity;
	int stop_bits;
	int speed;
	enum CHAR_SIZE {
		CS_5,
		CS_6,
		CS_7,
		CS_8
	} char_size;

	int GetCharSizeInt() const
	{
		int size = 0;
		switch (char_size) {
			case CS_8:
				size = 8;
				break;
			case CS_7:
				size = 7;
				break;
			case CS_6:
				size = 6;
				break;
			case CS_5:
				size = 5;
				break;
		}
		return size;
	}
};

#endif // __SERIALPORTCONF_H__
