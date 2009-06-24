/* 
  SZARP: SCADA software 

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

#define SZARP_CFG "/etc/"PACKAGE_NAME"/"PACKAGE_NAME".cfg"

DaemonConfig::DaemonConfig(const char *name)
{
	assert (name != NULL);
	m_daemon_name = name;
	assert (!m_daemon_name.empty());
	m_load_called = 0;
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

	m_units = NULL;
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

	delete m_units;
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

int DaemonConfig::Load(int *argc, char **argv, int libpardone)
{
	char *c;
	int l;
	char *ipk_path;
	
	assert (m_load_called == 0);

	/* Set initial logging. */
	loginit_cmdline(2, NULL, argc, argv);
	
	
	/* libpar command line*/
	libpar_read_cmdline(argc, argv);

	/* parse command line */
	if (ParseCommandLine(*argc, argv)) {
		return 2;
	}

	if (m_noconf) {
		m_load_called = 1;
		return 0;
	}

	/* libpar */
	libpar_init_with_filename(SZARP_CFG, 1);

	/* logging */
	c = libpar_getpar(m_daemon_name.c_str(), "log_level", 0);
        if (c == NULL)
                l = 2;
        else {
                l = atoi(c);
                free(c);
        }                                                           
        c = libpar_getpar(m_daemon_name.c_str(), "log", 0);
        if (c == NULL)
                asprintf(&c, "%s/%s",PREFIX"/logs", m_daemon_name.c_str());
        l = loginit(l, c);
        if (l < 0) {
               sz_log(0, "%s: cannot inialize log file %s, errno %d", 
				m_daemon_name.c_str(), c, errno);
                free(c);
                return 1;
        }
	free(c);
	
	/* other libpar params */
	ipk_path = libpar_getpar(m_daemon_name.c_str(), "IPK", 1);
	m_parcook_path = libpar_getpar(m_daemon_name.c_str(), "parcook_path", 1);
	m_linex_path = libpar_getpar(m_daemon_name.c_str(), "linex_cfg", 1);

	if (libpardone)
		libpar_done();

	
	/* load params.xml */
	if (LoadXML(ipk_path)) {
		free(ipk_path);
		return 1;
	}
	
	free(ipk_path);

	m_load_called = 1;

	InitUnits();

	return 0;
}

enum {
	DEVICE=128,
	SNIFF,
	SCAN,
	NOCONF,
	DUMPHEX,
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

int DaemonConfig::ParseCommandLine(int argc, char**argv)
{
	const char *arg_doc = "[<device number>] [ignored arguments ...]";
	char *doc;
	struct argp_option options[] = {
        {"debug", 'd', "n", 0, "Set initial debug level to n, n is from 0 \
(error only), to 10 (extreme debug), default is 2. This is overwritten by \
settings in config file."},
        {"D<param>", -1, "val", 0,
         "Set initial value of libpar variable <param> to val."},
        {"single", 's', 0, 0,
         "Single (debug) mode - do not fork, do not connect to parcook"},
	{"diagno", 'b', 0, 0,
         "Diagno (debug) mode - the same as single but connect to parcook"},
	{"device", DEVICE, "device file", 0,
         "Device daemon should use"},
	{"sniff", SNIFF, 0, 0,
         "Daemon does not initiate any transmission by itself only sniffs what is sent over the wire"},
	{"scan", SCAN, "ID range (e.g. 1-7)", 0,
         "Daemon scans for units from within provided ids' range"},
	{"speed", SPEED, "Port speed e.g. 9600", 0,
         "Port communiation speed"},
	{"noconf", NOCONF, 0, 0,
         "Do not load configuration"},
	{"dumphex", DUMPHEX, 0, 0,
         "Print trasmited bytes in hex-terminal format"},
	{"askdelay", ASKDELAY, "delay in seconds", 0, 
         "Sets delay between quieries to different units"},
        {0},
	};
                                                                                
	asprintf(&doc, "%s\v\
Config file:\n\
Configuration options are read from file "SZARP_CFG"\n\
from section '%s' or from global section.\n\
These options are mandatory if configuration is loaded:\n\
        IPK             full path to configuration file\n\
        parcook_path    path to file used to create identifiers for IPC\n\
                        resources, this file must exist.\n\
        linex_cfg       path to file used to create identifiers for IPC\n\
                        resources, this file must exist.\n\
These options are optional:\n\
        log             path to log file, default is " PREFIX "/log/%s\n\
        log_level       log level, from 0 to 10, default is from command line\n\
                        or 2.\n\
Note: not all options are handled by every daemon.\n\
%s",
		!m_usage_header.empty() ? m_usage_header.c_str() : "SZARP line daemon\n",
		m_daemon_name.c_str(),
		m_daemon_name.c_str(),
		!m_usage_footer.empty() ? m_usage_footer.c_str() : "\n");
	
	struct argp argp = { options, parse_opt, arg_doc, doc };

	struct arguments arguments;

	arguments.diagno = 0;
	arguments.single = 0;
	arguments.noconf = 0;
	arguments.device_path = NULL;
	arguments.id1 = 0;
	arguments.id2 = 0;
	arguments.scanner = 0;
	arguments.device = 0;
	arguments.noconf = 0;
	arguments.speed = 0;
	arguments.dumphex = 0;
	arguments.sniff = 0;
	arguments.askdelay = 0;

	if (argp_parse(&argp, argc, argv, 0, 0, &arguments)) {
		free(doc);
		return 1;
	}

	m_diagno = arguments.diagno;
	m_single = arguments.single;
	m_noconf = arguments.noconf;
	m_device_path = arguments.device_path != NULL? arguments.device_path : "";
	m_id1 = arguments.id1;
	m_id2 = arguments.id2;
	m_scanner = arguments.scanner;
	m_device = arguments.device;
	m_noconf = arguments.noconf;
	m_speed = arguments.speed;
	m_dumphex = arguments.dumphex;
	m_sniff = arguments.sniff;
	m_askdelay = arguments.askdelay;

	free(doc);
	return 0;
}


int DaemonConfig::LoadXML(char *path)
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
	m_device_obj = m_ipk->DeviceById(m_device);
	if (m_device_obj == NULL)
		return 1;
		
	return 0;
}

int DaemonConfig::GetLineNumber()
{
	assert (m_load_called != 0);
	return m_device;
}



TSzarpConfig* DaemonConfig::GetIPK()
{
	assert (m_load_called != 0);
	return m_ipk;
}

TDevice* DaemonConfig::GetDevice()
{
	assert (m_load_called != 0);
	return m_device_obj;
}

xmlDocPtr DaemonConfig::GetXMLDoc()
{
	assert (m_load_called != 0);
	return m_ipk_doc;
}

xmlNodePtr DaemonConfig::GetXMLDevice()
{
	assert (m_load_called != 0);
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

const char* DaemonConfig::GetParcookPath()
{
	assert (m_load_called != 0);
	return m_parcook_path.c_str();
}

const char* DaemonConfig::GetLinexPath()
{
	assert (m_load_called != 0);
	return m_linex_path.c_str();
}

int DaemonConfig::GetSingle()
{
	assert (m_load_called != 0);
	return m_single;
}

int DaemonConfig::GetDiagno()
{
	assert (m_load_called != 0);
	return m_diagno;
}

const char* DaemonConfig::GetDevicePath() 
{
	assert (m_load_called != 0);
	if (!m_device_path.empty())
		return m_device_path.c_str();
	if (m_device_obj) { 
		m_device_path = SC::S2A(m_device_obj->GetPath());
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
		return m_device_obj->GetSpeed();
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

void DaemonConfig::InitUnits() {
	assert(m_ipk);

	UnitInfo *cu = NULL;

	for (TUnit* unit = GetDevice()->GetFirstRadio()->GetFirstUnit();
			unit;
			unit = unit->GetNext()) {
		
		UnitInfo* ui = new UnitInfo(unit->GetId(), unit->GetSendParamsCount());

		if (cu) 
			cu->SetNext(ui);
		else
			m_units = ui;	

		cu = ui;
	}

}

void DaemonConfig::CloseIPK() {

	if (m_device_path.empty()) 
		m_device_path = SC::S2A(m_device_obj->GetPath());

	if (m_speed == 0)
		m_speed = m_device_obj->GetSpeed();

	delete m_ipk;

	m_ipk = NULL;
	m_device_obj = NULL;
}

DaemonConfig::UnitInfo* DaemonConfig::GetFirstUnitInfo() {
	return m_units;
}

DaemonConfig::UnitInfo::UnitInfo(char id, int send_count) : m_id(id), m_send_count(send_count), m_next(NULL)
{}

char DaemonConfig::UnitInfo::GetId() {
	return m_id;
}

int DaemonConfig::UnitInfo::GetSendParamsCount() {
	return m_send_count;
}

DaemonConfig::UnitInfo* DaemonConfig::UnitInfo::GetNext() {
	return m_next;
}


void DaemonConfig::UnitInfo::SetNext(UnitInfo* next) {
	m_next = next;
}

DaemonConfig::UnitInfo::~UnitInfo() {
	delete m_next;
}

#endif 

