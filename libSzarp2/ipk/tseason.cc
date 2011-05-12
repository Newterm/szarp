/* 
  SZARP: SCADA software 

*/
/* 
 * IPK
 *
 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * IPK 'season' class implementation.
 *
 * $Id$
 */

#include "conversion.h"
#include "szarp_config.h"
#include "liblog.h"

int TSSeason::parseXML(xmlTextReaderPtr reader) {

//TODO: remove all printf
printf("name seasons: parseXML\n");

	const xmlChar *attr_name = NULL;
	const xmlChar *attr = NULL;
	const xmlChar *name = NULL;
	bool isEmpty = false;

#define IFNAME(N) if (xmlStrEqual( name , (unsigned char*) N ) )
#define NEEDATTR(ATT) attr = xmlTextReaderGetAttribute(reader, (unsigned char*) ATT); \
	if (attr == NULL) { \
		sz_log(1, "XML parsing error: expected '%s' (line %d)", ATT, xmlTextReaderGetParserLineNumber(reader)); \
		return 1; \
	}
//#define IFATTR(ATT) attr = xmlTextReaderGetAttribute(reader, (unsigned char*) ATT); if (attr != NULL)
#define IFATTR(ATT) if (xmlStrEqual(attr_name, (unsigned char*) ATT) )
#define DELATTR xmlFree(attr)
#define IFBEGINTAG if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT)
#define IFISEMPTYELEM if (xmlTextReaderIsEmptyElement(reader))
#define IFENDTAG if (xmlTextReaderNodeType(reader) == 15)
#define IFCOMMENT if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_COMMENT)
//TODO: check return value - 0 or 1 - in all files 
#define NEXTTAG if (xmlTextReaderRead(reader) != 1) \
	return 1; \
	goto begin_process_tseasons;
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

	IFISEMPTYELEM {
		isEmpty = true;
	}

	const char* need_attr[] = { "month_start", "day_start", "month_end", "day_end" };
	CHECKNEEDEDATTR(need_attr);

	FORALLATTR {
		GETATTR

		IFATTR("month_start") {
			if (sscanf((const char*) attr, "%d", &defs.month_start) != 1) {
				XMLERROR("Invalid start date of default summer season definition");
				return 1;
			}
		} else
		IFATTR("day_start") {
			if (sscanf((const char*) attr, "%d", &defs.day_start) != 1) {
				XMLERROR("Invalid start date of default summer season definition");
				return 1;
			}
		} else
		IFATTR("month_end") {
			if (sscanf((const char*) attr, "%d", &defs.month_end) != 1) {
				XMLERROR("Invalid end date of default summer season definition");
				return 1;
			}
		} else
		IFATTR("day_end") {
			if (sscanf((const char*) attr, "%d", &defs.day_end) != 1) {
				XMLERROR("Invalid end date of default summer season definition");
				return 1;
			}
		} else {
			printf("<seasons> not known attr: %s\n",attr_name);
//			assert(0 == 1 && "not known attr");
		}
	} // FORALLATTR

	if (isEmpty)
		return 0;

	NEXTTAG

begin_process_tseasons:

	name = xmlTextReaderConstLocalName(reader);
	IFNAME("#text") {
		NEXTTAG
	}
	IFCOMMENT {
		NEXTTAG
	}

	IFNAME("season") {
		IFBEGINTAG {

			int year;
			Season s;

			const char* need_attr_seasn[] = {"year", "month_start",  "month_end", "day_start", "day_end" };
			CHECKNEEDEDATTR(need_attr_seasn);

			FORALLATTR {
				GETATTR

				IFATTR("year") {
					if (sscanf((const char*) attr, "%d", &year) != 1) {
						XMLERROR("Invalid year definition");
						return 1;
					}
				} else
				IFATTR("month_start") {
					if (sscanf((const char*) attr, "%d", &s.month_start) != 1) {
						XMLERROR("Invalid start date of summer season definition");
						return 1;
					}
				} else
				IFATTR("day_start") {
					if (sscanf((const char*) attr, "%d", &s.day_start) != 1) {
						XMLERROR("Invalid start date of summer season definition");
					return 1;
					}
				} else
				IFATTR("month_end") {
					if (sscanf((const char*) attr, "%d", &s.month_end) != 1) {
						XMLERROR("Invalid end date of summer season definition");
						return 1;
					}
				} else
				IFATTR("day_end") {
					if (sscanf((const char*) attr, "%d", &s.day_end) != 1) {
						XMLERROR("Invalid end date of summer season definition");
						return 1;
					}
				} else {
					printf("not known attr=%s\n",attr_name);
					assert(0 == 1 && "not known attr");
				}
			} // FORALLATTR

			seasons[year] = s;

		}
		NEXTTAG
	} else
	IFNAME("seasons") {
	}
	else {
		printf("ERROR: not know name = %s\n",name);
		assert(name == NULL && "not know name");
	}

printf("name seasons parseXML END\n");

#undef IFNAME
#undef NEEDATTR
#undef IFATTR
#undef DELATTR
#undef IFBEGINTAG
#undef IFISEMPTYELEM
#undef IFENDTAG
#undef IFCOMMENT
#undef NEXTTAG
#undef XMLERROR
#undef FORALLATTR
#undef GETATTR
#undef CHECKNEEDEDATTR

	return 0;

}

int TSSeason::parseXML(xmlNodePtr node) {

	char *c = NULL;

#define NOATR(p, n) \
	{ \
	sz_log(1, "XML parsing error: attribute '%s' in node '%s' not\
 found (line %ld)", \
 			n, p->name, xmlGetLineNo(p)); \
			return 1; \
	}
#define NEEDATR(p, n) \
	if (c) free(c); \
	c = (char *)xmlGetProp(p, (xmlChar *)n); \
	if (!c) NOATR(p, n);
#define X (xmlChar*)

	NEEDATR(node, X"month_start");
	if (sscanf(c, "%d", &defs.month_start) != 1) {
		sz_log(1, "Invalid start date of default summer season definition (line: %ld)", 
			xmlGetLineNo(node));
		free(c);
		return 1;
	}

	NEEDATR(node, X"day_start");
	if (sscanf(c, "%d", &defs.day_start) != 1) {
		sz_log(1, "Invalid start date of default summer season definition (line: %ld)", 
			xmlGetLineNo(node));
		free(c);
		return 1;
	}

	NEEDATR(node, X"month_end");
	if (sscanf(c, "%d", &defs.month_end) != 1) {
		sz_log(1, "Invalid end date of default summer season definition (line: %ld)", 
			xmlGetLineNo(node));
		free(c);
		return 1;

	}

	NEEDATR(node, X"day_end");
	if (sscanf(c, "%d", &defs.day_end) != 1) {
		sz_log(1, "Invalid end date of default summer season definition (line: %ld)", 
			xmlGetLineNo(node));
		free(c);
		return 1;

	}

	for (node = node->children; node; node = node->next) {
		if (strcmp((char*)node->name, "season"))
			continue;

		int year;
		NEEDATR(node, X"year");
		if (sscanf(c, "%d", &year) != 1) {
			sz_log(1, "Invalid year definition (line: %ld)", 
				xmlGetLineNo(node));
			free(c);
			return 1;
		}

		Season s;
		NEEDATR(node, X"month_start");
		if (sscanf(c, "%d", &s.month_start) != 1) {
			sz_log(1, "Invalid start date of summer season definition (line: %ld)", 
				xmlGetLineNo(node));
			free(c);
			return 1;
		}

		NEEDATR(node, X"day_start");
		if (sscanf(c, "%d", &s.day_start) != 1) {
			sz_log(1, "Invalid start date of summer season definition (line: %ld)", 
				xmlGetLineNo(node));
			free(c);
			return 1;
		}

		NEEDATR(node, X"month_end");
		if (sscanf(c, "%d", &s.month_end) != 1) {
			sz_log(1, "Invalid end date of summer season definition (line: %ld)", 
				xmlGetLineNo(node));
			free(c);
			return 1;
		}

		NEEDATR(node, X"day_end");
		if (sscanf(c, "%d", &s.day_end) != 1) {
			sz_log(1, "Invalid end date of summer season definition (line: %ld)", 
				xmlGetLineNo(node));
			free(c);
			return 1;
		}
		free(c);
		c = NULL;

		seasons[year] = s;

	}

	return 0;

#undef X
#undef NEEDATR
#undef NOATR

}

xmlNodePtr TSSeason::generateXMLNode() const {
#define ITOA(x) snprintf(buffer, sizeof(buffer), "%d", x)
#define X (const xmlChar*) 

	char buffer[30];
	xmlNodePtr r = xmlNewNode(NULL, X"seasons");

	ITOA(defs.month_start);
	xmlSetProp(r, X"month_start", SC::A2U(buffer).c_str());

	ITOA(defs.day_start);
	xmlSetProp(r, X"day_start", SC::A2U(buffer).c_str());

	ITOA(defs.month_end);
	xmlSetProp(r, X"month_end", SC::A2U(buffer).c_str());

	ITOA(defs.day_end);
	xmlSetProp(r, X"day_end", SC::A2U(buffer).c_str());

	for (tSeasons::const_iterator i = seasons.begin();
			i != seasons.end();
			++i) {

		int year = i->first;
		const Season& s = i->second;

		xmlNodePtr n = xmlNewNode(NULL, X"season");

		ITOA(year);
		xmlSetProp(n, X"year", SC::A2U(buffer).c_str());

		ITOA(s.month_start);
		xmlSetProp(r, X"month_start", SC::A2U(buffer).c_str());

		ITOA(s.day_start);
		xmlSetProp(r, X"day_start", SC::A2U(buffer).c_str());

		ITOA(s.month_end);
		xmlSetProp(r, X"month_end", SC::A2U(buffer).c_str());

		ITOA(s.day_end);
		xmlSetProp(r, X"day_end", SC::A2U(buffer).c_str());

		xmlAddChild(r, n);
	}

	return r;

}

bool TSSeason::CheckSeason(const Season &s, int month, int day) const {
	bool result = false;

	if ((s.month_start < month || (s.month_start == month && s.day_start <= day))
		&& (s.month_end > month || (s.month_end == month && s.day_end > day)))
		result = true;

	return result;

}

const TSSeason::Season& TSSeason::GetSeason(int year) const {
	tSeasons::const_iterator i = seasons.find(year);

	if (i != seasons.end())
		return (*i).second;
	else
		return defs;
}

bool TSSeason::IsSummerSeason(int year, int month, int day) const {
	const Season& s = GetSeason(year);
	return CheckSeason(s, month, day);
}

bool TSSeason::IsSummerSeason(time_t time) const {
	struct tm *t;
#ifndef HAVE_LOCALTIME_R
	t = localtime(&time);
#else
	struct tm ptm;
	t = localtime_r(&time, &ptm);
#endif
	return IsSummerSeason(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
}

