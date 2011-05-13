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

	wstring pathes[] = { 
		L"/opt/szarp/debian/config/params.xml",
		L"/opt/szarp/snew/config/params.xml",
		L"/opt/szarp/gcwp/config/params.xml",
L"/root/xml/rww6.xml",
L"/root/xml/rww1.xml",
//L"/root/xml/glwX.xml", // !!
//L"/root/xml/gcie.xml", // ?
L"/root/xml/ldw2.xml", // identical
L"/root/xml/star.xml", // identical
L"/root/xml/rww10.xml", // identical
//L"/root/xml/kato.xml",// empty send --  <send/>  -- check it
L"/root/xml/tgw7.xml", // identical
L"/root/xml/orzy.xml", // cd
L"/root/xml/btw5.xml", // identical
L"/root/xml/btw9.xml", // identical
L"/root/xml/bilg.xml", // prec in param are different?
//L"/root/xml/leg3.xml", // !! 
L"/root/xml/niwk.xml", // identical
L"/root/xml/jgor.xml", // cd and more
L"/root/xml/rcek.xml", // identical
L"/root/xml/staX.xml", // !
L"/root/xml/zamo.xml", // identical
//L"/root/xml/mysl.xml", // !!
L"/root/xml/skis.xml", // identical
//L"/root/xml/leg4.xml",// empty send --  <send/>  -- check it
L"/root/xml/zmp2.xml", // identical
L"/root/xml/newterm.xml", // cd
L"/root/xml/ldw5.xml", // identical
L"/root/xml/kepn.xml", // identical
L"/root/xml/mdw1.xml", // identical
//L"/root/xml/rawc.xml", // empty send --  <send/>  -- check it
L"/root/xml/sztX.xml", // !
L"/root/xml/rww5.xml", // identical
L"/root/xml/tgw1.xml", // identical
L"/root/xml/dbms.xml", // identical
L"/root/xml/psw2.xml", // identical
L"/root/xml/bytX.xml", // !
L"/root/xml/btwa.xml", // identical
L"/root/xml/gcwp.xml", // cd
L"/root/xml/trgX.xml", // !
L"/root/xml/leg1.xml", // old parser in <device> takes modbus:speed as speed 
L"/root/xml/szcz.xml", // more "send_no_data" ?
L"/root/xml/econ.xml", // identical
L"/root/xml/szom.xml", // cd
//L"/root/xml/gliw.xml", // <radio> inside <device>
//L"/root/xml/wuje.xml", // !!
L"/root/xml/radm.xml", // more "send_no_data" ?
L"/root/xml/rww3.xml", // identical
L"/root/xml/glws.xml", // identical
L"/root/xml/libz.xml", // identical
L"/root/xml/racX.xml", // !
L"/root/xml/zmw7.xml", // identical
L"/root/xml/murc.xml", // identical
L"/root/xml/prw1.xml", // identical
L"/root/xml/kras.xml", // identical
L"/root/xml/wodz.xml", // identical
L"/root/xml/btw2.xml", // identical
L"/root/xml/wabr.xml",// more "send_no_data" ?
L"/root/xml/rww2.xml", // identical
L"/root/xml/glw2.xml", // identical
//L"/root/xml/sztu.xml",// empty send --  <send/>  -- check it
L"/root/xml/snew.xml", // more "special" ?
L"/root/xml/przX.xml", // !
L"/root/xml/zmw9.xml",
L"/root/xml/byto.xml",
L"/root/xml/glw3.xml",
L"/root/xml/rww4.xml",
L"/root/xml/gnw1.xml",
//L"/root/xml/sunX.xml",// tag name: rauort <- BUG IN xml line: 2857
L"/root/xml/zmw2.xml",
L"/root/xml/prw2.xml",
L"/root/xml/mied.xml",
L"/root/xml/btw7.xml",
L"/root/xml/prud.xml",
//L"/root/xml/wloc.xml",// empty send --  <send/>  -- check it
L"/root/xml/rww8.xml",
L"/root/xml/gizy.xml",
L"/root/xml/rfko.xml",
L"/root/xml/btw6.xml",
L"/root/xml/chw3.xml",
L"/root/xml/chrz.xml",
L"/root/xml/btw3.xml",
L"/root/xml/glw4.xml",
L"/root/xml/ruda.xml",
L"/root/xml/malo.xml",
L"/root/xml/szw3.xml",
L"/root/xml/chw1.xml",
L"/root/xml/elsw.xml",
L"/root/xml/page.xml",
L"/root/xml/test.xml",
L"/root/xml/sosn.xml",
//L"/root/xml/raci.xml", // empty send --  <send/>  -- check it
L"/root/xml/btw8.xml",
L"/root/xml/chw2.xml",
L"/root/xml/szw2.xml",
L"/root/xml/zmw1.xml",
L"/root/xml/pasX.xml",
L"/root/xml/psw3.xml",
L"/root/xml/elk1.xml",
L"/root/xml/tgw4.xml",
L"/root/xml/rww7.xml",
L"/root/xml/orne.xml",
L"/root/xml/zmw6.xml",
L"/root/xml/trgr.xml",
L"/root/xml/btw1.xml",
//L"/root/xml/legX.xml", // !!
//L"/root/xml/zory.xml", // ?
L"/root/xml/szw1.xml",
L"/root/xml/pasl.xml",
L"/root/xml/atex.xml",
L"/root/xml/rawX.xml",
//L"/root/xml/stpd.xml", // param inside params! -> line:  1735   -> "zadana roznica cisnien kolektorowych"
L"/root/xml/zmw5.xml",
L"/root/xml/pabi.xml",
L"/root/xml/kleo.xml",
L"/root/xml/fame.xml",
L"/root/xml/ldwr.xml",
L"/root/xml/tgw6.xml",
L"/root/xml/prza.xml",
L"/root/xml/weso.xml",
L"/root/xml/tgw2.xml",
L"/root/xml/rww9.xml",
L"/root/xml/chrw.xml",
L"/root/xml/zmw4.xml",
L"/root/xml/rww11.xml",
L"/root/xml/zmw8.xml",
L"/root/xml/zmw3.xml",
L"/root/xml/kazi.xml",
L"/root/xml/tgw3.xml",
L"/root/xml/glw6.xml",
L"/root/xml/ebox.xml",
L"/root/xml/ldw4.xml",
L"/root/xml/ldw3.xml",
L"/root/xml/zmk1.xml",
L"/root/xml/poligon.xml",
L"/root/xml/leg2.xml",
//L"/root/xml/sun.xml", // tag name: rauort <- BUG IN xml line: 3622
L"/root/xml/gnie.xml", //param
L"/root/xml/btw4.xml",
L"/root/xml/wxxx.xml",
//L"/root/xml/pore.xml", // !!
L"/root/xml/swie.xml", // param/
L"/root/xml/swid.xml", // param/ script
L"/root/xml/glw1.xml",
L"/root/xml/modl.xml", // param/
L"/root/xml/mdw2.xml", // identical
L"/root/xml/glw5.xml",
L"/root/xml/ldw1.xml",
L"/root/xml/ldw7.xml",
L"/root/xml/tgw5.xml",
L"/root/xml/psw1.xml",
L"/root/xml/sunc.xml",
L"/root/xml/chrX.xml",
L"/root/xml/rcw1.xml",
L"/root/xml/prat.xml",
L"/root/xml/zamX.xml",
L"/root/xml/test1.xml" // identical
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
