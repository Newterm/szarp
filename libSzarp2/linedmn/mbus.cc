/* 
  SZARP: SCADA software 

*/
#include "mbus.h"
#ifndef MINGW32

#include <string>
#include <iostream>
#include <iomanip>
#include <vector>

#include <termio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "conversion.h"

// These are the value, media and unit information look-up tables used
// to change the codes transmitted in M-Bus frames to the corresponding
// information in human-readable form. These are used only in debugging
// mode, when information about the data received from the other device
// is printed in details.
const char *MBus::fixed_data_media[] = { "Other", 
                                         "Oil", 
                                         "Electricity", 
                                         "Gas",
                                         "Heat", 
                                         "Steam", 
                                         "Hot Water", 
                                         "Water",
                                         "Heat Cost Allocator", 
                                         "Reserved", 
                                         "Gas Mode 2", 
                                         "Heat Mode 2",
                                         "Hot Water Mode 2", 
                                         "Water Mode 2", 
                                         "Heat Cost Allocator Mode 2", 
                                         "Reserved" };

const char *MBus::fixed_data_units[] = { "h,m,s", 
                                         "D,M,Y", 
                                         "Wh", 
                                         "Wh * 10", 
                                         "Wh * 100",
                                         "kWh", 
                                         "kWh * 10", 
                                         "kWh * 100",
                                         "MWh", 
                                         "MWh * 10", 
                                         "MWh * 100",
                                         "kJ", 
                                         "kJ * 10", 
                                         "kJ * 100",
                                         "MJ", 
                                         "MJ * 10", 
                                         "MJ * 100",
                                         "GJ", 
                                         "GJ * 10", 
                                         "GJ * 100",
                                         "W", 
                                         "W * 10", 
                                         "W * 100",
                                         "kW", 
                                         "kW * 10", 
                                         "kW * 100",
                                         "MW", 
                                         "MW * 10", 
                                         "MW * 100",
                                         "kJ/h", 
                                         "kJ/h * 10", 
                                         "kJ/h * 100",
                                         "MJ/h", 
                                         "MJ/h * 10", 
                                         "MJ/h * 100",
                                         "GJ/h", 
                                         "GJ/h * 10", 
                                         "GJ/h * 100",
                                         "ml", 
                                         "ml * 10", 
                                         "ml * 100",
                                         "l", 
                                         "l * 10", 
                                         "l * 100",
                                         "m3", 
                                         "m3 * 10", 
                                         "m3 * 100",
                                         "ml/h", 
                                         "ml/h * 10", 
                                         "ml/h * 100",
                                         "l/h", 
                                         "l/h * 10", 
                                         "l/h * 100",
                                         "m3/h", 
                                         "m3/h * 10", 
                                         "m3/h * 100",
                                         "°C * 10^-3", 
                                         "HCA units", 
                                         "reserved", 
                                         "reserved", 
                                         "reserved", 
                                         "reserved",
                                         "same as unit 1 but historic",
                                         "without any unit" };

const char *MBus::variable_data_media[] = { "Other", 
                                            "Oil", 
                                            "Electricity", 
                                            "Gas",
                                            "Heat (at return temperature)", 
                                            "Steam", 
                                            "Hot Water", 
                                            "Water",
                                            "Heat Cost Allocator", 
                                            "Compressed Air",  
                                            "Cooling load meter (at return temperature", 
                                            "Cooling load meter (at flow temperature)",
                                            "Heat (at flow temperature)", 
                                            "Heat / Cooling load meter", 
                                            "Bus / System", 
                                            "Unknown",
                                            "Reserved", 
                                            "Reserved", 
                                            "Reserved", 
                                            "Reserved", 
                                            "Reserved", 
                                            "Reserved", 
                                            "Cold Water", 
                                            "Dual Water", 
                                            "Pressure", 
                                            "A/D Converter", 
                                            "Reserved" };

const char *MBus::variable_data_units_primary[] = { "Energy [Wh], * 0.001", 
                                                    "Energy [Wh], * 0.01", 
                                                    "Energy [Wh], * 0.1", 
                                                    "Energy [Wh], * 1", 
                                                    "Energy [Wh], * 10", 
                                                    "Energy [Wh], * 100", 
                                                    "Energy [Wh], * 1 000", 
                                                    "Energy [Wh], * 10 000", 
                                                    "Energy [J], * 1", 
                                                    "Energy [J], * 10", 
                                                    "Energy [J], * 100", 
                                                    "Energy [J], * 1 000", 
                                                    "Energy [J], * 10 000", 
                                                    "Energy [J], * 100 000", 
                                                    "Energy [J], * 1 000 000", 
                                                    "Energy [J], * 10 000 000", 
                                                    "Volume [m3], * 0.000001", 
                                                    "Volume [m3], * 0.00001", 
                                                    "Volume [m3], * 0.0001", 
                                                    "Volume [m3], * 0.001", 
                                                    "Volume [m3], * 0.01", 
                                                    "Volume [m3], * 0.1", 
                                                    "Volume [m3], * 1", 
                                                    "Volume [m3], * 10", 
                                                    "Mass [kg], * 0.001", 
                                                    "Mass [kg], * 0.01", 
                                                    "Mass [kg], * 0.1", 
                                                    "Mass [kg], * 1", 
                                                    "Mass [kg], * 10", 
                                                    "Mass [kg], * 100", 
                                                    "Mass [kg], * 1 000", 
                                                    "Mass [kg], * 10 000", 
                                                    "On Time [s]", 
                                                    "On Time [min]", 
                                                    "On Time [h]", 
                                                    "On Time [days]", 
                                                    "Operation Time [s]", 
                                                    "Operation Time [min]", 
                                                    "Operation Time [h]", 
                                                    "Operation Time [days]", 
                                                    "Power [W], * 0.001", 
                                                    "Power [W], * 0.01", 
                                                    "Power [W], * 0.1", 
                                                    "Power [W], * 1", 
                                                    "Power [W], * 10", 
                                                    "Power [W], * 100", 
                                                    "Power [W], * 1 000", 
                                                    "Power [W], * 10 000", 
                                                    "Power [J/h], * 1", 
                                                    "Power [J/h], * 10", 
                                                    "Power [J/h], * 100", 
                                                    "Power [J/h], * 1 000", 
                                                    "Power [J/h], * 10 000", 
                                                    "Power [J/h], * 100 000", 
                                                    "Power [J/h], * 1 000 000", 
                                                    "Power [J/h], * 10 000 000", 
                                                    "Volume Flow [m3/h], * 0.000001", 
                                                    "Volume Flow [m3/h], * 0.00001", 
                                                    "Volume Flow [m3/h], * 0.0001", 
                                                    "Volume Flow [m3/h], * 0.001", 
                                                    "Volume Flow [m3/h], * 0.01", 
                                                    "Volume Flow [m3/h], * 0.1", 
                                                    "Volume Flow [m3/h], * 1", 
                                                    "Volume Flow [m3/h], * 10", 
                                                    "Volume Flow ext. [m3/min], * 0.0000001", 
                                                    "Volume Flow ext. [m3/min], * 0.000001", 
                                                    "Volume Flow ext. [m3/min], * 0.00001", 
                                                    "Volume Flow ext. [m3/min], * 0.0001", 
                                                    "Volume Flow ext. [m3/min], * 0.001", 
                                                    "Volume Flow ext. [m3/min], * 0.01", 
                                                    "Volume Flow ext. [m3/min], * 0.1", 
                                                    "Volume Flow ext. [m3/min], * 1", 
                                                    "Volume Flow ext. [m3/s], * 0.000000001", 
                                                    "Volume Flow ext. [m3/s], * 0.00000001", 
                                                    "Volume Flow ext. [m3/s], * 0.0000001", 
                                                    "Volume Flow ext. [m3/s], * 0.000001", 
                                                    "Volume Flow ext. [m3/s], * 0.00001", 
                                                    "Volume Flow ext. [m3/s], * 0.0001", 
                                                    "Volume Flow ext. [m3/s], * 0.001", 
                                                    "Volume Flow ext. [m3/s], * 0.01", 
                                                    "Mass Flow [kg/h], * 0.001", 
                                                    "Mass Flow [kg/h], * 0.01", 
                                                    "Mass Flow [kg/h], * 0.1", 
                                                    "Mass Flow [kg/h], * 1", 
                                                    "Mass Flow [kg/h], * 10", 
                                                    "Mass Flow [kg/h], * 100", 
                                                    "Mass Flow [kg/h], * 1 000", 
                                                    "Mass Flow [kg/h], * 10 000", 
                                                    "Flow Temperature [°C], * 0.001", 
                                                    "Flow Temperature [°C], * 0.01", 
                                                    "Flow Temperature [°C], * 0.1", 
                                                    "Flow Temperature [°C], * 1", 
                                                    "Return Temperature [°C], * 0.001",
                                                    "Return Temperature [°C], * 0.01",
                                                    "Return Temperature [°C], * 0.1",
                                                    "Return Temperature [°C], * 1",
                                                    "Temperature Difference [K], * 0.001", 
                                                    "Temperature Difference [K], * 0.01", 
                                                    "Temperature Difference [K], * 0.1", 
                                                    "Temperature Difference [K], * 1", 
                                                    "External Temperature [°C], * 0.001", 
                                                    "External Temperature [°C], * 0.01", 
                                                    "External Temperature [°C], * 0.1", 
                                                    "External Temperature [°C], * 1", 
                                                    "Pressure [bar], * 0.001", 
                                                    "Pressure [bar], * 0.01", 
                                                    "Pressure [bar], * 0.1", 
                                                    "Pressure [bar], * 1", 
                                                    "Time Point (date)", 
                                                    "Time Point (time and date)", 
                                                    "HCA units", 
                                                    "Reserved", 
                                                    "Averaging Duration [s]", 
                                                    "Averaging Duration [min]", 
                                                    "Averaging Duration [h]", 
                                                    "Averaging Duration [days]", 
                                                    "Actuality Duration [s]",
                                                    "Actuality Duration [min]",
                                                    "Actuality Duration [h]",
                                                    "Actuality Duration [days]",
                                                    "Fabrication Number", 
                                                    "Identification", 
                                                    "Bus Address" };

const char *MBus::variable_data_units_secondary1[] = { "Energy [MWh], * 0.1",
                                                       "Energy [MWh], * 1",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Energy [GJ], * 0.1",
                                                       "Energy [GJ], * 1",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Volume [m3], * 100",
                                                       "Volume [m3], * 1 000",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Mass [t], * 100",
                                                       "Mass [t], * 1 000",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Volume [feet^3], * 0.1",
                                                       "Volume [american gallon], * 0.1",
                                                       "Volume [american gallon], * 1",
                                                       "Volume flow [american gallon/min], * 0.001",
                                                       "Volume flow [american gallon/min], * 1",
                                                       "Volume flow [american gallon/h], * 1",
                                                       "Reserved",
                                                       "Power [MW], * 0.1",
                                                       "Power [MW], * 1",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Power [GJ/h], * 0.1",
                                                       "Power [GJ/h], * 1",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Flow temperature [°F], * 0.001",
                                                       "Flow temperature [°F], * 0.01",
                                                       "Flow temperature [°F], * 0.1",
                                                       "Flow temperature [°F], * 1",
                                                       "Return temperature [°F], * 0.001",
                                                       "Return temperature [°F], * 0.01",
                                                       "Return temperature [°F], * 0.1",
                                                       "Return temperature [°F], * 1",
                                                       "Temperature difference [°F], * 0.001",
                                                       "Temperature difference [°F], * 0.01",
                                                       "Temperature difference [°F], * 0.1",
                                                       "Temperature difference [°F], * 1",
                                                       "External temperature [°F], * 0.001",
                                                       "External temperature [°F], * 0.01",
                                                       "External temperature [°F], * 0.1",
                                                       "External temperature [°F], * 1",
                                                       "Reserved",
                                                       "Cold/Warm temperature limit [°F], * 0.001",
                                                       "Cold/Warm temperature limit [°F], * 0.01",
                                                       "Cold/Warm temperature limit [°F], * 0.1",
                                                       "Cold/Warm temperature limit [°F], * 1",
                                                       "Cold/Warm temperature limit [°C], * 0.001",
                                                       "Cold/Warm temperature limit [°C], * 0.01",
                                                       "Cold/Warm temperature limit [°C], * 0.1",
                                                       "Cold/Warm temperature limit [°C], * 1",
                                                       "Cumulative counter - max power [W], * 0.001",
                                                       "Cumulative counter - max power [W], * 0.01",
                                                       "Cumulative counter - max power [W], * 0.1",
                                                       "Cumulative counter - max power [W], * 1",
                                                       "Cumulative counter - max power [W], * 10",
                                                       "Cumulative counter - max power [W], * 100",
                                                       "Cumulative counter - max power [W], * 1 000",
                                                       "Cumulative counter - max power [W], * 10 000" };

const char *MBus::variable_data_units_secondary2[] = { "Credit of 0.001 of the nominal local legal currency units",
                                                       "Credit of 0.01 of the nominal local legal currency units",
                                                       "Credit of 0.1 of the nominal local legal currency units",
                                                       "Credit of 1 of the nominal local legal currency units",
                                                       "Debit of 0.001 of the nominal local legal currency units",
                                                       "Debit of 0.01 of the nominal local legal currency units",
                                                       "Debit of 0.1 of the nominal local legal currency units",
                                                       "Debit of 1 of the nominal local legal currency units",
                                                       "Access Number",
                                                       "Medium",
                                                       "Manufacturer",
                                                       "Parameter set identification",
                                                       "Model / Version",
                                                       "Hardware version #",
                                                       "Firmware version #",
                                                       "Software version #",
                                                       "Customer location",
                                                       "Customer",
                                                       "User Access Code",
                                                       "Operator Access Code",
                                                       "System Operator Access Code",
                                                       "Developer Access Code",
                                                       "Password",
                                                       "Error flags",
                                                       "Error mask",
                                                       "Reserved",
                                                       "Digital Output",
                                                       "Digital Input",
                                                       "Baudrate",
                                                       "Respond Delay Time",
                                                       "Retry",
                                                       "Reserved",
                                                       "First storage # for cyclic storage",
                                                       "Last storage # for cyclic storage",
                                                       "Size of storage block",
                                                       "Reserved",
                                                       "Storage interval [s]",
                                                       "Storage interval [min]",
                                                       "Storage interval [h]",
                                                       "Storage interval [days]",
                                                       "Storage interval [months]",
                                                       "Storage interval [years]",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Duration since last readout [s]",
                                                       "Duration since last readout [min]",
                                                       "Duration since last readout [h]",
                                                       "Duration since last readout [days]",
                                                       "Start (date/time) of tariff",
                                                       "Duration of tariff [min]",
                                                       "Duration of tariff [h]",
                                                       "Duration of tariff [days]",
                                                       "Period of tariff [s]",
                                                       "Period of tariff [min]",
                                                       "Period of tariff [h]",
                                                       "Period of tariff [days]",
                                                       "Period of tariff [months]",
                                                       "Period of tariff [years]",
                                                       "dimensionless",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Voltage [V], * 0.000000001",
                                                       "Voltage [V], * 0.00000001",
                                                       "Voltage [V], * 0.0000001",
                                                       "Voltage [V], * 0.000001",
                                                       "Voltage [V], * 0.00001",
                                                       "Voltage [V], * 0.0001",
                                                       "Voltage [V], * 0.001",
                                                       "Voltage [V], * 0.01",
                                                       "Voltage [V], * 0.1",
                                                       "Voltage [V], * 1",
                                                       "Voltage [V], * 10",
                                                       "Voltage [V], * 100",
                                                       "Voltage [V], * 1 000",
                                                       "Voltage [V], * 10 000",
                                                       "Voltage [V], * 100 000",
                                                       "Voltage [V], * 1 000 000",
                                                       "Current [A], * 0.000000000001",
                                                       "Current [A], * 0.00000000001",
                                                       "Current [A], * 0.0000000001",
                                                       "Current [A], * 0.000000001",
                                                       "Current [A], * 0.00000001",
                                                       "Current [A], * 0.0000001",
                                                       "Current [A], * 0.000001",
                                                       "Current [A], * 0.00001",
                                                       "Current [A], * 0.0001",
                                                       "Current [A], * 0.001",
                                                       "Current [A], * 0.01",
                                                       "Current [A], * 0.1",
                                                       "Current [A], * 1",
                                                       "Current [A], * 10",
                                                       "Current [A], * 100",
                                                       "Current [A], * 1 000",
                                                       "Reset counter",
                                                       "Cumulation counter",
                                                       "Control signal",
                                                       "Day of week",
                                                       "Week number",
                                                       "Time point of day change",
                                                       "State of parameter activation",
                                                       "Special supplier information",
                                                       "Duration since last cumulation [h]",
                                                       "Duration since last cumulation [days]",
                                                       "Duration since last cumulation [months]",
                                                       "Duration since last cumulation [years]",
                                                       "Operating time battery [h]",
                                                       "Operating time battery [days]",
                                                       "Operating time battery [months]",
                                                       "Operating time battery [years]",
                                                       "Date and time of battery change",
                                                       "Reserved" };

const char *MBus::variable_data_units_extensions[] = { "Error: None",
                                                       "Error: Too many DIFEs",
                                                       "Error: Storage number not implemented",
                                                       "Error: Unit number not implemented",
                                                       "Error: Tariff number not implemented",
                                                       "Error: Function not implemented",
                                                       "Error: Data class not implemented",
                                                       "Error: Data size not implemented",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "Error: Too many VIFEs",
                                                       "Error: Illegal VIF-Group",
                                                       "Error: Illegal VIF-Exponent",
                                                       "Error: VIF/DIF mismatch",
                                                       "Error: Unimplemented action",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "Error: No data available (undefined value)",
                                                       "Error: Data overflow",
                                                       "Error: Data underflow",
                                                       "Error: Data error",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "Error: Premature end of record",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "Error: Reserved",
                                                       "per second",
                                                       "per minute",
                                                       "per hour",
                                                       "per day",
                                                       "per week",
                                                       "per month",
                                                       "per year",
                                                       "per revolution/measurement",
                                                       "increment per input pulse on channel #0",
                                                       "increment per input pulse on channel #1",
                                                       "increment per output pulse on channel #0",
                                                       "increment per output pulse on channel #1",
                                                       "per liter",
                                                       "per m3",
                                                       "per kg",
                                                       "per K",
                                                       "per kWh",
                                                       "per GJ",
                                                       "per kW",
                                                       "per K*l",
                                                       "per V",
                                                       "per A",
                                                       "multiplied by second",
                                                       "multiplied by second/V",
                                                       "multiplied by second/A",
                                                       "start date/time of",
                                                       "VIF contains uncorrected unit instead of corrected unit",
                                                       "Accumulation only if positive contributions",
                                                       "Accumulation of absolute values only if negative contributions",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Lower limit value",
                                                       "Number of exceeds of lower limit",
                                                       "Date/time of the beginning of first lower limit exceed",
                                                       "Date/time of the end of first lower limit exceed",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Date/time of the beginning of last lower limit exceed",
                                                       "Date/time of the end of last lower limit exceed",
                                                       "Duration of the first lower limit exceed",
                                                       "Duration of the first lower limit exceed",
                                                       "Duration of the first lower limit exceed",
                                                       "Duration of the first lower limit exceed",
                                                       "Duration of the last lower limit exceed",
                                                       "Duration of the last lower limit exceed",
                                                       "Duration of the last lower limit exceed",
                                                       "Duration of the last lower limit exceed",
                                                       "Upper limit value",
                                                       "Number of exceeds of upper limit",
                                                       "Date/time of the beginning of first upper limit exceed",
                                                       "Date/time of the end of first upper limit exceed",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Date/time of the beginning of last upper limit exceed",
                                                       "Date/time of the end of last upper limit exceed",
                                                       "Duration of the first upper limit exceed",
                                                       "Duration of the first upper limit exceed",
                                                       "Duration of the first upper limit exceed",
                                                       "Duration of the first upper limit exceed",
                                                       "Duration of the last upper limit exceed",
                                                       "Duration of the last upper limit exceed",
                                                       "Duration of the last upper limit exceed",
                                                       "Duration of the last upper limit exceed",
                                                       "Duration of first"
                                                       "Duration of first"
                                                       "Duration of first"
                                                       "Duration of first"
                                                       "Duration of last"
                                                       "Duration of last"
                                                       "Duration of last"
                                                       "Duration of last"
                                                       "Reserved",
                                                       "Reserved",
                                                       "Date/time of the beginning of first",
                                                       "Date/time of the end of first",
                                                       "Reserved",
                                                       "Reserved",
                                                       "Date/time of the beginning of last",
                                                       "Date/time of the end of last",
                                                       "Multiplicative correction factor: 0.000001",
                                                       "Multiplicative correction factor: 0.00001",
                                                       "Multiplicative correction factor: 0.0001",
                                                       "Multiplicative correction factor: 0.001",
                                                       "Multiplicative correction factor: 0.01",
                                                       "Multiplicative correction factor: 0.1",
                                                       "Multiplicative correction factor: 1",
                                                       "Multiplicative correction factor: 10",
                                                       "Additive correction constant: 0.001 * unit of VIF",
                                                       "Additive correction constant: 0.01 * unit of VIF",
                                                       "Additive correction constant: 0.1 * unit of VIF",
                                                       "Additive correction constant: 1 * unit of VIF",
                                                       "Reserved",
                                                       "Multiplicative correction factor: 1 000",
                                                       "Future value",
                                                       "Next VIFE's and data of this block are manufacturer specific" };

MBus::byte MBus::checksum(std::string message) {
    byte checksum = 0;

    for (std::string::iterator i = message.begin(); i != message.end(); i++) // M-Bus checksum is simply the sum of all bytes modulo 0xFF (to fit one byte)
        checksum += *i;

    return checksum;
}

std::string MBus::create_short_frame(byte function, byte address) { // Short frame consist of the start byte (special for a short frame), the function field, the address field, the checksum and the stop byte
    std::string frame_content, frame;

    frame += short_frame_start;
    frame_content += std::string(1, static_cast<unsigned char>(function));
    frame_content += std::string(1, static_cast<unsigned char>(address));
    frame += frame_content;
    frame += checksum(frame_content);
    frame += frame_stop;

    return frame;
}

std::string MBus::create_control_frame(byte function, byte address, byte control_information) { // Control frame consist of the start byte, twice the length field (with the value of 0x03 for a control frame), another start byte, the function field, the address field, the CI (control information) field, the checksum and the stop byte
    std::string frame_content, frame;

    frame += frame_start;
    frame += control_frame_len_field;
    frame += control_frame_len_field;
    frame += frame_start;
    frame_content += std::string(1, static_cast<unsigned char>(function));
    frame_content += std::string(1, static_cast<unsigned char>(address));
    frame_content += std::string(1, static_cast<unsigned char>(control_information));
    frame += frame_content;
    frame += checksum(frame_content);
    frame += frame_stop;

    return frame;
}

std::string MBus::create_long_frame(byte function, byte address, byte control_information, std::vector<byte> data) { // Long frame consist of the start byte, twice the length field, another start byte, the function field, the address field, the CI (control information) field, the data (up to 252 bytes, actual length of the data is given in the length field), the checksum and the stop byte
    std::string frame_content, frame;

    frame += frame_start;

    byte frame_length = 3 + data.size();

    frame += std::string(1, static_cast<unsigned char>(frame_length));
    frame += std::string(1, static_cast<unsigned char>(frame_length));
    frame += frame_start;
    frame_content += std::string(1, static_cast<unsigned char>(function));
    frame_content += std::string(1, static_cast<unsigned char>(address));
    frame_content += std::string(1, static_cast<unsigned char>(control_information));

    for (std::vector<byte>::iterator i = data.begin(); i != data.end(); i++)
        frame_content += std::string(1, static_cast<unsigned char>(*i));

    frame_content = frame_content.substr(0, frame_content_max_len); // Limit the length of the frame content to the maximum of 255 bytes

    frame += frame_content;
    frame += checksum(frame_content);
    frame += frame_stop;

    return frame;
}

MBus::frame_type MBus::identify_frame(std::string frame) {
    if (frame == positive_reply)
        return single_char;

    unsigned int frame_length = frame.length();

    if (frame_length < frame_start_offset + frame_start_len + 1) // Check if the frame is long enough to include the start byte
        return unknown_frame;

    if (frame.substr(frame_start_offset, frame_start_len) == short_frame_start  // Check if the frame has a start byte specific for the short frame
        && frame_length == short_frame_stop_offset + 1                          // and if it is long enough to contain the stop byte
        && frame.substr(short_frame_stop_offset, frame_stop_len) == frame_stop) // and if the stop byte is valid
        return short_frame;

    if (frame.substr(frame_start_offset, frame_start_len) == frame_start) { // Check if the frame has a valid start byte
        if (frame_length < frame_len_offset2 + frame_len_len + frame_start_len) // Check if the frame is long enough to contain two length fields and the second start byte
            return unknown_frame;

        if (frame.substr(frame_len_offset, frame_len_len) == frame.substr(frame_len_offset2, frame_len_len)) { // Check if the two length fields are equal
            if (frame.substr(frame_len_offset, frame_len_len) == control_frame_len_field // Check if the length field is control frame-specific
                && frame_length == frame_start_len + 2 * frame_len_len + frame_start_len + control_frame_data_len + frame_checksum_len + frame_stop_len // Check if the frame length matches the expected length of a control frame
                && frame.substr(control_frame_stop_offset, frame_stop_len) == frame_stop) // Check if the stop byte is valid
                return control_frame;

            unsigned int wanted_frame_length = frame_start_len + 2 * frame_len_len + frame_start_len + static_cast<unsigned int>(frame.at(frame_len_offset)) + frame_checksum_len + frame_stop_len;

            if (frame.substr(frame_start_offset, frame_start_len) == frame_start // Check if the frame has a valid start byte
                && frame_length == wanted_frame_length                           // and its length matches the length given in the length field
                && frame.substr(frame_length - frame_stop_len) == frame_stop)    // and it has a valid stop byte
                return long_frame;
        }
    } 
    
    return unknown_frame;
}

std::string MBus::get_frame_content(std::string frame) {
    std::string frame_content = erroneus_frame_content;

    switch (identify_frame(frame)) {
        case single_char:
            frame_content = frame;

            break;
        case short_frame:
            frame_content = frame.substr(short_frame_data_offset, short_frame_data_len);

            if (!is_checksum_valid(frame_content, static_cast<byte>(frame.at(frame.length() - frame_checksum_offset))))
                frame_content = erroneus_frame_content;

            break;
        case long_frame:
            frame_content = frame.substr(frame_data_offset, static_cast<byte>(frame.at(frame_len_offset)));

            if (!is_checksum_valid(frame_content, static_cast<byte>(frame.at(frame.length() - frame_checksum_offset))))
                frame_content = erroneus_frame_content;

            break;
        case control_frame:
            frame_content = frame.substr(frame_data_offset, control_frame_data_len);

            if (!is_checksum_valid(frame_content, static_cast<byte>(frame.at(frame.length() - frame_checksum_offset))))
                frame_content = erroneus_frame_content;

            break;
        case unknown_frame:
        default:
            frame_content = erroneus_frame_content;

            break;
    }

    return frame_content;
}

std::vector<MBus::value_t> MBus::parse_data_from_rsp_ud(std::string frame_content, 
        bool debug, std::ostream& os, OutputPrefix output_prefix) {
    std::vector <value_t> values;

    if (frame_content == erroneus_frame_content) // Don't parse anything if it's not a valid frame
        return values;

    byte control_information_field = static_cast<byte>(frame_content.at(frame_content_control_information_field_offset));
    bool is_msb_first = (control_information_field & msb_first);
    bool is_bcd = false;

    if (debug) {
        if (is_msb_first) 
            os << output_prefix << "Frame contains data with MSB first\n";
        else
            os << output_prefix << "Frame contains data with LSB first\n";
    }

    switch (control_information_field) {
        case general_error: // General error frames are not parsed, only an information is printed when in debug mode
            if (debug) {
                os << output_prefix << "RSP_UD frame parsed as a general error frame" << std::endl;
            }
            
            return values;
        case alarm: // Alarm frames are not parsed, only an information is printed when in debug mode
            if (debug)
                os << output_prefix << "RSP_UD frame parsed as an alarm frame" << std::endl;
            
            return values;
        case variable_data:
        case variable_data_msb_first:
            if (debug)
                os << output_prefix << "RSP_UD frame parsed as a variable data structure frame\n";
            
            if (debug) {
                quad_byte serial = 0;

                for (int i = 0; i < variable_data_serial_len; i++)
                    serial = append_byte(serial, static_cast<byte>(frame_content.at(variable_data_serial_offset + i)), is_msb_first, i);

                serial = decode_bcd(unsigned_value_t(serial)); // The serial number is always BCD-encoded

                os << output_prefix << "Serial number: " << std::dec << serial << "\n";

                double_byte manufacturer = 0;

                for (int i = 0; i < variable_data_manufacturer_len; i++)
                    manufacturer = append_byte(manufacturer, static_cast<byte>(frame_content.at(variable_data_manufacturer_offset + i)), is_msb_first, i);

                os << output_prefix << "Manufacturer ID: " << build_manufacturer_string(manufacturer) << "\n"; // Manufacturer information needs to be decoded according to IEC 870

                byte version = static_cast<byte>(frame_content.at(variable_data_version_offset));
                os << output_prefix << "Version number: " << std::dec << static_cast<unsigned int>(version) << "\n";

                byte medium = static_cast<byte>(frame_content.at(variable_data_medium_offset));

                if (medium >= sizeof(variable_data_media)) // Sanity check
                    os << output_prefix << "Wrong value of the medium parameter: " << static_cast<unsigned int>(medium) << "\n";
                else
                    os << output_prefix << "Medium: " << variable_data_media[medium] << "\n";

                byte access = static_cast<byte>(frame_content.at(variable_data_access_offset));
                os << output_prefix << "Access number: " << std::dec << static_cast<unsigned int>(access) << "\n";

                os << output_prefix << "Decoding status bits:\n";
                byte status = static_cast<byte>(frame_content.at(variable_data_status_offset));

                os << ++output_prefix << "Application status bits: " << get_application_status(status) << "\n";

                if (is_power_low_status(status))
                    os << output_prefix << "* Power low\n";

                if (is_permanent_error_status(status))
                    os << output_prefix << "* Permanent error\n";

                if (is_temporary_error_status(status))
                    os << output_prefix << "* Temporary error\n";

                os << output_prefix << "Manufacturer status bits: " << get_manufaturer_specific_status(status) << "\n";
                output_prefix--;

                double_byte signature = 0;

                for (int i = 0; i < variable_data_signature_len; i++)
                    signature = append_byte(signature, static_cast<byte>(frame_content.at(variable_data_signature_offset + i)), is_msb_first, i);

                os << output_prefix << "Signature: " << std::hex << signature << "\n";

                os << "\n";
            }

            {
                unsigned short int previous_data_blocks_length = 0;
                bool rest_manufacturer_specific = false; /**< Flag indicating that rest of the data in this telegram is manufacturer specific */

                while (frame_content.length() > static_cast<unsigned int>(variable_data_dif_offset + previous_data_blocks_length) && !rest_manufacturer_specific) { // End parsing the frame when all content is parsed or when the rest of the data is known to be manufacturer specific
                    byte dif = static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length));
                    unsigned short int current_data_block_length = variable_data_dif_len;

                    bool dif_extended = dif & dif_extension_bit;
                    eight_byte storage_number = (dif & dif_storage_lsb_bit) >> dif_storage_lsb_bit_offset; // LSB of the storage bit is transmitted directly in the DIF
                    unsigned short int dife_num = 0;
                    unsigned short int vife_num = 0;

                    if (debug) {
                        os << output_prefix << "Data:\n";
                        os << ++output_prefix << "Function: ";
                        switch ((dif & dif_function_bits) >> dif_function_bits_offset) {
                            case dif_function_instantaneous_value:
                                os << "instantaneous value\n";
                                break;
                            case dif_function_maximum_value:
                                os << "maximum value\n";
                                break;
                            case dif_function_minimum_value:
                                os << "minimum value\n";
                                break;
                            case dif_function_erroneus_value:
                                os << "erroneous value\n";
                                break;
                        }
                    }

                    byte data_type = dif & dif_data_type_bits;

                    if (debug) 
                        os << output_prefix << "Data type: ";

                    switch (data_type) {
                        case dif_data_type_no_data:
                            if (debug)
                                os << "no data\n";

                            break;
                        case dif_data_type_8bit_int:
                            if (debug)
                                os << "8-bit integer\n";

                            break;
                        case dif_data_type_16bit_int:
                            if (debug)
                                os << "16-bit integer\n";

                            break;
                        case dif_data_type_24bit_int:
                            if (debug)
                                os << "24-bit integer\n";

                            break;
                        case dif_data_type_32bit_int:
                            if (debug)
                                os << "32-bit integer\n";

                            break;
                        case dif_data_type_32bit_real:
                            if (debug)
                                os << "32-bit real\n";

                            break;
                        case dif_data_type_48bit_real:
                            if (debug)
                                os << "48-bit real\n";

                            break;
                        case dif_data_type_64bit_real:
                            if (debug)
                                os << "64-bit real\n";

                            break;
                        case dif_data_type_selection:
                            if (debug)
                                os << "selection for readout\n";

                            break;
                        case dif_data_type_2digit_bcd:
                            if (debug)
                                os << "2 digit BCD\n";

                            break;
                        case dif_data_type_4digit_bcd:
                            if (debug)
                                os << "4 digit BCD\n";

                            break;
                        case dif_data_type_6digit_bcd:
                            if (debug)
                                os << "6 digit BCD\n";

                            break;
                        case dif_data_type_8digit_bcd:
                            if (debug)
                                os << "8 digit BCD\n";

                            break;
                        case dif_data_type_variable_length:
                            if (debug)
                                os << "variable length\n";

                            break;
                        case dif_data_type_12digit_bcd:
                            if (debug)
                                os << "12 digit BCD\n";

                            break;
                        case dif_data_type_special:
                            if (debug)
                                os << "special: ";

                            switch (dif) {
                                case dif_special_function_manufacturer_specific:
                                    if (debug)
                                        os << "manufacturer specific data follows\n";

                                    rest_manufacturer_specific = true;
                                    break;
                                case dif_special_function_manufacturer_specific_with_more_records:
                                    if (debug)
                                        os << "manufacturer specific data follows, more records in next telegram\n";

                                    rest_manufacturer_specific = true;
                                    break;
                                case dif_special_function_idle_filler:
                                    if (debug)
                                        os << "idle filler, DIF in next byte\n";

                                    previous_data_blocks_length += current_data_block_length;
                                    continue;
                                    break;
                                case dif_special_function_global_readout_request:
                                    if (debug)
                                        os << "global readout request\n";

                                    break;
                                default:
                                    if (debug)
                                        os << "reserved\n";

                                    break;
                            }

                            break;
                    }

                    if (rest_manufacturer_specific) {
                        previous_data_blocks_length += current_data_block_length;
                        break;
                    }

                    if (dif_extended) {
                        byte dife = 0;
                        double_byte unit = 0;
                        quad_byte tariff = 0;

                        do {
                            dife = static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length));
                            current_data_block_length += variable_data_dife_len;

                            if (debug) {
                                unit |= ((dife & dife_unit_bit) >> dife_unit_bit_offset) << dife_num;
                                tariff |= ((dife & dife_tariff_bits) >> dife_tariff_bits_offset) << (dife_num * dife_tariff_bits_num);
                                storage_number |= (dife & dife_storage_number_bits) << (dife_num * dife_storage_number_bits_num + 1);
                            }

                        } while (++dife_num < max_difes && (dife & dif_extension_bit)); // Parse only a maximum of 10 DIFEs

                        if (debug) {
                            os << output_prefix << "Unit: " << std::dec << static_cast<unsigned int>(unit) << "\n";
                            os << output_prefix << "Tariff: " << std::dec << tariff << "\n";
                        }
                    }

                    if (debug) 
                        os << output_prefix << "Storage number: " << std::dec << storage_number << "\n";

                    byte vif = static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length));
                    current_data_block_length += variable_data_vif_len;

                    bool vif_extended = vif & vif_extension_bit;

                    if (debug) {
                        os << output_prefix << "Value and unit information:\n";

                        switch (vif) {
                            case vif_plain_text:
                            case vif_plain_text_extended:
                                os << ++output_prefix << "Plain text value follows";
                                
                                if (vif_extended)
                                    os << ": ";
                                else // Will this ever happen?
                                    os << "\n";

                                break;
                            case vif_extension1:
                            case vif_extension2:
                                os << output_prefix << "Information from the extended VIF table:\n";
                                break;
                            case vif_any:
                            case vif_any_extended:
                                os << output_prefix << "Any VIF\n";
                                break;
                            case vif_manufacturer_specific:
                            case vif_manufacturer_specific_extended:
                                os << output_prefix << "Manufacturer specific VIF";

                                if (vif_extended)
                                    os << ": ";
                                else
                                    os << "\n";

                                break;
                            default:
                                if ((vif & ~vif_extension_bit) >= sizeof(variable_data_units_primary)) 
                                    os << output_prefix << "Wrong value of the VIF parameter: " << static_cast<unsigned int>(vif) << "\n";
                                else
                                    os << output_prefix << "Value information: " << variable_data_units_primary[vif & ~vif_extension_bit] << "\n";

                                break;
                        }
                    }
                    
                    if (vif_extended) {
                        byte vife = 0;

                        do {
                            vife = static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length));
                            current_data_block_length += variable_data_vife_len;

                            switch (vif) {
                                case vif_plain_text:
                                case vif_plain_text_extended:
                                    if (debug)
                                        os << static_cast<unsigned char>(vife & ~vif_extension_bit); // Just output the characters for a plain text VIF
                                    break;
                                case vif_extension1:
                                    if (debug) {
                                        if ((vife & ~vif_extension_bit) >= sizeof(variable_data_units_secondary1)) // Sanity check
                                            os << ++output_prefix << "Unknown VIF extension: " << static_cast<unsigned int>(vife);
                                        else
                                            os << output_prefix << variable_data_units_secondary1[vife & ~vif_extension_bit];
                                    }

                                    break;
                                case vif_extension2:
                                    if (debug) {
                                        if ((vife & ~vif_extension_bit) >= sizeof(variable_data_units_secondary2)) // Sanity check
                                            os << output_prefix << "Unknown VIF extension: " << static_cast<unsigned int>(vife);
                                        else
                                            os << output_prefix << variable_data_units_secondary2[vife & ~vif_extension_bit];
                                    }

                                    break;
                                case vif_manufacturer_specific:
                                case vif_manufacturer_specific_extended:
                                    if (debug)
                                        os << std::hex << static_cast<unsigned int>(vife) << " "; // Just output the value in hex for a manufacturer specific VIF

                                    break;
                                default:
                                    if (debug) {
                                        if ((vife & ~vif_extension_bit) >= sizeof(variable_data_units_extensions)) // Sanity check
                                            os << output_prefix << "Unknown VIF extension: " << static_cast<unsigned int>(vife);
                                        else
                                            os << output_prefix << variable_data_units_extensions[vife & ~vif_extension_bit];
                                    }

                                    if ((vife & ~vif_extension_bit) == vife_manufacturer_specific) // The case when the VIF is not manufacturer specific but one of the VIFEs is
                                        rest_manufacturer_specific = true;

                                    break;
                            }
                        } while (!rest_manufacturer_specific && ++vife_num < max_vifes && (vife & vif_extension_bit)); // Parse only a maximum of 10 VIFEs

                        if (debug)
                            os << "\n";

                        if (rest_manufacturer_specific) {
                            previous_data_blocks_length += current_data_block_length;
                            break;
                        }

                    }

                    unsigned short int data_len = 0;
                    value_t data = 0;

                    output_prefix -= 2;

                    switch (data_type) {
                        case dif_data_type_no_data:
                            data = SZARP_NO_DATA;

                            if (debug)
                                os << output_prefix << "Value: no data\n";

                            break;
                        case dif_data_type_8bit_int:
                            {
                                data_len = 1;

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                data &= one_byte_data_mask;

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_16bit_int:
                            {
                                data_len = 2;

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                data &= two_byte_data_mask;

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_24bit_int:
                            {
                                data_len = 3;

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                data &= three_byte_data_mask;

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_32bit_int:
                            {
                                data_len = 4;

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                data &= four_byte_data_mask;

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_32bit_real:
                            {
                                data_len = 4;

                                byte_representation<float> data_representation = { 0 };

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data_representation = append_byte(data_representation, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

				//to avoid overflows when shifting to desired precision,
				//store the intermidiary as double
				double d_value = data_representation.value;
                                for (unsigned int i = 0; i < precision; i++)
                                    d_value *= 10;

                                data = value_t(d_value);
                                
                                data &= four_byte_data_mask;

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_48bit_real: // 48-bit real data has no equivalent in C++ so it's not parsed as for now
                            {
                                data_len = 6;

                                data = SZARP_NO_DATA;

                                if (debug)
                                    os << output_prefix << "48-bit real data type not (yet) supported.\n";
                            }
                            break;
                        case dif_data_type_64bit_real:
                            {
                                data_len = 8;

                                byte_representation<double> data_representation = { 0 };

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data_representation = append_byte(data_representation, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                for (unsigned int i = 0; i < precision; i++)
                                    data_representation.value *= 10;

                                data = value_t(data_representation.value);

                                data &= four_byte_data_mask;
                                
                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_selection: // This is not interesting for someone wanting the data
                            data = SZARP_NO_DATA;

                            break;
                        case dif_data_type_2digit_bcd:
                            {
                                data_len = 1;

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                data &= one_byte_data_mask;

                                data = decode_bcd(unsigned_value_t(data));

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_4digit_bcd:
                            {
                                data_len = 2;

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                data &= two_byte_data_mask;

                                data = decode_bcd(unsigned_value_t(data));

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_6digit_bcd:
                            {
                                data_len = 3;

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                data &= three_byte_data_mask;

                                data = decode_bcd(unsigned_value_t(data));

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_8digit_bcd:
                            {
                                data_len = 4;

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                data &= four_byte_data_mask;

                                data = decode_bcd(unsigned_value_t(data));

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_variable_length: // Variable length data needs special handling
                            {
                                byte lvar = static_cast<byte>(frame_content.at(variable_data_dif_offset + dife_num + previous_data_blocks_length + current_data_block_length)); // LVAR, the first byte of the data, indicates the type and length of the data
                                current_data_block_length += lvar_len;

                                if (/*lvar >= lvar_ascii_min && -- always true due to range limits */ lvar <= lvar_ascii_max) {
                                    data_len = lvar & lvar_length_bits;

                                    if (debug)
                                        os << output_prefix << "ASCII value with " << std::dec << data_len << " characters: ";

                                    for (unsigned int i = 0; i < data_len; i++) { // For ASCII data, just return as much of the first characters as possible
                                        unsigned char data_char = static_cast<unsigned char>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i));

                                        data = append_byte(data, data_char, is_msb_first, i);

                                        if (debug)
                                            os << data_char; // Output all the characters
                                    }

                                    if (debug)
                                        os << "\n";
                                } else if (lvar >= lvar_positive_bcd_min && lvar <= lvar_positive_bcd_max) {
                                    data_len = lvar & lvar_length_bits;

                                    for (unsigned int i = 0; i < data_len; i++)
                                        data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);

                                    data = decode_bcd(unsigned_value_t(data));

                                    if (debug)
                                        os << output_prefix << "Positive BCD value with " << std::dec << data_len << " digits: " << data << "\n";

                                } else if (lvar >= lvar_negative_bcd_min && lvar <= lvar_negative_bcd_max) {
                                    data_len = lvar & lvar_length_bits;

                                    for (unsigned int i = 0; i < data_len; i++)
                                        data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);

                                    data = decode_bcd(unsigned_value_t(data));
                                    data = -data; // If its negative, make it really negative

                                    if (debug)
                                        os << output_prefix << "Negative BCD value with " << std::dec << data_len << " digits: " << data << "\n";

                                } else if (lvar >= lvar_binary_min && lvar <= lvar_binary_max) {
                                    data_len = lvar & lvar_length_bits;

                                    for (unsigned int i = 0; i < data_len; i++)
                                        data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);

                                    if (debug)
                                        os << output_prefix << "Binary value with " << std::dec << data_len << " bytes: " << data << "\n";

                                } else if (lvar >= lvar_float_min && lvar <= lvar_float_max) { // Floating point number with variable length is not well defined so it can't be supported
                                    data_len = lvar & lvar_length_bits;

                                    data = SZARP_NO_DATA;

                                    if (debug)
                                        os << output_prefix << "Variable length floating point number is not well defined and not (yet) supported.\n";
                                } else if (lvar >= lvar_reserved_min /* && lvar <= lvar_reserved_max -- always true due to range limits */) {
                                    data = SZARP_NO_DATA;

                                    if (debug)
                                        os << output_prefix << "Reserved LVAR value.\n";
                                } else { // Sanity check
                                    data = SZARP_NO_DATA;

                                    if (debug)
                                        os << output_prefix << "Wrong LVAR value in variable length data.\n";
                                }
                            }
                            break;
                        case dif_data_type_12digit_bcd:
                            {
                                data_len = 6;

                                for (unsigned int i = 0; i < data_len; i++) {
                                    data = append_byte(data, static_cast<byte>(frame_content.at(variable_data_dif_offset + previous_data_blocks_length + current_data_block_length + i)), is_msb_first, i);
                                }

                                data &= six_byte_data_mask;

                                data = decode_bcd(unsigned_value_t(data));

                                if (debug)
                                    os << output_prefix << std::dec << "Value: " << data << "\n";
                            }
                            break;
                        case dif_data_type_special: // Special types of data cannot be returned in a sane form
                            data = SZARP_NO_DATA;

                            break;
                    }

                    unsigned short int vif_index = vif & ~vif_extension_bit;

                    if (vif_index == variable_data_units_primary_time_point_date_index) { // Date comes in a M-Bus-specific format so it has to be decoded to calendar time (i.e. seconds since Epoch)
                        data = decode_date(data);

                        if (debug)
                            os << output_prefix << std::dec << "Decoded date value: " << static_cast<time_t>(data) << "\n";
                    } else if (vif_index == variable_data_units_primary_time_point_date_time_index) { // It's not really well described anywhere - it could be calendar time but there's no guarantee for it
                        if (debug)
                            os << output_prefix << "Time and date decoding not (yet) supported\n";
                    }

                    values.push_back(data);

                    previous_data_blocks_length += current_data_block_length + data_len;

                    if (debug)
                        os << "\n";
                }

                if (rest_manufacturer_specific && debug) { // Output manufacturer specific data in hex
                    os << output_prefix << "Manufacturer specific data: ";

                    dump_frame_hex(os, frame_content.substr(variable_data_dif_offset + previous_data_blocks_length, frame_content.length() - variable_data_dif_offset + previous_data_blocks_length));

                    os << "\n";
                }
            }

            output_prefix--;

            return values;
        case fixed_data:
        case fixed_data_msb_first:
            if (debug)
                os << output_prefix << "RSP_UD frame parsed as a fixed data structure frame\n";
            
            if (debug) {
                quad_byte serial = 0;

                for (unsigned int i = 0; i < fixed_data_serial_len; i++)
                    serial = append_byte(serial, static_cast<byte>(frame_content.at(fixed_data_serial_offset + i)), is_msb_first, i);

                os << output_prefix << "Serial number: " << std::dec << decode_bcd(unsigned_value_t(serial)) << "\n"; // Serial number is always BCD-encoded

                byte access = (byte) frame_content.at(fixed_data_access_offset);
                os << output_prefix << "Access number: " << std::dec << static_cast<unsigned int>(access) << "\n";
            }

            {
                byte status = static_cast<byte>(frame_content.at(fixed_data_status_offset));
                is_bcd = is_bcd_status(status);

                if (debug) {
                    os << output_prefix << "Decoding status bits:\n";

                    output_prefix++;

                    if (is_bcd)
                        os << output_prefix << "* Data in BCD\n";

                    if (is_historical_status(status))
                        os << output_prefix << "* Historical data\n";
                    else
                        os << output_prefix << "* Current data\n";

                    if (is_power_low_status(status))
                        os << output_prefix << "* Power low\n";

                    if (is_permanent_error_status(status))
                        os << output_prefix << "* Permanent error\n";

                    if (is_temporary_error_status(status))
                        os << output_prefix << "* Temporary error\n";

                    output_prefix--;

                    os << output_prefix << "Manufacturer status bits: " << get_manufaturer_specific_status(status) << "\n";
                }
            }

            if (debug) {
                byte medium = (((static_cast<byte>(frame_content.at(fixed_data_medium1_offset))) 
                                        & fixed_data_medium_bits) 
                                        >> fixed_data_medium_offset) 
                                    | ((((static_cast<byte>(frame_content.at(fixed_data_medium2_offset))) 
                                        & fixed_data_medium_bits) 
                                        >> fixed_data_medium_offset) 
                                    << 2); // Medium information is split between two bytes

                if (medium >= sizeof(fixed_data_media)) // Sanity check
                    os << output_prefix << "Wrong value of the medium parameter: " << std::showbase << std::hex << static_cast<unsigned int>(medium) << "\n";
                else
                    os << output_prefix << "Medium: " << fixed_data_media[medium] << "\n";

                byte unit1 = (static_cast<byte>(frame_content.at(fixed_data_medium1_offset))) & fixed_data_unit_bits;

/*                if (unit1 >= sizeof(fixed_data_units)) // Sanity check, always true due to range limits
                    os << output_prefix << "Wrong value of the 1st unit parameter: " << std::showbase << std::hex << static_cast<unsigned int>(unit1) << "\n";
                else */
                    os << output_prefix << "Unit of the 1st counter: " << fixed_data_units[unit1] << "\n";

                byte unit2 = (static_cast<byte>(frame_content.at(fixed_data_medium2_offset))) & fixed_data_unit_bits;

/*                if (unit2 >= sizeof(fixed_data_units)) // Sanity check - always true due to range limits
                    os << output_prefix << "Wrong value of the 2nd unit parameter: " << std::showbase << std::hex << static_cast<unsigned int>(unit2) << "\n";
                else */
                    os << output_prefix << "Unit of the 2nd counter: " << fixed_data_units[unit2] << "\n";
            }

            {
                value_t counter1 = 0, counter2 = 0;

                for (unsigned int i = 0; i < fixed_data_counter_len; i++) {
                    counter1 = append_byte(counter1, static_cast<byte>(frame_content.at(fixed_data_counter1_offset + i)), is_msb_first, i);
                    counter2 = append_byte(counter2, static_cast<byte>(frame_content.at(fixed_data_counter2_offset + i)), is_msb_first, i);
                }

                if (is_bcd) {
                    counter1 = decode_bcd(unsigned_value_t(counter1));
                    counter2 = decode_bcd(unsigned_value_t(counter2));
                }
                
                values.push_back(counter1);
                values.push_back(counter2);

                if (debug)
                    os << output_prefix << std::dec << "Value no. 1: " << counter1 << "\n" << output_prefix << "Value no. 2" << counter2 << "\n";
            }

            return values;
        default:
            return values;
    }
}

void MBus::dump_frame_hex(std::ostream& os, std::string frame) {
    os << std::hex << std::noshowbase << std::setfill('0');

    for (std::string::iterator i = frame.begin(); i != frame.end(); i++)
        os << std::setw(2) << static_cast<unsigned int>(*i & 0xFF) << " ";
}

void MBus::dump_frame(std::ostream& os, std::string frame) {
    os << "M-Bus frame:\n";

    std::string frame_content = get_frame_content(frame);
    byte address = 0x00;

    switch (identify_frame(frame)) {
        case single_char:
            os << "Frame type: single character frame - positive reply\n";

            break;
        case short_frame:
            os << "Frame type: short frame\n";

            os << "Control field: " << std::hex 
                << static_cast<unsigned int>(frame_content.at(frame_content_control_field_offset))
                << "\n";

            address = static_cast<byte>(frame_content.at(frame_content_address_field_offset));

            os << "Address field: ";
            os << std::hex << static_cast<unsigned int>(address);

            if (address == broadcast_with_reply)
                os << " (broadcast with reply)";
            else if (address == broadcast_without_reply) 
                os << " (broadcast without reply)";
            os << "\n";

            os << "Checksum: " <<
                (is_checksum_valid(frame_content, static_cast<byte>(frame.at(frame.length() - frame_checksum_offset))) 
                 ? "valid" : "invalid");
            os << "\n";

            break;
        case long_frame:
            os << "Frame type: long frame\n";

            address = static_cast<byte>(frame_content.at(frame_content_address_field_offset));

            os << "Address field: ";
            os << std::hex << static_cast<unsigned int>(address);

            if (address == broadcast_with_reply)
                os << " (broadcast with reply)";
            else if (address == broadcast_without_reply) 
                os << " (broadcast without reply)";
            os << "\n";

            if (is_rsp_ud(static_cast<byte>(frame_content.at(frame_content_control_field_offset)))) { // Parse a RSP_UD frame
                os << "RSP_UD frame:\n";

                std::vector<value_t> values = parse_data_from_rsp_ud(frame_content, true, os);
            } else { // Output raw data in hex for all other frames
                os << "Unknown type of long frame, printing raw data:\n";

                dump_frame_hex(os, frame);
            }

            os << "Checksum: " <<
                (is_checksum_valid(frame_content, static_cast<byte>(frame.at(frame.length() - frame_checksum_offset))) 
                 ? "valid" : "invalid");
            os << "\n";

            break;
        case control_frame:
            os << "Frame type: control frame\n";

            os << "Control field: " << std::showbase << std::hex 
                << static_cast<unsigned int>(frame_content.at(frame_content_control_field_offset))
                << "\n";

            address = static_cast<byte>(frame_content.at(frame_content_address_field_offset));

            os << "Address field: ";
            os << std::showbase << std::hex << static_cast<unsigned int>(address);

            if (address == broadcast_with_reply)
                os << " (broadcast with reply)";
            else if (address == broadcast_without_reply) 
                os << " (broadcast without reply)";
            os << "\n";

            os << "Control information field: " << std::showbase << std::hex 
                << static_cast<unsigned int>(frame_content.at(frame_content_control_information_field_offset)) // The CI field is just printed in hex
                << "\n";

            os << "Checksum: " <<
                (is_checksum_valid(frame_content, static_cast<byte>(frame.at(frame.length() - frame_checksum_offset))) 
                 ? "valid" : "invalid");
            os << "\n";


            break;
        case unknown_frame:
            os << "Unknown frame type, printing raw data:\n";

            dump_frame_hex(os, frame);

            break;
    }

    os << std::endl;
}

bool MBus::connect(std::string device, unsigned long int baudrate, unsigned long int byte_interval, unsigned int data_bits, unsigned int stop_bits, parity_type parity) {
    int fd = open(device.c_str(), O_RDWR | O_NDELAY | O_NONBLOCK);
    unsigned long int baudrate_flag, data_bits_flag, stop_bits_flag, parity_flag;

    if (fd < 0)
        return false;
    else {
        serial_port_fd = fd;
        serial_port_byte_interval = byte_interval;
    }

    switch (baudrate) {
        case 300:
            baudrate_flag = B300;
            break;
        case 600:
            baudrate_flag = B600;
            break;
        case 1200:
            baudrate_flag = B1200;
            break;
        case 2400:
            baudrate_flag = B2400;
            break;
        case 4800:
            baudrate_flag = B4800;
            break;
        case 9600:
            baudrate_flag = B9600;
            break;
        case 19200:
            baudrate_flag = B19200;
            break;
        case 38400:
            baudrate_flag = B38400;
            break;
        case 115200:
            baudrate_flag = B115200;
            break;
        default:
            baudrate_flag = B300;
            break;
    }

    switch (data_bits) {
        case 7:
            data_bits_flag = CS7;
            break;
        case 8:
            data_bits_flag = CS8;
            break;
        default:
            data_bits_flag = CS8;
            break;
    }

    switch (stop_bits) {
        case 1:
            stop_bits_flag = 0;
            break;
        case 2:
            stop_bits_flag = CSTOPB;
            break;
        default:
            stop_bits_flag = 0;
            break;
    }

    switch (parity) {
        case none:
            parity_flag = 0;
            break;
        case even:
            parity_flag = PARENB;
            break;
        case odd:
            parity_flag = PARENB | PARODD;
            break;
        default:
            parity_flag = 0;
            break;
    }

    // Configure the RS232 transceiver
    struct termios rsconf;

    tcgetattr(serial_port_fd, &rsconf);

    rsconf.c_cflag = baudrate_flag | data_bits_flag | stop_bits_flag | parity_flag | CLOCAL | CREAD;
    rsconf.c_iflag = rsconf.c_oflag = rsconf.c_lflag = 0;
    rsconf.c_cc[VMIN] = 0;
    rsconf.c_cc[VTIME] = 0;

    tcsetattr(serial_port_fd, TCSANOW, &rsconf);

    // Configure the serial port
    unsigned int serial_config;

    ioctl(serial_port_fd, TIOCMGET, &serial_config);

    serial_config |= TIOCM_DTR;
    serial_config |= TIOCM_RTS;

    ioctl(serial_port_fd, TIOCMSET, &serial_config);

    return true;
}

bool MBus::disconnect() {
    if (close(serial_port_fd) < 0)
        return false;
    else {
        serial_port_fd = 0;

        return true;
    }
}

bool MBus::send_frame(std::string frame) {
    int sent_bytes = 0;
    int last_sent_bytes;
    
    while ((last_sent_bytes = write(serial_port_fd, frame.substr(sent_bytes).c_str(), frame.length() - sent_bytes)) > 0) // Try sending the frame until all the bytes are sent or the transmission fails
        sent_bytes += last_sent_bytes;

    if (last_sent_bytes < 0) 
        throw mbus_read_data_error(std::string("Error while reading data ") + strerror(errno));
    else
        return true;
}

std::string MBus::receive_frame(int timeout_sec, int timeout_usec) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(serial_port_fd, &fds);

    struct timeval tv;
    tv.tv_sec = timeout_sec; 
    tv.tv_usec = timeout_usec;

    int select_retval = select(serial_port_fd + 1, &fds, NULL, NULL, &tv);

    if (select_retval == -1 || select_retval == 0)
        return erroneus_frame_content;
    else {
        unsigned char character = 0;
        std::string frame("");

        ssize_t r;
        while ((r = read(serial_port_fd, &character, 1)) > 0) {
            frame.insert(frame.length(), 1, character);
            usleep(serial_port_byte_interval);
        }
        if (r == -1)
            switch (errno) {
                case EAGAIN:
                    break;
                default:
                    throw mbus_read_data_error(std::string("Error while reading data ") + strerror(errno));
            }

        return frame;
    }
}

void MBus::wakeup_device(byte address) { 
    std::cout << "Waking up device with SND_NKE2 datagram.\n";
    
    if (send_frame(create_short_frame(SND_NKE2, address))) {
        std::cout << "SND_NKE2 sent successfully.\n";
    } else {
        std::cout << "Error while sending SND_NKE2.\n";
        return;
    }

    /** Device will not respond to this - we are just waiting for it to wakeup */ 
 
    std::string frame("");

    if (identify_frame(frame = receive_frame(1,0)) == single_char) {
        std::cout << "Device is already awake.\n";
    } else {
        if (frame == "") {
            std::cout << "Timed out while waiting for reply - device should be now awake.\n";
        }
        else {
            std::cout << "Invalid reply received.\n";
        }
    }
}


bool MBus::initialize_communication(byte address, bool sontex, bool debug, std::ostream& os) {
    
    if (sontex)
        wakeup_device(address);

    if (debug)
        os << "Sending the SND_NKE2 datagram.\n";
    
    if (send_frame(create_short_frame(SND_NKE2, address))) {
        if (debug)
            os << "SND_NKE2 sent successfully.\n";
    } else {
        if (debug)
            os << "Error while sending SND_NKE2.\n";
        
        return false;
    }

    std::string frame("");

    if (identify_frame(frame = receive_frame(5,0)) == single_char) {
        if (debug)
            os << "Received a positive reply.\n";
    } else {
        if (debug) {
            if (frame == "") 
                os << "Timed out while waiting for reply.\n";
            else { // Invalid frames are simply printed in hex
                os << "Invalid reply received.\nRaw data: ";

                dump_frame_hex(os, frame);

                os << "\n";
            }
        }

        return false;
    }

    return true;
}

std::vector<MBus::value_t> MBus::query_for_data(byte address, bool sontex, bool debug, bool dump_hex, std::ostream& os) {

    if (sontex)
        wakeup_device(address);

    if (debug)
        os << "Sending the REQ_UD2 datagram.\n";
    
    if (send_frame(create_short_frame(REQ_UD2, address))) {
        if (debug)
            os << "REQ_UD2 sent successfully.\n";
    } else {
        if (debug)
            os << "Error while sending REQ_UD2.\n";
        
        return std::vector<value_t>();
    }

    std::string frame = receive_frame(5,0);
    std::vector<value_t> values;

    if (dump_hex) {
        os << "M-Bus frame hex dump:\n";

        dump_frame_hex(os, frame);

        os << "\n";
    }

    if (identify_frame(frame) == long_frame) {
        std::string frame_content = get_frame_content(frame);

        if (debug) { // Print some additional information, useful for debugging
            os << "Long frame received, parsing.\n";

            byte address = static_cast<byte>(frame_content.at(frame_content_address_field_offset));

            os << "\tAddress: ";
            os << std::hex << static_cast<unsigned int>(address);

            if (address == broadcast_with_reply)
                os << " (broadcast with reply)";
            else if (address == broadcast_without_reply) 
                os << " (broadcast without reply)";
            os << "\n\n";

        }

        values = parse_data_from_rsp_ud(frame_content, debug, os);
    } else {
        if (debug) {
            if (frame == "") 
                os << "Timed out while waiting for reply.\n";
            else { // Invalid frames are simply printed in hex
                os << "Invalid reply received.\nRaw data: ";

                dump_frame_hex(os, frame);

                os << "\n";
            }
        }

        return std::vector<value_t>();
    }

    return values;
}

std::string MBus::query_for_status(byte address, bool sontex, bool debug, std::ostream& os) {
    
    if (sontex)
        wakeup_device(address);

    if (debug)
        os << "Sending the REQ_SKE datagram.\n";
    
    if (send_frame(create_short_frame(REQ_SKE, address))) {
        if (debug)
            os << "REQ_SKE sent successfully.\n";
    } else {
        if (debug)
            os << "Error while sending REQ_SKE.\n";
        
        return std::string("");
    }

    std::string frame = receive_frame(5,0);

    if (debug) {
        if (frame == "") 
            os << "Timed out while waiting for reply.\n";
        else { // Invalid frames are simply printed in hex
            os << "Reply received.\nRaw data: ";

            dump_frame_hex(os, frame);

            os << "\n";
        }
    }

    return frame;
}

bool MBus::reset_application(byte address, byte reset_type, bool sontex, bool debug, std::ostream& os) {
    
    if (sontex)
        wakeup_device(address);

    if (debug)
        os << "Sending the SND_UD datagram with application reset information.\n";
    
    std::vector<byte> data;
    data.push_back(reset_type);

    if (send_frame(create_long_frame(SND_UD, address, ci_application_reset, data))) {
        if (debug)
            os << "SND_UD sent successfully.\n";
    } else {
        if (debug)
            os << "Error while sending SND_UD.\n";
        
        return false;
    }

    std::string frame("");

    if (identify_frame(frame = receive_frame(5,0)) == single_char) {
        if (debug)
            os << "Received a positive reply.\n";
    } else {
        if (debug) {
            if (frame == "") 
                os << "Timed out while waiting for reply.\n";
            else { // Invalid frames are simply printed in hex
                os << "Invalid reply received.\nRaw data: ";

                dump_frame_hex(os, frame);

                os << "\n";
            }
        }

        return false;
    }

    return true;
}

std::vector<MBus::value_t> MBus::select_data_for_readout(byte address, std::vector<byte> data, bool sontex, bool debug, std::ostream& os) {
    std::vector<value_t> values;

    if (sontex)
        wakeup_device(address);

    if (debug)
        os << "Sending the SND_UD datagram with select data for readout information.\n";
    
    if (send_frame(create_long_frame(SND_UD, address, ci_data_send, data))) {
        if (debug)
            os << "SND_UD sent successfully.\n";
    } else {
        if (debug)
            os << "Error while sending SND_UD.\n";
        
        return values;
    }

    std::string frame("");
    frame_type reply_type = identify_frame(frame = receive_frame(5,0));

    // There are two possible replies to a selection of data for readout
    // request
    if (reply_type == single_char) {
        if (debug)
            os << "Received a positive reply.\n";
    } else if (reply_type == long_frame) {
        if (debug)
            os << "Received a long frame, parsing.\n";

        values = parse_data_from_rsp_ud(get_frame_content(frame), debug, os);
    } else {
        if (debug) {
            if (frame == "") 
                os << "Timed out while waiting for reply.\n";
            else { // Invalid frames are simply printed in hex
                os << "Invalid reply received.\nRaw data: ";

                dump_frame_hex(os, frame);

                os << "\n";
            }
        }
    }

    return values;
}

bool MBus::change_slave_address(byte address, byte new_address, bool sontex, bool debug, std::ostream& os) {
    
    if (sontex)
        wakeup_device(address);

    if (debug)
        os << "Sending the SND_UD datagram with address change information.\n";
    
    std::vector<byte> data;
    data.push_back(dif_change_slave_address);
    data.push_back(vif_change_slave_address);
    data.push_back(new_address);

    if (send_frame(create_long_frame(SND_UD, address, ci_data_send, data))) {
        if (debug)
            os << "SND_UD sent successfully.\n";
    } else {
        if (debug)
            os << "Error while sending SND_UD.\n";
        
        return false;
    }

    std::string frame("");

    if (identify_frame(frame = receive_frame(5,0)) == single_char) {
        if (debug)
            os << "Received a positive reply.\n";
    } else {
        if (debug) {
            if (frame == "") 
                os << "Timed out while waiting for reply.\n";
            else { // Invalid frames are simply printed in hex
                os << "Invalid reply received.\nRaw data: ";

                dump_frame_hex(os, frame);

                os << "\n";
            }
        }

        return false;
    }

    return true;
}
std::ostream& operator<<(std::ostream& os, OutputPrefix& op) {
    os << std::showbase << std::hex << op.prefix;
    return os;
}

OutputPrefix& OutputPrefix::operator+=(std::string suffix) {
    prefix += suffix;
    return *this;
}

OutputPrefix& OutputPrefix::operator++() {
    char last_char = prefix.at(prefix.length() - 1);
    prefix += last_char;
    return *this;
}

OutputPrefix OutputPrefix::operator++(int dummy) {
    OutputPrefix ret = *this;
    ++(*this); // Let the prefix operator do all the job
    return ret;
}

OutputPrefix& OutputPrefix::operator--() {
    prefix = prefix.substr(0, prefix.length() - 1);
    return *this;
}

OutputPrefix OutputPrefix::operator--(int dummy) {
    OutputPrefix ret = *this;
    --(*this); // Let the prefix operator do all the job
    return ret;
}

OutputPrefix& OutputPrefix::operator-=(int length) {
    prefix = prefix.substr(0, prefix.length() - length + 1);
    return *this;
}

#endif
