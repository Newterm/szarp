/* 
  libSzarp - SZARP library

*/
/*
 * Common defines for Modbus protocol devices.
 * 
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * $Id$
 */
#ifndef __MODBUS_H__
#define __MODBUS_H__

/** Maximum allowed modbus address. */
#define MB_MAX_ADDRESS 0xffff

/** Modbus exception codes. */
#define MB_ILLEGAL_FUNCTION 0x01
#define MB_ILLEGAL_DATA_ADDRESS 0x02
#define MB_ILLEGAL_DATA_VALUE 0x03
#define MB_SLAVE_DEVICE_FAILURE 0x04
#define MB_ACKNOWLEDGE 0x05
#define MB_SLAVE_DEVICE_BUSY 0x06
#define MB_MEMORY_PARITY_ERROR 0x08
#define MB_GATEWAY_PATH_UNAVAILABLE 0x0A
#define MB_GATEWAY_TARGET_DEVICE_FAILED_TO_RESPOND 0x0B

/** Modbus public function codes. */
#define MB_F_RHR 0x03		/* Read Holding registers */
#define MB_F_RIR 0x04		/* Read input registers */
#define MB_F_WSC 0x05		/* Write single Coil */
#define MB_F_WSR 0x06		/* Write single register */
#define MB_F_WMR 0x10		/* Write multiple registers */

/** Possible types of value types. */
typedef enum {
	MB_TYPE_ERR,		/**< undefined type (config error) */
	MB_TYPE_BCD,		/**< unsigned integer value in BCD encoding, 2 bytes, range 0 - 9999 */
	MB_TYPE_INT,		/**< integer type, 2 bytes */
	MB_TYPE_FLOAT,		/**< float type, 4 bytes (2 registers) */
	MB_TYPE_LONG,           /**< long integer type, 4 bytes (2 registers) */
} ModbusValType;

#endif
