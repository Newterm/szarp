#include "bserialport.h"
#include "daemonutils.h"

void BaseSerialPort::SerialPortConfigurationToTermios(
	const SerialPortConfiguration& config, struct termios* termios)
{
	termios->c_iflag = 0;
	termios->c_oflag = 0;
	termios->c_cflag = 0;
	termios->c_lflag = 0;

	termios->c_cflag |= speed_to_constant(config.speed);

	if (config.stop_bits == 2) {
		termios->c_cflag |= CSTOPB;
	}

	switch (config.parity) {
		case SerialPortConfiguration::EVEN:
			termios->c_cflag |= PARENB;
			break;
		case SerialPortConfiguration::ODD:
			termios->c_cflag |= PARENB | PARODD;
			break;
		case SerialPortConfiguration::MARK:
			termios->c_cflag |= PARENB | CMSPAR | PARODD;
			break;	
		case SerialPortConfiguration::SPACE:
			termios->c_cflag |= PARENB | CMSPAR;
			break;
		case SerialPortConfiguration::NONE:
			break;
	}

	termios->c_cflag |= CREAD | CLOCAL ;

	switch (config.char_size) {
		case SerialPortConfiguration::CS_8:
			termios->c_cflag |= CS8;
			break;
		case SerialPortConfiguration::CS_7:
			termios->c_cflag |= CS7;
			break;
		case SerialPortConfiguration::CS_6:
			termios->c_cflag |= CS6;
			break;
		case SerialPortConfiguration::CS_5:
			termios->c_cflag |= CS5;
			break;
	}
}
