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
 * IPK

 * Pawe³ Pa³ucha <pawel@praterm.com.pl>
 *
 * XML config for SZARP line daemons.
 * 
 * $Id$
 */

#include <config.h>

#include "dmncfg.h"

#ifndef MINGW32

#include <assert.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <argp.h>
#include "conversion.h"
#include "liblog.h"
#include "libpar.h"
#include "xmlutils.h"

#define SZARP_CFG "/etc/" PACKAGE_NAME "/" PACKAGE_NAME ".cfg"

DaemonConfig::DaemonConfig(const char *name)
{
	assert (name != NULL);
	m_daemon_name = name;
	assert (!m_daemon_name.empty());
	m_load_called = 0;
	m_load_xml_called = 0;
	m_ipk_doc = NULL;
	m_ipk_device = NULL;
	m_ipk = NULL;
	m_device_obj = NULL;

	m_single = 0;
	m_diagno = 0;
	m_noconf = 0;
	m_device = -1;
	m_scanner = 0;
	m_id1 = 0;
	m_id2 = 0;
	m_speed = 0;
	m_dumphex = 0;
	m_noconf = 0;
	m_sniff = 0;
}

DaemonConfig::~DaemonConfig()
{
#define FREE(x) \
	if (x) free(x)
	if (m_ipk_doc) {
		xmlFreeDoc(m_ipk_doc);
	}
	if (m_ipk)
		delete m_ipk;
#undef FREE
	xmlCleanupParser();
}
	
void DaemonConfig::SetUsageHeader(const char *header)
{
	assert (m_load_called == 0);
	if (header) {
		m_usage_header = header;
	} else
		m_usage_header.clear();
}

void DaemonConfig::SetUsageFooter(const char *footer)
{
	assert (m_load_called == 0);
	if (footer) {
		m_usage_footer = footer;
	} else
		m_usage_footer.clear();
}

int DaemonConfig::Load(const ArgsManager& args_mgr, TSzarpConfig* sz_cfg , int force_device_index, void* ) 
{
	assert (m_load_called == 0);

	if (args_mgr.has("noconf")) {
		m_load_called = 1;
		return 0;
	}

	szlog::init(args_mgr);

	ipc_info = IPCInfo{
		*args_mgr.get<std::string>("parcook_path"),
		*args_mgr.get<std::string>("linex_cfg")
	};

	ipk_path = *args_mgr.get<std::string>("IPK");
	ParseCommandLine(args_mgr);
	/* do not load params.xml */
	if( sz_cfg ) {
		/**
		 * Cannot change m_device to forced value,
		 * cause m_device is used than to get proper
		 * shared memory.
		 */
		if( force_device_index < 0 ) 
			force_device_index = m_device;
		if( LoadNotXML(sz_cfg,force_device_index) ) {
			return 1;
		}
	/* load params.xml */
	} else if (LoadXML(ipk_path.c_str())) {
		return 1;
	}
		
	m_load_called = 1;

	m_timeval.tv_sec = GetDevice()->getAttribute<int>("sec_period", 10);
	m_timeval.tv_usec = GetDevice()->getAttribute<int>("usec_period", 0);
	if (m_timeval.tv_sec == 0 && m_timeval.tv_usec == 0) m_timeval.tv_sec = 10;

	InitUnits(GetDevice()->GetFirstUnit());

	return 0;
}

int DaemonConfig::Load(int *argc, char **argv, int libpardone , TSzarpConfig* sz_cfg , int force_device_index, void* ) 
{
	assert (m_load_called == 0);
	args_mgr.initLibpar();

	ArgsManager args_mgr(m_daemon_name);
	args_mgr.parse(*argc, argv, DefaultArgs(), DaemonArgs());

	/* libpar command line*/
	libpar_read_cmdline(argc, argv);

	/* parse command line */
	if (ParseCommandLine(*argc, argv)) {
		return 2;
	}

	if (args_mgr.get<bool>("noconf").get_value_or(false)) {
		m_load_called = 1;
		return 0;
	}

	szlog::init(args_mgr, m_daemon_name);

	m_parcook_path = *args_mgr.get<std::string>("parcook_path");
	m_linex_path = *args_mgr.get<std::string>("linex_cfg");

	m_ipk_path = *args_mgr.get<std::string>("IPK");
	m_prefix = *args_mgr.get<std::string>("config_prefix");

	ParseCommandLine(args_mgr);
	/* do not load params.xml */
	if( sz_cfg ) {
		/**
		 * Cannot change m_device to forced value,
		 * cause m_device is used than to get proper
		 * shared memory.
		 */
		if( force_device_index < 0 ) 
			force_device_index = m_device;
		if( LoadNotXML(sz_cfg,force_device_index) ) {
			return 1;
		}
	/* load params.xml */
	} else if (LoadXML(ipk_path.c_str())) {
		return 1;
	}
		
	m_load_called = 1;

	InitUnits(GetDevice()->GetFirstUnit());

	return 0;
}

int DaemonConfig::Load(int *argc, char **argv, int libpardone , TSzarpConfig* sz_cfg , int force_device_index, void* ) 
{
	ArgsManager args_mgr(m_daemon_name);
	args_mgr.parse(*argc, argv, DefaultArgs(), DaemonArgs());

	return Load(args_mgr, sz_cfg, force_device_index);
}

enum {
	DEVICE=128,
	SNIFF,
	SCAN,
	NOCONF,
	DUMPHEX,
	NOPARCOOK,
	SPEED,
	ASKDELAY,
};

/**Command line arguments recognized by DaemonConfig*/
struct arguments
{
   	int single;
	int diagno;
	int device;
	int noconf;
	char* device_path;
	int scanner;
	char id1, id2;
	int speed;
	int dumphex;
	int sniff;
	int askdelay;
};
	
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
        struct arguments *arguments = (struct arguments *)state->input;
                                                                                
        switch (key) {
                case 'd':
                case -1:
                        break;
                case 's':
                        arguments->single = 1;
                        break;

		case 'b':
                        arguments->diagno = 1;
                        break;
			
		case ARGP_KEY_ARG:
			if (state->arg_num > 0) 
				break;
			char *ptr;
			arguments->device = strtol(arg, &ptr, 0);
			if ((arg == NULL) || (ptr[0] != 0) || 
					(arguments->device <= 0)) {
				argp_usage(state);
				return -1;
			}
			break;
		case DEVICE:
			arguments->device_path = strdup(arg);
			break;
		case NOCONF:
			arguments->noconf = 1;
			break;
		case SCAN:
			arguments->scanner = 1;
			if (sscanf(arg, "%c-%c", &arguments->id1, &arguments->id2) != 2) {
				argp_usage(state);
				return -1;
			}
			break;
		case DUMPHEX:
			arguments->dumphex = 1;
			break;
		case SNIFF:
			arguments->sniff = 1;
			break;
		case SPEED:
			arguments->speed = strtol(arg, &ptr, 0);
			if ((arg == NULL) || (ptr[0] != 0) || 
					(arguments->speed <= 0)) {
				argp_usage(state);
				return -1;
			}
			break;
		case ASKDELAY:
			arguments->askdelay = strtol(arg, &ptr, 0);
			if ((arg == NULL) || (ptr[0] != 0) || 
					(arguments->askdelay <= 0)) {
				argp_usage(state);
				return -1;
			}
			break;
		case ARGP_KEY_END:
			break;
		case ARGP_KEY_NO_ARGS:
			break;
                default:
                        return ARGP_ERR_UNKNOWN;
        }
        return 0;
}

void DaemonConfig::ParseCommandLine(const ArgsManager& args_mgr) {
	m_diagno = args_mgr.has("diagno");
	m_single = args_mgr.has("single");
	m_noconf = args_mgr.has("noconf");
	m_dumphex = args_mgr.has("dumphex");
	m_sniff = args_mgr.has("sniff");

	m_device = args_mgr.get<unsigned int>("device-no").get_value_or(0);
	m_speed = args_mgr.get<unsigned int>("speed").get_value_or(0);
	m_askdelay = args_mgr.get<unsigned int>("askdelay").get_value_or(0);

	m_device_path = args_mgr.get<std::string>("device-path").get_value_or("");
	auto scan = args_mgr.get<std::string>("scan").get_value_or("");
	if (!scan.empty()) {
		sscanf(scan.c_str(), "%c-%c", &m_id1, &m_id2);
		m_scanner = true;
	}
}

int DaemonConfig::LoadXML(const char *path)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	int i;
	
	assert (path != NULL);
	
	xmlInitParser();
	xmlInitializePredefinedEntities();
	xmlLineNumbersDefault(1);

	doc = xmlParseFile(path);
	if (doc == NULL) {
		sz_log(1, "XML document '%s' not wellformed", path);
		return 1;
	}
	m_ipk_doc = doc;

	/* check root element */
	node = doc->children;
	if (node == NULL) {
		sz_log(1, "XML document '%s' empty", path);
		return 1;
	}
	if (strcmp((char *)node->name, "params") || (!node->ns) || 
			strcmp((char *)node->ns->href, (char *)SC::S2U(IPK_NAMESPACE_STRING).c_str())) {
		sz_log(1, "Element ipk:params expected at line %ld of file %s", xmlGetLineNo(node),
				path);
		return 1;
	}
	/* search for device element */
	for (i = 0, node = node->children; node; node = node->next) {
		if (strcmp((char *)node->name, "device") || (!node->ns) ||
				strcmp((char *)node->ns->href, (char *)SC::S2U(IPK_NAMESPACE_STRING).c_str()))
			continue;
		i++;
		if (i == m_device)
			break;
	}
	if (node == NULL) {
		sz_log(1, "Device element number %d not exists in %s (only %d found)",
				m_device, path, i);
		return 1;
	}
	m_ipk_device = node;

	/* load IPK */
	m_ipk = new TSzarpConfig();
	assert (m_ipk != NULL);
	if (m_ipk->parseXML(doc))
		return 1;

	m_ipk->SetName(m_ipk->GetTitle(), SC::L2S(m_prefix));

	m_device_obj = m_ipk->DeviceById(m_device);
	if (m_device_obj == NULL)
		return 1;

	m_load_xml_called = 1;
		
	return 0;
}

int DaemonConfig::LoadNotXML( TSzarpConfig* cfg , int device_id )
{
	m_ipk = cfg;
	m_device_obj = m_ipk->DeviceById(device_id);
	if( m_device_obj == NULL ) return 1;
	return 0;
}

int DaemonConfig::GetLineNumber() const
{
	assert (m_load_called != 0);
	return m_device;
}

bool DaemonConfig::GetSingle() const
{
	assert (m_load_called != 0);
	return m_single;
}


TSzarpConfig* DaemonConfig::GetIPK() const
{
	assert (m_load_called != 0);
	return m_ipk;
}

TDevice* DaemonConfig::GetDevice() const
{
	assert (m_load_called != 0);
	return m_device_obj;
}

xmlDocPtr DaemonConfig::GetDeviceXMLDoc() const
{
	xmlDocPtr doc = GetXMLDoc();

	// select only device element, with its parent elements
	xmlDocPtr device_doc = xmlCopyDoc(doc, SHALLOW_COPY);
	xmlNodePtr root_node = xmlDocGetRootElement(doc);
	xmlNodePtr new_root = xmlCopyNode(root_node, COPY_ATTRS);
	xmlNodePtr new_device_element = xmlCopyNode(GetXMLDevice(), DEEP_COPY);
	xmlAddChild(new_root, new_device_element);
	xmlDocSetRootElement(device_doc, new_root);

	return device_doc;
}

std::string DaemonConfig::GetDeviceXMLString() const
{
	xmlDocPtr device_doc = GetDeviceXMLDoc();
	std::string xml_str = xmlToString(device_doc);
	xmlFreeDoc(device_doc);
	return xml_str;
}

std::string DaemonConfig::GetPrintableDeviceXMLString() const
{
	return SC::printable_string(GetDeviceXMLString());
}

xmlDocPtr DaemonConfig::GetXMLDoc() const
{
	assert (m_load_xml_called != 0);
	return m_ipk_doc;
}

xmlNodePtr DaemonConfig::GetXMLDevice() const
{
	assert (m_load_xml_called != 0);
	return m_ipk_device;
}

void DaemonConfig::CloseXML(int clean_parser)
{
	if (m_ipk_doc)
		xmlFreeDoc(m_ipk_doc);
	m_ipk_doc = NULL;
	if (clean_parser) {
		xmlCleanupParser();
	}
}

std::string DaemonConfig::GetIPKPath() const
{
	assert (m_load_xml_called != 0);
	return ipk_path;
}

int DaemonConfig::GetDiagno()
{
	assert (m_load_called != 0);
	return m_diagno;
}

const char* DaemonConfig::GetDevicePath() const
{
	assert (m_load_called != 0);
	if (!m_device_path.empty())
		return m_device_path.c_str();
	if (m_device_obj) { 
		m_device_path = m_device_obj->getAttribute("path");
		return m_device_path.c_str();
	}
	return "/dev/INVALID";
}

int DaemonConfig::GetSpeed() 
{
	assert (m_load_called != 0);
	if (m_speed != 0)
		return m_speed;
	if (m_device_obj) 
		return m_device_obj->getAttribute<int>("speed", -1);
	return 0;
}

int DaemonConfig::GetNoConf()
{
	assert (m_load_called != 0);
	return m_noconf;
}

int DaemonConfig::GetDumpHex()
{
	assert (m_load_called != 0);
	return m_dumphex;
}

int DaemonConfig::GetScanIds(char *id1, char *id2) {
	assert (m_load_called != 0);
	if (!m_scanner)
		return 0;

	*id1 = m_id1;
	*id2 = m_id2;

	return 1;

}

int DaemonConfig::GetSniff() {
	assert (m_load_called != 0);
	return m_sniff;
}

int DaemonConfig::GetAskDelay() {
	assert (m_load_called != 0);
	return m_askdelay;
}

void DaemonConfig::InitUnits(TUnit* unit) {
	for (; unit; unit = unit->GetNext()) {
		m_units.push_back(unit);
	}
}

size_t DaemonConfig::GetParamsCount() const {
	return m_device_obj->GetParamsCount();
}

size_t DaemonConfig::GetSendsCount() const {
	size_t no_sends = 0;
	
	for (auto unit: m_units) {
		no_sends += unit->GetSendParamsCount();
	}

	return no_sends;
}


void DaemonConfig::CloseIPK() {

	if (m_device_path.empty()) 
		m_device_path = m_device_obj->getAttribute("path");

	if (m_speed == 0)
		m_speed = m_device_obj->getAttribute<int>("speed", -1);

	delete m_ipk;

	m_ipk = NULL;
	m_device_obj = NULL;
}


#endif 

