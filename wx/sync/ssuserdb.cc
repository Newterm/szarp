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
*/
#include "ssuserdb.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

UserDB::~UserDB() {}

XMLUserDB::XMLUserDB(xmlDocPtr doc,const char *prefix) {
	m_document = doc;
	m_context = xmlXPathNewContext(m_document);
	xmlXPathRegisterNs(m_context, BAD_CAST "s", 
		BAD_CAST "http://www.praterm.com.pl/SZARP/sync-users");
	hostname = prefix;
}

char *XMLUserDB::EscapeString(const char* string) {
	char *result;
	size_t size;
	FILE* stream = open_memstream(&result, &size);

	const char* c = string;

	while (*c) {
		switch (*c) {
			case '\'':
				fprintf(stream, "&apos;");
				break;
			case '\"':
				fprintf(stream, "&quot;");
				break;
			default:
				fprintf(stream, "%c", *c);
				break;
		}
		c++;
	}

	fclose(stream);

	return result;
}


XMLUserDB* XMLUserDB::LoadXML(const char *path,const char *prefix) {
	XMLUserDB *result = NULL;
	xmlDocPtr doc = xmlParseFile(path);

	if (doc == NULL) {
		sz_log(0, "Could not load doc %s", path);
		return NULL;
	} 

	const char *schema_file = "file:///"PREFIX"/resources/dtd/sync-users.rng";
	xmlRelaxNGParserCtxtPtr pctx;
	pctx = xmlRelaxNGNewParserCtxt(schema_file);
	
	xmlRelaxNGPtr rng_schema = xmlRelaxNGParse(pctx);
	xmlRelaxNGFreeParserCtxt(pctx);
	if (rng_schema == NULL) {
		sz_log(0, "Could not load RelaxNG schema from file %s", schema_file);
		xmlFreeDoc(doc);
		return NULL;
	}

	xmlRelaxNGValidCtxtPtr vctx = xmlRelaxNGNewValidCtxt(rng_schema);
	int ret = xmlRelaxNGValidateDoc(vctx, doc);
	xmlRelaxNGFreeValidCtxt(vctx);
	if (ret < 0) {
		sz_log(0, "Internal libxml error while validating document %s", path);
		xmlFreeDoc(doc);
		return NULL;
	}
	if (ret > 0) {
		sz_log(0, "Error validating document %s, error ", BAD_CAST(xmlGetLastError()->message));
		xmlFreeDoc(doc);
		return NULL;
	}

	result = new XMLUserDB(doc,prefix);

	return result;
}

bool XMLUserDB::FindUser(const char* user, const char* passwd, char** basedir, char **sync) {
	xmlNodePtr node;
	bool result = false;

	char *_user = EscapeString(user);
	char *_passwd = EscapeString(passwd);

	char* expr;
	asprintf(&expr, "//s:user[@name='%s' and @password='%s']", _user, _passwd);
	xmlChar* uexpr = xmlCharStrdup(expr);

	xmlXPathObjectPtr rset = xmlXPathEvalExpression(uexpr, m_context);

	if (xmlXPathNodeSetIsEmpty(rset->nodesetval)) 
		goto cleanup;

	node = rset->nodesetval->nodeTab[0];

	*basedir = (char*) xmlGetProp(node, (xmlChar*)"basedir");
	*sync = (char*) xmlGetProp(node, (xmlChar*)"sync");
	
	result = true;
cleanup:
	xmlXPathFreeObject(rset);
	free(_user);
	free(_passwd);
	free(expr);
	xmlFree(uexpr);

	return result;
}

int XMLUserDB::CheckUser(int protocol,const char* user, const char* passwd, const char* key,char **msg) {
	xmlNodePtr node;

	char *_user = EscapeString(user);
	char *_passwd = EscapeString(passwd);
	char *prop_key = NULL;
	char *prop_server = NULL;
	char *prop_ip = NULL;
	char *expr;
	char *expired;
	char *cmd;


	asprintf(&expr, "//s:user[@name='%s' and @password='%s']", _user, _passwd);
	xmlChar* uexpr = xmlCharStrdup(expr);

	xmlXPathObjectPtr rset = xmlXPathEvalExpression(uexpr, m_context);

	if (xmlXPathNodeSetIsEmpty(rset->nodesetval)) {
		xmlFree(uexpr);
		return Message::INVALID_AUTH_INFO;
	}
	xmlFree(uexpr);

	node = rset->nodesetval->nodeTab[0];

	prop_key = (char *) xmlGetProp(node,(xmlChar*)"hwkey");
	prop_server = (char*) xmlGetProp(node, (xmlChar*)"server");
	expired = (char *) xmlGetProp(node, (xmlChar*)"expired");
	
		
	if(protocol != MessageType::AUTH2 && protocol != MessageType::AUTH3) 
		return Message::OLD_VERSION;

	
	if(strcmp(prop_key,key) && strlen(prop_key)>2 && strcmp(key,SPECIAL_WINDOWS_KEY) != 0  && strcmp(key,SPECIAL_LINUX_KEY) != 0 ) {
		sz_log(1, "HWKEY: INVALIDKEY '%s' USER '%s'", key, user);
		xmlFree(prop_key);
		xmlFree(prop_server);
		xmlFree(expired);
		return Message::INVALID_HWKEY;
	}
	if(strcmp(prop_key,key) && !strcmp(prop_key, "-1") && strcmp(key,SPECIAL_WINDOWS_KEY) && strcmp(key,SPECIAL_LINUX_KEY) != 0) {
		asprintf(&cmd,"/opt/szarp/bin/ssconf.py -u '%s' -c '%s'",user,key);
		int ret = system(cmd);
		free(cmd);
		if (ret) {
			sz_log(1, "HWKEY: NEWKEY '%s' USER '%s' FAILED, return code %d", key, user, ret);
		} else {
			sz_log(2, "HWKEY: NEWKEY '%s' USER '%s'", key, user);
		}
	}

	xmlFree(prop_key);
	
	if(strcmp(hostname,prop_server)!=0) {

		asprintf(&expr, "//s:server[@name='%s']", prop_server);
		xmlChar* uexpr = xmlCharStrdup(expr);

		xmlXPathObjectPtr rset = xmlXPathEvalExpression(uexpr, m_context);
	        if (xmlXPathNodeSetIsEmpty(rset->nodesetval)) {
			xmlFree(uexpr);
                       return Message::INVALID_AUTH_INFO;
		}
		xmlFree(uexpr);
				       
		node = rset->nodesetval->nodeTab[0];

		prop_ip = (char *) xmlGetProp(node,(xmlChar*)"ip");
		
		*msg = prop_ip;

		xmlFree(prop_server);
		xmlFree(expired);
		return Message::AUTH_REDIRECT;	
	}
	xmlFree(prop_server);


	
	time_t t = time(NULL);
	struct tm *date = localtime(&t);
	int exp_time = atoi(expired);
	int cur_time = (((date->tm_year)+1900)*10000)+(((date->tm_mon)+1)*100)+(date->tm_mday);

	
	if(cur_time-exp_time > 0 && exp_time != -1) {
		xmlFree(expired);
		return Message::ACCOUNT_EXPIRED;
	}
	
	if(exp_time-cur_time < 10 && exp_time != -1)
		return Message::NEAR_ACCOUNT_EXPIRED;


	xmlFree(expired);

	xmlXPathFreeObject(rset);
        free(_user);
        free(_passwd);
        free(expr);
					
	return Message::NULL_MESSAGE;
}

XMLUserDB::~XMLUserDB() {
	xmlXPathFreeContext(m_context);
	xmlFreeDoc(m_document);
}
