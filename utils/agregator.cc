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
/*
 * $Id$

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * Program for merging few IPK configuration into one, suitable for viewing in
 * SzarpDraw program as a single one. This makes sense only for szarpbase
 * configuration.
 *
 * Program is run with one argument - path to config file. Config file is XML
 * like this:
 *

<?xml version="1.0" encoding="ISO-8859-2"?>

<aggregate xmlns="http://www.praterm.com.pl/IPK/aggregate"
                xmlns:ipk="http://www.praterm.com.pl/IPK/params"
                prefix="zamX"
                title="Zamo¶æ">

        <config prefix="zamo">
		<remove xpath="//ipk:raport"/>
	</config>

        <config prefix="zmk1">
                <regexp xpath="//ipk:draw/@title">s/.+/KM-1\0/</regexp>
		<remove xpath="//ipk:raport"/>
		<attribute xpath="//ipk:draw" name="prior" value="100"/>
        </config>
</aggregate>


 * ... and many more ...
 * See docs for detailed explanation.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <getopt.h>

#include <sys/stat.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <tr1/unordered_map>

#include <openssl/evp.h>

using namespace std::tr1;

#include "liblog.h"
#include "szarp_config.h"
#include "szbase/szbbase.h"
#include "xmlutils.h"
#include "conversion.h"
#include "custom_assert.h"

#ifdef LIBXML_XPATH_ENABLED

#define AGGREGATE_HREF "http://www.praterm.com.pl/IPK/aggregate"


/* Makes regular expresion substitution.
 * @param content source string
 * @param substitute string
 * @param regs info about sub-expresions to substitute
 * @return result string (allocated with malloc) */
char *regsub(const char *content, char *subst, re_registers* regs)
{
	ASSERT(content != NULL);
	ASSERT(subst != NULL);

	char *ret;
	char *c;
	size_t retlen;
	size_t retsize;
	size_t len, n;

	int match_start;
	ASSERT(regs);
	ASSERT(regs->num_regs > 0);
	match_start = regs->start[0];

	retlen = match_start;
	retsize = 100 + match_start;
	ret = (char *) malloc (retsize);
	ASSERT(ret != NULL);
	/* copy part before matched string */
	strncpy(ret, content, match_start);

	c = subst;
	int substsize = strlen(subst);
	while (c - subst < substsize) {
		if (retlen >= (retsize - 1)) {
			retsize *= 2;
			ret = (char*) realloc(ret, retsize);
			ASSERT(ret != NULL);
		}
		if (*c != '\\') {
			ret[retlen++] = *c++;
			continue;
		}
		c++;
		if (!isdigit(*c)) {
			ret[retlen++] = *c++;
			continue;
		}
		n = *c++ - '0';
		if (n >= regs->num_regs) {
			free(ret);
			return NULL;
		}
		len = regs->end[n] - regs->start[n];

		if (retlen + len >= (retsize -1)) {
			retsize = retsize + 2 + len;
			ret = (char*) realloc(ret, retsize);
			ASSERT(ret != NULL);
		}
		strncpy(ret + retlen, content + regs->start[n], len);
		retlen += len;
	}
	int tocopy = strlen(content) - regs->end[0];
	if (tocopy > 0) {
		/* copy part after matched string */
		if ((retlen + tocopy) >= retsize) {
			retsize = retlen + tocopy + 1;
			ret = (char*) realloc(ret, retsize);
			ASSERT(ret != NULL);
		}
		strncpy(ret + retlen, content + regs->end[0], tocopy);
		retlen += tocopy;
	}
	ret[retlen] = 0;
	return ret;
}


/** Parse arguments and prints usage info.
 * @return 0 if arguments are OK, otherwise 1 */
int usage(int argc, char **argv, bool& verbose, bool& force)
{
	const char * helpmessage = "\
Usage: agregate <config file>\n\
\t-h, --help - print this help\n\
\t-f, --force - force overriding old output file\n\
\t-v, --verbose - make output verbose\n\
\n\
Makes one big configuration...\n";

	struct option long_options[] = {
		{"help", 0, NULL, 'n'},
		{"force", 0, NULL, 'f'},
		{"verbose", 0, NULL, 'v'},
		{0, 0, 0, 0}
	};

	int c;
	int option_index;

	verbose = false;
	force = false;

	while(((c = getopt_long(argc, argv, "hvf",long_options, &option_index)) >= 0)) {
		switch (c) {
			case 'h':
					printf(helpmessage);
					return 1;
				break;
			case 'v':
					verbose = true;
				break;
			case 'f':
					force = true;
				break;
			default:
				printf(helpmessage);
					return 1;
				break;
		}
	}
	if (argc - optind == 1)
		return 0;
	printf(helpmessage);
	return 1;
}

/** Add attribute to elements selected by XPath expresion.
 * @param doc XML document
 * @param xpath_expr XPath expresion
 * @param name name of attribute
 * @param value value of attribute
 */
int add_attribute(xmlDocPtr doc, xmlChar *xpath_expr,
		xmlChar *name, xmlChar *value)
{
	int ret;
	xmlXPathContextPtr xpath_ctx;
	xmlXPathObjectPtr xpath_obj;

	ASSERT(xpath_expr != NULL);
	ASSERT(doc != NULL);
	ASSERT(name != NULL);
	ASSERT(value != NULL);

	/* create new xpath context */
	xpath_ctx = xmlXPathNewContext(doc);
	ASSERT(xpath_ctx != NULL);

	/* register IPK namespace */
	ret = xmlXPathRegisterNs(xpath_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);

	/* evaluate expresion */
	xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		sz_log(0, "Error evaluating expresion '%s'",
				SC::U2A(xpath_expr).c_str());
		return 1;
	}

	/* process result nodes */
	if (xpath_obj->nodesetval && (xpath_obj->nodesetval->nodeNr > 0)) {
		for (int i = 0; i < xpath_obj->nodesetval->nodeNr; i++) {
			xmlNodePtr node = xpath_obj->nodesetval->nodeTab[i];
			xmlSetProp(node, name, value);

			/* from xpath2.c */
			if (xpath_obj->nodesetval->nodeTab[i]->type !=
					XML_NAMESPACE_DECL)
				xpath_obj->nodesetval->nodeTab[i] = NULL;
		}
	}

	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeContext(xpath_ctx);
	return 0;
}

/** Copy attribute to elements selected by XPath expresion.
 * @param doc XML document
 * @param xpath_expr XPath expresion
 * @param name name of attribute
 * @param copy path to attribute to copy, relative to node selected by
 * xpath_expr.
 */
int copy_attribute(xmlDocPtr doc, xmlChar *xpath_expr,
		xmlChar *name, xmlChar *copy)
{
	int ret;
	xmlXPathContextPtr xpath_ctx;
	xmlXPathObjectPtr xpath_obj;

	ASSERT(xpath_expr != NULL);
	ASSERT(doc != NULL);
	ASSERT(name != NULL);
	ASSERT(copy != NULL);

	/* create new xpath context */
	xpath_ctx = xmlXPathNewContext(doc);
	ASSERT(xpath_ctx != NULL);

	/* register IPK namespace */
	ret = xmlXPathRegisterNs(xpath_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);

	/* evaluate expresion */
	xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		sz_log(0, "Error evaluating expresion '%s'",
				SC::U2A(xpath_expr).c_str());
		return 1;
	}

	/* process result nodes */
	if (xpath_obj->nodesetval && (xpath_obj->nodesetval->nodeNr > 0)) {
		for (int i = 0; i < xpath_obj->nodesetval->nodeNr; i++) {
			xmlNodePtr node = xpath_obj->nodesetval->nodeTab[i];
			/* set current node to selected node */
			xpath_ctx->node = node;
			/* evaluate copy expresion */
			xmlXPathObjectPtr xpath_cobj = xmlXPathEvalExpression(copy,
					xpath_ctx);
			if (xpath_cobj == NULL) {
				sz_log(0, "Error evaluating expresion '%s'",
						SC::U2A(copy).c_str());
				return 1;
			}
			if (!xpath_cobj || (xpath_cobj->nodesetval->nodeNr != 1)) {
				sz_log(0, "Error, one object specified by expresion '%s' should exists",
						SC::U2A(copy).c_str());
				return 1;
			}
			xmlNodePtr cnode = xpath_cobj->nodesetval->nodeTab[0];
			if (cnode->type != XML_ATTRIBUTE_NODE) {
				sz_log(0, "Error, object specified by expresion '%s' must be an attribute",
						SC::U2A(copy).c_str());
				return 1;
			}
			if (!cnode->children || (cnode->children->type != XML_TEXT_NODE) ||
					!cnode->children->content) {
				sz_log(0, "Error, incorrect element pointed to by expression '%s'",
						SC::U2A(copy).c_str());
				return 1;
			}
			xmlSetProp(node, name, cnode->children->content);


			/* from xpath2.c */
			if (xpath_obj->nodesetval->nodeTab[i]->type !=
					XML_NAMESPACE_DECL)
				xpath_obj->nodesetval->nodeTab[i] = NULL;
			if (xpath_cobj->nodesetval->nodeTab[0]->type !=
					XML_NAMESPACE_DECL)
				xpath_cobj->nodesetval->nodeTab[0] = NULL;
			xmlXPathFreeObject(xpath_cobj);
		}
	}

	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeContext(xpath_ctx);
	return 0;
}

/** Remove elements selected by XPath expresion from document.
 * @param doc XML document
 * @param xpath_expr XPath expresion
 * @return 0 on success, 1 on error
 */
int remove_xpath(xmlDocPtr doc, xmlChar *xpath_expr)
{
	int ret;
	xmlXPathContextPtr xpath_ctx;
	xmlXPathObjectPtr xpath_obj;

	ASSERT(doc != NULL);
	ASSERT(xpath_expr != NULL);

	/* create new xpath context */
	xpath_ctx = xmlXPathNewContext(doc);
	ASSERT(xpath_ctx != NULL);

	/* register IPK namespace */
	ret = xmlXPathRegisterNs(xpath_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);

	/* evaluate expresion */
	xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		sz_log(0, "Error evaluating expresion '%s'",
				SC::U2A(xpath_expr).c_str());
		return 1;
	}

	/* process result nodes in reverse order */
	if (xpath_obj->nodesetval && (xpath_obj->nodesetval->nodeNr > 0)) {
		for (int i = xpath_obj->nodesetval->nodeNr - 1; i >= 0; i--) {
			xmlNodePtr node = xpath_obj->nodesetval->nodeTab[i];
			xmlUnlinkNode(node);
			if (xpath_obj->nodesetval->nodeTab[i]->type !=
					XML_NAMESPACE_DECL) {
				xmlFreeNode(node);
				xpath_obj->nodesetval->nodeTab[i] = NULL;
			}
		}
	}

	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeContext(xpath_ctx);
	return 0;
}

/**
 * Compiles substitute expresion.
 * @param expresion substitute expresion ('s#pattern#subst#')
 * @param match return argument - matched subexpresions, free it after use
 * @param pattern return argument - copy of pattern (used for matching
 * subexpresions, free it after use
 * @param subst - copy of substitute expresion, free it after use
 * @return 0 on success, 1 on error
 */
int compile_subst(xmlChar* expresion, re_pattern_buffer* preg,
		char **pattern, char **subst)
{
	char *regx = strdup(SC::U2A(expresion).c_str());
	if (strlen(regx) < 4) {
		sz_log(0, "Error parsing substitute expresion '%s', expresion to short",
				regx);
		return 1;
	}
	if (regx[0] != 's') {
		sz_log(0, "Error parsing substitute expresion '%s', first character should be 's'",
				regx);
		return 1;
	}
	char delim;
	delim = regx[1];
	char *c = index(regx + 2, delim);
	if (c == NULL) {
		sz_log(0, "Error parsing substitute expresion '%s', delimiter character '%c' found only once",
				regx, delim);
		return 1;
	}
	*c = 0;
	*pattern = strdup(regx + 2);
	char *c2 = index(c + 1, delim);
	if (c2 == NULL) {
		sz_log(0, "Error parsing substitute expresion '%s', delimiter character '%c' found only twice",
				regx, delim);
		return 1;
	}
	*c2 = 0;
	c2++;
	if (*c2 != 0 ) {
		sz_log(0, "Error parsing substitute expresion '%s', trailing characters",
				regx);
		return 1;

	}
	*subst = strdup(c + 1);
	free(regx);

	preg->translate = 0;
	preg->fastmap = 0;
	preg->buffer = 0;
	preg->allocated = 0;
	const char *err = 0;
	if ((err = re_compile_pattern(*pattern, strlen(*pattern), preg)) != 0) {
		sz_log(0, "Error compiling regular expresion '%s': %s",
				*pattern, err);
		return 1;
	}
	return 0;
}


/** Process selected nodes of document with regular expresion.
 * @param doc document to process
 * @param xpath_expr xpath expresion to select nodes
 * @param regexp regular expresion
 * @return 0 on success, 1 on error */
int process_regexp(xmlDocPtr doc, xmlChar *xpath_expr, xmlChar *regexp)
{
	int ret;
	xmlXPathContextPtr xpath_ctx;
	xmlXPathObjectPtr xpath_obj;
	regex_t preg;

	ASSERT(doc != NULL);
	ASSERT(xpath_expr != NULL);
	ASSERT(regexp != NULL);

	/* create new xpath context */
	xpath_ctx = xmlXPathNewContext(doc);
	ASSERT(xpath_ctx != NULL);

	/* register IPK namespace */
	ret = xmlXPathRegisterNs(xpath_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);

	/* evaluate expresion */
	xpath_obj = xmlXPathEvalExpression(xpath_expr, xpath_ctx);
	if (xpath_obj == NULL) {
		sz_log(0, "Error evaluating expresion '%s'",
				SC::U2A(xpath_expr).c_str());
		return 1;
	}

	/* compile regular expresion */
	char *pattern;
	char *subst;

	if (compile_subst(regexp, &preg, &pattern, &subst)) {
		return 1;
	}

	re_registers regs;

	/* process result nodes */
	if (xpath_obj->nodesetval && (xpath_obj->nodesetval->nodeNr > 0)) {
		for (int i = 0; i < xpath_obj->nodesetval->nodeNr; i++) {
			xmlNodePtr node = xpath_obj->nodesetval->nodeTab[i];
			if (node->children == NULL)
				continue;
			if (node->children->content == NULL)
				continue;
			//char *content = strdup((char *)node->children->content);
			xmlChar *u_content = xmlNodeGetContent(node);
			std::string content(SC::U2L(u_content));
			if (re_search(&preg, content.c_str(),
						content.size(),
						0, content.size() - 1,
						&regs) < 0) {
				xmlFree(u_content);
				continue;
			}
			char *nc = regsub(content.c_str(), subst, &regs);
			xmlNodeSetContent(node, SC::L2U(nc, true).c_str());

			xmlFree(u_content);
			free(nc);
			/* from xpath2.c */
			if (xpath_obj->nodesetval->nodeTab[i]->type !=
					XML_NAMESPACE_DECL)
				xpath_obj->nodesetval->nodeTab[i] = NULL;
		}
	}

	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeContext(xpath_ctx);
	free(pattern);
	free(subst);
	regfree(&preg);

	return 0;
}


/** Makes link in szarpbase directory.
 * @param prefix prefix of 'from' param
 * @param old name of 'from' param
 * @param result_prefix name of 'from' param
 * @param name prefix of 'from' param
 * @param dir 1 if we should make link for upper level directory, 0 if for
 * deeper directory (Status vi Status/Execute/bodas_error).
 * @return 0 on success, 1 on error
 */
int make_link(xmlChar *prefix, xmlChar *old, xmlChar *name, xmlChar *result_prefix,
		int dir = 1)
{
	std::wstring from, to;
	std::wstring fromname, toname;

	ASSERT(prefix != NULL);
	ASSERT(old != NULL);
	ASSERT(name != NULL);
	ASSERT(result_prefix != NULL);

	fromname = SC::U2S(old);
	toname = SC::U2S(name);
	if (dir) {
		size_t i;
		i = fromname.find(L':');
		if (i != std::wstring::npos)
			fromname = fromname.substr(0, i);
		i = toname.find(L':');
		if (i != std::wstring::npos)
			toname = toname.substr(0, i);
	}

	fromname = wchar2szb(fromname);
	toname = wchar2szb(toname);

	from = std::wstring(L"/opt/szarp/") + SC::U2S(prefix) + L"/szbase/" + fromname;
	to = std::wstring(L"/opt/szarp/") + SC::U2S(result_prefix) + L"/szbase/" + toname;

	if (szb_cc_parent(from)) {
		sz_log(0, "Error creating parent dir for '%ls', errno %d (%s)", from.c_str(), errno, strerror(errno));
		return 1;
	}

	if (szb_cc_parent(to)) {
		sz_log(0, "Error creating parent dir for '%ls', errno %d (%s)", to.c_str(), errno, strerror(errno));
		return 1;
	}

	if (unlink(SC::S2A(to).c_str()) && (errno != ENOENT)) {
		sz_log(0, "Error removing '%ls', errno %d (%s)", to.c_str(), errno, strerror(errno));
		return 1;
	}
	if (symlink(SC::S2A(from).c_str(), SC::S2A(to).c_str())) {
		sz_log(0, "Error creating link from '%ls' to '%ls', errno %d (%s)",
				from.c_str(), to.c_str(), errno, strerror(errno));
		return 1;
	}

	return 0;
}

/**
 * Add configuration prefix to the name of parameter and make appropriate link
 * for szarpbase data.
 * @param node XML node with parameter description
 * @param prefix configuration prefix, may be NULL
 * @param result_prefix prefix of output configuration
 * @return 0 on success, 1 on error
 */
int process_param(xmlNodePtr node, xmlChar *prefix, xmlChar *result_prefix, xmlChar *source_prefix)
{
	xmlChar *name;
	xmlChar *old;
	xmlChar *base_ind;

	ASSERT(node != NULL);

	old = xmlGetProp(node, BAD_CAST "name");
	ASSERT(old != NULL);

	base_ind = xmlGetProp(node, BAD_CAST "base_ind");
	if (prefix != NULL) {
		asprintf((char **)&name, "{%s} %s", BAD_CAST prefix,
				BAD_CAST old);
		ASSERT(name != NULL);
		xmlSetProp(node, BAD_CAST "name", name);

		if (base_ind && make_link(source_prefix, old, name,
					result_prefix))
			return 1;

		free(name);
	} else {
		if (base_ind && make_link(source_prefix, old, old,
					result_prefix))
			return 1;
	}
	xmlFree(old);
	if (base_ind)
		xmlFree(base_ind);
	return 0;
}

/** Adds prefix to all parameters in formula.
 * @return new modified formula or NULL if no modifications are made */
xmlChar *edit_formula(xmlChar *formula, xmlChar *prefix)
{
	char *ret = strdup((char*)formula);
	char *c, *c2, *c3, *c4, *c5, *c6;
	int l1, l2;
	int mod = 0;

	for (c = strchr((char *) ret, '('); c != NULL; c = strchr( c, '(')) {
		c++;
		if (*c == '*')
			continue;
		c2 = strchr(c, ')');
		if (c2 == NULL)
			return NULL;
		l1 = c2 - c;
		c3 = strndup(c, l1);
		l2 = c - ret - 1;
		c4 = strndup(ret, l2);
		if (*(c2+1) != 0) {
			c5 = strdup(c2+1);
		} else {
			c5 = NULL;
		}
		/*

		      c4           c3            c5
		   [........]([...........])[...........]
		   |         |             |
		   +ret      +c            +c2

		 */

		if (c5) {
			asprintf(&c6, "%s({%s} %s)%s", c4, (char *) prefix, c3, c5);
			free(c5);
		} else {
			asprintf(&c6, "%s({%s} %s)", c4, (char *) prefix, c3);
		}
		free(ret);
		ret = c6;
		free(c3);
		free(c4);
		c = ret + (l1 + l2 + 2);
		mod = 1;
	}
	if (mod) {
		return BAD_CAST ret;
	} else {
		free(ret);
		return NULL;
	}
}

/**
 * Modify formulas - add prefix to param names.
 * @param doc XML document
 * @param prefix configuration prefix
 * @return 0 on success, 1 on error
 */
int process_formulas(xmlDocPtr doc, xmlChar *prefix)
{
	xmlXPathContextPtr xpath_ctx;
	xmlXPathObjectPtr xpath_obj;
	int ret;

	ASSERT(doc != NULL);
	if (prefix == NULL)
		return 0;

	xpath_ctx = xmlXPathNewContext(doc);
	ASSERT(xpath_ctx != NULL);
	ret = xmlXPathRegisterNs(xpath_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);

	xpath_obj = xmlXPathEvalExpression(
			BAD_CAST "//ipk:param/ipk:define[@type='RPN']/@formula | //ipk:param/ipk:define[@type='DRAWDEFINABLE']/@formula",
			xpath_ctx);
	if (xpath_obj->nodesetval && (xpath_obj->nodesetval->nodeNr > 0)) {
		for (int i = 0;  i < xpath_obj->nodesetval->nodeNr; i++) {
			xmlNodePtr node = xpath_obj->nodesetval->nodeTab[i];

			xmlChar *form = edit_formula(node->children->content, prefix);
			if (form != NULL) {
				xmlNodeSetContent(node->children, form);
				xmlFree(form);
			}
		}
	}

	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeContext(xpath_ctx);
	return 0;
}

/** Add params from included configuration to output document.
 * @param out output (result) document
 * @param cfg included configuration
 * @param prefix prefix for included parameters, should be NULL for main
 * configuration
 * @param result_prefix prefix of output configuration
 * @return 0 on success, 1 on error
 */
int process_config(xmlDocPtr out, xmlDocPtr cfg, xmlChar *prefix, xmlChar *result_prefix,
		xmlChar *source_prefix)
{
	int ret;
	xmlXPathContextPtr xpath_ctx;
	xmlXPathContextPtr xpath_ctx_out;
	xmlXPathObjectPtr xpath_obj;
	xmlXPathObjectPtr insert_point;

	ASSERT(out != NULL);
	ASSERT(cfg != NULL);

	/* modify formulas according to prefix */
	if (process_formulas(cfg, prefix))
		return 1;

	/* create new xpath contexts */
	xpath_ctx = xmlXPathNewContext(cfg);
	ASSERT(xpath_ctx != NULL);
	xpath_ctx_out = xmlXPathNewContext(out);
	ASSERT(xpath_ctx_out != NULL);

	/* register IPK namespace */
	ret = xmlXPathRegisterNs(xpath_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);
	ret = xmlXPathRegisterNs(xpath_ctx_out, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);

	/* get device and defined parameters */
	xpath_obj = xmlXPathEvalExpression(BAD_CAST "//ipk:device//ipk:param | //ipk:defined//ipk:param",
			xpath_ctx);
	ASSERT(xpath_obj != NULL);

	insert_point = xmlXPathEvalExpression(BAD_CAST "//ipk:defined", xpath_ctx_out );
	ASSERT(insert_point != NULL);
	ASSERT(insert_point->nodesetval);
	ASSERT(insert_point->nodesetval->nodeNr == 1);

	/* copy nodes into result document */
	if (xpath_obj->nodesetval && (xpath_obj->nodesetval->nodeNr > 0)) {
		xmlNodePtr ins_parent = insert_point->nodesetval->nodeTab[0];
		for (int i = 0;  i < xpath_obj->nodesetval->nodeNr; i++) {
			xmlNodePtr node = xmlDocCopyNode(
					xpath_obj->nodesetval->nodeTab[i],
					out, 1);
			//xmlUnlinkNode(node);
			ASSERT(node != NULL);
			node = xmlAddChild(ins_parent, node);
			ASSERT(node != NULL);
			if (process_param(node, prefix, result_prefix, source_prefix))
				return 1;

			if ((prefix == NULL) && (i == 0)) {
				/* first param in config is used to set date
				 * range for drawdefinable params */
				xmlChar *c = xmlGetProp(node, BAD_CAST "name");
				if (make_link(source_prefix, c,
						BAD_CAST "fake:fake:fake",
						result_prefix, 0))
					return 1;
				xmlFree(c);
			}
		}
	}
	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeObject(insert_point);

	/* get drawdefinable parameters */
	xpath_obj = xmlXPathEvalExpression(BAD_CAST "//ipk:drawdefinable//ipk:param", xpath_ctx);
	ASSERT(xpath_obj != NULL);

	insert_point = xmlXPathEvalExpression(BAD_CAST "//ipk:drawdefinable", xpath_ctx_out );
	ASSERT(insert_point != NULL);
	ASSERT(insert_point->nodesetval);
	ASSERT(insert_point->nodesetval->nodeNr == 1);

	/* copy nodes into result document */
	if (xpath_obj->nodesetval && (xpath_obj->nodesetval->nodeNr > 0)) {
		xmlNodePtr ins_parent = insert_point->nodesetval->nodeTab[0];
		if (xpath_obj->nodesetval) for (int i = 0;  i < xpath_obj->nodesetval->nodeNr; i++) {
			xmlNodePtr node = xmlDocCopyNode(
					xpath_obj->nodesetval->nodeTab[i],
					out, 1);
			ASSERT(node != NULL);
			if (process_param(node, prefix, result_prefix, source_prefix))
				return 1;
			node = xmlAddChild(ins_parent, node);
			ASSERT(node != NULL);
		}
	}

	xmlXPathFreeObject(xpath_obj);
	xmlXPathFreeObject(insert_point);

	/* primary configuration - copy seasons definitions*/
	if (prefix == NULL) {
		insert_point = xmlXPathEvalExpression(BAD_CAST "//ipk:params", xpath_ctx_out );
		xpath_obj = xmlXPathEvalExpression(BAD_CAST "//ipk:params//ipk:seasons", xpath_ctx);

		ASSERT(insert_point != NULL);
		ASSERT(insert_point->nodesetval);
		ASSERT(insert_point->nodesetval->nodeNr == 1);
		ASSERT(xpath_obj != NULL);

		xmlNodePtr ins_parent = insert_point->nodesetval->nodeTab[0];
		for (int i = 0;  i < xpath_obj->nodesetval->nodeNr; i++) {
			xmlNodePtr node = xmlDocCopyNode(
					xpath_obj->nodesetval->nodeTab[i],
					out, 1);
			ASSERT(node != NULL);
			node = xmlAddChild(ins_parent, node);
			ASSERT(node != NULL);
		}

		xmlXPathFreeContext(xpath_ctx);
		xmlXPathFreeContext(xpath_ctx_out);

	}

	/* clean-up namespaces mess; doesn't work :-( */
	xmlReconciliateNs(out, out->children);
	return 0;
}

/** Process information about single included configuration.
 * @param doc result document
 * @param cfg included config info
 * @param is_primary 1 if config is primary (should be added without prefix),
 * 0 if not
 * @param result_prefix prefix of output configuration
 * @return 0 on success, 1 on error
 */
int add_config(xmlDocPtr doc, xmlNodePtr cfg, int is_primary, xmlChar *result_prefix, time_t *max_szbase_stamp_time)
{
	ASSERT(doc != NULL);
	ASSERT(cfg != NULL);
	xmlChar *prefix;
	xmlDocPtr cur;
	xmlNodePtr node;
	char *c;

	/* get included configuration prefix */
	prefix = xmlGetProp(cfg, BAD_CAST "prefix");
	if (prefix == NULL) {
		sz_log(0, "Error parsing <config> element - no 'prefix' attribute found, line %ld",
				xmlGetLineNo(cfg));
		return 1;
	}

	/* try update max max_szbase_stamp_time */
	struct stat st;
	asprintf(&c, "/opt/szarp/%s/szbase_stamp", SC::U2A(prefix).c_str());
	if (stat(c, &st) >= 0 ) {
		if(st.st_mtime > *max_szbase_stamp_time)
			*max_szbase_stamp_time = st.st_mtime;
	}
	free(c);

	/* try to load configuration */
	asprintf(&c, "/opt/szarp/%s/config/params.xml", SC::U2A(prefix).c_str());
	cur = xmlParseFile(c);

	if (cur == NULL) {
		sz_log(0, "Couldn't parse file '%s'", c);
		return 1;
	}
	free(c);

	/* check for 'regexp' and 'remove' nodes */
	for (node = cfg->children; node; node = node->next) {
		xmlChar *xpath_expr;
		xmlChar *regexp;

		if (node->type != XML_ELEMENT_NODE)
			continue;
		if (node->ns == NULL)
			continue;
		if (xmlStrcmp(node->ns->href, BAD_CAST AGGREGATE_HREF))
			continue;
		xpath_expr = xmlGetProp(node, BAD_CAST "xpath");
		if (!xmlStrcmp(node->name, BAD_CAST "attribute")) {
			if (xpath_expr == NULL) {
				sz_log(0, "Error, attribute 'xpath' not found for element 'attribute' on line %ld",
					xmlGetLineNo(node));
				return 1;
			}
			xmlChar *name = xmlGetProp(node, BAD_CAST "name");
			if (name == NULL) {
				sz_log(0, "Error, attribute 'name' not found for element 'attribute' on line %ld",
					xmlGetLineNo(node));
				return 1;
			}
			xmlChar *value = xmlGetProp(node, BAD_CAST "value");
			xmlChar *copy = xmlGetProp(node, BAD_CAST "copy");
			if ((value == NULL) && (copy == NULL)) {

				sz_log(0, "Error, attribute 'value' or 'copy' not found for element 'attribute' on line %ld",
					xmlGetLineNo(node));
				return 1;
			} else if (value && copy) {
				sz_log(0, "Error, only one of 'value' and 'copy' attributes allowed for element 'attribute' on line %ld",
						xmlGetLineNo(node));
			}
			if (value && add_attribute(cur, xpath_expr, name, value))
				return 1;
			if (copy && copy_attribute(cur, xpath_expr, name, copy))
				return 1;
			xmlFree(xpath_expr);
			xmlFree(name);
			if (value)
				xmlFree(value);
			if (copy)
				xmlFree(copy);
			continue;
		}
		if (!xmlStrcmp(node->name, BAD_CAST "remove")) {
			if (xpath_expr == NULL) {
				sz_log(0, "Error, attribute 'xpath' not found for element 'remove' on line %ld",
					xmlGetLineNo(node));
				return 1;
			}
			if (remove_xpath(cur, xpath_expr))
				return 1;
			xmlFree(xpath_expr);
			continue;

		}
		if (xmlStrcmp(node->name, BAD_CAST "regexp")) {
			if (xpath_expr)
				xmlFree(xpath_expr);
			continue;
		}
		if (xpath_expr == NULL) {
			sz_log(0, "Error, attribute 'xpath' not found for element 'regexp' on line %ld",
					xmlGetLineNo(node));
			return 1;
		}
		if ((node->children == NULL) ||
				(node->children->type != XML_TEXT_NODE) ||
				(node->children->content == NULL)) {
			sz_log(0, "Error, no text content for element 'regexp' on line %ld",
					xmlGetLineNo(node));
			return 1;
		}
		regexp = xmlStrdup(node->children->content);

		if (process_regexp(cur, xpath_expr, regexp))
			return 1;

		xmlFree(regexp);
		xmlFree(xpath_expr);
	}

	if (process_config(doc, cur, is_primary ? NULL : prefix , result_prefix, prefix))
		return 1;

	/* link status parameters */
	if (is_primary)
		make_link(prefix, BAD_CAST "Status", BAD_CAST "Status", result_prefix);

	xmlFreeDoc(cur);
	sz_log(8, "Adding config with prefix '%s'\n", SC::U2A(prefix).c_str());
	xmlFree(prefix);
	return 0;
}

/** Adds 'null 'formulas to every param without formula.
 * @return 0 on success, 1 on error
 */
int add_null_formulas(xmlDocPtr doc)
{
	xmlXPathContextPtr xpath_ctx;
	xmlXPathObjectPtr params, fake;
	int ret;

	xpath_ctx = xmlXPathNewContext(doc);
	ret = xmlXPathRegisterNs(xpath_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);

	params = xmlXPathEvalExpression(BAD_CAST "//ipk:defined//ipk:param[not(/ipk:define)]",
			xpath_ctx);
	ASSERT(params != NULL);

	if (params->nodesetval && (params->nodesetval->nodeNr > 0)) {
		fake = xmlXPathEvalExpression(BAD_CAST "//ipk:param[@name='fake:fake:fake']/ipk:define",
			xpath_ctx);
		ASSERT(fake != NULL);
		ASSERT(fake->nodesetval != NULL);

		xmlNodePtr define = fake->nodesetval->nodeTab[0];
		for (int i = 0;  i < params->nodesetval->nodeNr; i++) {
			xmlNodePtr node = params->nodesetval->nodeTab[i];
			ASSERT(node != NULL);
			node = xmlAddChild(node, xmlCopyNode(define, 1));
			ASSERT(node != NULL);
		}
		xmlXPathFreeObject(fake);
	}
	xmlXPathFreeObject(params);
	xmlXPathFreeContext(xpath_ctx);
	return 0;
}

/** Creates and return new, empty XML config tree.
 * @param title title of configuration
 * @param prefix configuration prefix
 * @return new config
 */
xmlDocPtr create_empty_config(xmlChar *title, xmlChar *prefix, xmlChar* documentation_base_url)
{
	char *c;
	xmlDocPtr doc;

	ASSERT(title != NULL);
	ASSERT(prefix != NULL);

	asprintf(&c, "\
<?xml version=\"1.0\"?>\n\
<params xmlns=\"http://www.praterm.com.pl/SZARP/ipk\"\n\
    version=\"1.0\" read_freq=\"10\" send_freq=\"10\"\n\
    title=\"%s\" documentation_base_url=\"%s\" >\n\
<!-- DO NO EDIT. AUTOMATICALY GENERATED FOR CONFIGURATION '%s' BY AGREGATOR> -->\n\
  <device daemon=\"/bin/true\" path=\"/dev/null\">\n\
    <unit id=\"x\" type=\"1\" subtype=\"1\" bufsize=\"1\">\n\
      <param name=\"fake:fake:fake\" short_name=\"x\" unit=\"-\" prec=\"0\" base_ind=\"auto\">\n\
	  <define type=\"RPN\" formula=\"null \"  />\n\
      </param>\n\
    </unit>\n\
  </device>\n\
  <defined/>\n\
  <drawdefinable/>\n\
</params>\n\
			", title, documentation_base_url == NULL ? "http://szarp.com.pl" : (char*) documentation_base_url, prefix);
	ASSERT(c != NULL);

	doc = xmlParseMemory(c, strlen(c) + 1);
	ASSERT(doc != NULL);

	free(c);
	return doc;
}


struct eqint {
	  bool operator()(int i1, int i2) const
	  {
		  return (i1 == i2);
	  }
};

typedef unordered_map<int, std::string> vars_map_t;
/** global hash for all variables used in document */
vars_map_t g_vars;

/**
 * Makes substitution on string with variables from vars.
 * @param str string to substitute
 * @param vars hash with variables
 * @return NULL if no substitution was made,
 * substituted string (mallocated copy) otherwise.
 */
xmlChar* subst_string(const xmlChar* str, vars_map_t vars)
{
	ASSERT(str != NULL);

	std::string tmp = "";
	const xmlChar *c = str;
	while (*c != 0) {
		if (*c == '\\') {
			char *s = (char*) c+1;
			if (!isdigit(*s)) {
				tmp += *c;
				c++;
				continue;
			}
			if (vars.find(*s - '0') != vars.end()) {
				std::string val = vars[*s - '0'];
				tmp += val;
			}
			c++;
		} else {
			tmp += *c;
		}
		c++;
	}
	return xmlStrdup(BAD_CAST tmp.c_str());
}

/** Add variable to hash of variables.
 * @param vars hash of variables
 * @param v XML node with description of variable, with xpath attribute
 * describing initial value of string; variable substitution is performed on
 * this attribute
 * @param ctx XPath context in which xpath variable is evaluated
 * @return 0 on success, 1 on error
 */
int add_template_variable(vars_map_t& vars, xmlNodePtr v,
		xmlXPathContextPtr ctx)
{
	xmlChar* c;
	c = xmlGetProp(v, BAD_CAST "id");
	if (c == NULL) {
		sz_log(0, "Attribute 'id' not found in element 'variable' (line %ld)",
				xmlGetLineNo(v));
		return 1;
	}
	int i = atoi((char*)c);
#if 0	/* now variables are global and can be redeclared */
	if (vars.find(i) != vars.end()) {
		sz_log(0, "Double 'id' for variable in template ('id' is '%s', converted to %d, line %ld)\n",
				c, i, xmlGetLineNo(v));
		return 1;
	}
#endif
	xmlFree(c);
	c = xmlGetProp(v, BAD_CAST "xpath");

	if (c == NULL) {
		c = xmlStrdup(BAD_CAST "");
	} else {
		/* we can also substitute something in xpath attribute */
		xmlChar* new_c = subst_string(c, vars);
		if (new_c != NULL) {
			xmlFree(c);
			c = new_c;
		}
		xmlXPathObjectPtr obj;
		obj = xmlXPathEvalExpression(c, ctx);
		if ((obj == NULL) || (obj->type != XPATH_STRING)) {
			sz_log(0, "XPath result for '%s' (line %ld) is not a string",
					c, xmlGetLineNo(v));
			return 1;
		}
		xmlFree(c);
		ASSERT(obj->stringval != NULL);
		c = xmlStrdup(obj->stringval);
	}
	xmlChar *subst = xmlNodeGetContent(v);
	if ((subst != NULL) && (xmlStrlen(subst) == 0)) {
		xmlFree(subst);
		subst = NULL;
	}
	if (subst != NULL) {
		char *pattern;
		char *subexpr;
		re_pattern_buffer preg;
		if (compile_subst(subst, &preg, &pattern, &subexpr)) {
			sz_log(1, "Error compiling substitute expresion '%s' for variable in line %ld",
					subst, xmlGetLineNo(v));
			return 1;
		}
		re_registers regs;
		if (re_search(&preg, (char *)c, xmlStrlen(c), 0, xmlStrlen(c) - 1, &regs) > 0) {
			char *nc = regsub((char *)c, subexpr, &regs);
			xmlFree(c);
			c = BAD_CAST nc;
		}
	}

	vars[i] = (char *) SC::A2U((char *) c, true).c_str();
	sz_log(8, "Setting variable '%d' to '%s'", i, c);
	xmlFree(c);

	return 0;
}

/**
 * Recursive substitution of node content and attributes.
 * @param node target node
 * @param vars variables to substitute
 * @return 0 on success (always)
 */
int subst_node(xmlNodePtr node, vars_map_t& vars)
{
	if (node->type == XML_TEXT_NODE) {
		xmlChar *content = xmlNodeGetContent(node);
		if (content != NULL) {
			xmlChar* new_content = subst_string(content, vars);
			xmlFree(content);
			if (new_content != NULL) {
				xmlNodeSetContent(node, content);
				xmlFree(new_content);
			}
		}
		return 0;
	}
	if (node->type != XML_ELEMENT_NODE) {
		return 0;
	}
	for (xmlAttrPtr attr = node->properties; attr;
			attr = (xmlAttrPtr)attr->next) {
		if (attr->type != XML_ATTRIBUTE_NODE) {
			continue;
		}
		xmlChar *val;
		if (attr->ns == NULL) {
			val = xmlGetProp(node, attr->name);
		} else {
			val = xmlGetNsProp(node, attr->name, attr->ns->href);
		}
		xmlChar* new_val = subst_string(val, vars);
		xmlFree(val);
		if (new_val != NULL) {
			if (attr->ns == NULL) {
				xmlSetProp(node, attr->name, new_val);
			} else {
				xmlSetNsProp(node, attr->ns, attr->name, new_val);
			}
			xmlFree(new_val);
		}
	}
	for (xmlNodePtr n = node->children; n; n = n->next) {
		if (subst_node(n, vars) < 0) {
			return -1;
		}
	}
	return 0;
}

/** Process template for one node.
 * @param templ template XML node
 * @param node node to process template for
 * @param drawdefinable pointer to drawdefinable node in result document
 * @param xpath_ctx XPath context for result document
 * @param name template name
 * @result 0 on success, 1 on error
 */
int template_for_node(xmlNodePtr templ, xmlNodePtr node,
		xmlNodePtr drawdefinable, xmlXPathContextPtr xpath_ctx,
		xmlChar* name)
{
	xpath_ctx->node = node;

	/* iterate through all 'template' children */
	for (xmlNodePtr n = templ->children; n; n = n->next) {
		/* ignore text, comments and so on */
		if (n->type != XML_ELEMENT_NODE) {
			continue;
		}
		/* check for variable declaration */
		if (n->ns
				&& !xmlStrcmp(n->ns->href, BAD_CAST AGGREGATE_HREF)
				&& !xmlStrcmp(n->name, BAD_CAST "variable")) {
			/** add variable */
			if (add_template_variable(g_vars, n, xpath_ctx)) {
				return 1;
			}
		}
		/* ignore other elements from aggregate namespace */
		if (n->ns && !xmlStrcmp(n->ns->href, BAD_CAST AGGREGATE_HREF)) {
			continue;
		}
		/* make copy of all other nodes (usually, IPK nodes) */
		xmlNodePtr nnode = xmlAddChild(drawdefinable, xmlCopyNode(n, 1));
		ASSERT(nnode != NULL);
		/* substitute content of newly copied node */
		if (subst_node(nnode, g_vars)) {
			return 1;
		}
	}
	return 0;
}

/** Process a drawdefinable template.
 * @param templ template XML node
 * @param drawdefinable pointer to drawdefinable node in result document
 * @param xpath_ctx XPath context for result document
 * @return 0 on success, 1 on error
 */
int process_template(xmlNodePtr templ, xmlNodePtr drawdefinable,
		xmlXPathContextPtr xpath_ctx)
{
	ASSERT(templ != NULL);
	ASSERT(templ != drawdefinable);
	ASSERT(xpath_ctx != NULL);

	xmlChar *name = xmlGetProp(templ, BAD_CAST "name");
	xmlChar *xpath = xmlGetProp(templ, BAD_CAST "xpath");
	if (xpath == NULL) {
		sz_log(0, "'xpath' attribute for element 'template' not found in config file (line %ld)",
				xmlGetLineNo(templ));
		return 1;
	}
	xpath_ctx->node = NULL;
	xmlXPathObjectPtr obj = xmlXPathEvalExpression(xpath, xpath_ctx);
	if (obj && obj->nodesetval) {
		for (int i = 0; i < obj->nodesetval->nodeNr; i++) {
			if (template_for_node(templ, obj->nodesetval->nodeTab[i],
					drawdefinable, xpath_ctx, name)) {
				return 1;
			}
		}
	}
	xmlXPathFreeObject(obj);

	xmlFree(xpath);
	if (name) xmlFree(name);

	return 0;
}

/** Adds selected 'drawdefinable' nodes to output document and process
 * templates.
 * @param drawdefs XPath result set with all children of ipk:drawdefinable
 * node
 * @param output result document
 * @return 0 on success, 1 on error
 */
int add_drawdefinable(xmlXPathObjectPtr children, xmlDocPtr output)
{
	xmlXPathContextPtr ctx;
	xmlXPathObjectPtr obj;
	int ret;

	ASSERT(children != NULL);
	ASSERT(output != NULL);

	if (children->nodesetval == NULL)
		return 0;
	ctx = xmlXPathNewContext(output);
	ASSERT(ctx != NULL);
	ret = xmlXPathRegisterNs(ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);
	obj = xmlXPathEvalExpression(BAD_CAST "//ipk:drawdefinable",
			ctx);
	ASSERT(obj != NULL);
	ASSERT(obj->nodesetval != NULL);
	ASSERT(obj->nodesetval->nodeNr == 1);

	for (int i = 0; i < children->nodesetval->nodeNr; i++) {
		xmlNodePtr node = children->nodesetval->nodeTab[i];
		ASSERT(node != NULL);
		if ((node->type == XML_ELEMENT_NODE)
				&& (node->ns)
				&& !xmlStrcmp(node->ns->href, BAD_CAST AGGREGATE_HREF)
				&& !xmlStrcmp(node->name, BAD_CAST "template"))
		{
			/* process template */
			process_template(node, obj->nodesetval->nodeTab[0], ctx);
			continue;
		}
		/* copy all other nodes */
		node = xmlDocCopyNode(node, output, 1);
		ASSERT(node != NULL);
		node = xmlAddChild(obj->nodesetval->nodeTab[0], node);
		ASSERT(node != NULL);
	}

	xmlXPathFreeObject(obj);
	xmlXPathFreeContext(ctx);

	return 0;
}

/** Get text md5 digest
 * @param text text
 * @param digest digest
 * @return digest length */
unsigned int get_digest(char *text, unsigned char *digest)
{
	EVP_MD_CTX mdctx;
	const EVP_MD *md;
	unsigned int md_len;

	OpenSSL_add_all_digests();
	md = EVP_get_digestbyname("md5");

	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, md, NULL);
	EVP_DigestUpdate(&mdctx, text, strlen(text));
	EVP_DigestFinal_ex(&mdctx, digest, &md_len);
	EVP_MD_CTX_cleanup(&mdctx);

	return md_len;
}


/** Main processing function.
 * @param path path to configuration file
 * @return 0 on success, 1 on error */
int process_file(char *path, bool verbose, bool force)
{
	int ret, i;
	xmlDocPtr cfg;
	xmlDocPtr output;
	xmlXPathContextPtr xpath_ctx;
	xmlXPathObjectPtr xpath_obj;
	xmlChar *prefix;
	xmlChar *title;
	xmlChar *doc_base_url;
	char *out_path, *szbase_stamp_path;
	struct stat st;

	ASSERT(path != NULL);

	/* load config file */
	cfg = xmlParseFile(path);
	if (cfg == NULL) {
		sz_log(0, "Couldn't open file '%s'", path);
		return 1;
	}

	/* check for elements and attributes */

	/* create XPath context */
	xpath_ctx = xmlXPathNewContext(cfg);
	ASSERT(xpath_ctx != NULL);
	/* register namespaces */
	ret = xmlXPathRegisterNs(xpath_ctx, BAD_CAST "aggr",
			BAD_CAST AGGREGATE_HREF);
	ASSERT(ret == 0);
	ret = xmlXPathRegisterNs(xpath_ctx, BAD_CAST "ipk",
			SC::S2U(IPK_NAMESPACE_STRING).c_str());
	ASSERT(ret == 0);

	prefix = uxmlXPathGetProp(BAD_CAST "/aggr:aggregate/@prefix",
			xpath_ctx);
	if (prefix == NULL)
		return 1;

	title = uxmlXPathGetProp(BAD_CAST "/aggr:aggregate/@title",
			xpath_ctx);
	if (title == NULL)
		return 1;

	doc_base_url = uxmlXPathGetProp(BAD_CAST "/aggr:aggregate/@documentation_base_url",
			xpath_ctx);

	/* create new empty config */
	output = create_empty_config(title, prefix, doc_base_url);

	/* output path and szbase_stamp path*/
	asprintf(&out_path, "/opt/szarp/%s/config/params.xml", SC::U2A(prefix).c_str());
	asprintf(&szbase_stamp_path, "/opt/szarp/%s/szbase_stamp", SC::U2A(prefix).c_str());

	/* retrive 'config' elements */
	xpath_obj = xmlXPathEvalExpression(
			BAD_CAST "/aggr:aggregate/aggr:config",
			xpath_ctx);
	ASSERT(xpath_obj != NULL);

	if (!xpath_obj->nodesetval) {
		sz_log(0, "Error parsing config - '/aggregate/config' not found");
		return 1;
	}

	time_t max_szbase_stamp_time = -1;

	/* process <config> elements */
	if (xpath_obj->nodesetval) for (i = 0; i < xpath_obj->nodesetval->nodeNr; i++) {
		if (add_config(output, xpath_obj->nodesetval->nodeTab[i], i == 0, prefix, &max_szbase_stamp_time))
			return 1;
	}
	xmlXPathFreeObject(xpath_obj);

	/* update szbase_stamp if needed */
	if (stat(szbase_stamp_path, &st) >= 0 ) {
		if(st.st_mtime <= max_szbase_stamp_time) {
			if(verbose) {
				printf("Touching %s.\n", szbase_stamp_path);
			}
			int fd = open (szbase_stamp_path, O_WRONLY | O_CREAT | O_TRUNC |O_NONBLOCK | O_NOCTTY,
                 S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

			if(fd < 0 && verbose) {
				perror("Error while touching szbase_stamp file.\n");
			}
		}
	}
	free(szbase_stamp_path);

	/* add empty formulas to defined elements */
	add_null_formulas(output);

	/* add drawdefinable params from config file (and process templates) */
	xpath_obj = xmlXPathEvalExpression(
			BAD_CAST "/aggr:aggregate/ipk:drawdefinable/*",
			xpath_ctx);
	ASSERT(xpath_obj != NULL);
	if (add_drawdefinable(xpath_obj, output))
		return 1;
	xmlXPathFreeObject(xpath_obj);

	/* remove text nodes */
	remove_xpath(output, (xmlChar *) "//text() and not(//script/text())");

	bool save = true;
	if (!force) {
		xmlChar *docptr;
		unsigned char md_value_dest[EVP_MAX_MD_SIZE];
		unsigned int md_len_dest = 0;

		if (stat(out_path, &st) >= 0 ) {

			int textlen;

			/* dump XML to string */
			xmlDocDumpFormatMemory(output, &docptr, &textlen, 1);

			if(st.st_size == textlen) {
				int indf;
				indf = open(out_path, O_RDONLY);
				if(indf > 0){
					char* file;
					int result = 0;
					int tmpresult = 0;
					file = (char*) malloc(textlen);

					do {
						tmpresult = read(indf, &(((char*)file)[result]), textlen - result);
						result += tmpresult;
					} while (result != textlen && tmpresult != 0);

					md_len_dest = get_digest(file, md_value_dest);

					if(verbose) {
						printf("Destination file digest is: ");
						for(i = 0; (uint)i < md_len_dest; i++) printf("%02x", md_value_dest[i]);
						printf("\n");
					}

					free(file);
				} else {
					if(verbose) {
						perror("Error while opening old output file.\n");
					}
				}

			} else {
				if(verbose) {
					printf("No old output file has diffirent length - overriding.\n");
				}
			}
		} else 	{
			if(verbose) {
				printf("No old output file for comparison.\n");
			}
		}

		if(md_len_dest > 0) {
			unsigned char md_value[EVP_MAX_MD_SIZE];
			unsigned int md_len;
			md_len = get_digest((char*)docptr, md_value);

			if(verbose) {
				printf("Generated config digest is: ");
				for(i = 0; (uint)i < md_len; i++) printf("%02x", md_value[i]);
				printf("\n");
			}

			if(md_len == md_len_dest) {
				for(i = 0; (uint)i < md_len; i++)
					if(md_value[i] != md_value_dest[i]) {
						break;
					}

				if((uint)i == md_len)
					save = false;
			}
		}
	}

	if(save){
		if(verbose) {
				printf("Saving new output file.\n");
		}

		/* save result to file */
		if (xmlSaveFormatFile(out_path, output, 1) < 0) {
			sz_log(0, "Error saving file %s", out_path);
			return 1;
		}
	} else {
		if(verbose) {
			printf("Files don`t differ.\n");
		}
	}

	xmlXPathFreeContext(xpath_ctx);
	free(out_path);
	xmlFree(title);
	xmlFree(prefix);
	xmlFreeDoc(output);
	xmlFreeDoc(cfg);

	return 0;
}

int main(int argc, char **argv)
{
	bool verbose, force;

	loginit_cmdline(2, NULL, &argc, argv);

	if (usage(argc, argv, verbose, force))
		return 1;

	xmlInitParser();
	LIBXML_TEST_VERSION
	xmlLineNumbersDefault(1);

	/* Set type of regular expresions used */
	re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;
	if (process_file(argv[argc-1], verbose, force))
		return 1;

	xmlCleanupParser();
	xmlMemoryDump();

	return 0;
}

#else

int main()
{
	printf("libxml2 xpath support not enabled!\n");
	return 1;
}

#endif
