/* 
  SZARP: SCADA software 

*/
#ifndef MBUS_H
#define MBUS_H

#include "config.h"

#ifndef MINGW32

#include <stdint.h>
#include <string>
#include <iostream>
#include <vector>
#include <ctime>

#include "szdefines.h"

/**
 * Represents a prefix which is prepended to the outputted text.
 * Basically it supports outputting a number of identical characters to
 * the output stream but can be easily extended to control the output
 * stream in any other way.
 * @see std::string
 * @see std::ostream
 */
class OutputPrefix {
    public:
        /** Creates an empty prefix. */
        OutputPrefix() : prefix("") 
        { }

        /** Creates a prefix with the given content. */
        OutputPrefix(std::string _prefix) : prefix(_prefix)
        { }

        /** Appends a string at the end of current prefix.
         * @param suffix the string to append
         * @return a reference to the modified OutputPrefix object
         */
        OutputPrefix& operator+=(std::string suffix);

        /** Removes last characters from the prefix.
         * @param length length of the string (or number of characters)
         * to remove
         * @return a reference to the modified OutputPrefix object
         */
        OutputPrefix& operator-=(int length);

        /** Duplicates the last character of the prefix (a prefix operator).
         * @return a reference to the modified OutputPrefix object
         */
        OutputPrefix& operator++();

        /** Duplicates the last character of the prefix (a postfix operator).
         * @param dummy parameter for overloading purposes
         * @return the modified OutputPrefix object
         */
        OutputPrefix operator++(int dummy);

        /** Removes the last character from the prefix (a prefix
         * operator).
         * @return a reference to the modified OutputPrefix object
         */
        OutputPrefix& operator--();

        /** Removes the last character from the prefix (a postfix
         * operator).
         * @param dummy parameter for overloading purposes
         * @return the modified OutputPrefix object
         */
        OutputPrefix operator--(int dummy);

        /** Returns the current length of the prefix
         * @see std::string::length()
         * @return current length of the prefix
         */
        inline int length() {
            return prefix.length();
        }

        friend std::ostream& operator<<(std::ostream& os, OutputPrefix& op);

    private:
        std::string prefix; /**< The prefix itself. */
};

/** Outputs the current prefix to the given output stream.
 * @see OutputStream
 * @see std::ostream
 * @param os the output stream to output the prefix to
 * @param op the output prefix to output
 * @return a reference to the modified output prefix
 */
std::ostream& operator<<(std::ostream& os, OutputPrefix& op);

/** Represents and handles connections in the M-Bus protocol.
 * This class provides various methods, types, constants etc. which are
 * used for connecting and communicating with other devices using the
 * M-Bus protocol. Currently it supports:
 * - connecting to the device using the serial port with the given
 *   baudrate, byte_interval, number of data and stop bits and parity,
 * - initializing the communication (i.e. sending a SND_NKE2 frame),
 * - performing an application reset on the other device (i.e. sending
 *   a SND_UD frame containing a request to do a full application reset
 *   or a setup application reset),
 * - selecting data for readout from the other device (i.e. sending a 
 *   SND_UD frame with appropriate content; although the provided method 
 *   is quite general and may be used to select any type of data, a
 *   constant is only provided for selecting all the data (a.k.a. global
 *   readout),
 * - changing the slave's primary address (i.e. sending a SND_UD frame
 *   with appropriate content),
 * - querying the other device for status (i.e. sending a REQ_SKE frame
 *   and outputting the reply),
 * - querying the other device for data (i.e. sending a REQ_UD2 frame
 *   and parsing the received RSP_UD frame),
 * - dumping a M-Bus frame with comments or as hex values.
 */
class MBus {
    public:
        typedef uint8_t byte; /**< A platform-independent type representing one byte of data. */
        typedef uint16_t double_byte; /**< A platform-independent type representing two bytes of data. */
        typedef uint32_t quad_byte; /**< A platform-independent type representing four bytes of data. */
        typedef uint64_t eight_byte; /**< A platform-independent type representing eight bytes of data. */
        typedef long long value_t; /**< A type used to represent the values received from the slave device. */
        typedef unsigned long long unsigned_value_t; /**< A type used to represent the values received from the slave device when representing the sign is unnecessary. */

        /** Type of a M-Bus frame. */
        enum frame_type {
            single_char, /**< A single-char frame, i.e. a positive reply. */
            short_frame, /**< A short frame, used mainly for sending requests to the slave device. */
            long_frame, /**< A long frame, used for transferring large blocks of data. */
            control_frame, /**< A control frame, used for transmitting simple commands. */
            unknown_frame /**< An unknown type of frame, usually this means an invalid and/or corrupted M-Bus frame. */
        };

        /** Type of parity bits used in the transmission. */ 
        enum parity_type {
            none, /**< No parity bits are used whatsoever. */
            even, /**< Even parity bits are used. */
            odd /**< Odd parity bits are used. */
        };

        static const byte broadcast_with_reply = 0xFE; /**< Value which should be given to the address field to generate a broadcast message with reply request. */
        static const byte broadcast_without_reply = 0xFF; /**< Value which should be given to the address field to generate a broadcast message without reply request. */

        // Needs to be pushed back to a vector so cannot be static
        const byte dif_global_readout; /**< Value of the DIF field meaning a global readout request (i.e. a request to all the devices to send all their data to the master device). */

        // Needs to be pushed back to a vector so cannot be static
        const byte full_application_reset; /**< Value of the data field meaning a full application request (i.e. a request to restore all the application data to the manufacturer's defaults). */
        // Needs to be pushed back to a vector so cannot be static
        const byte setup_application_reset; /**< Value of the data field meaning a setup application request (i.e. a request to restore only the address and fixed dates to the manufacturer's defaults). */

        /** Create a new instance of the M-Bus handler class.
         * This constructor creates an empty, unconnected object of the
         * M-Bus handler class. It initializes some constants which
         * cannot be initialized otherwise and also sets the precision
         * filed.
         * @see precision
         * @param _precision a value to be given to the precision field
         */
        MBus(unsigned short int _precision = 4) : 
            dif_global_readout(0x7F),
            full_application_reset(0x00),
            setup_application_reset(0x80),
            positive_reply(std::string(1, static_cast<unsigned char>(0xE5))),
            short_frame_start(std::string(1, static_cast<unsigned char>(0x10))),
            frame_start(std::string(1, static_cast<unsigned char>(0x68))),
            frame_stop(std::string(1, static_cast<unsigned char>(0x16))),
            control_frame_len_field(std::string(1, '3')),
            erroneus_frame_content(""),
            dif_change_slave_address(0x01),
            vif_change_slave_address(0x7A),
            serial_port_fd(0), serial_port_byte_interval(0), 
            precision(_precision) { }

        /** Opens a connection to the device.
         * Performs all the needed actions to open a useful connection
         * to the slave device:
         * - opens the given serial port device,
         * - sets it's baudrate, number of data and stop bits and parity
         *   type,
         * - initializes it's electrical state,
         * - initializes the file descriptor and bit time fields.
         * @see serial_port_fd
         * @see serial_port_byte_interval
         * @see open(const char*, int)
         * @see tcgetattr(int, struct termios *)
         * @see tcsetattr(int, int, const struct termios *)
         * @see ioctl()
         * @param device the device to open as the serial port
         * @param baudrate baudrate to use in communication with the other device
         * @param byte_interval time to wait between reading bytes
         * @param data_bits number of data bits to use in communication with the other device
         * @param stop_bits number of stop bits to use in communication with the other device
         * @param parity type of parity to use in communication with the other device
         * @return true if the connection and parameters' setting succeeded, false otherwise
         */
        bool connect(std::string device, unsigned long int baudrate = 300, unsigned long int byte_interval = 10000, 
                unsigned int data_bits = 8, unsigned int stop_bits = 1, parity_type parity = none);

        /** Closes the connection to the device.
         * Simply closes the serial port's file descriptor.
         * @see serial_port_fd
         * @see close(int)
         * @return true on success, false otherwise
         */
        bool disconnect();

        /** Initializes the communication in terms of the M-Bus
         * protocol.
         * Sends a SND_NKE2 request to the other device (i.e. a request
         * to perform a data link reset), reads the reply and checks for
         * success. It also initializes the other device's address field
         * and prints debugging information to the given output stream,
         * if requested.
         * @see SND_NKE2
         * @see send_frame
         * @see receive_frame
         * @see destination_address
         * @param address address of the other device
         * @param debug indicates if the debugging information should be printed
         * @param os output stream to print the debugging information to
         * @return true if the initialization succeeded (i.e. a positive reply was received from the other device), false otherwise
         */
        bool initialize_communication(byte address = broadcast_with_reply, bool sontex = false, bool debug = false, std::ostream& os = std::cerr);

        /** Performs an application reset on the other device.
         * Send a SND_UD frame to the other device containing a request
         * to perform a application request of the given type. This
         * class provides constants for two types of application resets:
         * a full application reset and a setup application reset. Also,
         * it prints debugging information to the given output stream,
         * if requested.
         * @see SND_UD
         * @see send_frame
         * @see receive_frame
         * @see full_application_reset
         * @see setup_application_reset
         * @param address address of the other device
         * @param reset_type type of reset to perform (given as a byte to send in the M-Bus protocol)
         * @param debug indicates if the debugging information should be printed
         * @param os output stream to print the debugging information to
         * @return true if the initialization succeeded (i.e. a positive reply was received from the other device), false otherwise
         */
        bool reset_application(byte address, byte reset_type, bool sontex = false, bool debug = false, std::ostream& os = std::cerr);

        /** Selects data for readout from the other device.
         * Sends a SND_UD frame to the other device containing a request
         * to return selected data. This class provides only one 
         * constant: for performing a global readout request (i.e.
         * selecting all the data from all the connected devices). Also,
         * it prints debugging information to the given output stream,
         * if requested.
         * @see SND_UD
         * @see send_frame
         * @see receive_frame
         * @see dif_global_readout
         * @see parse_data_from_rsp_ud
         * @param address address of the other device
         * @param data data to be read (given as a vector of bytes to send in the M-Bus protocol)
         * @param debug indicates if the debugging information should be printed
         * @param os output stream to print the debugging information to
         * @return values read from the other device or an empty vector if the request was ignored, the other device didn't reply giving the values or parsing the reply failed
         */
        std::vector<value_t> select_data_for_readout(byte address, std::vector<byte> data, bool sontex = false, bool debug = false, std::ostream& os = std::cerr);

        /** Changes the other device's primary address
         * Sends a SND_UD frame containing a request to change the
         * other's device primary address to the given one. Also, it
         * prints debugging information to the given output stream, if
         * requested.
         * @see SND_UD
         * @see send_frame
         * @see receive_frame
         * @see dif_change_slave_address
         * @see vif_change_slave_address
         * @param address address of the other device
         * @param new_address the address to which the other device's primary address should be changed
         * @param debug indicates if the debugging information should be printed
         * @param os output stream to print the debugging information to
         * @return true if the address change succeeded (i.e. a positive reply was received from the other device), false otherwise
         */
        bool change_slave_address(byte address, byte new_address, bool sontex = false, bool debug = false, std::ostream& os = std::cerr);

        /** Queries the other device for data of M-Bus type 2.
         * Sends a REQ_UD2 frame to the other device, then receives the
         * reply, performs parsing on it and returns values to the
         * caller. Also, it prints debugging information and dumps frames
         * in hex to the given output stream, if requested.
         * @see REQ_UD2
         * @see send_frame
         * @see receive_frame
         * @see parse_data_from_rsp_ud
         * @see dump_frame_hex
         * @param address address of the other device
         * @param debug indicates if the debugging information should be printed
         * @param dump_hex indicates if the frame content should be printed in hex
         * @param os output stream to print the debugging information to
         * @return values read from the other device or an empty vector if the request was ignored, the other device didn't reply giving the values or parsing the reply failed
         */
        std::vector<value_t> query_for_data(byte address, bool sontex = false, bool debug = false, bool dump_hex = false, std::ostream& os = std::cerr);

        /** Queries the other device for status information.
         * Sends a REQ_SKE frame to the other device and returns the
         * reply (i.e. a complete M-Bus frame received) to the caller.
         * Also, it prints debugging information to the given output
         * stream, if requested.
         * @see REQ_SKE
         * @see send_frame
         * @see receive_frame
         * @param address address of the other device
         * @param debug indicates if the debugging information should be printed
         * @param os output stream to print the debugging information to
         * @return the M-Bus frame received from the other device
         */
        std::string query_for_status(byte address, bool sontex = false, bool debug = false, std::ostream& os = std::cerr);

        /** Parses a M-Bus frame and prints it's contents to the given
         * output stream with comments.
         * @param os output stream to print the frame contents to
         * @param frame the frame to parse
         */
        void dump_frame(std::ostream& os, std::string frame); // Uses non-static members so cannot be static

        /** Prints a M-Bus frame in hex.
         * Prints the hex values of all the bytes in the given M-Bus
         * frame.
         * @param os output stream to print the data to
         * @param frame the frame to parse
         */
        static void dump_frame_hex(std::ostream& os, std::string frame);

    private:
        /** Allows direct access to all the bytes of a value of any
         * type.
         * This class treats a value of any type as an union of this
         * value and an array of bytes of appropriate size. This way
         * it's possible to operate on both the value itself and it's
         * individual bytes.
         */
        template<typename T> union byte_representation {
            T value; /**< The value itself. */
            byte bytes[sizeof(T)]; /**< An array of individual bytes of the value. */
        };

        // Needs to be initialized in a special way so cannot be static
        std::string positive_reply; /**< Represents a M-Bus frame containing the positive reply. */ 
        // Needs to be initialized in a special way so cannot be static
        std::string short_frame_start; /**< Represents the start field of a M-Bus short frame. */
        // Needs to be initialized in a special way so cannot be static
        std::string frame_start; /**< Represents the start field of a M-Bus control or long frame. */
        // Needs to be initialized in a special way so cannot be static
        std::string frame_stop; /**< Represents the stop field of a M-Bus frame. */
        // Needs to be initialized in a special way so cannot be static
        std::string control_frame_len_field; /**< Represents the length field of a M-Bus control frame. */
        // Needs to be initialized in a special way so cannot be static
        std::string erroneus_frame_content; /**< Represents a frame content used internally to signalize an erroneous frame. */

        static const byte SND_NKE2 = 0x40; /**< The value of the C field used when sending a SND_NKE2 (data link reset)  frame. */
        static const byte REQ_SKE = 0x49; /**< The value of the C field used when sending a REQ_SKE (status request) frame. */
        static const byte SND_UD = 0x53; /**< The value of the C field used when sending a SND_UD (user-data send) frame. */
        static const byte REQ_UD1 = 0x5A; /**< The value of the C field used when sending a REQ_UD1 (request of type 1 data) frame. */
        static const byte REQ_UD2 = 0x5B; /**< The value of the C field used when sending a REQ_UD2 (request of type 2 data) frame. */
        static const byte RSP_UD = 0x08; /**< The value of the C field used when receiving a RSP_UD (reply with data) frame. */

        static const unsigned short int frame_content_max_len = 255;

        static const byte ci_application_reset = 0x50; /**< The value of the CI field when sending an application reset request. */
        static const byte ci_data_send = 0x51; /**< The value of the CI field when sending some data to the other device (e.g. when selecting data for readout or changing the primary address). */

        // Needs to be pushed back to a vector so cannot be static
        const byte dif_change_slave_address; /**< The value of the DIF field when sending a request to change the other device's primary address. */
        // Needs to be pushed back to a vector so cannot be static
        const byte vif_change_slave_address; /**< The value of the VIF field when sending a request to change the other device's primary address. */

        static const byte general_error = 0x70; /**< The value of the CI field when receiving a general error frame. */
        static const byte alarm = 0x71; /**< The value of the CI field when receiving an alarm frame. */
        static const byte variable_data = 0x72; /**< The value of the CI field when receiving a variable data structure frame. */
        static const byte fixed_data = 0x73; /**< The value of the CI field when receiving a fixed data structure frame. */
        static const byte variable_data_msb_first = 0x76; /**< The value of the CI field when receiving a variable data structure frame with MSB first. */
        static const byte fixed_data_msb_first = 0x77; /**< The value of the CI field when receiving a fixed data structure frame with MSB first. */
        static const byte msb_first = 0x04; /**< Bit mask to be applied to the CI field to check if received data has MSB first. */

        static const unsigned short int short_frame_data_offset = 1; /**< Offset (in bytes) of the beginning of data in a short frame (counted from the beginning of the frame). */
        static const unsigned short int short_frame_data_len = 2; /**< Length (in bytes) of data in a short frame. */
        static const unsigned short int short_frame_stop_offset = 4; /**< Offset (in bytes) of the stop field in a short frame (counted from the beginning of the frame). */
        static const unsigned short int frame_data_offset = 4; /**< Offset (in bytes) of the beginning of data in a control or long frame (counted from the beginning of the frame). */
        static const unsigned short int control_frame_data_len = 3; /**< Length (in bytes) of data in a control frame. */
        static const unsigned short int control_frame_stop_offset = 4; /**< Offset (in bytes) of the stop field in a control frame (counted from the beginning of the frame). */
        static const unsigned short int frame_start_offset = 0; /**< Offset (in bytes) of the start field in any frame (counted from the beginning of the frame). */
        static const unsigned short int frame_start_len = 1; /**< Length (in bytes) of the start field in any frame. */
        static const unsigned short int frame_stop_len = 1; /**< Length (in bytes) of the stop field in any frame. */
        static const unsigned short int frame_len_offset = 1; /**< Offset (in bytes) of the first occurrence frame length field in a control or long frame (counted from the beginning of the frame). */
        static const unsigned short int frame_len_offset2 = 2; /**< Offset (in bytes) of the second occurrence frame length field in a control or long frame (counted from the beginning of the frame). */
        static const unsigned short int frame_len_len = 1; /**< Length (in bytes) of the frame length field in a control or long frame. */
        static const unsigned short int frame_content_control_field_offset = 0; /**< Offset (in bytes) of the C field in a control or long frame (counted from the beginning of data in the frame). */
        static const unsigned short int frame_content_address_field_offset = 1; /**< Offset (in bytes) of the A field in a control or long frame (counted from the beginning of data in the frame). */
        static const unsigned short int frame_content_control_information_field_offset = 2; /**< Offset (in bytes) of the CI field in a control or long frame (counted from the beginning of data in the frame). */
        static const unsigned short int frame_checksum_offset = 2; /**< Offset (in bytes) of the frame checksum (counted from the end of the frame). */
        static const unsigned short int frame_checksum_len = 1; /**< Length (in bytes) of the frame checksum. */

        static const unsigned short int fixed_data_serial_offset = 3; /**< Offset (in bytes) of the serial number in the fixed data structure (counted from the beginning of data in the frame). */
        static const unsigned short int fixed_data_serial_len = 4; /**< Length (in bytes) of the serial number in the fixed data structure. */
        static const unsigned short int fixed_data_access_offset = 7; /**< Offset (in bytes) of the access number in the fixed data structure (counted from the beginning of data in the frame). */
        static const unsigned short int fixed_data_status_offset = 8; /**< Offset (in bytes) of the status information field in the fixed data structure (counted from the beginning of data in the frame). */
        static const unsigned short int fixed_data_medium1_offset = 9; /**< Offset (in bytes) of the medium and unit information field for the first counter in the fixed data structure (counted from the beginning of data in the frame). */
        static const unsigned short int fixed_data_medium2_offset = 10; /**< Offset (in bytes) of the medium and unit information field for the second counter in the fixed data structure (counted from the beginning of data in the frame). */
        static const unsigned short int fixed_data_counter1_offset = 11; /**< Offset (in bytes) of the first counter in the fixed data structure (counted from the beginning of data in the frame). */
        static const unsigned short int fixed_data_counter2_offset = 15; /**< Offset (in bytes) of the second counter in the fixed data structure (counted from the beginning of data in the frame). */
        static const unsigned short int fixed_data_counter_len = 4; /**< Length (in bytes) of each of the counters. */

        static const unsigned short int variable_data_serial_offset = 3; /**< Offset (in bytes) of the serial number in the variable data structure (counted from the beginning of data in the frame). */
        static const unsigned short int variable_data_serial_len = 4; /**< Length (in bytes) of the serial number in the variable data structure. */
        static const unsigned short int variable_data_manufacturer_offset = 7; /**< Offset (in bytes) of the manufacturer ID in the variable data structure (counted from the beginning of data in the frame). */
        static const unsigned short int variable_data_manufacturer_len = 2; /**< Length (in bytes) of the manufacturer ID in the variable data structure. */
        static const unsigned short int variable_data_version_offset = 9; /**< Offset (in bytes) of the version field in the variable data structure (counted from the beginning of data in the frame). */
        static const unsigned short int variable_data_medium_offset = 10; /**< Offset (in bytes) of the medium information field in the variable data structure (counted from the beginning of data in the frame). */
        static const unsigned short int variable_data_access_offset = 11; /**< Offset (in bytes) of the access number in the variable data structure (counted from the beginning of data in the frame). */
        static const unsigned short int variable_data_status_offset = 12; /**< Offset (in bytes) of the status information field in the variable data structure (counted from the beginning of data in the frame). */
        static const unsigned short int variable_data_signature_offset = 13; /**< Offset (in bytes) of the signature information field in the variable data structure (counted from the beginning of data in the frame). */
        static const unsigned short int variable_data_signature_len = 2; /**< Length (in bytes) of the signature information field in the variable data structure. */
        static const unsigned short int variable_data_dif_offset = 15; /**< Offset (in bytes) of the DIF field in the variable data structure (counted from the beginning of data in the frame). */
        static const unsigned short int variable_data_dif_len = 1; /**< Length (in bytes) of the DIF field in the variable data structure. */
        static const unsigned short int variable_data_dife_offset = 16; /**< Offset (in bytes) of the DIFE fields in the variable data structure (counted from the beginning of data in the frame). */
        static const unsigned short int variable_data_dife_len = 1; /**< Length (in bytes) of a DIFE field in the variable data structure. */

        static const byte bcd_status_bit = 0x01; /**< Bit mask to be applied to the status information field to check if the data in the fixed data structure is in BCD. */
        static const byte historical_status_bit = 0x02; /**< Bit mask to be applied to the status information field to check if the data in the fixed data structure is historical. */
        static const byte application_status_bits = 0x01 | 0x02; /**< Bit mask to be applied to the status information field in the variable data structure to retrieve the application specific status bits. */
        static const byte power_low_status_bit = 0x04; /**< Bit mask to be applied to the status information field to check if there is a power low condition in the device. */
        static const byte permanent_error_status_bit = 0x08; /**< Bit mask to be applied to the status information field to check if there is a permanent error condition in the device. */
        static const byte temporary_error_status_bit = 0x10; /**< Bit mask to be applied to the status information field to check if there is a temporary error condition in the device. */
        static const byte manufacturer_specific_status_bits = 0x20 | 0x40 | 0x80; /**< Bit mask to be applied to the status information field to retrieve the manufacturer specific status bits. */
        static const unsigned short int manufacturer_specific_status_bits_offset = 5; /**< Offset (in bits) of the manufacturer specific status bits (counted from the beginning of the status information field byte). */

        static const byte fixed_data_medium_bits = 0x40 | 0x80; /**< Bit mask to be applied to the medium and unit information field of the fixed data structure to retrieve the medium information. */
        static const unsigned short int fixed_data_medium_offset = 6; /**< Offset (in bits) of the medium information status bits (counted from the beginning of the medium and unit information field byte). */
        static const byte fixed_data_unit_bits = 0x01 | 0x02 | 0x04 | 0x08 | 0x10 | 0x20; /**< Bit mask to be applied to the medium and unit information field of the fixed data structure to retrieve the unit information. */

        static const unsigned short int max_difes = 10; /**< Maximum number of DIFE fields in one frame. */
        static const byte dif_extension_bit = 0x80; /**< Bit mask to be applied to the DIF or DIFE field to check if there is an extension following. */
        static const byte dif_storage_lsb_bit = 0x40; /**< Bit mask to be applied to the DIF field to retrieve the LSB of the storage number. */
        static const unsigned short int dif_storage_lsb_bit_offset = 6; /**< Offset (in bits) of the storage number bit (counted from the beginning of the DIF field byte). */
        static const byte dif_function_bits = 0x10 | 0x20; /**< Bit mask to be applied to the DIF field to retrieve the function field. */
        static const unsigned short int dif_function_bits_offset = 4; /**< Offset (in bits) of the function field (counted from the beginning of the DIF field byte). */
        static const byte dif_function_instantaneous_value = 0x00; /**< Value of the function field indicating that the sent value is an instantaneous value. */
        static const byte dif_function_maximum_value = 0x01; /**< Value of the function field indicating that the sent value is a maximum value. */
        static const byte dif_function_minimum_value = 0x02; /**< Value of the function field indicating that the sent value is a minimum value. */
        static const byte dif_function_erroneus_value = 0x03; /**< Value of the function field indicating that the sent value is an erroneous value. */
        static const byte dif_data_type_bits = 0x01 | 0x02 | 0x04 | 0x08; /**< Bit mask to be applied to the DIF field to retrieve the data type field. */
        static const byte dif_data_type_no_data = 0x00; /**< Value of the data type field indicating that there is no data. */
        static const byte dif_data_type_8bit_int = 0x01; /**< Value of the data type field indicating that the data is an 8-bit integer sent in binary form. */
        static const byte dif_data_type_16bit_int = 0x02; /**< Value of the data type field indicating that the data is a 16-bit integer sent in binary form. */
        static const byte dif_data_type_24bit_int = 0x03; /**< Value of the data type field indicating that the data is a 24-bit integer sent in binary form. */
        static const byte dif_data_type_32bit_int = 0x04; /**< Value of the data type field indicating that the data is a 32-bit integer sent in binary form. */
        static const byte dif_data_type_32bit_real = 0x05; /**< Value of the data type field indicating that the data is a 32-bit real number sent in binary form (as described in IEEE754). */
        static const byte dif_data_type_48bit_real = 0x06; /**< Value of the data type field indicating that the data is a 48-bit real number sent in binary form (as described in IEEE754). */
        static const byte dif_data_type_64bit_real = 0x07; /**< Value of the data type field indicating that the data is a 64-bit real number sent in binary form (as described in IEEE754). */
        static const byte dif_data_type_selection = 0x08; /**< Value of the data type field indicating that the frame is only a selection for readout and doesn't carry any real data. */
        static const byte dif_data_type_2digit_bcd = 0x09; /**< Value of the data type field indicating that the data is a 2 digit number sent in BCD form. */
        static const byte dif_data_type_4digit_bcd = 0x0A; /**< Value of the data type field indicating that the data is a 4 digit number sent in BCD form. */
        static const byte dif_data_type_6digit_bcd = 0x0B; /**< Value of the data type field indicating that the data is a 6 digit number sent in BCD form. */
        static const byte dif_data_type_8digit_bcd = 0x0C; /**< Value of the data type field indicating that the data is a 8 digit number sent in BCD form. */
        static const byte dif_data_type_variable_length = 0x0D; /**< Value of the data type field indicating that the data is in variable length form. */
        static const byte dif_data_type_12digit_bcd = 0x0E; /**< Value of the data type field indicating that the data is a 12 digit number sent in BCD form. */
        static const byte dif_data_type_special = 0x0F; /**< Value of the data type field indicating that the sent data have a special interpretation. */
        static const byte dif_special_function_manufacturer_specific = 0x0F; /**< Value of the DIF field indicating that the sent data are manufacturer specific. */
        static const byte dif_special_function_manufacturer_specific_with_more_records = 0x1F; /**< Value of the DIF field indicating that the sent data are manufacturer specific and more records will follow in next telegram. */
        static const byte dif_special_function_idle_filler = 0x2F; /**< Value of the DIF field indicating that the sent data are only an idle-filler and are not to be interpreted. */
        static const byte dif_special_function_global_readout_request = 0x7F; /**< Value of the DIF field indicating that the frame is a global readout request and contains no real data. */ 

        static const byte dife_unit_bit = 0x40; /**< Bit mask to be applied to the DIFE field to retrieve the unit information bit. */
        static const unsigned short int dife_unit_bit_offset = 6; /**< Offset (in bits) of the unit information bit (counted from the beginning of the DIFE field byte). */
        static const byte dife_tariff_bits = 0x10 | 0x20; /**< Bit mask to be applied to the DIFE field to retrieve the tariff information bits. */
        static const unsigned short int dife_tariff_bits_offset = 4; /**< Offset (in bits) of the tariff information bits (counted from the beginning of the DIFE field byte). */
        static const unsigned short int dife_tariff_bits_num = 2; /**< Number of bits representing the tariff information in each DIFE field. */
        static const byte dife_storage_number_bits = 0x01 | 0x02 | 0x04 | 0x08; /**< Bit mask to be applied to the DIFE field to retrieve the storage number bits. */
        static const unsigned short int dife_storage_number_bits_num = 4; /**< Number of bits representing the storage number in each DIFE field. */

        static const unsigned short int max_vifes = 10; /**< Maximum number of VIFE fields in one frame. */
        static const byte vif_extension_bit = 0x80; /**< Bit mask to be applied to the VIF or VIFE field to check if there is an extension following. */
        static const byte vif_plain_text = 0x7C; /**< Value of the VIF field indicating that the value information is sent as plain text. */
        static const byte vif_plain_text_extended = 0xFC; /**< Value of the VIF field indicating that the value information is sent as plain text and more information follows in next extension. */
        static const byte vif_extension1 = 0xFB; /**< Value of the VIF field indicating that the value information is to be read from the extended value information table no. 1 and the index to this table will follow in next extension. */
        static const byte vif_extension2 = 0xFD; /**< Value of the VIF field indicating that the value information is to be read from the extended value information table no. 2 and the index to this table will follow in next extension. */
        static const byte vif_any = 0x7E; /**< Value of the VIF field indicating that there is no value information (any value could be send). */
        static const byte vif_any_extended = 0xFE; /**< Value of the VIF field indicating that there is no value information (any value could be send) but more information will follow in next extension. */
        static const byte vif_manufacturer_specific = 0x7F; /**< Value of the VIF field indicating that the value information is manufacturer specific. */
        static const byte vif_manufacturer_specific_extended = 0xFF; /**< Value of the VIF field indicating that the value information is manufacturer specific and more information will follow in next extension. */
        static const unsigned short int variable_data_vif_len = 1; /**< Length (in bytes) of the VIF field in the variable data structure. */
        static const byte vife_manufacturer_specific = 0x7F; /**< Value of the VIFE field indicating that information is this extension is manufacturer specific. */
        static const unsigned short int variable_data_vife_len = 1; /**< Length (in bytes) of the VIFE field in the variable data structure. */

        static const unsigned short int lvar_len = 1; /**< Length (in bytes) of the variable length data header field (the LVAR field). */
        static const byte lvar_ascii_min = 0x00; /**< Minimal value of the variable length data header when sending ASCII data. */
        static const byte lvar_ascii_max = 0xBF; /**< Maximal value of the variable length data header when sending ASCII data. */
        static const byte lvar_positive_bcd_min = 0xC0; /**< Minimal value of the variable length data header when sending data as a positive BCD-encoded number. */
        static const byte lvar_positive_bcd_max = 0xCF; /**< Maximal value of the variable length data header when sending data as a positive BCD-encoded number. */
        static const byte lvar_negative_bcd_min = 0xD0; /**< Minimal value of the variable length data header when sending data as a negative BCD-encoded number. */
        static const byte lvar_negative_bcd_max = 0xDF; /**< Maximal value of the variable length data header when sending data as a negative BCD-encoded number. */
        static const byte lvar_binary_min = 0xE0; /**< Minimal value of the variable length data header when sending data as a integer binary number. */
        static const byte lvar_binary_max = 0xEF; /**< Maximal value of the variable length data header when sending data as a integer binary number. */
        static const byte lvar_float_min = 0xF0; /**< Minimal value of the variable length data header when sending data as a floating point binary number. */
        static const byte lvar_float_max = 0xFA; /**< Maximal value of the variable length data header when sending data as a floating point binary number. */
        static const byte lvar_reserved_min = 0xFB; /**< Minimal value of the reserved variable data length values. */
        static const byte lvar_reserved_max = 0xFF; /**< Maximal value of the reserved variable data length values. */
        static const byte lvar_length_bits = 0x0F; /**< Bit mask to be applied to the variable length data header to retrieve the length of the data. */

        static const char *fixed_data_media[]; /**< Look-up table of media used in the fixed data structure. */
        static const char *fixed_data_units[]; /**< Look-up table of units used in the fixed data structure. */
        static const char *variable_data_media[]; /**< Look-up table of media used in the variable data structure. */
        static const char *variable_data_units_primary[]; /**< Primary look-up table of units used in the variable data structure. */
        static const char *variable_data_units_secondary1[]; /**< Secondary look-up table no. 1 of units used in the variable data structure. */
        static const char *variable_data_units_secondary2[]; /**< Secondary look-up table no. 2 of units used in the variable data structure. */
        static const char *variable_data_units_extensions[]; /**< Look-up table of extended value information used in the variable data structure. */

        static const unsigned short int variable_data_units_primary_time_point_date_index = 108; /**< Index to the primary look-up table of units used in the variable data structure indicating the time point (date only) entry. */
        static const unsigned short int variable_data_units_primary_time_point_date_time_index = 109; /**< Index to the primary look-up table of units used in the variable data structure indicating the time point (date and time) entry. */

        static const double_byte date_day_bits = 0x0001 | 0x0002 | 0x0004 | 0x0008 | 0x0010; /**< Bit mask to be applied to a date representation to retrieve the day field. */
        static const double_byte date_month_bits = 0x0100 | 0x0200 | 0x0400 | 0x0800; /**< Bit mask to be applied to a date representation to retrieve the month field. */
        static const unsigned short int date_month_offset = 8; /**< Offset (in bits) of the month field (counted from the beginning of the date representation double byte word). */
        static const double_byte date_year_bits1 = 0x0020 | 0x0040 | 0x0080; /**< Bit mask to be applied to a date representation to retrieve first part of the year field. */
        static const unsigned short int date_year_offset1 = 5; /**< Offset (in bits) of the first part of the year field (counted from the beginning of the date representation double byte word). */
        static const double_byte date_year_bits2 = 0x1000 | 0x2000 | 0x4000 | 0x8000; /**< Bit mask to be applied to a date representation to retrieve second part of the year field. */
        static const unsigned short int date_year_offset2 = 12; /**< Offset (in bits) of the second part of the year field (counted from the beginning of the date representation double byte word). */
        static const unsigned short int date_year_bits2_offset = 3; /**< Offset (in bits) of the second part of the year field from the first part of the year field. */
        static const unsigned short int date_year_years_offset = 100; /**< Number of years to be added to the two-digit year number received when converting the date representation to calendar time format. */

        static const eight_byte one_byte_data_mask = 0xFF; /**< Bit mask to be applied to remove all the bits from to value except for the first byte. */
        static const eight_byte two_byte_data_mask = 0xFFFF; /**< Bit mask to be applied to remove all the bits from to value except for the first two bytes. */
        static const eight_byte three_byte_data_mask = 0xFFFFFF; /**< Bit mask to be applied to remove all the bits from to value except for the first three bytes. */
        static const eight_byte four_byte_data_mask = 0xFFFFFFFF; /**< Bit mask to be applied to remove all the bits from to value except for the first four bytes. */
        static const eight_byte six_byte_data_mask = 0xFFFFFFFFFFFFLL; /**< Bit mask to be applied to remove all the bits from to value except for the first six bytes. */
        static const eight_byte eight_byte_data_mask = 0xFFFFFFFFFFFFFFLL; /**< Bit mask to be applied to remove all the bits from to value except for the first eight bytes. */

        int serial_port_fd; /**< File descriptor of the open serial port device, 0 if no serial port device is open. */
        unsigned long int serial_port_byte_interval; /**< Time to wait between reading bytes. */
        unsigned short int precision; /**< Precision which should be used when converting floating point numbers to fixed point ones. */

        /** Checks if the given C field represents a RSP_UD frame.
         * @see RSP_UD
         * @param control_field the C field to check
         * @return true if the control field represents a RSP_UD frame, false otherwise
         */
        static inline bool is_rsp_ud(byte control_field) {
            byte masked_control_field = control_field & RSP_UD;

            return masked_control_field == RSP_UD;
        }

        /** Checks if the given checksum is valid for the given frame
         * content.
         * Simply computes the checksum for the given frame content and
         * then compares it with the given checksum.
         * @see checksum
         * @param frame_content frame content to check the checksum
         * @param checksum_value value of the checksum to check
         * @return true if the given checksum is valid, false otherwise
         */
        static inline bool is_checksum_valid(std::string frame_content, byte checksum_value) {
            return checksum(frame_content) == checksum_value;
        }

        /** Converts a BCD-encoded number to normal binary form
         * @param bcd the BCD-encoded number to decode
         * @return the decoded binary value
         */
        template<typename T> static inline T decode_bcd(T bcd) {
            T result = 0;
            T i = 1; // Multiplier for the decimal value which is being build

            while (bcd) {
                result += bcd % 0x10 * i;
                i *= 10;
                bcd /= 0x10;
            }

            return result;
        }

        /** Checks if the given status byte holds an information about
         * the value being BCD-encoded.
         * @see bcd_status_bit
         * @param status the status byte to check
         * @return true if the value is BCD-encoded, false otherwise
         */
        static inline bool is_bcd_status(byte status) {
            return !(status & bcd_status_bit);
        }

        /** Checks if the given status byte holds an information about
         * the value being historical.
         * @see historical_status_bit
         * @param status the status byte to check
         * @return true if the value is historical, false otherwise
         */
        static inline bool is_historical_status(byte status) {
            return (status & historical_status_bit);
        }

        /** Checks if the given status byte holds an information about
         * a power low condition in the other device.
         * @see power_low_status_bit
         * @param status the status byte to check
         * @return true if there is a power low condition in the other device, false otherwise
         */
        static inline bool is_power_low_status(byte status) {
            return (status & power_low_status_bit);
        }

        /** Checks if the given status byte holds an information about
         * a permanent error condition in the other device.
         * @see permanent_error_status_bit
         * @param status the status byte to check
         * @return true if there is a permanent error condition in the other device, false otherwise
         */
        static inline bool is_permanent_error_status(byte status) {
            return (status & permanent_error_status_bit);
        }

        /** Checks if the given status byte holds an information about
         * a temporary error condition in the other device.
         * @see temporary_error_status_bit
         * @param status the status byte to check
         * @return true if there is a temporary error condition in the other device, false otherwise
         */
        static inline bool is_temporary_error_status(byte status) {
            return (status & temporary_error_status_bit);
        }

        /** Retrieves the manufacturer specific status bits from the
         * status byte and represents them in a human-readable form - as 
         * a string with ones and zeros.
         * @see manufacturer_specific_status_bits
         * @param status status byte to extract the information from
         * @return a string representing the manufacturer specific status bits
         */
        static inline std::string get_manufaturer_specific_status(byte status) {
            byte masked_status = status & manufacturer_specific_status_bits;
            masked_status >>= manufacturer_specific_status_bits_offset;

            std::string bits = "";

            do {
                if (masked_status & 0x01)
                    bits += "1";
                else
                    bits += "0";
            } while (masked_status >>= 1);

            return bits;
        }

        /** Retrieves the application specific status bits from the
         * status byte and represents them in a human-readable form - as 
         * a string with ones and zeros.
         * @see application_status_bits
         * @param status status byte to extract the information from
         * @return a string representing the application specific status bits
         */
        static inline std::string get_application_status(byte status) {
            byte masked_status = status & application_status_bits;

            std::string bits = "";

            do {
                if (masked_status & 0x01)
                    bits += "1";
                else
                    bits += "0";
            } while (masked_status >>= 1);

            return bits;
        }

        /** Appends a bytes to a value.
         * Depending on the value of the msb_first parameter, the given
         * byte is appended to the given value as the most- or
         * least-significant byte.
         * @param current_value value to append the byte to
         * @param last_byte byte to append to the value
         * @param msb_first if true, the byte will be appended as the least-significant byte of the value, otherwise it will be appended as the most-significant byte of the value
         * @param byte_number sequential number of the byte, needed when appending the byte as the most-significant byte of the value
         * @return the value with the given byte appended
         */
        template<typename T> static inline T append_byte(T current_value, byte last_byte, bool msb_first = false, int byte_number = 0) {
            if (msb_first)
                return (current_value << 8) | (last_byte & one_byte_data_mask);
            else
                return ((last_byte & one_byte_data_mask) << (8 * byte_number)) | current_value;
        }

        /** Appends a bytes to a value represented as a
         * byte_representation union.
         * Depending on the value of the msb_first parameter, the given
         * byte is appended to the given value as the most- or
         * least-significant byte.
         * @see byte_representation
         * @param current_value value to append the byte to
         * @param last_byte byte to append to the value
         * @param msb_first if true, the byte will be appended as the least-significant byte of the value, otherwise it will be appended as the most-significant byte of the value
         * @param byte_number sequential number of the byte, needed when appending the byte as the most-significant byte of the value
         * @return the value with the given byte appended
         */
        template<typename T> static byte_representation<T> generic_append_byte_representation(byte_representation<T> current_value, byte last_byte, bool msb_first = false, int byte_number = 0);

        /** Decodes the date representation in M-Bus protocol format to
         * calendar time (i.e. seconds since Epoch).
         * @see date_day_bits
         * @see date_month_bits
         * @see date_year_bits1
         * @see date_year_bits2
         * @see mktime(struct tm *)
         * @param date_representation the date representation in M-Bus protocol format to decode
         * @return the calendar representation of the given date (i.e. seconds since Epoch)
         */
        static inline value_t decode_date(double_byte date_representation) {
            byte day = date_representation & date_day_bits;
            byte month = (date_representation & date_month_bits) >> date_month_offset;
            byte year = ((date_representation & date_year_bits1) >> date_year_offset1) 
                        | (((date_representation & date_year_bits2) >> date_year_offset2) << date_year_bits2_offset);

            struct tm time_representation = { 0, 0, 0, day, month - 1, year + date_year_years_offset, 0, 0, -1 };

            return value_t(mktime(&time_representation));
        }

        /** Builds a three letter manufacturer code (as in EN 61107) from
         * a binary form encoded according to IEC 870.
         * @param manufacturer the IEC 870 binary representation of the manufacturer code
         * @return the three letter manufacturer code (as in EN 61107)
         */
        static inline std::string build_manufacturer_string(double_byte manufacturer) {
            std::string manufacturer_string("");

            while (manufacturer) {
                unsigned char character = manufacturer % 32 + 64;
                manufacturer_string.insert(0, 1, character);
                manufacturer /= 32;
            }
        
            return manufacturer_string;
        }

        /** Computes the checksum of the given message.
         * The checksum is simply computed by adding all the bytes of
         * the message and truncating the result to the
         * least-significant byte (as described in the M-Bus standard).
         * @param message the message to compute the checksum from
         * @return the value of the checksum
         */
        static byte checksum(std::string message);

        /** Retrieves the frame content from a M-Bus frame.
         * Detects the frame type of the given frame and depending on
         * it, returns a part of the frame which should contain data. It
         * also checks the checksum of the frame to provide that the
         * reurned frame content is valid.
         * @see identify_frame
         * @see erroneus_frame_content
         * @param frame the frame to retrieve content from
         * @return part of the given frame containing data or erroneus_frame_content when the checksum is invalid
         */
        std::string get_frame_content(std::string frame); // Uses non-static members so cannot be static

        /** Creates a short frame with the given function and address.
         * The crated frame is a full-featured M-Bus frame with all the
         * needed fields filled, including checksum.
         * @see checksum
         * @param function function to embed in the frame
         * @param address address to embed in the frame
         * @return the created frame
         */
        std::string create_short_frame(byte function, byte address); // Uses non-static members so cannot be static

        /** Creates a control frame with the given function, address and
         * CI field.
         * The crated frame is a full-featured M-Bus frame with all the
         * needed fields filled, including checksum.
         * @see checksum
         * @param function function to embed in the frame
         * @param address address to embed in the frame
         * @param control_information the CI field value to embed in the frame
         * @return the created frame
         */
        std::string create_control_frame(byte function, byte address, byte control_information); // Uses non-static members so cannot be static

        /** Creates a control frame with the given function, address, CI 
         * field and data.
         * The crated frame is a full-featured M-Bus frame with all the
         * needed fields filled, including checksum.
         * @see checksum
         * @param function function to embed in the frame
         * @param address address to embed in the frame
         * @param control_information the CI field value to embed in the frame
         * @param data the data to embed in the frame
         * @return the created frame
         */
        std::string create_long_frame(byte function, byte address, byte control_information, std::vector<byte> data); // Uses non-static members so cannot be static

        /** Identifies the frame type of the given frame.
         * Inspects some basic characteristics of the frame and on this
         * basis decides what type of frame is it. It also does some 
         * basic checks to provide that the frame is error-free.
         * @see frame_type
         * @param frame the frame to identify
         * @return type of the given frame
         */
        frame_type identify_frame(std::string frame); // Uses non-static members so cannot be static

        /** Parses data from a received RSP_UD frame and returns a
         * vector of values retrieved from this frame.
         * Parses the content of a received RSP_UD frame according to
         * the M-Bus standard, supporting (nearly) all features of the
         * M-Bus protocol. It tries to retrieve all the available data
         * from the frame's content and return it to the caller. Also,
         * it prints debugging messages to the given output stream if
         * requested.
         * @see OutputPrefix
         * @param frame_content content of the frame to retrieve values from
         * @param debug indicates if the debugging information should be printed
         * @param os output stream to print the debugging information to
         * @param output_prefix prefix to prepend to all printed debugging messages
         * @return values retrieved from the frame content
         */
        std::vector<value_t> parse_data_from_rsp_ud(std::string frame_content, 
            bool debug = false, std::ostream& os = std::cerr, OutputPrefix output_prefix = OutputPrefix("\t")); // Uses non-static members so cannot be static

        /** Sends a frame to the other device.
         * Simply transmits a frame over the serial connection.
         * @see write(int, const void *, size_t)
         * @param frame frame to send
         * @return true if the frame was sent successfully, false otherwise
         */
        bool send_frame(std::string frame);

        /** Receives a frame from the other device.
         * Simply reads a frame over the serial connection, observing
         * the delays between bytes.
         * @see read(int, void *, size_t)
         * @return the received frame
         */
        std::string receive_frame(int timeout_sec, int timeout_usec);

        /** Just for Sontex Supercal 531 */
        void wakeup_device(byte address);
};

template<typename T> MBus::byte_representation<T> MBus::generic_append_byte_representation(byte_representation<T> current_value, byte last_byte, bool msb_first, int byte_number) {
    byte_representation<T> return_value = current_value;

    if (msb_first) {
        for (int i = byte_number; i > 0; i--) // Shift all the bytes one place right
            return_value.bytes[i] = return_value.bytes[i - 1];
        return_value.bytes[0] = last_byte; // Append the byte on the last position
    } else {
        return_value.bytes[byte_number] = last_byte; // Append the byte on the first position
    }

    return return_value;
}

// Specializations written explicite because partial function
// specializations are not supported in C++
template<> inline MBus::byte_representation<double> MBus::append_byte< MBus::byte_representation<double> >(byte_representation<double> current_value, byte last_byte, bool msb_first, int byte_number) {
    return generic_append_byte_representation(current_value, last_byte, msb_first, byte_number);
}

template<> inline MBus::byte_representation<float> MBus::append_byte< MBus::byte_representation<float> >(byte_representation<float> current_value, byte last_byte, bool msb_first, int byte_number) {
    return generic_append_byte_representation(current_value, last_byte, msb_first, byte_number);
}

//error that requires connection restart
struct mbus_read_data_error {
    std::string error;
    mbus_read_data_error(std::string _error) : error(_error) {}
};

#endif

#endif // MBUS_H
