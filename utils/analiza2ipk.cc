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
 * SZARP - Praterm S.A.   
 * 2005 
 *
 * $Id$
 * Program dodajacy konfiguracje analizy(plik analiza.cfg) do IPK.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <sys/stat.h>
#include <libxml/tree.h>
#include <libxml/xmlIO.h>
#include <libgen.h>
#include <iostream>
#include <algorithm>
#include <exception>
#include <vector>

#include "libpar_int.h"
#include "liblog.h"
#include "ptt_act.h"
#include "libpar.h"
#include "szarp_config.h" //only for generation of analiza specific xml nodes 


using std::cout;
using std::endl;
using std::vector;

void Usage() { 
	cout << "Merges SZARP 2.1 analiza.cfg with IPK.\n";
	cout << "Usage: analiza2ipk {--help|-h}	print info end exit\n";
	cout << "       analiza2ipk [--force|-f] [--output|-o file_name] [input_dir] merge configurations\n\n";
	cout << "       --output | -o         name of the file where result of a merge ought to be saved\n";
	cout << "                             (params.xml if not specified)\n";
	cout << "       --force  | -f         this option if present allows application to override existing output file\n";
	cout << "       input_dir             directory with configuration files (params.xml, analiza.cfg, PTT.act)\n"; 
	cout << "                             - current is default)\n";
	cout << "You can also specify '--debug=<n>' option to set debug level from 0 (errors\n  only) to 9.\n";
}

/** maps names of params in an old config file to corresponding enum values*/
struct ipc_param_name {
	const char* config_name;		/**< old name of a param*/
	TAnalysis::AnalysisParam id; 	/**< correspoding paramter type*/
} ipc_params_names[] = {
		{"InData.a_enable", 	TAnalysis::ANALYSIS_ENABLE},
		{"InData.work_timer_h", TAnalysis::REGULATOR_WORK_TIME},
		{"InData.Vc",		TAnalysis::COAL_VOLUME},
		{"InData.En_Vc",	TAnalysis::ENERGY_COAL_VOLUME_RATIO},
		{"InData.Fp_4m3",	TAnalysis::AIR_STREAM},		
		{"InData.Vr_l",		TAnalysis::LEFT_GRATE_SPEED},
		{"InData.Vr_p",		TAnalysis::RIGHT_GRATE_SPEED},
		{"InData.Min_Fp_4m3",	TAnalysis::MIN_AIR_STREAM},
		{"InData.Max_Fp_4m3",	TAnalysis::MAX_AIR_STREAM},
		{"InData.Hw_l",		TAnalysis::LEFT_COAL_GATE_HEIGHT},
		{"InData.Hw_p",		TAnalysis::RIGHT_COAL_GATE_HEIGHT},
		{"OutData.Fp_4m3",	TAnalysis::AIR_STREAM_RESULT},
		{"OutData.AnalyseStatus", TAnalysis::ANALYSIS_STATUS},
};

/**<desribes analysis param pointing to a shared memory segment*/
struct param {
	int ipc_index;	/**< index in ipc memory segment*/
	TAnalysis param;/**< a param itself*/
};

/**bioler configuration*/
struct boiler_conf {
	vector<param> ipc_params; /**< list of associated params*/
	TBoiler boiler;		  /**< boiler description*/
};

/** Wraps XML document containing IPK*/
class IPKXMLWrapper {

	xmlDocPtr config;	/**<pointer to a document*/

	/**Finds param node with given ipc index
	 *@param ipc_index index of reqested param node
	 @return pointer to a node or NULL if node with speficied 
	 	index is not found*/
	xmlNodePtr FindParamByIpc(int ipc_index) {
		int current_index = 0;
		return DigForIpc(config->children, ipc_index, current_index);
	}

	/**Scans children of supplied node in search for param node with
	 * given ipc index
	 * @param parent pointer to a parent node
	 * @param ipc_index index of reqested paramter
	 * @param current_index current index
	 * @return pointer to a node or NULL if node with speficied 
	 	index is not found*/
	xmlNodePtr DigForIpc(xmlNodePtr parent, int ipc_index, int& current_index) {
		for (xmlNodePtr ch = parent->children; ch; ch = ch->next) {
			if (!strcmp("unit",(char*)ch->name) || !strcmp("defined",(char*)ch->name)) {
				xmlNodePtr ch2 = ch->children;
				for (;ch2; ch2 = ch2->next)  
					if (!strcmp("param",(char*)ch2->name) && (current_index++ == ipc_index)) 
						return ch2;
			} else if (!strcmp("device",(char*)ch->name) || !strcmp("radio",(char*)ch->name)) {
					xmlNodePtr result = DigForIpc(ch, ipc_index, current_index);
					if (result) 
						return result;
			}
		}
		return NULL;
	}
					
	/**@return top analysis element in wrapped XML doc, NULL if not found*/
	xmlNodePtr GetBoilersElement() {
		xmlNodePtr ch=config->children->children;
		for (;ch;ch = ch->next) 
			if (!strcmp("boilers",(char*)ch->name))
				return ch;
		return NULL;
	}

	/**Appends analysis node to a param node
	 * @param ipc_index ipc index of param node to which node should
	 * 	be added
	 * @param analysis object desribing the alasisys node*/
	void AppendAnalysisParam(int ipc_index, TAnalysis& analysis) {
		xmlNodePtr param = FindParamByIpc(ipc_index);
		if (!param) {
			sz_log(0,"Unable to find param of ipc index = %d", ipc_index);
			throw std::exception();
		}
		xmlNodePtr analysis_param = analysis.generateXMLNode();
		if (!analysis_param)
			throw std::exception();
		xmlAddChild(param, analysis_param);
	}
	
	/** Appends boiler configuration to a document
	 * @param boiler object desribing boiler node*/
	void AppendBoiler(TBoiler& boiler) {
		xmlNodePtr analysis = GetBoilersElement();
		if (!analysis) 
			analysis = xmlNewChild(config->children, NULL, (xmlChar*)"boilers", NULL);
		xmlNodePtr boiler_node = boiler.generateXMLNode();
		if  (!boiler_node)
			throw std::exception();
		xmlAddChild(analysis, boiler_node);
	}


public:
	IPKXMLWrapper(xmlDocPtr _config) : config(_config)
	{};


	/**@return true if main analysis element is present in the configuration*/
	bool HasBoilersElement() {
		return GetBoilersElement() != NULL;
	}

	/**Appends boiler configuration to a node
	 * @param boiler boiler's configuration
	 */
	void AppendBoierConfig(boiler_conf& boiler) {
		vector<param>::iterator i;
		for (i = boiler.ipc_params.begin(); i != boiler.ipc_params.end() ; ++i)
			AppendAnalysisParam(i->ipc_index, i->param);

		AppendBoiler(boiler.boiler);
	}
					
	/**@return wrapped document*/
	xmlDocPtr GetDoc() {
		return config;
	}

};

/**Retrieves float param from analia.cfg file
 * @param boiler_name name of boiler being also section in configuration file
 * @param param_name name of a param
 * @param r output parameter - result*/
void GetParamValue(const char *boiler_name, const char* param_name, float &r) {
	bool ok ;
	char* p = libpar_getpar(boiler_name, param_name,1);
	if (sscanf(p, "%f", &r) != 1 || r < 0) {
		sz_log(1, "Invalid %s param value for %s", param_name, boiler_name);
		ok = false;
	}
	else
		ok = true;
	free(p);
	if (!ok)
		throw std::exception();
}

/**Retrieves int param from analia.cfg file
 * @param boiler_name name of boiler being also section in configuration file
 * @param param_name name of a param
 * @param r output parameter - result*/
void GetParamValue(const char *boiler_name, const char* param_name, int &r) {
	bool ok;
	char* p = libpar_getpar(boiler_name, param_name, 1);
	if (sscanf(p, "%d", &r) != 1 || r < 0) {
		sz_log(1, "Invalid %s param value for %s", param_name , boiler_name);
		ok = false;
	}
	else
		ok = true;
	free(p);
	if (!ok)
		throw std::exception();
}

/**Retrieves grate speed bound from config file. A bit messy because
 * anliza.cfg does not conform to libpar file syntax.
 * Result is grate speed in mm/h.
 * @param boiler_name name of boiler being also section in configuration file
 * @param param_name name of a param
 * @param r output parameter - result*/
void GetGrateSpParamValue(char *boiler_name, char* param_name, int &r) {

	char *endptr;
	
	char* w = libpar_getpar(boiler_name,param_name, 1);
	bool ok = true;

	int meters = strtol(w, &endptr, 10);
	int milimeters;

	if (*endptr == '.' || *endptr == ',')  {
		char* mptr = ++endptr;
		char milimeter[] = "000";

		strtol(mptr, &endptr, 10);
		strncpy(milimeter, mptr, 
			std::min(sizeof(milimeter) - 1, (size_t)(endptr - mptr)));

		milimeters = strtol(milimeter, NULL, 10);
		r = meters * 1000 + milimeters;
	}
	else if (*endptr == '\0')
		r = meters * 1000;
	else
		ok = false;
	
	free(w);
	if (!ok){
		throw std::exception();
	}
}


/**Gets interval configuration for given boiler
 * @param boiler_name name of boiler being also section in configuration file
 * @param interval_no string containig roman numeral - number of interval
 * @return object desribing interval*/
TAnalysisInterval* GetInterval(char* boiler_name, const char* interval_no) {
	const int param_len = 40;
	char duration_name[param_len];
	char grate_speed_upper_name[param_len];
	char grate_speed_lower_name[param_len];

	int duration;
	int grate_speed_upper;
	int grate_speed_lower;

	snprintf(grate_speed_lower_name, param_len, "InData.Vr_Part%s_Low", interval_no);
	snprintf(grate_speed_upper_name, param_len, "InData.Vr_Part%s_Hi", interval_no);
	snprintf(duration_name, param_len, "InData.Vr_Part%s_Duration", interval_no);

	GetParamValue(boiler_name, duration_name, duration);	
	GetGrateSpParamValue(boiler_name, grate_speed_lower_name, grate_speed_lower);
	GetGrateSpParamValue(boiler_name, grate_speed_upper_name, grate_speed_upper);

	return new TAnalysisInterval(grate_speed_lower, grate_speed_upper, duration);
}

/**Retrieves boiler configuration`
 * @param boiler_no number of boiler
 * @param boiler_name name of boiler
 * @param conf output param where boiler configuration is stored*/
void GetBoilerConfig(int boiler_no, char* boiler_name, boiler_conf& conf) {

	for (unsigned i = 0; i < sizeof(ipc_params_names)/sizeof(ipc_param_name); ++i) {
		const ipc_param_name& param = ipc_params_names[i];
		int ptt_line_no;
		GetParamValue(boiler_name, param.config_name, ptt_line_no);
		struct param newparam;
		newparam.ipc_index = ptt_line_no - 1;
		newparam.param = TAnalysis(boiler_no, param.id);
		conf.ipc_params.push_back(newparam);
	}

	float grate_speed;
	GetParamValue(boiler_name, "InData.d_Vr", grate_speed);
	conf.boiler.SetGrateSpeed(grate_speed);

	float coal_gate_height;
	GetParamValue(boiler_name, "InData.d_Hw", coal_gate_height);
	conf.boiler.SetCoalGateHeight(coal_gate_height);

	int int_boiler_type;
	GetParamValue(boiler_name, "InData.WRtype", int_boiler_type);
	TBoiler::BoilerType boiler_type;
	switch (int_boiler_type) {
		case 0:
			boiler_type = TBoiler::WR25;
			break;
		case 1:
			boiler_type = TBoiler::WR10;
			break;
		case 2: 
			boiler_type = TBoiler::WR5;
			break;
		case 3:
			boiler_type = TBoiler::WR2_5;
			break;
		case 4:
			boiler_type = TBoiler::WR1_25;
			break;
		default:
			sz_log(1,"Unknown boiler type '%d' in %s", int_boiler_type, boiler_name);
			throw std::exception();
	}
	conf.boiler.SetBoilerType(boiler_type);

	TAnalysisInterval *i = GetInterval(boiler_name, "I");
	conf.boiler.AddInterval(i);

	i = GetInterval(boiler_name, "II");
	conf.boiler.AddInterval(i);

	i = GetInterval(boiler_name, "III");
	conf.boiler.AddInterval(i);

}

/** This procudure removes all blank text nodes from tree thus forcing libxml
 * to reformat an xml document during write. Otherwise new nodes won't 
 * be formatted. Potentially dangereous if white spaces are significant
 * (I guess it does not refer to params.xml).
 * @param root pointer to main tree's element*/
void RemoveBlankTextNodes(xmlNodePtr root) 
{
	xmlNodePtr node;
	xmlNodePtr next;

	node = root->children;

	while(node != 0)
	{
		next = node->next;
		if(xmlIsBlankNode(node))
		{
			xmlUnlinkNode(node);
			xmlFreeNode(node);
		}
		else
			RemoveBlankTextNodes(node);
		node = next;
	}
}

/** Writes document to a given file. Is quite paranoic to not partially
 * overrite existing file. Removes blank text nodes from a doc to ensure
 * nice formatting.
 * @param doc  document to be written
 * @param filename name of the file where doc is saved*/
bool WriteDoc(xmlDocPtr doc,char *filename) {

	char *_filename = strdup(filename);
	char *dir = dirname(_filename);
	xmlOutputBufferPtr buffer;

	char* tmpname; 
	asprintf(&tmpname, "%s/.ipkXXXXXX", dir);
	free(_filename);
	int fd = mkstemp(tmpname);

	if (fd == -1) {
		cout << "Unable to create temporary file" << endl;
		return false;
	}

	xmlCharEncodingHandlerPtr encoding_handler = xmlFindCharEncodingHandler("ISO-8859-2");
	if (!encoding_handler) {
		cout << "Unable to find ISO-8859-2 encoding handler" << endl;
		goto bad;
	}

	RemoveBlankTextNodes(doc->children);	
	buffer = xmlOutputBufferCreateFd(fd,encoding_handler);
	if (xmlSaveFormatFileTo(buffer, doc, NULL, 1) == -1) 
		goto bad;

	close(fd);
	if (rename(tmpname, filename)) {
		cout << "Unable to write to output file:" << filename << endl;
		unlink(tmpname);
		goto bad2;
	}
	free(tmpname);

	chmod(filename, 0644);

	return true;
bad:
	close(fd);
bad2:
	free(tmpname);
	unlink(tmpname);
	return false;
}

/**Merges analiza configuration with IPK.
 * @param ipk_path file containig ipk configuration
 * @param analiza_path file configuration configuration of analiza program
 * @return xml document - result of a merge*/
xmlDocPtr Merge(char* ipk_path, char* analiza_path) {
	const int max_number_of_boilers = 10;
	xmlDocPtr doc = xmlParseFile(ipk_path);

	try {
		if (!doc) {
			cout << "Unable to read file:"<< ipk_path << endl;
			throw std::exception();
		}

		IPKXMLWrapper ipk(doc);
		if (ipk.HasBoilersElement()) {
			cout << "Source params.xml file already contains analysis data" << endl;
			throw std::exception();
		}
		
		libpar_init_with_filename(analiza_path,1);

		for (int i = 0; i < max_number_of_boilers ; ++i ) {
			boiler_conf conf;
	
			char boiler_name[10];
			snprintf(boiler_name, 10, "KOCIOL_%d", i);
			if (!SeekSect(boiler_name))
				continue;

			GetBoilerConfig(i, boiler_name, conf );
			conf.boiler.SetBoilerNo(i);
			ipk.AppendBoierConfig(conf);
		}
		libpar_done();

		return ipk.GetDoc();
	}

	catch (std::exception) {
		libpar_done();
		xmlFree(doc);
		throw;
	}

}

int main(int argc, char* argv[]) {

	const char *output_ipk = NULL;
	char *dir = NULL;
	struct stat stat_buf;
	bool force = false;

	loginit_cmdline(2, NULL, &argc, argv);

	xmlInitParser();
	xmlIndentTreeOutput = 1;

	while (argc > 1) {
		if (!strcmp(argv[1], "-f") || (!strcmp(argv[1], "--force")))
			force = 1;
		else if (!strcmp(argv[1], "--help") || (!strcmp(argv[1],
			"-h"))) {
			Usage();
			return 0;
		} else if (!strcmp(argv[1], "--output") || (!strcmp(argv[1],
			"-o"))) {
			argc--;
			argv++;
			if (argc > 1) 
				output_ipk = argv[1];
			else {
				Usage();
				return 1;
			}

		} else {
			dir = strdup(argv[1]);
			goto out;
		}
		argc--;
		argv++;
	}
out:

	if (!dir) {
		dir = strdup(".");
		cout << "Input dir not given, assuming   : "<< dir << endl;
	}
	else
		cout << "Input dir                       : "<< dir << endl;

	if (!output_ipk) {
		output_ipk = "params.xml";
		cout << "Output file not given, assuming : "<< output_ipk << endl;
	}
	else
		cout << "Output file                     : "<< output_ipk << endl;
			

	char *ipk_path; 
	char *analiza_path; 
	char *output_path; 
	asprintf(&ipk_path, "%s/%s", dir, "params.xml");
	asprintf(&analiza_path, "%s/%s", dir, "analiza.cfg");
	output_path = strdup(output_ipk);

	try {
		xmlDocPtr newdoc = Merge(ipk_path, analiza_path);
		if (newdoc != NULL)  {
			if (!stat(output_ipk, &stat_buf)) {
				if (!force) {
					cout << "Output file: " << output_ipk << " exsists use '-f|--force' to overwrite it" <<endl;
					return 1;
				}
			}
			if (WriteDoc(newdoc,output_path))
				cout << "OK" << endl;
			else
				cout << "Failed to write new IPK file" << endl;
		}
		else
			cout << "Merge failed" << endl;
		
	}
	catch (std::exception) {
		cout << "Failed" << endl;
		return 1;
	}

	return 0;

}
