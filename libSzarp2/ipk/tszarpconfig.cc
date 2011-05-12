/* 
  SZARP: SCADA software 

*/
/* 
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'params' (root element) class implementation.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>

#include <errno.h>
#include <ctype.h>
#include <string>
#include <dirent.h>
#include <assert.h>
#include <sstream>

#include <algorithm>

#ifdef MINGW32
#include "mingw32_missing.h"
#else
#include <libgen.h>
#endif

#include "szarp_config.h"
#include "parcook_cfg.h"
#include "line_cfg.h"
#include "ptt_act.h"
#include "sender_cfg.h"
#include "raporter.h"
#include "ekrncor.h"
#include "liblog.h"
#include "insort.h"
#include "conversion.h"
#include "definable_parser.h"

using namespace std;

#define FREE(x)	if (x != NULL) free(x)

/**
 * Swap bit first (from right, from 0) with second in data. */
void
swap_bits(unsigned long *data, int first, int second)
{
#define GET_BIT(d,i) (((d) & (1UL << (i))) && 1)
#define SET_BIT1(d,i) (d) |= (1UL << i)
#define SET_BIT0(d,i) (d) &= ~(1UL << i)
    int fb, sb;
    fb = GET_BIT(*data, first);
    sb = GET_BIT(*data, second);
    if (fb)
	SET_BIT1(*data, second);
    else
	SET_BIT0(*data, second);
    if (sb)
	SET_BIT1(*data, first);
    else
	SET_BIT0(*data, first);
   sz_log(9, "swap_bits(): data 0x%lx first %d (%d) second %d (%d)", *data, first, fb, second, sb);
#undef GET_BIT
#undef SET_BIT1
#undef SET_BIT0
}

/**
 * Data type for third argument of comp_draws and swap_draws functions.
 * @see comp_draws
 * @see swap_draws
 */
struct sort_draws_data_str {
    tDraw *draws;		/**< Table of draws. */
    unsigned long *id;		/**< Pointer to draws' window identifier. */
};
typedef struct sort_draws_data_str sort_draws_data_type;

/**
 * Function used to compare draws, @see insort.
 */
int
comp_draws(int a, int b, void *data)
{
    tDraw *draws = ((sort_draws_data_type *) data)->draws;

    if (draws[a].data >= 0) {
	if (draws[b].data >= 0) {
	    if (draws[a].data < draws[b].data)
		return -1;
	    if (draws[a].data > draws[b].data)
		return 1;
	    if (a < b)
		return -1;
	    if (a > b)
		return 1;
	    return 0;
	} else
	    return -1;
    } else if (draws[b].data >= 0)
	return 1;
    else {
	if (a < b)
	    return -1;
	if (a < b)
	    return 1;
    }
    return 0;
}

/**
 * Function used to swap draws, @see insort.
 */
void
swap_draws(int a, int b, void *data)
{
    tDraw tmp;
    tDraw *draws = ((sort_draws_data_type *) data)->draws;
    unsigned long *id = ((sort_draws_data_type *) data)->id;

    memcpy(&tmp, &draws[a], sizeof(tDraw));
    memcpy(&draws[a], &draws[b], sizeof(tDraw));
    memcpy(&draws[b], &tmp, sizeof(tDraw));

    swap_bits(id, a, b);
}



TSzarpConfig::~TSzarpConfig(void)
{
    delete devices;
    delete defined;
    delete drawdefinable;
    delete seasons;
}

const std::wstring&
TSzarpConfig::GetTitle()
{
    return title;
}

const std::wstring&
TSzarpConfig::GetDocumentationBaseURL()
{
    return documentation_base_url;
}

void TSzarpConfig::SetName(const std::wstring& title, const std::wstring& prefix)
{
    this->title=title;
    this->prefix=prefix;
}

const std::wstring& 
TSzarpConfig::GetPrefix() const
{
    return prefix;
}

void
TSzarpConfig::AddDefined(TParam * p)
{
    assert(p != NULL);
    if (defined == NULL) {
	defined = p;
    } else {
	defined->Append(p);
    }
}

void
TSzarpConfig::AddDrawDefinable(TParam * p)
{
    assert(p != NULL);
    if (drawdefinable == NULL) {
	drawdefinable = p;
    } else {
	drawdefinable->Append(p);
    }
}

void TSzarpConfig::RemoveDrawDefinable(TParam *p)
{
	if (drawdefinable == p) {
		drawdefinable = drawdefinable->GetNext(false);
		return;
	}


	TParam *pv = drawdefinable, *c = drawdefinable->GetNext(false); 
	while (c && c != p) {
		pv = c;
		c = c->GetNext(false);
	}

	if (c)
		pv->SetNext(c->GetNext(false));
}

TParam *
TSzarpConfig::getParamByIPC(int ipc_ind)
{
    TParam *p = GetFirstParam();
    if (p == NULL)
	return NULL;
    return p->GetNthParam(ipc_ind, true);
}

xmlDocPtr
TSzarpConfig::generateXML(void)
{
    using namespace SC;
#define X (unsigned char *)
#define ITOA(x) snprintf(buffer, 10, "%d", x)
#define BUF X(buffer)
    char buffer[10];
    xmlDocPtr doc;
    xmlNodePtr node;

    doc = xmlNewDoc(X "1.0");
    doc->children = xmlNewDocNode(doc, NULL, X "params", NULL);
    xmlNewNs(doc->children, SC::S2U(IPK_NAMESPACE_STRING).c_str(), NULL);
    xmlSetProp(doc->children, X "version", X "1.0");
    ITOA(read_freq);
    xmlSetProp(doc->children, X "read_freq", BUF);
    ITOA(send_freq);
    xmlSetProp(doc->children, X "send_freq", BUF);
    if (!title.empty())
	xmlSetProp(doc->children, X "title", S2U(title).c_str());
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
	xmlAddChild(doc->children, d->generateXMLNode());
    if (defined > 0) {
	node = xmlNewChild(doc->children, NULL, X "defined", NULL);
	for (TParam * p = defined; p; p = p->GetNext())
	    xmlAddChild(node, p->generateXMLNode());
    }
    if (drawdefinable) {
	node = xmlNewChild(doc->children, NULL, X "drawdefinable", NULL);
	for (TParam * p = drawdefinable; p; p = p->GetNext())
	    xmlAddChild(node, p->generateXMLNode());
    }
    if (boilers) {
	node = xmlNewChild(doc->children, NULL, X"boilers", NULL);
	for (TBoiler* boiler = GetFirstBoiler(); boiler; boiler = boiler->GetNext()) {
	    xmlNodePtr boiler_node = boiler->generateXMLNode();	
	    if (boiler_node)
		xmlAddChild(node,boiler_node);
        }
    }
    if (seasons) {
	node = seasons->generateXMLNode();
	xmlAddChild(doc->children, node);
    }
    return doc;
#undef X
#undef ITOA
#undef BUF
}

TBoiler* 
TSzarpConfig::GetFirstBoiler() {
	return boilers;
}	

int
TSzarpConfig::saveXML(const std::wstring &path)
{
    xmlDocPtr d;
    int r;

    d = generateXML();
    if (d == NULL)
	return -1;
    r = xmlSaveFormatFileEnc(SC::S2A(path).c_str(), d, "ISO-8859-2", 1);
    xmlFreeDoc(d);
    return r;
}

int
TSzarpConfig::loadXML(const std::wstring &path, const std::wstring &prefix)
{

    xmlDocPtr doc;
    int ret;

    this->prefix = prefix;

    xmlLineNumbersDefault(1);
    doc = xmlParseFile(SC::S2A(path).c_str());
    if (doc == NULL) {
	sz_log(1, "XML document not wellformed\n");
	return 1;
    }

    ret = parseXML(doc);
    xmlFreeDoc(doc);

    return ret;
}

int
TSzarpConfig::processNodeReader(xmlTextReaderPtr reader)
{
//TODO: remove all printf
//TODO: check return value in function

#define IFNAME(N) if (xmlStrEqual( name , (unsigned char*) N ) )
#define NEEDATTR(ATT) attr = xmlTextReaderGetAttribute(reader, (unsigned char*) ATT); \
	if (attr == NULL) { \
		sz_log(1, "XML parsing error: expected '%s' (line %d)", ATT, xmlTextReaderGetParserLineNumber(reader)); \
		return 1; \
	}
#define IFATTR(ATT) if (xmlStrEqual(attr_name, (unsigned char*) ATT) )
#define DELATTR xmlFree(attr)
#define IFBEGINTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
#define IFENDTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT)
#define IFCOMMENT if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_COMMENT)
//TODO: check return value - 0 or 1 - in all files 
#define NEXTTAG if (xmlTextReaderRead(reader) != 1) \
	return 1; \
	goto begin_process_node;
#define XMLERROR(STR) sz_log(1,"XML file error: %s (line,%d)", STR, xmlTextReaderGetParserLineNumber(reader));
#define XMLERRORATTR(ATT) sz_log(1,"XML parsing error: expected attribute '%s' (line: %d)", ATT, xmlTextReaderGetParserLineNumber(reader));
#define FORALLATTR for (int __atr = xmlTextReaderMoveToFirstAttribute(reader); __atr > 0; __atr =  xmlTextReaderMoveToNextAttribute(reader) )
#define GETATTR attr_name = xmlTextReaderConstLocalName(reader); attr = xmlTextReaderConstValue(reader);
#define CHECKNEEDEDATTR(LIST) \
	if (sizeof(LIST) > 0) { \
		std::set<std::string> __tmpattr(LIST, LIST + (sizeof(LIST) / sizeof(LIST[0]))); \
		FORALLATTR { GETATTR; __tmpattr.erase((const char*) attr_name); } \
		if (__tmpattr.size() > 0) { XMLERRORATTR(__tmpattr.begin()->c_str()); return 1; } \
	}

	int i = 0;
	TParam *p = NULL;
	TDevice *td = NULL;
	const xmlChar *attr_name = NULL;
	const xmlChar *attr = NULL;
	const xmlChar *name = NULL;

	assert(devices == NULL);
	assert(defined == NULL);

begin_process_node:

	name = xmlTextReaderConstName(reader);
//TODO:check return value
	if (name == NULL)
		return 1;

	IFNAME("#text") {
		NEXTTAG
	}
	IFCOMMENT {
		NEXTTAG
	}

//	xmlChar *value = xmlTextReaderValue(reader);

/*
	printf(">>>> new node <<<\n");
	printf("%d %d %s\n",
		xmlTextReaderDepth(reader),
		xmlTextReaderNodeType(reader),
		name);
*/


	IFNAME("params") {
		IFBEGINTAG {
			printf("-- name: params\n");

			const char* need_attr_params[] = { "read_freq" , "send_freq", "version" };
			CHECKNEEDEDATTR(need_attr_params);

			FORALLATTR {
				GETATTR

				IFATTR("read_freq") {
					if ((i = atoi((const char*)attr)) <= 0) {
						XMLERROR("read_freq attribute <= 0");
						return 0;
					}
					read_freq = i;
				} else
				IFATTR("send_freq") {
					if ((i = atoi((const char*)attr)) <= 0) {
						XMLERROR("send_freq attribute <= 0");
						return 0;
					}
					send_freq = i;
				} else
				IFATTR("version") {
					if (!xmlStrEqual(attr, (unsigned char*) "1.0")) {
						XMLERROR("incorrect version (1.0 expected)");
						return 0;
					}
				} else
				IFATTR("title") {
					title = SC::U2S(attr);
				} else
				IFATTR("documentation_base_url") {
					documentation_base_url = SC::U2S(attr);
				} else
				IFATTR("xmlns") {
				} else {
					printf("ERROR: not known attr: %s\n",attr_name);
//					assert(0 == 1 && "not known attr");
				}
			} // FORALLATTR

			NEXTTAG
		}
	} else
	IFNAME("device") {
		IFBEGINTAG {
			printf("-- name: device\n");

			if (devices == NULL)
				devices = td = new TDevice(this);
			else
				td = td->Append(new TDevice(this));
			assert(devices != NULL);
			td->parseXML(reader);
		}
		NEXTTAG
	} else
	IFNAME("defined") {
		IFBEGINTAG {
			printf("-- name: defined\n");
			TParam * _par = TDefined::parseXML(reader,this);
			if (_par)
				defined = _par;
		}
		NEXTTAG
	} else
	IFNAME("drawdefinable") {
		IFBEGINTAG {
			printf("-- name: drawdefinable\n");
			TParam * _par = TDrawdefinable::parseXML(reader,this);
			if (_par)
				drawdefinable = _par;
		}
		NEXTTAG
	} else
	IFNAME("seasons") {
		IFBEGINTAG {
			printf("-- name: seasons\n");
			seasons = new TSSeason();
			if (seasons->parseXML(reader)) {
				delete seasons;
				seasons = NULL;
//TODO: return error
				printf("FATAL ERROR <seasons>\n");
			}
		}
		NEXTTAG
	} else
	IFNAME("boilers") {
		IFBEGINTAG {
			TBoiler* boiler = TBoiler::parseXML(reader);
			if (!boiler) {
//TODO: return error
				printf("FATAL ERROR <boilers>\n");
			} else {
				boiler->SetParent(this);
				AddBoiler(boiler);
			}
		}
		NEXTTAG
	} else
	IFNAME("checker:rules") {
		// omit all tree
		xmlTextReaderNext(reader);
//TODO: make it without goto; goto - > while or better
		goto begin_process_node;
	} else {
		if (name)
			printf("ERROR<szarpconfig>: not known tag: %s\n",name);
		assert(name == NULL);
	}

#undef IFNAME
#undef NEEDATTR
#undef IFATTR
#undef DELATTR
#undef IFBEGINTAG
#undef IFENDTAG
#undef IFCOMMENT
#undef NEXTTAG
#undef XMLERROR
#undef FORALLATTR
#undef GETATTR
#undef CHECKNEEDEDATTR

// why? copy/paste from parse reader
	if (seasons == NULL)
		seasons = new TSSeason();

//	if (value) xmlFree(value);

	return 0;
}

int
TSzarpConfig::parseReader(const std::wstring &path)
{
//TODO: remove printf
printf("NEW PARSER\n");
	int ret = 0;
	xmlTextReaderPtr reader;

	reader = xmlNewTextReaderFilename(SC::S2A(path).c_str());
	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);
		while (ret == 1) {
			ret = processNodeReader(reader);
			if (ret != 1)
				break;
			ret = xmlTextReaderRead(reader);
		}
		xmlFreeTextReader(reader);
		if (ret != 0) {
			sz_log(1, "Failed to parse xml file\n");
		}
	} else
		sz_log(1,"Unable to open params.xml\n");

printf("END NEW PARSER\n");
	return ret;
}

int
TSzarpConfig::parseXML(xmlDocPtr doc)
{
    using namespace SC;

#define NEED(p, n) \
	if (!p) { \
	sz_log(1, "XML parsing error: expected '%s' (line %ld)", n, \
			xmlGetLineNo(p)); \
		return 1; \
	} \
	if (strcmp((const char*)p->name, n)) { \
	sz_log(1, "XML parsing error: expected '%s', found '%s' \
 (line %ld)", \
			n, U2A(p->name).c_str(), xmlGetLineNo(p)); \
		return 1; \
	}
#define NOATR(p, n) \
	{ \
	sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, U2A(p->name).c_str(), xmlGetLineNo(p)); \
			return 1; \
	}
#define NEEDATR(p, n) \
	if (c) free(c); \
	c = xmlGetProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (xmlChar *)

    unsigned char *c = NULL;
    int i;
    int ret = 1;
    xmlChar *_ps_addres, *_ps_port, *_documenation_base_url;
    TDevice *td = NULL;
    TParam *p = NULL;
    xmlNodePtr node, ch;

    node = doc->children;
    NEED(node, "params");
    NEEDATR(node, "version");

    unsigned char* _language = xmlGetProp(node, X "language");
    if (_language) {
	nativeLanguage = SC::U2S(_language);
	xmlFree(_language);
    } else
    	nativeLanguage = L"pl";
	 

    unsigned char* _title = xmlGetProp(node, X "title");
    title = U2S(_title);
    xmlFree(_title);

    if (strcmp((char*)c, "1.0")) {
sz_log(1, "XML parsing error: incorrect version (1.0 expected) (line %ld)", xmlGetLineNo(node));
	goto at_end;
    }

    _documenation_base_url = xmlGetProp(node, X "documentation_base_url");
    if (_documenation_base_url) {
	    documentation_base_url = U2S(_documenation_base_url);
	    xmlFree(_documenation_base_url);
    }

    _ps_addres =  xmlGetProp(node, X "ps_address");
    _ps_port = xmlGetProp(node, X "ps_port");
    if (_ps_addres)
	    ps_address =  U2S(_ps_addres);
    if (_ps_port)
	    ps_port = U2S(_ps_port);
    xmlFree(_ps_addres);
    xmlFree(_ps_port);

    NEEDATR(node, "read_freq");
    if ((i = atoi((char*)c)) <= 0) {
sz_log(1, "XML file error: read_freq attribute <= 0 (line %ld)", xmlGetLineNo(node));
	goto at_end;
    }
    read_freq = i;
    NEEDATR(node, "send_freq");
    if ((i = atoi((char*)c)) <= 0) {
sz_log(1, "XML file error: send_freq attribute <= 0 (line %ld)", xmlGetLineNo(node));
	goto at_end;
    }
    free(c);
    c = NULL;
    send_freq = i;

    assert(devices == NULL);
    assert(defined == NULL);

    for (i = 0, ch = node->children; ch; ch = ch->next) {
	if (!strcmp((char *) ch->name, "device")) {
	    if (devices == NULL)
		devices = td = new TDevice(this);
	    else
		td = td->Append(new TDevice(this));
	    assert(devices != NULL);
	    if (td->parseXML(ch))
		goto at_end;
	}
	else if (!strcmp((char *) ch->name, "defined")) {
	    for (xmlNodePtr ch2 = ch->children; ch2; ch2 = ch2->next)
		if (!strcmp((char *) ch2->name, "param")) {
		    if (defined == NULL) {
			defined = p = new TParam(NULL, this);
		    } 
		    else {
			p = p->Append(new TParam(NULL, this));
		    }
		    assert(p != NULL);
		    if (p->parseXML(ch2))
			goto at_end;
		}
	}
	else if (!strcmp((char *) ch->name, "drawdefinable")) {
	    for (xmlNodePtr ch2 = ch->children; ch2; ch2 = ch2->next)
		if (!strcmp((char *) ch2->name, "param")) {
		    if (drawdefinable == NULL) {
			drawdefinable = p = new TParam(NULL, this);
		    } else {
			p = p->Append(new TParam(NULL, this));
		    }
		    assert(p != NULL);
		    if (p->parseXML(ch2))
			goto at_end;
		}
	}
	else if (!strcmp((char *)ch->name, "boilers")) {
	    for (xmlNodePtr ch2 = ch->children; ch2; ch2 = ch2->next) 
		if (!strcmp((char *)ch2->name,"boiler")) {
		    TBoiler* boiler = TBoiler::parseXML(ch2);
			if (!boiler) 
			    goto at_end;
			boiler->SetParent(this);
			AddBoiler(boiler);

		}
	}
	else if (!strcmp((char *)ch->name, "seasons")) {
		seasons = new TSSeason();
		if (seasons->parseXML(ch)) {
			delete seasons;
			seasons = NULL;
			goto at_end;
		}
	}
    }
    if (devices == NULL) {
sz_log(1, "XML file error: 'device' elements not found");
	goto at_end;
    }

    if (seasons == NULL)
	    seasons = new TSSeason();

    ret = 0;
  at_end:
    return ret;
#undef X
}

int
TSzarpConfig::PrepareDrawDefinable()
{
    if (NULL == drawdefinable)
	return 0;

    TParam * p = drawdefinable;
    while (p) {
	if (p->IsDefinable())
		p->PrepareDefinable();
	p = p->GetNext();
    }

    return 0;
}

#ifndef MINGW32
int
TSzarpConfig::saveSzarpConfig(const std::wstring& directory, int force)
{
    saveParcookCfg(directory, force);
    saveLineCfg(directory, force);
    savePTT(directory, force);
    saveSenderCfg(directory, force);
    SaveRaporters(directory, force);
    return 0;
}
#endif

TParam *
TSzarpConfig::getParamByName(const std::wstring& name)
{
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	if (!p->GetName().empty() && !name.empty() && p->GetName() == name)
	    return p;

    for (TParam * p = drawdefinable; p; p = p->GetNext())
	if (!p->GetName().empty() && !name.empty() && p->GetName() == name)
	    return p;

    return NULL;
}

TParam *
TSzarpConfig::getParamByBaseInd(int base_ind)
{
    TParam *p;
    int max, i;

    for (p = GetFirstParam(); p; p = GetNextParam(p)) {
	if (p->GetBaseInd() == base_ind) {
	    return p;
	}
    }
    /* draw definables have base indexes starting from max */
    max = GetParamsCount() + GetDefinedCount();
    for (i = max, p = drawdefinable; p; p = p->GetNext(), i++) {
	if (i == base_ind) {
	    return p;
	}
    }
    return NULL;
}


#ifndef MINGW32
int
TSzarpConfig::saveParcookCfg(const std::wstring& directory, int force)
{
    FILE *f;
    string s;
    const char *c;
    struct stat tmp;

    using namespace SC;

    s += S2A(directory);
    s += "/parcook.cfg";
    c = s.c_str();
    if ((force == 0) && (stat(c, &tmp) == 0)) {
sz_log(1, "Cannot create file %s - file exists", c);
	return 0;
    }
    f = fopen(c, "w");
    if (f == NULL) {
sz_log(1, "Error while creating file '%s' (errno %d)", c, errno);
	return 0;
    }
    fprintf(f, "%d %d %d\n", GetDevicesCount(), read_freq, GetDefinedCount());
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d)) {
	fprintf(f, "%d %d", d->GetNum() + 1, d->GetParamsCount());
	if (!d->GetDaemon().empty())
	    fprintf(f, " %s", S2A(d->GetDaemon()).c_str());
	if (!d->GetPath().empty())
	    fprintf(f, " %s", S2A(d->GetPath()).c_str());
	if (d->GetSpeed() > 0)
	    fprintf(f, " %d", d->GetSpeed());
	if (d->GetStopBits() >= 0)
	    fprintf(f, " %d", d->GetStopBits());
	if (d->GetProtocol() >= 0)
	    fprintf(f, " %d", d->GetProtocol());
	if (!d->GetOptions().empty())
	    fprintf(f, " %s", S2A(d->GetOptions()).c_str());
	fprintf(f, "\n");
    }
    fprintf(f, "%d\n", GetAllDefinedCount());
    /* Print all params definitions */
    for (TParam * cp = GetFirstParam(); cp; cp = GetNextParam(cp)) {
	    const std::wstring& formula = cp->GetParcookFormula();
	    if (!formula.empty())
		    fprintf(f, "%s", S2A(formula).c_str());
    }
    fclose(f);

    return 1;
}
#endif

#ifndef MINGW32
int
TSzarpConfig::saveLineCfg(const std::wstring& directory, int force)
{
    int l = 0;
    FILE *f;
    struct stat tmp;
    const char *c;
    char buf[5];
    string w;

    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d)) {
	w = SC::S2A(directory);
	w += "/line";
	snprintf(buf, 5, "%d", d->GetNum() + 1);
	w += buf;
	w += ".cfg";
	c = w.c_str();
	if ((force == 0) && (stat(c, &tmp) == 0)) {
	   sz_log(1, "Cannot create file %s - file exists", c);
	    continue;
	}
	f = fopen(c, "w");
	if (f == NULL) {
	   sz_log(1, "Error while creating file '%s' (errno %d)", c, errno);
	    return 0;
	}
	if (d->GetFirstRadio()->GetId())
	    fprintf(f, "R%d\n", d->GetRadiosCount());
	for (TRadio * r = d->GetFirstRadio(); r; r = d->GetNextRadio(r)) {
	    if (r->GetId()) {
		std::wstringstream wss;
		wss << r->GetId();
		fprintf(f, "%s\n", SC::S2A(wss.str()).c_str());
	    }
	    if (d->IsSpecial())
		fprintf(f, "%d\n", d->GetSpecial());
	    else
		fprintf(f, "%d\n", r->GetUnitsCount());
	    for (TUnit * u = r->GetFirstUnit(); u; u = r->GetNextUnit(u)) {
		std::wstringstream wss;
		wss << u->GetId();
		fprintf(f, "%s %d %d %d %d %d\n",
			SC::S2A(wss.str()).c_str(), u->GetType(), u->GetSubType(), u->GetParamsCount(), u->GetSendParamsCount(), u->GetBufSize());
	    }
	}
	fclose(f);
	l++;
    }
    return l;
}
#endif

#ifndef MINGW32
int
TSzarpConfig::savePTT(const std::wstring& directory, int force)
{
    string s;
    const char *c;
    FILE *f;
    struct stat tmp;
    int m;
    TParam **tab = NULL;

    s += SC::S2A(directory);
    s += "/PTT.act";
    c = s.c_str();
    if ((force == 0) && (stat(c, &tmp) == 0)) {
sz_log(1, "Cannot create file %s - file exists", c);
	return 0;
    }
    f = fopen(c, "w");
    if (f == NULL) {
sz_log(1, "Error while creating file '%s' (errno %d)", c, errno);
	return 0;
    }

    m = GetMaxBaseInd();

    if (m >= 0) {

	tab = (TParam **) calloc(m + 1, sizeof(TParam *));
	for (TParam * p = GetFirstParam(); p; p = GetNextParam(p)) {
	    if (p->IsAutoBase()) {
	sz_log(1, "Param '%s' has auto index, do not try szarp2ipk, see docs.", SC::S2A(p->GetName()).c_str());
	    }
	    if ((p->GetBaseInd() < 0) || (p->GetBaseInd() > m))
		continue;
	    tab[p->GetBaseInd()] = p;
	}
    } else {
sz_log(1, "Base indexes not found, if you are using auto indexes do not try szarp2ipk, see docs.");
    }

    fprintf(f, "1 %d %d\n", m + 1, GetParamsCount() + GetDefinedCount());

    for (int i = 0; i <= m; i++) {
	if (tab[i] == NULL) {
	   sz_log(0, "Param with base index %d (starting from 0) missing", i);
	    return 0;
	}
	fprintf(f, "%d", tab[i]->GetIpcInd());
	if (tab[i]->GetFirstValue() == NULL) {
	    fprintf(f, " %d", tab[i]->GetPrec());
	} else {
	    assert(tab[i]->GetFirstValue() != NULL);
	    fprintf(f, " %d", tab[i]->GetFirstValue()->getSzarpId());
	}
	fprintf(f, " %s %s [%s];%s", SC::S2A(tab[i]->GetShortName()).c_str(),
		SC::S2A(tab[i]->GetName()).c_str(), 
		!tab[i]->GetUnit().empty() ? SC::S2A(tab[i]->GetUnit()).c_str() : "-", 
		!tab[i]->GetDrawName().empty() ? SC::S2A(tab[i]->GetDrawName()).c_str() : "-");
	if (tab[i]->GetShortName().length() > 5)
	   sz_log(1, "Short name '%s' for param '%s' to long (more then 5 characters)", SC::S2A(tab[i]->GetShortName()).c_str(), SC::S2A(tab[i]->GetName()).c_str());
	if (!tab[i]->GetDrawName().empty() && (tab[i]->GetDrawName().length() > 22))
	   sz_log(1, "Draw name '%s' for param '%s' to long (more then 22 characters)", SC::S2A(tab[i]->GetDrawName()).c_str(), SC::S2A(tab[i]->GetName()).c_str());
	fprintf(f, " # (%d, %d)\n", i / 15 + 1, i % 15);
    }
    if (tab != NULL)
	free(tab);

    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p)) {
	if (p->GetBaseInd() >= 0)
	    continue;
	if (p->GetFormulaType() == TParam::DEFINABLE)
	    continue;
	fprintf(f, "%d", p->GetIpcInd());
	if (p->GetFirstValue() == NULL) {
	    fprintf(f, " %d", p->GetPrec());
	} else {
	    fprintf(f, " %d", p->GetFirstValue()->getSzarpId());
	}
	fprintf(f, " %s %s [%s];%s\n", 
		SC::S2A(p->GetShortName()).c_str(), 
		SC::S2A(p->GetName()).c_str(), 
		!p->GetUnit().empty() ? SC::S2A(p->GetUnit()).c_str() : "-", 
		!p->GetDrawName().empty() ? SC::S2A(p->GetDrawName()).c_str() : "");
    }
    fclose(f);
    return 1;
}
#endif

#ifndef MINGW32
int
TSzarpConfig::saveSenderCfg(const std::wstring& directory, int force)
{
    string s;
    const char *c;
    int n, pc;
    TSendParam *sp;
    TParam *p;
    struct stat tmp;
    FILE *f;

    pc = 0;
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
	for (TRadio * r = d->GetFirstRadio(); r; r = d->GetNextRadio(r))
	    for (TUnit * u = r->GetFirstUnit(); u; u = r->GetNextUnit(u))
		for (sp = u->GetFirstSendParam(); sp; sp = u->GetNextSendParam(sp))
		    if (sp->IsConfigured())
			pc++;
    if (pc <= 0)
	return 0;

    s += SC::S2A(directory);
    s += "/sender.cfg";
    c = s.c_str();
    if ((force == 0) && (stat(c, &tmp) == 0)) {
sz_log(1, "Cannot create file %s - file exists", c);
	return 0;
    }
    f = fopen(c, "w");
    if (f == NULL) {
sz_log(1, "Error while creating file '%s' (errno %d)", c, errno);
	return 0;
    }

    fprintf(f, "%d %d\n", pc, send_freq);

    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
	for (TRadio * r = d->GetFirstRadio(); r; r = d->GetNextRadio(r))
	    for (TUnit * u = r->GetFirstUnit(); u; u = r->GetNextUnit(u))
		for (n = 0, sp = u->GetFirstSendParam(); sp; sp = u->GetNextSendParam(sp)) {
		    if (!sp->IsConfigured())
			continue;
		    if (!sp->GetParamName().empty()) {
			p = getParamByName(sp->GetParamName());
			if (p == NULL) {
			   sz_log(1, "Cannot find parameter to send (%s)", SC::S2A(sp->GetParamName()).c_str());
			    fclose(f);
			    return 0;
			}
			fprintf(f, "%d", p->GetIpcInd());
		    } else
			fprintf(f, "#%d", sp->GetValue());
		    std::wstringstream ss;
		    ss << u->GetId();
		    fprintf(f, " %d %s %d %d %d %d\n",
			    d->GetNum() + 1, SC::S2A(ss.str()).c_str(), n, sp->GetRepeatRate(), sp->GetProbeType(), sp->GetSendNoData());
		    n++;
		}

    fclose(f);
    return 1;
}
#endif

std::wstring
TSzarpConfig::absoluteName(const std::wstring &name, const std::wstring &ref)
{
    int i;
    std::wstring w1, w2;

    assert(!name.empty());
    assert(!ref.empty());
    w1 = name;
    w2 = ref;
    if (name.length() > 4 && name.substr(0, 4) == L"*:*:") {
	i = w2.rfind(':');
	w1.replace(0, 3, w2.substr(0, i));
    } else if (name.length() > 2 && name.substr(0, 2) == L"*:") {
	i = w2.find(':');
	w1.replace(0, 1, w2.substr(0, i));
    }
    return w1;
}

int
TSzarpConfig::AddDefined(const std::wstring &formula)
{
    TParam *p = new TParam(NULL, this, formula);

    assert(p != NULL);
    p->SetFormulaType(TParam::RPN);
    if (defined == NULL)
	defined = p;
    else
	defined->Append(p);
    return GetParamsCount() + GetDefinedCount() - 1;
}

int
TSzarpConfig::GetMaxBaseInd()
{
    int m = -1, n;
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p)) {
	n = p->GetBaseInd();
	if (n > m)
	    m = n;
    }
    return m;
}

TParam *
TSzarpConfig::GetFirstParam()
{
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
	for (TRadio * r = d->GetFirstRadio(); r; r = d->GetNextRadio(r))
	    for (TUnit * u = r->GetFirstUnit(); u; u = r->GetNextUnit(u)) {
		TParam *p = u->GetFirstParam();
		if (p)
		    return p;
	    }
    if (defined)
	return defined;
    if (drawdefinable)
	return drawdefinable;
    return NULL;
}

TParam *
TSzarpConfig::GetNextParam(TParam * p)
{
    return (NULL == p ? NULL : p->GetNext(true));
}

TDevice *
TSzarpConfig::AddDevice(TDevice * d)
{
    assert(d != NULL);
    if (devices == NULL)
	devices = d;
    else
	devices->Append(d);
    return d;
}

TDevice *
TSzarpConfig::GetNextDevice(TDevice * d)
{
    if (d == NULL)
	return NULL;
    return d->GetNext();
}

int
TSzarpConfig::GetParamsCount()
{
    int i = 0;
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
	i += d->GetParamsCount();
    return i;
}

int
TSzarpConfig::GetDevicesCount()
{
    int i = 0;
    for (TDevice * d = GetFirstDevice(); d; d = GetNextDevice(d))
	i++;
    return i;
}

int
TSzarpConfig::GetDefinedCount()
{
    int i = 0;
    for (TParam * p = GetFirstDefined(); p; p = p->GetNext())
	i++;
    return i;
}

int
TSzarpConfig::GetDrawDefinableCount()
{
    int i = 0;
    for (TParam * p = drawdefinable; p; p = p->GetNext())
	i++;
    return i;
}

int
TSzarpConfig::GetAllDefinedCount()
{
    int i = 0;
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	if ((!p->GetFormula().empty() || !p->GetParentUnit()) && (p->GetFormulaType() != TParam::DEFINABLE))
	    i++;
    return i;
}

TDevice *
TSzarpConfig::DeviceById(int id)
{
    TDevice *d;
    for (d = GetFirstDevice(); d && (id > 1); d = GetNextDevice(d), id--);
    return d;
}

std::wstring 
TSzarpConfig::GetFirstRaportTitle()
{
    std::wstring title;
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (title.empty() || r->GetTitle().compare(title) < 0)
		title = r->GetTitle();
    return title;
}

std::wstring
TSzarpConfig::GetNextRaportTitle(const std::wstring &cur)
{
    std::wstring title;
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (r->GetTitle().compare(cur) > 0 && (title.empty() || r->GetTitle().compare(title) < 0))
		title = r->GetTitle();
    return title;
}

TRaport *
TSzarpConfig::GetFirstRaportItem(const std::wstring& title)
{
    for (TParam * p = GetFirstParam(); p; p = GetNextParam(p))
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (title == r->GetTitle())
		return r;
    return NULL;
}

TRaport *
TSzarpConfig::GetNextRaportItem(TRaport * cur)
{
    std::wstring title = cur->GetTitle();
    for (TRaport * r = cur->GetNext(); r; r = r->GetNext())
	if (title == r->GetTitle())
	    return r;
    for (TParam * p = GetNextParam(cur->GetParam()); p; p = GetNextParam(p))
	for (TRaport * r = p->GetRaports(); r; r = r->GetNext())
	    if (title == r->GetTitle())
		return r;
    return NULL;
}

#ifndef MINGW32
int
TSzarpConfig::SaveRaporters(const std::wstring& directory, int force)
{
    string w;
    int i = 0;
    struct stat tmp;
    for (std::wstring title = GetFirstRaportTitle(); !title.empty(); title = GetNextRaportTitle(title)) {
	TRaport *rap = GetFirstRaportItem(title);
	w = SC::S2A(directory);
	w += "/";
	w += SC::S2A(rap->GetFileName());
	const char *c = w.c_str();
	if ((force == 0) && (stat(w.c_str(), &tmp) == 0)) {
	   sz_log(1, "Cannot create file %s - file exists", c);
	    continue;
	}
	FILE *f = fopen(c, "w");
	if (f == NULL) {
	   sz_log(1, "Error while creating file %s (errno %d)", c, errno);
	    continue;
	}
	fprintf(f, "#Raporter Pattern File#\n");
	int n = 0;
	for (TRaport * raptmp = rap; raptmp; raptmp = GetNextRaportItem(raptmp), n++);
	fprintf(f, "%d %s", n, SC::S2A(rap->GetTitle()).c_str());

	// Table for sorting lines
	double *orders = (double *) malloc(n * sizeof(double));
	// Table for storing lines
#define BUFSIZE 200
	char **strings = (char **) calloc(n, sizeof(char *));
	for (int j = 0; j < n; j++)
	    strings[j] = (char *) calloc(BUFSIZE, sizeof(char));

	for (int j = 0; rap; rap = GetNextRaportItem(rap), j++) {
	    TParam *p = rap->GetParam();
	    int prec;
	    int is_test = 0;
	    orders[j] = rap->GetOrder();
	    // special handling of test raports
	    if (rindex(c, '/') && (!strcasecmp(rindex(c, '/') + 1, "test.rap")))
		is_test = 1;
	    else if (!strcasecmp(c, "test.rap") || (!strcasecmp(SC::S2A(title).c_str(), "RAPORT TESTOWY")))
		is_test = 1;
	    if (p->GetFirstValue()) {
		prec = p->GetFirstValue()->getSzarpId();
	    } else
		prec = p->GetPrec();
	    if (is_test) {
		int num = 0;
		if (p->GetDevice())
		    num = p->GetDevice()->GetNum() + 1;
		snprintf(strings[j], BUFSIZE, "\n%d 5 S%d %s [-]", p->GetIpcInd(), num, SC::S2A(rap->GetDescr()).c_str());
	    } else {
		snprintf(strings[j], BUFSIZE,
			 "\n%d  %d  %s  %s [%s]", p->GetIpcInd(), prec, SC::S2A(p->GetShortName()).c_str(), SC::S2A(rap->GetDescr()).c_str(), SC::S2A(p->GetUnit()).c_str());
	    }
	}
#undef BUFSIZE
	// Print sorted
	double last = -1.0;
	while (1) {
	    int lasti = -1;
	    // Find smallest greater then last
	    for (int j = 0; j < n; j++)
		if ((orders[j] >= 0.0) && (orders[j] > last) && ((lasti < 0) || (orders[j] < orders[lasti])))
		    lasti = j;
	    // If not found, break from loop
	    if ((lasti < 0) || (orders[lasti] == last) || (orders[lasti] == -1.0))
		break;
	    // Currently found
	    last = orders[lasti];
	    // Print all matching
	    for (int j = 0; j < n; j++)
		if (orders[j] == last)
		    fprintf(f, "%s", strings[j]);
	}
	// Print all without priorities
	for (int j = 0; j < n; j++)
	    if (orders[j] < 0)
		fprintf(f, "%ss", strings[j]);
	fclose(f);
	free(orders);
	for (int j = 0; j < n; j++)
	    free(strings[j]);
	free(strings);
	i++;
    }
    return i;
}
#endif

TBoiler* TSzarpConfig::AddBoiler(TBoiler *boiler) {
	if (!boilers)
		return boilers = boiler;
	else
		return boilers->Append(boiler);
}

int TSzarpConfig::checkRepetitions(int quiet)
{
	std::vector<std::wstring> str;
        int all_repetitions_number = 0, the_same_repetitions_number=0;
        
	for (TParam* p = GetFirstParam(); p; p = GetNextParam(p)) {
		str.push_back(p->GetSzbaseName());
	}
	std::sort(str.begin(), str.end());
	
	for(size_t j = 0 ; j < str.size() - 1; j++){
		if (!str[j].compare(str[j + 1])){ 
			the_same_repetitions_number += 1;
			all_repetitions_number += 1;
		}
		
		else if ( j > 0 && !str[j].compare(str[j - 1])) {
			
			the_same_repetitions_number += 1;
			all_repetitions_number += 1;
			
			if (!quiet)
				sz_log(1, "There is %d repetitions of: %s", the_same_repetitions_number, SC::S2A(str[j-1]).c_str());
			
			the_same_repetitions_number = 0;
		}
	}
	
	if (all_repetitions_number > 0) {
		return 1;
	} else {
		return 0;
	}
}				

void TSzarpConfig::ConfigureSeasonsLimits() {
	if (seasons == NULL)
		seasons = new TSSeason();
}

const TSSeason* TSzarpConfig::GetSeasons() const {
	return seasons;
}
