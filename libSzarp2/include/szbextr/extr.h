/* 
  SZARP: SCADA software 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA


  Library for extracting data from SzarpBase to other formats.

  Pawe³ Pa³ucha <pawel@praterm.com.pl>
  $Id$
*/
#ifndef __SZBEXTR_EXTR_H__
#define __SZBEXTR_EXTR_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <time.h>

#include <libxslt/transform.h>
#include <libxml/xmlwriter.h>

#include "szarp_config.h"
#include "szbase/szbbase.h"

#define _SZBEXTRACTOR_PRIV_BUFFER_SIZE 100

/**
 * Class for extracting SzarpBase data to XML format and converting to other
 * formats using XSLT stylesheets.
 * You can use object of this class many times setting different options,
 * parameters and time of extraction.
 */
class SzbExtractor {
	public:
		/** available params types */
		typedef enum {
			TYPE_AVERAGE ,
			TYPE_END ,
		} ParamType;
		/** Error codes */
		typedef enum {
			ERR_OK = 0,	/** no error */
			ERR_NOIMPL,	/** function not implemented */
			ERR_CANCEL,	/** canceled by user */
			ERR_XMLWRITE,	/** error writing to XMLWriter */
			ERR_SYSERR,	/** system error, see errno */
			ERR_ZIPCREATE,	/** error creating zip file */
			ERR_ZIPADD,	/** error adding file to zip */
			ERR_LOADXSL,	/** error loading XSL stylsheet */
			ERR_TRANSFORM,	/** error applying XSL stylesheet */
		} ErrorCode;
		/** Param description needed by SzbExtractor */
		struct Param {
			Param( const std::wstring& n , szb_buffer_t*b , ParamType t ) :
				name(n) , szb(b) , type(t) {}
			std::wstring name;
			szb_buffer_t* szb;
			ParamType type;
		};
		/**
		 * Constructor. LibXML and LibXSLT should be initialized.
		 * @param ipk pointer to initialized TSzarpConfig object
		 */
		SzbExtractor(TSzarpConfig *ipk);
		/**
		 * Free memory.
		 */
		~SzbExtractor();

		/** 
		 * @brief returns probe value for param
		 * 
		 * @param p
		 * @param tp
		 * @param t
		 * @param st
		 * @param ct
		 * 
		 * @return 
		 */
		SZBASE_TYPE get_probe( const Param& p , TParam*tp , time_t t , SZARP_PROBE_TYPE st , int ct );

		/**
		 * Set array of params to extract.
		 * @param array of param names, may not be NULL; only pointer
		 * is stored so do not delete array before end of extraction
		 * @param szb array of pointers to szb_buffer_t structures,
		 * one pointer per parameter, only pointer to array is stored,
		 * so do not delete array before end of extraction
		 * @length length of param array, at least 1
		 * @return 0 on success or index of incorrect parameter (not
		 * found in IPK configuration or with wrong type) starting
		 * from 1
		 */
		int SetParams(const std::vector<Param>& params);
		/**
		 * Set period of time to extract and type of probes.
		 * @param period_type type of period, one of PT_MIN10,
		 * PT_HOUR, PT_DAY, PT_WEEK, PT_CUSTOM
		 * @param start first date for extraction, it may be converted
		 * according to given period
		 * @param end last date for coversion (inclusive), may be also
		 * converted, must not be earlier then start date
		 * @param custom_period if probe_type is PT_CUSTOM this is
		 * length of period in seconds, start and end date are rounded
		 * down by custom_period values, it must be positive, values
		 * less then SZBASE_DATA_SPAN have no sense
		 */
		void SetPeriod(SZARP_PROBE_TYPE period_type, 
				time_t start, time_t end, 
				int custom_period = 0);
		/**
		 * Return date format string for given period type.
		 * @param period_type type of period
		 * @return date format string
		 */
		const char *SetFormat(SZARP_PROBE_TYPE period_type);
		/** Set period of time to given month.
		 * @param period_type @see SetPeriod
		 * @param year year, must be greater then 1970
		 * @param month month, from 1 to 12
		 * @param custom_probe length of custom probe in seconds, @see
		 * SetPeriod
		 */
		void SetMonth(SZARP_PROBE_TYPE period_type, 
				int year, int month, int custom_period = 0);
		/**
		 * Set if rows without any values should be printed.
		 * @param empty 1 for printing empty rows, 0 otherwise (this
		 * is default)
		 */
		void SetEmpty(int empty);
		/**
		 * Set string used when no data is available. 
		 * Internal copy of parameter is made. Default (initial) value
		 * is "NO_DATA".
		 * @param str string to print when no data is available
		 */
		void SetNoDataString(const std::wstring &str);
		/**
		 * Set function used to watch extraction progress.
		 * This function is called time to time with two arguments.
		 * The second one is data pointer passed to the method - for
		 * internal use. The first one is percent of extraction
		 * progress (time is measured). Values are from 0 to 100.
		 * Value 101 means XSLT processing, value 102 means zip
		 * compression, 103 means dumping data to output, 104
		 * means 'completed'.
		 * There's no guarantee that watcher function will be called.
		 * @param watcher progress watching function, NULL means no
		 * watcher
		 * @param data pointer passed as second argument for watcher
		 * function
		 */
		void SetProgressWatcher(void*(*watcher)(int, void*), 
				void *data);
		/**
		 * Method allows to set value used for canceling extraction
		 * process. We check value with given address during
		 * extraction, if it is different from 0, we cancel extraction
		 * process. We use value instead of function call because it's
		 * faster and we want to check it often. Passing NULL value
		 * means no cancel checking (it is default). After canceling
		 * you have to set value to 0 again, otherwise extraction is
		 * to be canceled again.
		 * @param cancel_pointer pointer to cancel-control value, NULL
		 * for no-control
		 */
		void SetCancelValue(int *cancel_pointer);
		/**
		 * Set character used as decimal separator. The default behaviour
		 * is to use separator from localization settings. 
		 * @param dec_sep character used as separator, \000 to reset to
		 * default behaviour
		 */
		void SetDecDelim(wchar_t dec_sep);
		/** Writes extracted data in CSV format to given output
		 * stream.
		 * @param output output stream, ready for writing
		 * @param delimiter delimieter string, not NULL
		 * @return ERR_OK on success or error code
		 */
		ErrorCode ExtractToCSV(FILE *output, const std::wstring &delimiter);
		/** Writes sums of extracted data in CSV format
		 * @param output output stream, ready for writing
		 * @param delimiter delimieter string, not NULL
		 * @return ERR_OK on success or error code
		 */
		ErrorCode ExtractSum(FILE *output, const std::wstring &delimiter);
		/** Writes extracted data in CSV format to given output
		 * stream.
		 * @param output output stream, ready for writing
		 * @param output encoding
		 * @return ERR_OK on success, or error code
		 */
		ErrorCode ExtractToXML(FILE *output, 
				const std::wstring &encoding = L"ISO-8859-2");
		/** Creates OpenOffice spreadsheat document with given path.
		 * @param path path to output file
		 * @return ERR_OK on success or error code
		 */
		ErrorCode ExtractToOpenOffice(const std::wstring& path);
	protected:
		/** Creates OpenOffice content.xml file, which has to be
		 * compressed. Used by ExtractToOpenOffice.
		 * @param output output file pointer
		 * @return ERR_OK on success or error code
		 */
		ErrorCode ExtractToOOXML(FILE* output);
		/** Prints OpenOffice XML header.
		 * @param output output file pointer
		 * @param param_num number of parameters
		 * @return ERR_OK on success or -1
		 */ 
		int PrintHeaderToOOXML(FILE* output, int param_num);
		/** Prints end of previous row, begining of current and time
		 * cell. Used internally by ExtractToOOXML.
		 * @param output output stream
		 * @param datebuf utf-8 encoded date string
		 * @param period period type
		 * @return ERRO_OK on success or -1
		 */
		//int PrintHeaderToOO2XML(FILE *output, int param_num);
		/** Prints header of content.xml file for OpenOffice2
		 * //end of previous row, begining of current and time
		 * //cell. 
		 * Used internally by ExtractToOO2XML.
		 * //@param output output stream
		 * //@param datebuf utf-8 encoded date string
		 * //@param period period type
		 * //@return ERRO_OK on success or -1
		 * @param output string
		 * @param param_num number of parameters
		 * @return ERR_OK on success or -1
		 */
		int PrintTimeToOOXML(FILE* output, char *datebuf, 
				SZARP_PROBE_TYPE period);
		/** Prints cell with value for OpenOffice XML. Used internally
		 * by ExtractoToOOXML.
		 * @param output output stream
		 * @param data value to print
		 * @param param pointer to parameter object
		 * @param doc pointer to XML document for PrintValueToFile
		 * @return ERRO_OK on success or -1
		 */
		int PrintValueToOOXML(FILE* output, SZBASE_TYPE data,
				TParam *param, xmlDocPtr doc);
	public:
		/** Extract document to XML, transform is with XSLT stylesheet
		 * and write to output stream.
		 * @param output output stream
		 * @param stylesheet stylesheet for transformation
		 * @param params a NULL terminated array of parameters
		 * names/values tuples (UTF-8 encoded)
		 * @return ERR_OK on success or error code
		 */
		ErrorCode ExtractAndTransform(FILE *output, 
				xsltStylesheetPtr stylesheet,
				char **params = NULL);
		/** Extract document to XML, transform is with XSLT stylesheet
		 * and write to output stream.
		 * @param output output stream
		 * @param stylesheet_path path to file with stylesheet for 
		 * transformation
		 * @param params a NULL terminated array of parameters
		 * names/values tuples (UTF-8 encoded)
		 * @return ERR_OK on success or error code
		 */
		ErrorCode ExtractAndTransform(FILE *output, 
				char *stylesheet_path,
				char **params = NULL);
	protected:
		/**
		 * Extract XML data using XML Writer interface. Document is
		 * printed to given initialized writer object.
		 * @param writer XMLWriter object 
		 * @return ERR_OK on success or error code
		 */
		ErrorCode ExtractToXMLWriter(xmlTextWriterPtr writer, 
				const std::wstring& encoding = L"ISO-8859-2");
		/** 
		 * Print properly converted param element to writer.
		 * @param writer pointer to writer
		 * @param data SzarpBase parameter value
		 * @param p pointer to parameter object (requiered to
		 * get precision info)
		 * @return 0 on success, -1 on error (same as
		 * xmlTextWriterWriteElement)
		 */
		int PrintValueToWriter(xmlTextWriterPtr writer,
				SZBASE_TYPE data, TParam *p);
		/** 
		 * Print properly converted param element to stream.
		 * @param output pointer to file stream
		 * @param data SzarpBase parameter value
		 * @param p pointer to parameter object (requiered to
		 * get precision info)
		 * @param toutf8 true for converting to utf8 and escaping XML
		 * entities
		 * @param doc XML doc pointer needed for escaping XML chars,
		 * may be pointer to any XML document; must not be null if
		 * toutf8 is true
		 * @return return from fprintf
		 */
		int PrintValueToFile(FILE *output, 
				SZBASE_TYPE data, 
				TParam *p,
				bool toutf8 = false,
				xmlDocPtr doc = NULL);
/*		int PrintValueToStream(wxOutputStream& output,
				SZBASE_TYPE data, 
				TParam *p, 
				bool toutf8 = false, 
				xmlDocPtr doc = NULL);*/
		/**
		 * Replaces all comma and dot characters in string with dec_sep character.
		 * Does nothing if dec_sep is \000. Used for printing values with custom
		 * separator instead of default from locales.
		 */
		void ReplaceSeparator(wchar_t *buf);

		TSzarpConfig *ipk;	/**< IPK object */
		szb_buffer_t *szb;	/**< SzarpBase buffer */
		std::vector<Param> params;		/** array of param names to extract */
		std::vector<TParam*> params_objects;
					/**< array of pointers to param objects
					 */
		time_t start;		/**< first date */
		time_t end;		/**< last date */
		SZARP_PROBE_TYPE period_type;
					/**< type of probes to extract */
		int custom_period;	/**< length of custom time period */
		int empty;		/**< should we write empty rows ? */
		std::wstring nodata_str;	/**< string printed when no data is
					  available */
		void*(*progress_watcher)(int, void*);
					/**< function used to show extraction
					 * progress to user */
		void *watcher_data;	/**< data to pass to progress watching
					  function */
		int* cancel_pointer;	/**< pointer to cancel-control value */
		char dec_sep;		/**< decimal separator, 0 for using localization
					  settings */
		int private_zero;	/**< private zero value, used when
					  no cancel control is used */
		wchar_t priv_buffer[_SZBEXTRACTOR_PRIV_BUFFER_SIZE];
					/**< private buffer */
};

#endif
