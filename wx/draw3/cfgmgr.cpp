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
 * draw3
 * SZARP

* Pawe³ Pa³ucha pawel@praterm.com.pl
*
 * $Id$
 * Configuration manager.
 */

#include <functional>
#include <algorithm>

#include <assert.h>

#include <wx/settings.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/dir.h>
#include <wx/config.h>
#include <wx/tokenzr.h>

#include "fonts.h"

#include "ids.h"
#include "classes.h"

#include "cfgmgr.h"
#include "libpar.h"
#include "drawpsc.h"

#include "cconv.h"
#include "coobs.h"
#include "defcfg.h"
#include "dbmgr.h"
#include "splashscreen.h"
#include "drawapp.h"
#include "dcolors.h"
#include "errfrm.h"
#include "szarp_base_common/lua_strings_extract.h"
#include "xmlutils.h"


wxString DrawParam::GetBasePrefix() const {
	assert (m_param != NULL);
	return m_param->GetSzarpConfig()->GetPrefix().c_str();
}

wxString DrawParam::GetParamName() const {
	assert (m_param != NULL);
	return m_param->GetTranslatedName().c_str();
}

wxString DrawParam::GetShortName() {
	assert (m_param != NULL);
	return m_param->GetTranslatedShortName().c_str();
}

wxString DrawParam::GetDrawName() {
	assert (m_param != NULL);
	return m_param->GetTranslatedDrawName().c_str();
}

wxString DrawParam::GetUnit() {
	assert (m_param != NULL);
	return m_param->GetUnit().c_str();
}

wxString DrawParam::GetSumUnit() {
	return m_param->GetSumUnit().c_str();
}

const double& DrawParam::GetSumDivisor() {
	return m_param->GetSumDivisor();
}

int DrawParam::GetPrec() {
	assert (m_param != NULL);
	return m_param->GetPrec();
}

bool DrawParam::IsValid() const {
	return m_param != NULL;
}

TParam* DrawParam::GetIPKParam() {
	return m_param;
}
/** Converts hex digit character to its numerical value. */
int ctohex(int c)
{
	if (c >= 'A')
		return c - 'A' + 10;
	return c - '0';
}

DrawInfo::DrawInfo() {
	d = NULL;
	p = NULL;
	c = wxNullColour;
	avm = AVERAGE_VALUE_CALCULATION_AVERAGE;
}

DrawInfo::DrawInfo(TDraw *d, DrawParam *p)
{
	assert (d != NULL);
	assert (p != NULL);

	this->d = d;
	this->p = p;
	c = wxNullColour;
	avm = AVERAGE_VALUE_CALCULATION_AVERAGE;

	if (d->GetColor().empty())
		return;

	wxString s = d->GetColor().c_str();
	if (s.GetChar(0) != '#') {
		c = wxColour(s);
		if (!c.Ok()) {
			c = wxNullColour;
		}
		return;
	}
	if (s.Len() != 7)
		return;
	for (int i = 1; i < 7; i++)
		if (!isxdigit(s.GetChar(i)))
			return;

	int r = ctohex(s.GetChar(1)) * 16 + ctohex(s.GetChar(2));
	int g = ctohex(s.GetChar(3)) * 16 + ctohex(s.GetChar(4));
	int b = ctohex(s.GetChar(5)) * 16 + ctohex(s.GetChar(6));
	c = wxColour(r, g, b);
}

bool
DrawInfo::CompareDraws(DrawInfo *first, DrawInfo *second)
{
	if (first->d->GetOrder() == second->d->GetOrder())
		return false;

	if (first->d->GetOrder() > 0)
		return first->d->GetOrder() < second->d->GetOrder();

	return false;
}

wxString
DrawInfo::GetBasePrefix() const
{
	assert (p != NULL);
	return p->GetBasePrefix();
}

wxString DrawInfo::GetSetName()
{
	assert (d != NULL);
	return d->GetTranslatedWindow().c_str();
}

wxString
DrawInfo::GetName()
{
	assert (p != NULL);
	wxString name = p->GetDrawName().c_str();
	if (!name.IsEmpty())
		return name;

	name = GetParamName();

	int cp = name.Find(wxChar(':'), true);

	if (cp == -1)
		return name;

	return name.Mid(cp + 1);
}

wxString
DrawInfo::GetParamName() const
{
	assert (p != NULL);
	return p->GetParamName();
}

wxString
DrawInfo::GetShortName() {
	assert (p != NULL);
	return p->GetShortName();
}

DrawParam*
DrawInfo::GetParam() const
{
	return p;
}

TDraw *
DrawInfo::GetDraw()
{
	return d;
}

double
DrawInfo::GetMin() {
	assert (d != NULL);
	return d->GetMin();
}

double
DrawInfo::GetMax() {
	assert (d != NULL);
	return d->GetMax();
}

int
DrawInfo::GetScale() {
	assert (d != NULL);
	return d->GetScale();
}

double
DrawInfo::GetScaleMin() {
	assert (d != NULL);
	return d->GetScaleMin();
}

double
DrawInfo::GetScaleMax() {
	assert (d != NULL);
	return d->GetScaleMax();
}

TDraw::SPECIAL_TYPES
DrawInfo::GetSpecial() {
	assert (d != NULL);
	return d->GetSpecial();
}

wxColour
DrawInfo::GetDrawColor()
{
	return c;
}

double
DrawInfo::GetPrior()
{
	return (d->GetPrior());
}

void
DrawInfo::SetDrawColor(wxColour color)
{
	c = color;
}

int
DrawInfo::GetPrec() {
	return p->GetPrec();
}

wxString
DrawInfo::GetUnit() {
	assert (p != NULL);
	return p->GetUnit();
}

wxString
DrawInfo::GetSumUnit() {
	assert (p != NULL);
	return p->GetSumUnit();
}

const double&
DrawInfo::GetSumDivisor() {
	assert (p != NULL);
	return p->GetSumDivisor();
}

bool DrawInfo::IsValid() const {
	if (p == NULL || d == NULL) return false;
	return (p->IsValid());
}

wxString
DrawInfo::GetValueStr(const double &val, const wxString& no_data_str, const int prec) {
	if (p == NULL) {
		return no_data_str;
	}

	return p->GetIPKParam()->PrintValue(val, no_data_str.wc_str(), prec);
}

AverageValueCalculationMethod DrawInfo::GetAverageValueCalculationMethod() const {
	return avm;
}

void DrawInfo::SetAverageValueCalculationMethod(AverageValueCalculationMethod _avm) {
	avm = _avm;
}

IPKDrawInfo::IPKDrawInfo(TDraw *d, TParam *p, DrawsSets *_ds) :
	DrawInfo(d, new DrawParam(p)), ds(_ds)
{}

DrawsSets*
IPKDrawInfo::GetDrawsSets() {
	return ds;
}

DrawSet::DrawSet(const DrawSet& drawSet)
{
	m_prior = drawSet.m_prior;
	m_number = drawSet.m_number;

	m_name = drawSet.m_name;

	m_draws = drawSet.m_draws;
	m_cfg = drawSet.m_cfg;
}

DrawSet::~DrawSet()
{
	if (m_draws) {
		for (DrawInfoArray::iterator i = m_draws->begin();
		        i != m_draws->end();
		        i++)
			delete (*i);

		delete m_draws;
	}
}

DrawsSets* DrawSet::GetDrawsSets() {
	return m_cfg;
}

void
DrawSet::Add(DrawInfo* drawInfo)
{
	m_draws->push_back(drawInfo);

	/* update prior */
	double prior = drawInfo->GetPrior();
	if (prior > 0) {
		if (m_prior > 0)
			m_prior = m_prior > prior ? prior : m_prior;
		else
			m_prior = prior;
	}
}

bool
DrawInfo::isBadOrder() const
{
	return d->isBadOrder();
}

void
DrawSet::SortDraws()
{
	//vector for arguments with given order
	DrawInfoArray ordered;
	//vector for arguments without given order
	DrawInfoArray nonordered;

	for(auto it = m_draws->begin(); it != m_draws->end(); ++it)
	{
		if((*it)->isBadOrder())
		{
			nonordered.push_back(*it);
		}
		else
		{
			ordered.push_back(*it);
		}
	}
	//we only sort arguments with given order. The rest of them will be added to the end, with order equal to that in file params.xml
	std::stable_sort(ordered.begin(), ordered.end(), DrawInfo::CompareDraws);
	m_draws->clear();
	m_draws->insert(m_draws->end(), ordered.begin(), ordered.end());
	m_draws->insert(m_draws->end(), nonordered.begin(), nonordered.end());
}

int
DrawSet::CompareSets(DrawSet * d1, DrawSet * d2)
{
	double p1 = d1->GetPrior();
	double p2 = d2->GetPrior();

	if (p1 < defined_draws_prior_start) {
		if (p2 < defined_draws_prior_start) {
			p1 = fabs(p1);
			p2 = fabs(p2);
		} else
			return 1;
	}

	if (p2 < defined_draws_prior_start)
		return -1;

	if (p1 < 0)
		p1 = unassigned_prior_value;

	if (p2 < 0)
		p2 = unassigned_prior_value;

	if (p2 == p1)
		return d1->GetNumber() - d2->GetNumber();


	return (p1 > p2 ? 1 : -1);
}

double
DrawSet::GetPrior()
{
	double prior = -1;

	/* find smallest positive prior */
	for (size_t j = 0; j < m_draws->size(); j++) {
		double current_prior = m_draws->at(j)->GetPrior();
		if ( current_prior > 0 && ( current_prior < prior || prior <= 0 ) )
			prior = current_prior;
	}

	m_prior = prior;

	return m_prior;
}

wxString &
DrawSet::GetName()
{
	if (!m_name.IsEmpty())
		return m_name;

	if (m_draws->size() == 0) {
		return m_name;
	}

	m_name = m_draws->at(0)->GetSetName();

	return m_name;
}

void
DrawSet::SetName(const wxString & name) {
	m_name = name;
};

void
DrawSet::InitNullColors()
{
	size_t first_free = 0;

	wxLogInfo(_T("InitNullColors: draws count: %zu"), m_draws->size());

	std::set<size_t> ex;

	for (size_t i = 0; i < m_draws->size(); i++) {
		if (m_draws->at(i)->GetDrawColor() == wxNullColour)
			continue;
		int j = DrawDefaultColors::FindIndex(m_draws->at(i)->GetDrawColor());
		if (j != -1)
			ex.insert(j);
	}

	/* now assign null colors */
	for (size_t i = 0; i < m_draws->size(); i++) {
		if (m_draws->at(i)->GetDrawColor() == wxNullColour) {
			/* search for next free color */
			while (ex.find(first_free) != ex.end()) {
				first_free++;
			}
			ex.insert(first_free);

			/* set color */
			m_draws->at(i)->SetDrawColor(DrawDefaultColors::MakeColor(first_free));
		}
	}
}

DrawInfo *
DrawSet::GetDraw(int index)
{
	if (index < (int) m_draws->size())
		return m_draws->at(index);
	else
		return NULL;
}

wxString
DrawSet::GetDrawName(int index)
{
	if (index < (int) m_draws->size())
		return m_draws->at(index)->GetName();
	else
		return wxEmptyString;
}

wxString
DrawSet::GetParamName(int index) const
{
	if (index < (int) m_draws->size())
		return m_draws->at(index)->GetParamName();
	else
		return wxEmptyString;
}

wxColour
DrawSet::GetDrawColor(int index)
{
	if (index >= (int) m_draws->size())
		return DRAW3_BG_COLOR;

	wxColour col = m_draws->at(index)->GetDrawColor();

	if (col == wxNullColour) {
		InitNullColors();
		col = m_draws->at(index)->GetDrawColor();
	}

	return col;
}


const double DrawSet::unassigned_prior_value = std::numeric_limits<double>::max() / 4 * 3;

const double DrawSet::defined_draws_prior_start = -2;

ConfigManager::ConfigManager(wxString szarp_data_dir, IPKContainer *ipks, const wxString &prefix) : m_szarp_data_dir(szarp_data_dir), m_ipks(ipks), splashscreen(NULL), m_base_prefix(prefix)
{
	m_defined_sets = NULL;

	LoadDefinedDrawsSets();

	psc = new DrawPsc(szarp_data_dir);
}

bool ConfigManager::IsPSC(wxString prefix) {
	return psc->IsSettable(prefix);
}

void ConfigManager::EditPSC(wxString prefix, wxString param) {
	psc->Edit(prefix, param);
}

DefinedDrawsSets* ConfigManager::GetDefinedDrawsSets() {
	return m_defined_sets;
}

DrawsSets *
ConfigManager::LoadConfig(const wxString& prefix, const wxString &config_path, bool logparams )
{
	if(splashscreen != NULL) {
		wxString msg = _("Loading configuration: ");
		msg += prefix;
		splashscreen->PushStatusText(msg);
	}

	m_logparams = logparams;

	TSzarpConfig *ipk = NULL;
	if (config_path == wxEmptyString)
		ipk = m_ipks->LoadConfig(prefix.wc_str(),std::wstring(),logparams);


	DrawsSets* ret;
	if (ipk == NULL)
		ret = NULL;
	else {
		ret = AddConfig(ipk);
		FinishConfigurationLoad(prefix);
	}

	if (splashscreen != NULL) {
		splashscreen->PopStatusText();
	}

	return ret;
}

DrawInfo *
ConfigManager::GetDraw(const wxString prefix, const wxString set, int index)
{
	return config_hash[prefix]->GetDrawsSets()[set]->GetDraw(index);
}


wxString
ConfigManager::GetSzarpDir() const
{
	wxString folder_path = m_szarp_data_dir + m_base_prefix;
	return folder_path + '/';
}

wxString
IPKConfig::GetID()
{
	return m_id;
}

wxString
IPKConfig::GetPrefix() const
{
	return m_sc->GetPrefix().c_str();
}


IPKConfig::IPKConfig(TSzarpConfig *c, ConfigManager *mgr) : DrawsSets(mgr), defined_attached(false)
{
	assert (c != NULL);

	m_sc = c;

	m_id = c->GetTranslatedTitle();

	int drawset_number = 0;
	DrawSet *pds;
	DrawInfo *pdi;
	wxString name;

	for (TParam *p = m_sc->GetFirstParam(); p; p = m_sc->GetNextParam(p)) {
		for (TDraw *d = p->GetDraws(); d; d = d->GetNext()) {
			pdi = new IPKDrawInfo(d, p, this);

			name = d->GetTranslatedWindow().c_str();

			pds = drawSets[name];

			if (NULL == pds)
				pds = new DrawSet(name, drawset_number++, this);

			pds->Add(pdi);

			drawSets[name] = pds;
			m_tree_root.AddSet(d, pds);
		}
	}

	DrawSetsHash::iterator it;
	/* sort draws */
	for (it = drawSets.begin(); it != drawSets.end(); ++it) {
		it->second->SortDraws();
	}

	/*init null colors*/
	for (it = drawSets.begin(); it != drawSets.end(); ++it)
		it->second->InitNullColors();

	baseDrawSets = drawSets;

	m_tree_root.Sort();
}

void IPKConfig::AttachDefinedSet(DefinedDrawSet* set) {
	DrawSet* ds = set->MakeShallowCopy(this);
	drawSets[set->GetName()] = ds;
	m_tree_root.AddUserSet(ds);
}

void IPKConfig::AttachDefined() {
	if (defined_attached)
		return;

	DefinedDrawsSets *d = m_cfgmgr->GetDefinedDrawsSets();

	for (DrawSetsHash::iterator i = d->GetRawDrawsSets().begin();
		        i != d->GetRawDrawsSets().end();
		        i++) {
		DefinedDrawSet *df = dynamic_cast<DefinedDrawSet*>(i->second);
		SetsNrHash& pc = df->GetPrefixes();

		if (pc.find(GetPrefix()) == pc.end())
			continue;

		AttachDefinedSet(df);

	}
	defined_attached = true;
}

DrawSetsHash&
IPKConfig::GetDrawsSets(bool all) {
	if (all) {
		AttachDefined();
		return drawSets;
	} else
		return baseDrawSets;

}

DrawSetsHash&
IPKConfig::GetRawDrawsSets() {

	return drawSets;
}

IPKConfig::~IPKConfig()
{

	DrawSetsHash::iterator it = baseDrawSets.begin();
	for (; it != baseDrawSets.end(); it++) {
		delete it->second;
	}

}

bool DrawTreeNodeCmp(const DrawTreeNode* n1, const DrawTreeNode* n2) {
	double p1 = n1->GetPrior();
	double p2 = n2->GetPrior();
	if (p1 < 0)
		if (p2 < 0)
			return p1 > p2;
		else
			return false;
	else if (p2 < 0)
		return true;
	else
		return p1 < p2;
}


class DrawTreeNodeNameEq {
	const wxString& m_name;
public:
	DrawTreeNodeNameEq(const wxString& name) : m_name(name) {}
	bool operator()(const DrawTreeNode* node) const {
		return node->GetName() == m_name;
	}
};

DrawTreeNode::DrawTreeNode() {
	m_prior = -1;
	m_elemet_type = NODE;
	m_child_set = NULL;
}

void DrawTreeNode::Sort() {
	std::stable_sort(m_child_nodev.begin(), m_child_nodev.end(), DrawTreeNodeCmp);
	std::for_each(m_child_nodev.begin(), m_child_nodev.end(), std::mem_fun(&DrawTreeNode::Sort));
}

wxString DrawTreeNode::GetName() const {
	return m_name;
}

double DrawTreeNode::GetPrior() const {
	return m_prior;
}

const std::vector<DrawTreeNode*>& DrawTreeNode::GetChildren() const {
	return m_child_nodev;
}

DrawSet* DrawTreeNode::GetDrawSet() const {
	return m_child_set;
}

DrawTreeNode::ELEMENT_TYPE DrawTreeNode::GetElementType() const {
	return m_elemet_type;
}

DrawTreeNode::~DrawTreeNode() {
	for (std::vector<DrawTreeNode*>::iterator i = m_child_nodev.begin();
		i != m_child_nodev.end();
		i++)
		delete *i;
}

DrawTreeRoot::DrawTreeRoot() {
	m_user_subtree = NULL;
}

DrawTreeNode* DrawTreeRoot::AddTNode(TTreeNode *tnode) {
	if (tnode == NULL)
		return this;
	DrawTreeNode* pnode = AddTNode(tnode->GetParent());
	std::vector<DrawTreeNode*>::iterator i =
	    std::find_if(pnode->m_child_nodev.begin(), pnode->m_child_nodev.end(), DrawTreeNodeNameEq(tnode->GetName()));
	DrawTreeNode* node;
	if (i == pnode->m_child_nodev.end()) {
		node = new DrawTreeNode;
		node->m_name = tnode->GetName();
		node->m_prior = tnode->GetPrior();
		node->m_child_set = NULL;
		node->m_elemet_type = NODE;
		pnode->m_child_nodev.push_back(node);
	} else {
		node = *i;
	}
	return node;
}

void DrawTreeRoot::AddSet(TDraw* tdraw, DrawSet *drawsset) {
	const std::vector<TTreeNode*>& tnodev = tdraw->GetTreeNode();
	for (std::vector<TTreeNode*>::const_iterator i = tnodev.begin();
		i != tnodev.end();
		i++) {
		DrawTreeNode* pnode = AddTNode(*i);
		if (pnode->GetElementType() == DrawTreeNode::LEAF)
			//already present
			continue;
		DrawTreeNode* node = new DrawTreeNode();
		node->m_name = drawsset->GetName();
		node->m_prior = (*i)->GetDrawPrior();
		node->m_child_set = drawsset;
		node->m_elemet_type = LEAF;
		pnode->m_child_nodev.push_back(node);
	}
}

void DrawTreeRoot::AddUserSet(DrawSet* _draw_set) {
	DefinedDrawSet* draw_set = dynamic_cast<DefinedDrawSet*>(_draw_set);
	if (m_user_subtree == NULL) {
		m_user_subtree = new DrawTreeNode();
		m_user_subtree->m_name = _("Users' sets");
		m_user_subtree->m_prior = -2;
		m_user_subtree->m_child_set = NULL;
		m_user_subtree->m_elemet_type = NODE;
		m_child_nodev.push_back(m_user_subtree);
	}

	bool network = draw_set->IsNetworkSet();
	wxString name;
	if (network)
		name = draw_set->GetUserName();
	else
		name = _("Local sets");

	DrawTreeNode *node = NULL;
	std::vector<DrawTreeNode*>::iterator i =
		std::find_if(m_user_subtree->m_child_nodev.begin(), m_user_subtree->m_child_nodev.end(), DrawTreeNodeNameEq(name));
	if (i != m_user_subtree->m_child_nodev.end()) {
		node = *i;
	} else {
		node = new DrawTreeNode();
		node->m_name = name;
		node->m_prior = network ? node->m_child_nodev.size() + 1 : 0;
		node->m_child_set = NULL;
		node->m_elemet_type = NODE;
		m_user_subtree->m_child_nodev.push_back(node);
		m_user_subtree->Sort();
	}

	DrawTreeNode* snode = new DrawTreeNode();
	snode->m_name = draw_set->GetName();
	snode->m_prior = node->m_child_nodev.size() + 1;
	snode->m_child_set = draw_set;
	snode->m_elemet_type = LEAF;

	node->m_child_nodev.push_back(snode);
}

void DrawTreeRoot::RemoveUserSet(wxString name, DrawSet *_set) {
	DefinedDrawSet* draw_set = dynamic_cast<DefinedDrawSet*>(_set);
	if (m_user_subtree == NULL || draw_set == NULL)
		return;

	wxString nname;
	if (draw_set->IsNetworkSet())
		nname = draw_set->GetUserName();
	else
		nname = _("Local sets");

	std::vector<DrawTreeNode*>::iterator i;
	i = std::find_if(m_user_subtree->m_child_nodev.begin(), m_user_subtree->m_child_nodev.end(), DrawTreeNodeNameEq(nname));
	if (i == m_user_subtree->m_child_nodev.end())
		return;

	std::vector<DrawTreeNode*>::iterator j;
	j = std::find_if((*i)->m_child_nodev.begin(), (*i)->m_child_nodev.end(), DrawTreeNodeNameEq(name));
	if (j != (*i)->m_child_nodev.end()) {
		delete *j;
		(*i)->m_child_nodev.erase(j);
	}
}

void DrawTreeRoot::RenameUserSet(wxString oname, DrawSet *set) {
	DefinedDrawSet* draw_set = dynamic_cast<DefinedDrawSet*>(set);
	if (m_user_subtree == NULL || set == NULL)
		return;

	wxString nname;
	if (draw_set->IsNetworkSet())
		nname = draw_set->GetUserName();
	else
		nname = _("Local sets");

	std::vector<DrawTreeNode*>::iterator i;
	i = std::find_if(m_user_subtree->m_child_nodev.begin(), m_user_subtree->m_child_nodev.end(), DrawTreeNodeNameEq(nname));
	if (i == m_user_subtree->m_child_nodev.end())
		return;

	std::vector<DrawTreeNode*>::iterator j;
	j = std::find_if((*i)->m_child_nodev.begin(), (*i)->m_child_nodev.end(), DrawTreeNodeNameEq(oname));
	if (j != (*i)->m_child_nodev.end()) {
		(*j)->m_name = set->GetName();
		(*j)->m_child_set = set;
	}
}

void DrawTreeRoot::SubstituteUserSet(wxString oname, DrawSet *set) {
	DefinedDrawSet* draw_set = dynamic_cast<DefinedDrawSet*>(set);
	if (m_user_subtree == NULL || set == NULL)
		return;

	wxString nname;
	if (draw_set->IsNetworkSet())
		nname = draw_set->GetUserName();
	else
		nname = _("Local sets");

	std::vector<DrawTreeNode*>::iterator i;
	i = std::find_if(m_user_subtree->m_child_nodev.begin(), m_user_subtree->m_child_nodev.end(), DrawTreeNodeNameEq(nname));
	if (i == m_user_subtree->m_child_nodev.end())
		return;

	std::vector<DrawTreeNode*>::iterator j;
	j = std::find_if((*i)->m_child_nodev.begin(), (*i)->m_child_nodev.end(), DrawTreeNodeNameEq(oname));
	if (j != (*i)->m_child_nodev.end()) {
		(*j)->m_name = set->GetName();
		(*j)->m_child_set = set;
	}
}


DrawsSets::DrawsSets(ConfigManager *cfg) : m_cfgmgr(cfg) {}

SortedSetsArray
DrawsSets::GetSortedDrawSetsNames(bool all)
{
	AttachDefined();
	SortedSetsArray sorted(DrawSet::CompareSets);

	for (DrawSetsHash::iterator it = GetDrawsSets(all).begin(); it != GetDrawsSets(all).end(); it++) {
		sorted.Add(it->second);
	}

	return sorted;
}


DrawsSets::~DrawsSets() {}

ConfigManager*
DrawsSets::GetParentManager()
{
	return m_cfgmgr;
}

DrawTreeNode& DrawsSets::GetDrawTree() {
	return m_tree_root;
}


void DrawsSets::AddUserSet(DefinedDrawSet *s) {
	GetRawDrawsSets()[s->GetName()] = s;
	m_tree_root.AddUserSet(s);
}

void DrawsSets::RemoveUserSet(wxString name) {
	m_tree_root.RemoveUserSet(name, GetDrawsSets()[name]);
	GetRawDrawsSets().erase(name);
}

void DrawsSets::RenameUserSet(wxString oname, DefinedDrawSet *set) {
	GetRawDrawsSets().erase(oname);
	GetRawDrawsSets()[set->GetName()] = set;
	m_tree_root.RenameUserSet(oname, set);
}

void DrawsSets::SubstituteUserSet(wxString oname, DefinedDrawSet *set) {
	GetRawDrawsSets().erase(oname);
	GetRawDrawsSets()[set->GetName()] = set;
	m_tree_root.SubstituteUserSet(oname, set);
}


DrawsSets*
ConfigManager::AddConfig(TSzarpConfig *ipk)
{
	wxString prefix = ipk->GetPrefix().c_str();

	if (config_hash.find(prefix) != config_hash.end())
		delete config_hash[prefix];

	IPKConfig *c = new IPKConfig(ipk, this);

	config_hash[prefix] = c;

	return c;
}

DrawsSets*
ConfigManager::GetConfigByPrefix(const wxString& prefix, bool load)
{
	DrawsSetsHash::iterator i = config_hash.find(prefix);
	if (i == config_hash.end())
		if (load)
			return LoadConfig(prefix);
		else
			return NULL;
	else
		return i->second;

}

DrawsSets*
ConfigManager::GetConfig(const wxString& id)
{
	DrawsSets *c = NULL;
	for (DrawsSetsHash::iterator i = config_hash.begin(); i != config_hash.end(); ++i)
		if (i->second->GetID() == id) {
			c = i->second;
			break;
		}

	return c;
}

DrawsSetsHash& ConfigManager::GetConfigurations() {
	return config_hash;
}

ConfigNameHash& ConfigManager::GetConfigTitles() {
	wxArrayString hidden_databases;
	wxString tmp;
	wxConfigBase* config = wxConfigBase::Get(true);
	if (config->Read(_T("HIDDEN_DATABASES"), &tmp))
	{
		wxStringTokenizer tkz(tmp, _T(","), wxTOKEN_STRTOK);
		while (tkz.HasMoreTokens())
		{
			wxString token = tkz.GetNextToken();
			token.Trim();
			if (!token.IsEmpty())
				hidden_databases.Add(token);
		}
	}

	if (m_config_names.empty()) {
		m_config_names = ::GetConfigTitles(m_szarp_data_dir, &hidden_databases);
		m_config_names[DefinedDrawsSets::DEF_PREFIX]
		= wxGetTranslation(DefinedDrawsSets::DEF_NAME);
	}

	return m_config_names;
}

wxArrayString ConfigManager::GetPrefixes() {

	wxArrayString result;

	wxDir dir(m_szarp_data_dir);
	wxString path;

	if (dir.GetFirst(&path, wxEmptyString, wxDIR_DIRS))
		do
			if (wxFile::Exists(m_szarp_data_dir+ path + _T("/config/params.xml")))
				result.Add(path);
		while (dir.GetNext(&path));

	return result;

}

void ConfigManager::RegisterConfigObserver(ConfigObserver *observer) {
	m_observers.push_back(observer);
}

void ConfigManager::DeregisterConfigObserver(ConfigObserver *observer) {
	std::vector<ConfigObserver*>::iterator i;
	i = std::remove(m_observers.begin(),
		m_observers.end(),
		observer);
	m_observers.erase(i, m_observers.end());
}

void ConfigManager::NotifySetRemoved(wxString prefix, wxString name) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
		i != m_observers.end();
		i++)
		(*i)->SetRemoved(prefix, name);
}

void ConfigManager::NotifyStartConfigurationReload(wxString prefix) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			i++)
		(*i)->ConfigurationIsAboutToReload(prefix);
}

void ConfigManager::NotifyEndConfigurationReload(wxString prefix) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			i++)
		(*i)->ConfigurationWasReloaded(prefix);
}

void ConfigManager::NotifySetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			i++)
		(*i)->SetRenamed(prefix, from, to, set);
}

void ConfigManager::NotifySetModified(wxString prefix, wxString name, DrawSet *set) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			i++)
		(*i)->SetModified(prefix, name, set);
}

void ConfigManager::NotifyParamSubsitute(DefinedParam *d, DefinedParam *n) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			i++)
		(*i)->ParamSubstituted(d, n);
}

void ConfigManager::NotifyParamDestroy(DefinedParam *d) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			i++)
		(*i)->ParamDestroyed(d);
}

void ConfigManager::NotifySetAdded(wxString prefix, wxString name, DrawSet *set) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			i++)
		(*i)->SetAdded(prefix, name, set);
}

void ConfigManager::SetDatabaseManager(DatabaseManager *db_mgr) {
	m_db_mgr = db_mgr;
	m_db_mgr->AddParams(m_defined_sets->GetDefinedParams());
}

bool ConfigManager::ReloadConfiguration(wxString prefix) {
	wxCriticalSectionLocker locker(m_reload_config_CS);
	std::wstring wprefix = prefix.wc_str();
	if (m_ipks->ReadyConfigurationForLoad(wprefix,m_logparams) == false)
		return false;
	m_db_mgr->InitiateConfigurationReload(prefix);

	return true;
}

void ConfigManager::FinishConfigurationLoad(wxString prefix) {
	std::set<wxString> configs_to_load;

	m_db_mgr->ConfigurationReloadFinished(prefix);
	m_defined_sets->ConfigurationWasLoaded(prefix, &configs_to_load);

	for (std::set<wxString>::iterator i = configs_to_load.begin();
		i != configs_to_load.end();
		i++) {

		if (config_hash.find(*i) != config_hash.end())
			continue;

		wxLogInfo(_T("Load of configuration %s entails loading configuration %s"), prefix.c_str(), (*i).c_str());
		if (LoadConfig(*i) == NULL)
			m_defined_sets->RemoveAllRelatedToPrefix(*i);
	}

	psc->ConfigurationLoaded(dynamic_cast<IPKConfig*>(config_hash[prefix]));
}


void ConfigManager::ConfigurationReadyForLoad(wxString prefix) {
	NotifyStartConfigurationReload(prefix);

	TSzarpConfig *cfg = m_ipks->LoadConfig(prefix.wc_str(), std::wstring());
	AddConfig(cfg);
	FinishConfigurationLoad(prefix);

	NotifyEndConfigurationReload(prefix);
}

void ConfigManager::LoadDefinedDrawsSets() {
	if (m_defined_sets)
		return;

	m_defined_sets = new DefinedDrawsSets(this);

	wxFileName sd(wxGetHomeDir(), wxEmptyString);
	sd.AppendDir(_T(".szarp"));
	wxFileName p(sd.GetPath(), _T("defined.xml"));

	if (p.FileExists()) {
		m_defined_sets->LoadSets(p.GetFullPath());
		config_hash[m_defined_sets->GetPrefix()] = m_defined_sets;
	}

}

bool ConfigManager::SaveDefinedDrawsSets() {
	if (m_defined_sets == NULL)
		return true;

	wxFileName sd(wxGetHomeDir(), wxEmptyString);
	sd.AppendDir(_T(".szarp"));
	if (!wxDirExists(sd.GetFullPath()))
		if (sd.Mkdir() == false) {
			wxMessageBox(wxString::Format(_("Failed to create directory %s. Defined draws cannot be saved"), sd.GetFullPath().c_str()),
				_("Error"),
				wxOK | wxICON_ERROR,
				wxGetApp().GetTopWindow());
			return false;
		}

	wxFileName p(sd.GetPath(), _T("defined.xml"));

	m_defined_sets->SaveSets(p.GetFullPath());

	return true;

}

bool ConfigManager::RemoveDefinedParam(DefinedParam *p) {
	DrawSetsHash& dsh = m_defined_sets->GetRawDrawsSets();
	std::vector<std::pair<DefinedDrawSet*, DefinedDrawInfo*> > ddiv;

	for (DrawSetsHash::iterator i = dsh.begin();
			i != dsh.end();
			i++) {
		DefinedDrawSet* ds = dynamic_cast<DefinedDrawSet*>(i->second);
		assert(ds);
		DrawInfoArray* dia = ds->GetDraws();
		for (size_t j = 0; j < dia->size(); j++) {
			DefinedDrawInfo *dp = dynamic_cast<DefinedDrawInfo*>((*dia)[j]);
			assert(dp);

			if (dp->GetParam() == p)
				ddiv.push_back(std::make_pair(ds, dp));
		}
	}

	m_db_mgr->RemoveParams(std::vector<DefinedParam*>(1, p));
	bool deleted = m_defined_sets->RemoveParam(p);
	m_defined_sets->GetParentManager()->NotifyParamDestroy(p);

	delete p;

	for (std::vector<std::pair<DefinedDrawSet*, DefinedDrawInfo*> >::iterator i = ddiv.begin();
			i != ddiv.end();
			i++) {
		i->second->SetValid(false);

		std::vector<DefinedDrawSet*>* c = i->first->GetCopies();
		for (std::vector<DefinedDrawSet*>::iterator j = c->begin(); j != c->end(); j++)
			NotifySetModified((*j)->GetDrawsSets()->GetPrefix(), (*j)->GetName(), *j);
	}

	return deleted;
}

bool ConfigManager::RemoveDefinedParam(wxString prefix, wxString name) {
	std::vector<DefinedParam*>& dps = m_defined_sets->GetDefinedParams();

	std::vector<DefinedParam*>::iterator i;
	for (i = dps.begin(); i != dps.end(); i++)
		if ((*i)->GetBasePrefix() == prefix && (*i)->GetParamName() == name) {
			break;
	}

	if (i != dps.end())
		return RemoveDefinedParam(*i);
	else
		return false;
}

void ConfigManager::RemoveDefinedParams(std::vector<DefinedParam*>& dp) {
	for (std::vector<DefinedParam*>::iterator i = dp.begin();
			i != dp.end();
			i++)
		RemoveDefinedParam((*i)->GetBasePrefix(), (*i)->GetParamName());
}

bool ConfigManager::SubstituteOrAddDefinedParams(const std::vector<DefinedParam*>& dp) {
	std::vector<DefinedParam*> to_rem, to_add, new_pars;
	std::vector<DefinedParam*>& dps = m_defined_sets->GetDefinedParams();
	for (std::vector<DefinedParam*>::const_iterator i = dp.begin();
			i != dp.end();
			i++) {
		std::vector<DefinedParam*>::iterator j;
		for (j = dps.begin(); j != dps.end(); j++)
			if ((*j)->GetBasePrefix() == (*i)->GetBasePrefix() && (*j)->GetParamName() == (*i)->GetParamName()) {
				if (!((*i)->IsNetworkParam() && (*j)->IsNetworkParam() && (*i)->GetModificationTime() == (*j)->GetModificationTime())) {
					to_rem.push_back(*j);
					to_add.push_back(*i);
				}
				break;
			}
		if (j == dps.end())
			new_pars.push_back(*i);
	}
	dps.insert(dps.end(), new_pars.begin(), new_pars.end());
	m_db_mgr->AddParams(new_pars);
	SubstiuteDefinedParams(to_rem, to_add);

	return to_add.size() > 0 || new_pars.size() > 0;
}

void ConfigManager::SubstiuteDefinedParams(const std::vector<DefinedParam*>& to_rem, const std::vector<DefinedParam*>& to_add) {

	std::map<wxString, bool> us;

	DrawSetsHash& dsh = m_defined_sets->GetRawDrawsSets();
	for (size_t i = 0; i < to_rem.size(); i++)
		for (DrawSetsHash::iterator j = dsh.begin();
			j != dsh.end();
			j++) {
			DrawInfoArray* dia = j->second->GetDraws();
			for (size_t k = 0; k < dia->size(); k++) {
				DefinedDrawInfo *dp = dynamic_cast<DefinedDrawInfo*>((*dia)[k]);
				if (dp->GetParam() == to_rem[i]) {
					dp->SetParam(to_add[i]);
					us[j->first] = true;
				}
			}
		}

	m_db_mgr->RemoveParams(to_rem);
	m_db_mgr->AddParams(to_add);

	for (std::map<wxString, bool>::iterator i = us.begin();
			i != us.end();
			++i) {
		DefinedDrawSet *ds = dynamic_cast<DefinedDrawSet*>(dsh[i->first]);
		std::vector<DefinedDrawSet*>* c = ds->GetCopies();
		for (std::vector<DefinedDrawSet*>::iterator j = c->begin(); j != c->end(); j++)
			NotifySetModified((*j)->GetDrawsSets()->GetPrefix(), i->first, *j);
	}

	for (size_t i = 0; i < to_rem.size(); i++) {
		m_defined_sets->RemoveParam(to_rem[i]);
		m_defined_sets->AddDefinedParam(to_add[i]);
		NotifyParamSubsitute(to_rem[i], to_add[i]);
		delete to_rem[i];
	}

}

void ConfigManager::DrawInfoAverageValueCalculationChanged(DrawInfo *d) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			i++)
		(*i)->DrawInfoAverageValueCalculationChanged(d);
}

class ExportImportSet {
	ConfigManager *m_cfg_mgr;
	DatabaseManager *m_db_mgr;
	DefinedDrawsSets *m_def_sets;
	DefinedParam* FindDefinedParam(wxString gname);
	void ConvertParam(DefinedParam *dp, wxString our_name, std::map<DefinedParam*, DefinedParam*>& converted);
	bool FindSetName(DefinedDrawSet *ds);
public:
	ExportImportSet(ConfigManager *cfg_mgr, DatabaseManager *db_mgr, DefinedDrawsSets *def_sets) :
		m_cfg_mgr(cfg_mgr), m_db_mgr(db_mgr), m_def_sets(def_sets) {}

	void ExportSet(DefinedDrawSet *set, wxString our_name);
	void ImportSet();
};

DefinedParam* ExportImportSet::FindDefinedParam(wxString gname) {
	const std::vector<DefinedParam*>& dps = m_def_sets->GetDefinedParams();
	for (size_t i = 0; i < dps.size(); i++) {
		if (dps[i]->GetBasePrefix() + _T(":") + dps[i]->GetParamName() == gname)
			return dps[i];
	}
	return NULL;
}

void ExportImportSet::ConvertParam(DefinedParam *dp, wxString our_name, std::map<DefinedParam*, DefinedParam*>& converted) {
	if (converted.find(dp) != converted.end())
		return;
	wxString pname = dp->GetParamName();
	DefinedParam *np = new DefinedParam(*dp);
	converted[dp] = np;
	if (pname.StartsWith(_("User:Param:")) == false)
		return;
	wxString nname = pname.BeforeFirst(_T(':')) + _T(":") + our_name + _T(":") + pname.AfterLast(_T(':'));
	np->SetParamName(nname);
	wxString formula = dp->GetFormula();
#ifdef LUA_PARAM_OPTIMISE
	std::vector<std::wstring> strings;
	extract_strings_from_formula(formula.wc_str(), strings);
	for (size_t i = 0; i < strings.size(); i++) if (DefinedParam* d = FindDefinedParam(strings[i])) {
			ConvertParam(d, our_name, converted);
			formula.Replace(_T("\"") + d->GetBasePrefix() + _T(":") + d->GetParamName() + _T("\""),
			_T("\"") + converted[d]->GetBasePrefix() + _T(":") + converted[d]->GetParamName() + _T("\""));
		}
#endif /* LUA_PARAM_OPTIMISE */
	np->SetFormula(formula);
}

void ExportImportSet::ExportSet(DefinedDrawSet *set, wxString our_name) {
	wxFileDialog dlg(wxGetApp().GetTopWindow(), _("Choose a file"), _T(""), _T(""),
		_("SZARP draw set (*.xds)|*.xds|All files (*.*)|*.*"),
		wxFD_SAVE | wxFD_CHANGE_DIR);
	szSetDefFont(&dlg);
	if (dlg.ShowModal() != wxID_OK)
		return;
	wxString path = dlg.GetPath();
	if (path.Right(4).Find('.') < 0)
		path += _T(".xds");

	DefinedDrawSet *dset = set->MakeDeepCopy();
	std::vector<DefinedDrawSet*> dsv;
	dsv.push_back(dset);

	std::map<DefinedParam*, DefinedParam*> converted_params;
	DrawInfoArray* dia = dset->GetDraws();
	for (size_t i = 0; i < dia->size(); i++) {
		DefinedDrawInfo* di = dynamic_cast<DefinedDrawInfo*>((*dia)[i]);
		DrawParam* drp = di->GetParam();
		if (DefinedParam* dep = dynamic_cast<DefinedParam*>(drp)) {
			ConvertParam(dep, our_name, converted_params);
			di->SetParamName(converted_params[dep]->GetParamName());
		}
	}
	std::vector<DefinedParam*> dpv;
	for (std::map<DefinedParam*, DefinedParam*>::iterator i = converted_params.begin();
		i != converted_params.end();
		i++)
		dpv.push_back(i->second);

	if (m_def_sets->SaveSets(path, dsv, dpv) == false)
		wxMessageBox(wxString::Format(_("Export of set to file: %s failed"), path.c_str()), _("Error!"), wxICON_ERROR, wxGetApp().GetTopWindow());
	for (size_t i = 0; i < dpv.size(); i++)
		delete dpv[i];
	delete dset;
}

bool ExportImportSet::FindSetName(DefinedDrawSet *ds) {
	DrawSetsHash& dh = m_def_sets->GetRawDrawsSets();
	wxString title = ds->GetName();
	while (dh.find(title) != dh.end()) {
		int n = 0;
		wxString ntitle = title;
		while (dh.find(ntitle) != dh.end()) {
			if (n == 0)
				ntitle = wxString::Format(_("%s (%s)"), title.c_str(), _("imported"));
			else
				ntitle = wxString::Format(_("%s (%s <%d>)"), title.c_str(), _("imported"), n);
			n += 1;
		}
		title = wxGetTextFromUser(
			wxString::Format(_("Set with name %s already exists. Give a name for a set:"), title.c_str()),
			_("Set already exists"),
			ntitle,
			wxGetApp().GetTopWindow());
		if (title.IsEmpty())
			return false;
	}
	ds->SetName(title);
	return true;
}

void ExportImportSet::ImportSet() {
	wxFileDialog dlg(wxGetApp().GetTopWindow(), _("Choose a file"), _T(""), _T(""),
		_("SZARP draw set (*.xds)|*.xds|All files (*.*)|*.*"));
	szSetDefFont(&dlg);
	if (dlg.ShowModal() != wxID_OK)
		return;
	wxString path = dlg.GetPath();
	std::vector<DefinedDrawSet*> draw_sets;
	std::vector<DefinedParam*> defined_params;
	try {
		m_def_sets->LoadSets(path, draw_sets, defined_params);

		m_cfg_mgr->SubstituteOrAddDefinedParams(defined_params);
		for (size_t i = 0; i < draw_sets.size(); i++) {
			std::vector<wxString> removed;
			DefinedDrawSet* ds = draw_sets[i];
			SetsNrHash& prefixes = ds->GetPrefixes();

			for (SetsNrHash::iterator i = prefixes.begin(); i != prefixes.end(); i++) {
				DrawsSetsHash& config_hash = m_cfg_mgr->GetConfigurations();
				if (config_hash.find(i->first) != config_hash.end())
					continue;

				if (m_cfg_mgr->GetConfigByPrefix(i->first) == NULL) {
					throw DrawImportException(wxString::Format(_("Failed to load configuration: %s"), i->first.c_str()));
				}

				ErrorFrame::NotifyError(wxString::Format(_("Configuration for prefix: %s loaded"), i->first.c_str()));
			}

			ds->SyncWithAllPrefixes();
		}

		for (size_t i = 0; i < draw_sets.size(); i++) {
			if (FindSetName(draw_sets[i]) == false) {
				throw DrawImportException(wxString::Format(_("Failed to load set: %s"), draw_sets[i]->GetName().c_str()));
			}
		}

		wxString importedSets = wxT("");
		for (size_t i = 0; i < draw_sets.size(); i++) {
			m_def_sets->AddSet(draw_sets[i]);
			importedSets+=draw_sets[i]->GetName();
			if (i != draw_sets.size() -1) importedSets+=wxT(", ");
		}

		wxMessageBox(wxString::Format(_("Operation completed successfully imported %d set(s): "), draw_sets.size())+importedSets, _("Operation successful."), wxOK | wxICON_INFORMATION, wxGetApp().GetTopWindow());

	} catch (std::exception& e) {

		wxString msg = wxString::Format(_("Failed to import set from file: %s\n"), path.c_str())+wxString(e.what(), wxConvLibc);
		wxMessageBox(msg, _("Operation failed."), wxOK | wxICON_ERROR, wxGetApp().GetTopWindow());

		for (size_t i = 0; i < draw_sets.size(); i++) {// TODO: make draw_sets use smart pointers
			if (draw_sets[i]) delete draw_sets[i];
		}
		for (size_t i = 0; i < defined_params.size(); i++) { // TODO: make defined_params use smart pointers
			if (defined_params[i]) delete defined_params[i];
		}
	}

}


void ConfigManager::ExportSet(DefinedDrawSet *set, wxString our_name) {
	ExportImportSet es(this, m_db_mgr, m_defined_sets);
	es.ExportSet(set, our_name.IsEmpty() ? wxString(_("imported")) : our_name);
}

void ConfigManager::ImportSet() {
	ExportImportSet is(this, m_db_mgr, m_defined_sets);
	is.ImportSet();
}

ConfigManager::~ConfigManager() {
	SaveDefinedDrawsSets();

	delete m_defined_sets;
	m_defined_sets = NULL;

}
