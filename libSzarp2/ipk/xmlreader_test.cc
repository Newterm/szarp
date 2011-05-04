#include <iostream>
#include <string>
#include "conversion.h"
#include "liblog.h"


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
	cout <<"begin test" << endl;
	wstring path = L"/opt/szarp/debian/config/params.xml";
//	wstring path = L"/opt/szarp/snew/config/params.xml";
//	wstring path = L"/root/zamX.xml";
//	parseFile(path);

	TSzarpConfig conf;
//	conf.loadXML(path,L"debian");
	conf.parseReader(path);
//	conf.saveXML(L"/tmp/out.xml");
	return 0;
}
