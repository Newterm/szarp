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
 * Pawe³ Pa³ucha 2002
 *
 * ptt2xml.c - PTT.act to XML conversion.
 *
 * $Id$
 */

#include <sstream>
#include <iomanip>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <math.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "libpar.h"
#include "liblog.h"
#include "ipcdefines.h"

#include "ptt2xml.h"
#include "ptt_act.h"
#include "server.h"
#include "utf8.h"
#include "tokens.h"

/**
 * ISL namespace string.
 */
#define	ISL_NAMESPACE_STRING	"http://www.praterm.com.pl/ISL/params"

#if 0
/**
 * Reads param table from PTT.act file and returns it as an XML tree.
 * @return XML tree with params, NULL on error
 */
xmlDocPtr PTTLoader::get_params()
{
	PTT_table_t *PTT_table;
	
	PTT_table = parse_PTT(ptt_path);
	if (PTT_table == NULL)
		return NULL;
	return ptt2xml(PTT_table);
}

/** character converting macro */
#define X 	(xmlChar *)
/** character converting macro */
#define U(a)	toUTF8(a)

/**
 * Converts params' table to XML tree.
 * @param PTT_table SZARP PTT params table
 * @return created tree
 */
xmlDocPtr PTTLoader::ptt2xml(PTT_table_t *PTT_table)
{
	xmlDocPtr doc;
	char buffer[20];
	PTT_param_t *p;
	
	doc = xmlNewDoc(X"1.0"); 
	doc->children = xmlNewDocNode(doc, NULL, X"params", NULL);
	xmlNewNs(doc->children, X(ISL_NAMESPACE_STRING), NULL);
	snprintf(buffer, 19, "%d", PTT_table->params);
	xmlSetProp(doc->children, X"all", X(buffer));
	snprintf(buffer, 19, "%d", PTT_table->baseparams);
	xmlSetProp(doc->children, X"inbase", X(buffer));
	for (p = PTT_table->param_list; p; p = p->next)
		insert_param(doc->children, p);
	return doc;
}

/**
 * Insert new param to tree.
 * @param node tree root
 * @param p inserted param
 */
void PTTLoader::insert_param(xmlNodePtr node, PTT_param_t *p)
{
	char buffer[20];
	char **toks;
	int tokc = 0;
	int i;
	xmlNodePtr n;

	tokenize_d(p->full_name, &toks, &tokc, ":");
	if (tokc < 0)
		return;
	n = node;
	for (i = 0; i < tokc-1; i++) 
		n = create_node(n, toks[i]);
	n = xmlNewChild(n, NULL, X"param", NULL); 
	xmlSetProp(n, X"name", U(toks[tokc-1]));
	xmlSetProp(n, X"full_name", U(p->full_name));
	xmlSetProp(n, X"short_name", U(p->short_name));
//	xmlSetProp(n, X"draw_name", U(p->draw_name));
	xmlSetProp(n, X"unit", U(p->unit));
//	snprintf(buffer, 19, "%d", p->base_ind);
//	xmlSetProp(n, X"base_ind", X(buffer));
	snprintf(buffer, 19, "%d", p->ipc_ind);
	xmlSetProp(n, X"ipc_ind", X(buffer));
	snprintf(buffer, 19, "%d", p->prec);
	xmlSetProp(n, X"prec", X(buffer));
	xmlSetProp(n, X"value", X"unknown");
	tokenize_d(NULL, &toks, &tokc, NULL);
}

/**
 * Creates and returns new node. If node with given name already exist, no new
 * node is created, existed is returned.
 * @param parent parent node
 * @param name node name
 * @param existing or newly created node
 * @return created or already existing node, NULL on error
 */
xmlNodePtr PTTLoader::create_node(xmlNodePtr parent, char *name)
{
	xmlNodePtr n;
	xmlChar *str = NULL;
	
	for (n = parent->children; n; n = n->next) {
		str = xmlGetProp(n, X"name");
		if (!str) {
			sz_log(1, "TREE not properly built");
			return NULL;
		};
		if (!strcmp((char *)str, (char *)(U(name)))) {
			xmlFree(str);
			break;
		}
		xmlFree(str);
	}
	if (!n) {
		n = xmlNewChild(parent, NULL, X"node", NULL);
		xmlSetProp(n, X"name", U(name));
	}
	return n;
}
#undef X
#undef U

#endif

namespace {

const  int sems[] = {SEM_PROBE, SEM_MINUTE, SEM_MIN10, SEM_HOUR };
const  int shms[] = {SHM_PROBE, SHM_MINUTE, SHM_MIN10, SHM_HOUR };

}

void PTTParamProxy::update(void) {
	for (size_t i = 0; i < sizeof(sems) / sizeof(sems[0]); i++)
		update(values[i], shms[i], sems[i]);
}
/**
 * Updates param values table. Tries to connect to parcook shared memory
 * segment and copies param values.
 */
void PTTParamProxy::update(std::vector<short>& values, int shm_type, int sem_type) 
{
	int shmd, semd;
	short *probes;
	struct sembuf Sem[4];
	struct shmid_ds shm_info;
	
	values.resize(0);

    	if ((semd = semget(ftok(parcook_path, SEM_PARCOOK), SEM_LINE, 00666)) < 0) {
		sz_log(4, "cannot get parcook semaphore descriptor");
		return;
	}
	shmd = shmget(ftok(parcook_path, shm_type), 0, 00666);
	if (shmd < 0) {
		sz_log(4, "cannot get parcook memory segment descriptor");
		return;
	}
	
	if (shmctl(shmd, IPC_STAT, &shm_info) < 0) {
		sz_log(4, "cannot read parcook memory segment size");
		return;
	}
	int size = shm_info.shm_segsz / sizeof(short);
	values.resize(size);
	if (size > 0) {
		
		if ((probes = (short *) 
			shmat(shmd, (void *) 0, 0)) == (void *) -1){ 
			sz_log(4, "cannot attache parcook memory segment");
			return; 
		}
		Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
		Sem[0].sem_num = sem_type + 1;
		Sem[0].sem_op = 0;
		Sem[1].sem_num = sem_type;
		Sem[1].sem_op = 1;
		semop(semd, Sem, 2);
		memcpy(&values[0], probes, size * sizeof(short));
		Sem[0].sem_flg = SEM_UNDO;
		Sem[0].sem_num = sem_type;
		Sem[0].sem_op = -1;
		semop(semd, Sem, 1);
		shmdt((void *) probes);

		return;
	}
}

/**
 * Returns text representation of param value. Value is converted according to
 * given precision.
 * @param ind param index in table
 * @param prec param precision
 * @return text representation of param value, "unknown" if segment is not
 * attached or SZARP_NO_DATA occurred, it's up to user to free the memory
 */
std::wstring PTTParamProxy::getValue(int ind, int prec, PROBE_TYPE probe_type)
{
	int i, j, k;
	
	if ((ind < 0) || (ind >= (int)values[probe_type].size()))
		return L"unknown";
	i = values[probe_type][ind];
	if ((prec > 7) || (i == SZARP_NO_DATA))
		return L"unknown";
	if (prec == 5) {
		if (i)
			return L"Tak";
		else
			return L"Nie";
	}
	if (prec == 6) {
		if (i == 0)
			return L"Pochmurno";
		else if (i == 1)
			return L"Zmiennie";
		else
			return L"S\u0142onecznie";
	}
	if (prec == 7) {
		if (i >= 0)
			return L"Plus";
		else
			return L"Minus";
	}

	std::wstringstream wss;
	for (j = 0, k = 1; j < prec; j++, k*=10) ;
	if (k <= 1)
		wss << i;
	else {
		if (i < 0) 
			wss << L"-" << -i/k << L"." << std::setw(prec) << std::setfill(L'0') << -i%k;
		else
			wss << i/k << L"." << std::setw(prec) << std::setfill(L'0') << i%k;
	}
	return wss.str();
}

/**
 * Returns text representation of combined param value. Value is converted according to
 * given precision.
 * @param indmsw msw param index in table
 * @param indlsw lsw param index in table
 * @param prec param precision
 * @return text representation of param value, "unknown" if segment is not
 * attached or SZARP_NO_DATA occurred, it's up to user to free the memory
 */
std::wstring PTTParamProxy::getCombinedValue(int indmsw, int indlsw, int prec, PROBE_TYPE probe_type)
{
	int j, k;
	
	if ((indmsw < 0) || (indmsw >= (int)values[probe_type].size()) || (indlsw < 0) || (indlsw >= (int)values[probe_type].size()))
		return L"unknown";

	short sm = values[probe_type][indmsw];
	short sl = values[probe_type][indlsw];

	if ((prec > 7) || (sm == SZARP_NO_DATA) || (sl == SZARP_NO_DATA))
		return L"unknown";

	int i = int(((unsigned int) sm << 16) | (unsigned short)(sl));
	std::wstringstream wss;
	for (j = 0, k = 1; j < prec; j++, k*=10) ;
	if (k <= 1)
		wss << i;
	else {
		if (i < 0) 
			wss << L"-" << -i/k << L"." << std::setw(prec) << std::setfill(L'0') << -i%k;
		else
			wss << i/k << L"." << std::setw(prec) << std::setfill(L'0') << i%k;
	}
	return wss.str();
}

/**
 * Sets param value. Value is converted according to given precision. Also
 * updates values.
 * @param ind param index in table
 * @param prec param precision
 * @param value text representation of new param value
 * @return -1 on error, 0 on success
 */
int PTTParamProxy::setValue(int ind, int prec, const std::wstring &value)
{
	int shmd, semd;
	short *probes;
	struct sembuf Sem[4];
	struct shmid_ds shm_info;
	int val;

	if (str2val(value, prec, &val))
		return -1;
	values[VT_PROBE].resize(0);
    	if ((semd = semget(ftok(parcook_path, SEM_PARCOOK), SEM_LINE, 00666)) < 0) {
		sz_log(4, "cannot get parcook semaphore descriptor");
		return -1;
	}
	shmd = shmget(ftok(parcook_path, SHM_PROBE), 0, 00666);
	if (shmd < 0) {
		sz_log(4, "cannot get parcook memory segment descriptor");
		return -1;
	}
	
	if (shmctl(shmd, IPC_STAT, &shm_info) < 0) {
		sz_log(4, "cannot read parcook memory segment size");
		return -1;
	}
	int size = shm_info.shm_segsz / sizeof(short);
	if (size <= ind) {
		sz_log(1, "cannot set param value - index to large");
		return -1;
	}
	if ((probes = (short *) 
		shmat(shmd, (void *) 0, 0)) == (void *) -1){ 
		sz_log(4, "cannot attache parcook memory segment");
		return -1; 
	}

	values[VT_PROBE].resize(size);
	
	Sem[0].sem_flg = Sem[1].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_PROBE;
	Sem[0].sem_op = 0;
	Sem[1].sem_num = SEM_PROBE + 1;
	Sem[1].sem_op = 1;
	semop(semd, Sem, 2);
		
	probes[ind] = val;
	memcpy(&values[VT_PROBE][0], probes, size * sizeof(short));
		
	Sem[0].sem_flg = SEM_UNDO;
	Sem[0].sem_num = SEM_PROBE + 1;
	Sem[0].sem_op = -1;
	semop(semd, Sem, 1);
	
	shmdt((void *) probes);
	
	return 0;
}

/**
 * This function converts text value to integer param value according to given
 * precision.
 * @param text representation of value
 * @param prec precision (values grater then 4 are treated special)
 * @param val buffer for writing converted value
 * @return 0 on success, -1 on error
 */
int PTTParamProxy::str2val(const std::wstring &value, int prec, int *val)
{
	double num;
	int i;
	
	if (prec > 7)
		return -1;
	if (prec == 7) {
		if (value == L"Plus")
			*val = 1;
		else if (value ==  L"Minus")
			*val = -1;
		else
			return -1;
		return 0;
	}
	if (prec == 6) {
		if (value ==  L"Pochmurno")
			*val = 0;
		else if (value == L"Zmiennie")
			*val = 1;
		else if (value == L"S\u0142onecznie")
			*val = 2;
		else
			return -1;
		return 0;
	}
	if (prec == 5) {
		if (value == L"Tak")
			*val = 1;
		else if (value == L"Nie")
			*val = 0;
		else
			return -1;
		return 0;
	}

	std::wistringstream oss(value);
	oss >> num;

	if (oss.bad()) 
		return -1;
	if (isnan(num)) {
		*val = SZARP_NO_DATA;
	} else {
		for (i = prec; i > 0; i--, num *= 10);
		*val = (int) num;
	}
	return 0;
}

