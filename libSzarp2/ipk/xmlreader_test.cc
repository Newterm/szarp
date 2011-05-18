#include <iostream>
#include <string>
#include "conversion.h"
#include "liblog.h"
#include <boost/typeof/std/locale.hpp>
#include <libxml/xmlreader.h>
#include "szarp_config.h"
using namespace std;


#include <boost/crc.hpp>  // for boost::crc_32_type

#include <cstdlib>    // for EXIT_SUCCESS, EXIT_FAILURE
#include <exception>  // for std::exception
#include <fstream>    // for std::ifstream
#include <ios>        // for std::ios_base, etc.
#include <iostream>   // for std::cerr, std::cout
#include <ostream>    // for std::endl

// Redefine this to change to processing buffer size
#ifndef PRIVATE_BUFFER_SIZE
#define PRIVATE_BUFFER_SIZE  1024
#endif

// Global objects
std::streamsize const  buffer_size = PRIVATE_BUFFER_SIZE;


/* only for ASCII */
wstring char2wstr(const char* s) {
	return wstring(s, s + strlen(s));
}

boost::crc_32_type getCRC(const char *fname) {
	boost::crc_32_type  result;
	try
	{

			std::ifstream  ifs( fname, std::ios_base::binary );

			if ( ifs )
			{
				do
				{
					char  buffer[ buffer_size ];

					ifs.read( buffer, buffer_size );
					result.process_bytes( buffer, ifs.gcount() );
				} while ( ifs );
			}
			else
			{
				std::cerr << "Failed to open file '" << fname << "'."  << std::endl;
			}
	}
	catch ( std::exception &e )
	{
		std::cerr << "Found an exception with '" << e.what() << "'." << std::endl;
	}
	catch ( ... )
	{
		std::cerr << "Found an unknown exception." << std::endl;
	}
	return result;
}

void processNodeReader(xmlTextReaderPtr reader) {
	xmlChar *name, *value;

	name = xmlTextReaderName(reader);
	if (name == NULL)
		name = xmlStrdup(BAD_CAST "--");
	value = xmlTextReaderValue(reader);

	cout << "depth: " << xmlTextReaderDepth(reader) << " node type: " <<
		xmlTextReaderNodeType(reader) << " name: " <<
		name << " isEmptyElement: " << xmlTextReaderIsEmptyElement(reader);

	if (value == NULL)
		cout << endl;
	else {
		cout << "value: " << value << endl;
		xmlFree(value);
	}

	value = xmlTextReaderGetAttribute(reader, (unsigned char*)  "title");

	while(value != NULL) {
		cout <<"atrr: " << value << endl;
		xmlFree(value);
		value = xmlTextReaderGetAttribute(reader,name);
	}

	xmlFree(name);
}

int parseFile(const wstring &file) {
	xmlTextReaderPtr reader;
	int ret = 0;

	reader = xmlNewTextReaderFilename(SC::S2A(file).c_str());
	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);
		while (ret == 1) {
			processNodeReader(reader);
			ret = xmlTextReaderRead(reader);
		}
		xmlFreeTextReader(reader);
		if (ret != 0) {
			cout << "faild parse";
		}
	} else {
		cout <<  "unable to open file";
	}

	return ret;
}

int main() {

const char* pathes[] = {
"/root/xml/bilg.xml", // identical after fix (set "prec" = "dde:prec", prec before dde:prec)
"/root/xml/gcie.xml", // identical after fix (added missing IDs ( moved on front ) )
"/root/xml/poligon.xml", // identical after fix (added "speed" before "modbus:speed" )
"/root/xml/jgor.xml", // identical after fix (added "speed" before "modbus:speed")
"/root/xml/leg1.xml", // identical after fix (added "speed" before "modbus:speed")
"/root/xml/gliw.xml", // identical only one configuration with <radio> inside <device>
"/root/xml/test1.xml", // identical (workaround: <define>)
"/root/xml/page.xml", // identical (workaround: <define>)
"/root/xml/sosn.xml", // identical (workaround: <define>)
"/root/xml/glwX.xml", // identical (workaround: <define>)
"/root/xml/staX.xml", // identical (workaround: <define>)
"/root/xml/sztX.xml", // identical (workaround: <define>)
"/root/xml/bytX.xml", // identical (workaround: <define>)
"/root/xml/trgX.xml", // identical (workaround: <define>)
"/root/xml/racX.xml", // identical (workaround: <define>)
"/root/xml/przX.xml", // identical (workaround: <define>)
"/root/xml/pasX.xml", // identical (workaround: <define>)
"/root/xml/zamX.xml", // identical (workaround: <define>)
"/root/xml/chrX.xml", // identical (workaround: <define>)
"/root/xml/rawX.xml", // identical (workaround: <define>)
"/root/xml/legX.xml", // identical (workaround: <define>)
"/root/xml/sunX.xml", // identical (workaround: <define>)
"/root/xml/sun.xml", // identical
"/opt/szarp/debian/config/params.xml", // identical
"/opt/szarp/snew/config/params.xml", // identical
"/opt/szarp/gcwp/config/params.xml", // identical
"/root/xml/stpd.xml", // identical
"/root/xml/rww6.xml", // identical
"/root/xml/rww1.xml", // identical
"/root/xml/ldw2.xml", // identical
"/root/xml/star.xml", // identical
"/root/xml/rww10.xml", // identical
"/root/xml/kato.xml", // identical
"/root/xml/tgw7.xml", // identical
"/root/xml/orzy.xml", // identical
"/root/xml/btw5.xml", // identical
"/root/xml/btw9.xml", // identical
"/root/xml/leg3.xml", // identical
"/root/xml/niwk.xml", // identical
"/root/xml/rcek.xml", // identical
"/root/xml/zamo.xml", // identical
"/root/xml/mysl.xml", // identical
"/root/xml/skis.xml", // identical
"/root/xml/leg4.xml", // identical
"/root/xml/zmp2.xml", // identical
"/root/xml/newterm.xml", // identical
"/root/xml/ldw5.xml", // identical
"/root/xml/kepn.xml", // identical
"/root/xml/mdw1.xml", // identical
"/root/xml/rawc.xml", // identical
"/root/xml/rww5.xml", // identical
"/root/xml/tgw1.xml", // identical
"/root/xml/dbms.xml", // identical
"/root/xml/psw2.xml", // identical
"/root/xml/btwa.xml", // identical
"/root/xml/gcwp.xml", // identical
"/root/xml/szcz.xml", // identical
"/root/xml/econ.xml", // identical
"/root/xml/szom.xml", // identical
"/root/xml/wuje.xml", // identical
"/root/xml/radm.xml", // identical
"/root/xml/rww3.xml", // identical
"/root/xml/glws.xml", // identical
"/root/xml/libz.xml", // identical
"/root/xml/zmw7.xml", // identical
"/root/xml/murc.xml", // identical
"/root/xml/prw1.xml", // identical
"/root/xml/kras.xml", // identical
"/root/xml/wodz.xml", // identical
"/root/xml/btw2.xml", // identical
"/root/xml/wabr.xml", // identical
"/root/xml/rww2.xml", // identical
"/root/xml/glw2.xml", // identical
"/root/xml/sztu.xml", // identical
"/root/xml/snew.xml", // identical
"/root/xml/zmw9.xml", // identical
"/root/xml/byto.xml", // identical
"/root/xml/glw3.xml", // identical
"/root/xml/rww4.xml", // identical
"/root/xml/gnw1.xml", // identical
"/root/xml/zmw2.xml", // identical
"/root/xml/prw2.xml", // identical
"/root/xml/mied.xml", // identical
"/root/xml/btw7.xml", // identical
"/root/xml/prud.xml", // identical
"/root/xml/wloc.xml", // identical
"/root/xml/rww8.xml", // identical
"/root/xml/gizy.xml", // identical
"/root/xml/rfko.xml", // identical
"/root/xml/btw6.xml", // identical
"/root/xml/chw3.xml", // identical
"/root/xml/chrz.xml", // identical
"/root/xml/btw3.xml", // identical
"/root/xml/glw4.xml", // identical
"/root/xml/ruda.xml", // identical
"/root/xml/malo.xml", // identical
"/root/xml/szw3.xml", // identical
"/root/xml/chw1.xml", // identical
"/root/xml/elsw.xml", // identical
"/root/xml/test.xml", // identical
"/root/xml/raci.xml", // identical
"/root/xml/btw8.xml", // identical
"/root/xml/chw2.xml", // identical
"/root/xml/szw2.xml", // identical
"/root/xml/zmw1.xml", // identical
"/root/xml/psw3.xml", // identical
"/root/xml/elk1.xml", // identical
"/root/xml/tgw4.xml", // identical
"/root/xml/rww7.xml", // identical
"/root/xml/orne.xml", // identical
"/root/xml/zmw6.xml", // identical
"/root/xml/trgr.xml", // identical
"/root/xml/btw1.xml", // identical
"/root/xml/zory.xml", // identical
"/root/xml/szw1.xml", // identical
"/root/xml/pasl.xml", // identical
"/root/xml/atex.xml", // identical
"/root/xml/zmw5.xml", // identical
"/root/xml/pabi.xml", // identical
"/root/xml/kleo.xml", // identical
"/root/xml/fame.xml", // identical
"/root/xml/ldwr.xml", // identical
"/root/xml/tgw6.xml", // identical
"/root/xml/prza.xml", // identical
"/root/xml/weso.xml", // identical
"/root/xml/tgw2.xml", // identical
"/root/xml/rww9.xml", // identical
"/root/xml/chrw.xml", // identical
"/root/xml/zmw4.xml", // identical
"/root/xml/rww11.xml", // identical
"/root/xml/zmw8.xml", // identical
"/root/xml/zmw3.xml", // identical
"/root/xml/kazi.xml", // identical
"/root/xml/tgw3.xml", // identical
"/root/xml/glw6.xml", // identical
"/root/xml/ebox.xml", // identical
"/root/xml/ldw4.xml", // identical
"/root/xml/ldw3.xml", // identical
"/root/xml/zmk1.xml", // identical
"/root/xml/leg2.xml", // identical
"/root/xml/gnie.xml", // identical
"/root/xml/btw4.xml", // identical
"/root/xml/wxxx.xml", // identical
"/root/xml/pore.xml", // identical
"/root/xml/swie.xml", // identical
"/root/xml/swid.xml", // identical
"/root/xml/glw1.xml", // identical
"/root/xml/modl.xml", // identical
"/root/xml/mdw2.xml", // identical
"/root/xml/glw5.xml", // identical
"/root/xml/ldw1.xml", // identical
"/root/xml/ldw7.xml", // identical
"/root/xml/tgw5.xml", // identical
"/root/xml/psw1.xml", // identical
"/root/xml/sunc.xml", // identical
"/root/xml/rcw1.xml", // identical
"/root/xml/prat.xml", // identical
		};

	for (int i =0 ; i < sizeof(pathes)/ sizeof(pathes[0]) ; ++i) {
		std::wcout << L"------------ " << char2wstr(pathes[i]) << L" ----------------" << endl;
		TSzarpConfig confOld;
//TODO: take tmp file from system
		const char* outOld = "/tmp/outOld.xml";
		confOld.loadXML(char2wstr(pathes[i]),L"test");
		confOld.saveXML(char2wstr(outOld));

//TODO: take tmp file from system,
		const char* outNew = "/tmp/outNew.xml";
		TSzarpConfig confNew;
		confNew.parseReader(char2wstr(pathes[i]));
		confNew.saveXML(char2wstr(outNew));

		boost::crc_32_type crc1 = getCRC(outOld);
		boost::crc_32_type crc2 = getCRC(outNew);

		if (crc1.checksum() != crc2.checksum()) {
		std::wcout << L"DIFFERENT CRC!" << endl;
		}
	}

//	while(1) {}

	return 0;
}
