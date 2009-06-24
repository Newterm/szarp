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
/* $Id$ */

#ifndef __SVG_GENERATOR_H__
#define __SVG_GENERATOR_H__

#include <libxml/parser.h>
#include "szbase/szbbase.h"
#include "szarp_config.h"

class Szbase;
class IPKContainer;

class SVGGenerator {
	Szbase *m_szbase;
	xmlDocPtr m_request_doc;
	std::vector<std::wstring> m_params_names;
	std::vector<TParam*> m_params;
	std::vector<std::wstring> m_params_colors;
	time_t m_start_time;
	SZARP_PROBE_TYPE m_probe_type;	
	int m_custom_length;
	int m_graph_points_no;

	std::vector<std::vector<double> > m_params_values;
	std::vector<time_t> m_probes_times;

	bool GetParamsFromXml();
	bool GetGraphTimeParams();
	bool GetParamsValues();
	IPKContainer *m_ipk_container;
	xmlNodePtr GenerateTimeInfo(xmlDocPtr doc);
	xmlDocPtr StartDoc();
	xmlDocPtr GenerateDoc();
	xmlNodePtr GenerateParam(size_t draw_no, xmlDocPtr doc);
public:
	SVGGenerator(Szbase *szbase, xmlDocPtr request_doc);
	xmlChar* GenerateSVG(int *code);
};

#endif
