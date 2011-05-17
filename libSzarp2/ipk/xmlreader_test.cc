#include <iostream>
#include <string>
#include "conversion.h"
#include "liblog.h"
#include <boost/typeof/std/locale.hpp>

#include <libxml/xmlreader.h>

#include "szarp_config.h"
using namespace std;


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
	if (value == NULL)
		cout << "NO ATTRIBUTES" << endl;
	else
		cout <<"ATTR E X I S T S" << endl;

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

// removed bilg from tests
wstring pathes[] = {
/*
//L"/root/xml/gcie.xml", // not valid - no "id" in meny <unit> tags
//L"/root/xml/poligon.xml", // more "speed" in old parser ->  modbus:speed
//L"/root/xml/jgor.xml", // more "speed" in old parser ->  modbus:speed
//L"/root/xml/leg1.xml", // more "speed" in old parser ->  modbus:speed
*/
L"/root/xml/gliw.xml", // identical only one configuration with <radio> inside <device>
L"/root/xml/test1.xml", // identical (workaround: <define>)
L"/root/xml/page.xml", // identical (workaround: <define>)
L"/root/xml/sosn.xml", // identical (workaround: <define>)
L"/root/xml/glwX.xml", // identical (workaround: <define>)
L"/root/xml/staX.xml", // identical (workaround: <define>)
L"/root/xml/sztX.xml", // identical (workaround: <define>)
L"/root/xml/bytX.xml", // identical (workaround: <define>)
L"/root/xml/trgX.xml", // identical (workaround: <define>)
L"/root/xml/racX.xml", // identical (workaround: <define>)
L"/root/xml/przX.xml", // identical (workaround: <define>)
L"/root/xml/pasX.xml", // identical (workaround: <define>)
L"/root/xml/zamX.xml", // identical (workaround: <define>)
L"/root/xml/chrX.xml", // identical (workaround: <define>)
L"/root/xml/rawX.xml", // identical (workaround: <define>)
L"/root/xml/legX.xml", // identical (workaround: <define>)
L"/root/xml/sunX.xml", // identical (workaround: <define>)
L"/root/xml/sun.xml", // identical
L"/opt/szarp/debian/config/params.xml", // identical
L"/opt/szarp/snew/config/params.xml", // identical
L"/opt/szarp/gcwp/config/params.xml", // identical
L"/root/xml/stpd.xml", // identical
L"/root/xml/rww6.xml", // identical
L"/root/xml/rww1.xml", // identical
L"/root/xml/ldw2.xml", // identical
L"/root/xml/star.xml", // identical
L"/root/xml/rww10.xml", // identical
L"/root/xml/kato.xml", // identical - raport inside unit - line 3428 <fix on server>
L"/root/xml/tgw7.xml", // identical
L"/root/xml/orzy.xml", // identical
L"/root/xml/btw5.xml", // identical
L"/root/xml/btw9.xml", // identical
L"/root/xml/leg3.xml", // identical
L"/root/xml/niwk.xml", // identical
L"/root/xml/rcek.xml", // identical
L"/root/xml/zamo.xml", // identical
L"/root/xml/mysl.xml", // identical
L"/root/xml/skis.xml", // identical
L"/root/xml/leg4.xml", // identical
L"/root/xml/zmp2.xml", // identical
L"/root/xml/newterm.xml", // identical
L"/root/xml/ldw5.xml", // identical
L"/root/xml/kepn.xml", // identical
L"/root/xml/mdw1.xml", // identical
L"/root/xml/rawc.xml", // identical
L"/root/xml/rww5.xml", // identical
L"/root/xml/tgw1.xml", // identical
L"/root/xml/dbms.xml", // identical
L"/root/xml/psw2.xml", // identical
L"/root/xml/btwa.xml", // identical
L"/root/xml/gcwp.xml", // identical
L"/root/xml/szcz.xml", // identical
L"/root/xml/econ.xml", // identical
L"/root/xml/szom.xml", // identical
L"/root/xml/wuje.xml", // identical
L"/root/xml/radm.xml", // identical
L"/root/xml/rww3.xml", // identical
L"/root/xml/glws.xml", // identical
L"/root/xml/libz.xml", // identical
L"/root/xml/zmw7.xml", // identical
L"/root/xml/murc.xml", // identical
L"/root/xml/prw1.xml", // identical
L"/root/xml/kras.xml", // identical
L"/root/xml/wodz.xml", // identical
L"/root/xml/btw2.xml", // identical
L"/root/xml/wabr.xml", // identical
L"/root/xml/rww2.xml", // identical
L"/root/xml/glw2.xml", // identical
L"/root/xml/sztu.xml", // identical
L"/root/xml/snew.xml", // identical
L"/root/xml/zmw9.xml", // identical
L"/root/xml/byto.xml", // identical
L"/root/xml/glw3.xml", // identical
L"/root/xml/rww4.xml", // identical
L"/root/xml/gnw1.xml", // identical
L"/root/xml/zmw2.xml", // identical
L"/root/xml/prw2.xml", // identical
L"/root/xml/mied.xml", // identical
L"/root/xml/btw7.xml", // identical
L"/root/xml/prud.xml", // identical
L"/root/xml/wloc.xml", // identical
L"/root/xml/rww8.xml", // identical
L"/root/xml/gizy.xml", // identical
L"/root/xml/rfko.xml", // identical
L"/root/xml/btw6.xml", // identical
L"/root/xml/chw3.xml", // identical
L"/root/xml/chrz.xml", // identical
L"/root/xml/btw3.xml", // identical
L"/root/xml/glw4.xml", // identical
L"/root/xml/ruda.xml", // identical
L"/root/xml/malo.xml", // identical
L"/root/xml/szw3.xml", // identical
L"/root/xml/chw1.xml", // identical
L"/root/xml/elsw.xml", // identical
L"/root/xml/test.xml", // identical
L"/root/xml/raci.xml", // identical
L"/root/xml/btw8.xml", // identical
L"/root/xml/chw2.xml", // identical
L"/root/xml/szw2.xml", // identical
L"/root/xml/zmw1.xml", // identical
L"/root/xml/psw3.xml", // identical
L"/root/xml/elk1.xml", // identical
L"/root/xml/tgw4.xml", // identical
L"/root/xml/rww7.xml", // identical
L"/root/xml/orne.xml", // identical
L"/root/xml/zmw6.xml", // identical
L"/root/xml/trgr.xml", // identical
L"/root/xml/btw1.xml", // identical
L"/root/xml/zory.xml", // identical
L"/root/xml/szw1.xml", // identical
L"/root/xml/pasl.xml", // identical
L"/root/xml/atex.xml", // identical
L"/root/xml/zmw5.xml", // identical
L"/root/xml/pabi.xml", // identical
L"/root/xml/kleo.xml", // identical
L"/root/xml/fame.xml", // identical
L"/root/xml/ldwr.xml", // identical
L"/root/xml/tgw6.xml", // identical
L"/root/xml/prza.xml", // identical
L"/root/xml/weso.xml", // identical
L"/root/xml/tgw2.xml", // identical
L"/root/xml/rww9.xml", // identical
L"/root/xml/chrw.xml", // identical
L"/root/xml/zmw4.xml", // identical
L"/root/xml/rww11.xml", // identical
L"/root/xml/zmw8.xml", // identical
L"/root/xml/zmw3.xml", // identical
L"/root/xml/kazi.xml", // identical
L"/root/xml/tgw3.xml", // identical
L"/root/xml/glw6.xml", // identical
L"/root/xml/ebox.xml", // identical
L"/root/xml/ldw4.xml", // identical
L"/root/xml/ldw3.xml", // identical
L"/root/xml/zmk1.xml", // identical
L"/root/xml/leg2.xml", // identical
L"/root/xml/gnie.xml", // identical
L"/root/xml/btw4.xml", // identical
L"/root/xml/wxxx.xml", // identical
L"/root/xml/pore.xml", // identical
L"/root/xml/swie.xml", // identical
L"/root/xml/swid.xml", // identical
L"/root/xml/glw1.xml", // identical
L"/root/xml/modl.xml", // identical
L"/root/xml/mdw2.xml", // identical
L"/root/xml/glw5.xml", // identical
L"/root/xml/ldw1.xml", // identical
L"/root/xml/ldw7.xml", // identical
L"/root/xml/tgw5.xml", // identical
L"/root/xml/psw1.xml", // identical
L"/root/xml/sunc.xml", // identical
L"/root/xml/rcw1.xml", // identical
L"/root/xml/prat.xml", // identical
		};

	for (int i =0 ; i < sizeof(pathes)/ sizeof(pathes[0]) ; ++i) {
		std::wcout << L"------------ " << pathes[i] << L" ----------------" << endl;
		TSzarpConfig confOld;
		confOld.loadXML(pathes[i],L"debian");
		confOld.saveXML(L"/tmp/outOld.xml");

		TSzarpConfig confNew;
		confNew.parseReader(pathes[i]);
		confNew.saveXML(L"/tmp/outNew.xml");

	}

//	while(1) {}

	return 0;
}
