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

#include <set>

#include <assert.h>

#include <wx/arrimpl.cpp>
#include <wx/settings.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/dir.h>
#include <wx/config.h>
#include <wx/tokenzr.h>

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



wxString DrawParam::GetBasePrefix() {
	assert (m_param != NULL);
	return m_param->GetSzarpConfig()->GetPrefix().c_str();
}

wxString DrawParam::GetParamName() {
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
}

DrawInfo::DrawInfo(TDraw *d, DrawParam *p)
{
    assert (d != NULL);
    assert (p != NULL);

    this->d = d;
    this->p = p;
    c = wxNullColour;

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
DrawInfo::GetBasePrefix()
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
DrawInfo::GetParamName()
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
DrawInfo::GetParam()
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

wxString
DrawInfo::GetValueStr(const double &val, const wxString& no_data_str) {
	assert (p != NULL);
	return p->GetIPKParam()->PrintValue(val, no_data_str.c_str());
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
    if (m_draws->size() >= MAX_DRAWS_COUNT) {
	    wxLogWarning(_T("Too many draws in window '%s'"), m_name.c_str());
	    delete drawInfo;
	    return;
    }

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

void
DrawSet::SortDraws()
{
	std::stable_sort(m_draws->begin(), m_draws->end(), DrawInfo::CompareDraws);
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
DrawSet::SetName(const wxString & name) { m_name = name; };

void
DrawSet::InitNullColors()
{
    if (NULL == m_cfg)
	return;

    wxColour * DefaultColors = m_cfg->GetParentManager()->GetDefaultColors();
    int ex[MAX_DRAWS_COUNT];
    int first_free = 0;

    /* check for used colors */
    for (int i = 0; i < MAX_DRAWS_COUNT; i++)
	ex[i] = 0;

    wxLogInfo(_T("InitNullColors: draws count: %zu"), m_draws->size());

    for (size_t i = 0; i < m_draws->size(); i++) {
	for (int j = 0; j < MAX_DRAWS_COUNT; j++)
	    if (m_draws->at(i)->GetDrawColor() == DefaultColors[j]) {
		ex[j] = 1;
		//only one color in table can be equal
		break;
	    }
    }

    /* now assign null colors */
    for (size_t i = 0; i < m_draws->size(); i++) {
	if (m_draws->at(i)->GetDrawColor() == wxNullColour) {
	    /* search for next free color */
	    while (ex[first_free]) {
		first_free++;
	    }

	    assert (first_free < MAX_DRAWS_COUNT);

	    /* set color */
	    m_draws->at(i)->SetDrawColor(DefaultColors[first_free]);

	    /* mark color as used */
	    ex[first_free] = 1;
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
DrawSet::GetParamName(int index)
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

ConfigManager::ConfigManager(DrawApp* app, IPKContainer *ipks) : m_app(app), m_ipks(ipks), splashscreen(NULL)
{
	for (int i = 0; i < MAX_DRAWS_COUNT; i++) {
		DefaultColors[i] = DrawDefaultColors::MakeColor(i);
	}

	m_defined_sets = NULL;

	LoadDefinedDrawsSets();

	IPKContainer* ic = IPKContainer::GetObject();

	const std::vector<DefinedParam*>& vp = m_defined_sets->GetDefinedParams();

	for (std::vector<DefinedParam*>::const_iterator i = vp.begin();
			i != vp.end();
			i++)
		ic->AddExtraParam((*i)->GetBasePrefix().c_str(), (*i)->GetIPKParam());

	psc = new DrawPsc();
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
ConfigManager::LoadConfig(const wxString& prefix, const wxString &config_path)
{
	if(splashscreen != NULL) {
		wxString msg = _("Loading configuration: ");
		msg += config_path;
		splashscreen->PushStatusText(msg);
	}

	TSzarpConfig *ipk = NULL;
	if (config_path == wxEmptyString)
		ipk = m_ipks->GetConfig(prefix.c_str());

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
ConfigManager::GetDrawName(const wxString prefix, const wxString set, int index)
{
    return config_hash[prefix]->GetDrawsSets()[set]->GetDrawName(index);
}

wxString
ConfigManager::GetParamName(const wxString prefix, const wxString set, int index)
{
    return config_hash[prefix]->GetDrawsSets()[set]->GetParamName(index);
}

wxColour
ConfigManager::GetDrawColor(const wxString prefix, const wxString set, int index)
{
    return config_hash[prefix]->GetDrawsSets()[set]->GetDrawColor(index);
}

wxColour*
ConfigManager::GetDefaultColors()
{
    return DefaultColors;
}


wxString
IPKConfig::GetID()
{
    return m_id;
}

wxString
IPKConfig::GetPrefix()
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

	if (pc.find(GetPrefix()) != pc.end())
		drawSets[df->GetName()] = df->MakeShallowCopy(this);

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

DrawsSets::DrawsSets(ConfigManager *cfg) : m_cfgmgr(cfg) {}

SortedSetsArray *
DrawsSets::GetSortedDrawSetsNames(bool all)
{
    AttachDefined();
    SortedSetsArray * sorted = new SortedSetsArray(DrawSet::CompareSets);

    for (DrawSetsHash::iterator it = GetDrawsSets(all).begin(); it != GetDrawsSets(all).end(); it++) {
	sorted->Add(it->second);
    }

    return sorted;
}


DrawsSets::~DrawsSets() {}

ConfigManager*
DrawsSets::GetParentManager()
{
    return m_cfgmgr;
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
ConfigManager::GetConfigByPrefix(const wxString& prefix)
{
	DrawsSetsHash::iterator i = config_hash.find(prefix);
	if (i == config_hash.end()) {
		DrawsSets *s = LoadConfig(prefix);
		return s;
	}

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


#if 0
wxString ConfigManager::GetSzarpDir() {
	return m_app->GetSzarpDir();
}
#endif

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
		m_config_names = ::GetConfigTitles(m_app->GetSzarpDataDir(), &hidden_databases);
		m_config_names[DefinedDrawsSets::DEF_PREFIX]
			= wxGetTranslation(DefinedDrawsSets::DEF_NAME);
	}

	return m_config_names;
}

wxArrayString ConfigManager::GetPrefixes() {

	wxArrayString result;

	wxDir dir(m_app->GetSzarpDataDir());
	wxString path;

	if (dir.GetFirst(&path, wxEmptyString, wxDIR_DIRS))
		do
			if (wxFile::Exists(m_app->GetSzarpDataDir()+ path + _T("/config/params.xml")))
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

void ConfigManager::NotifySetAdded(wxString prefix, wxString name, DrawSet *set) {
	for (std::vector<ConfigObserver*>::iterator i = m_observers.begin();
			i != m_observers.end();
			i++)
		(*i)->SetAdded(prefix, name, set);
}

void ConfigManager::SetDatabaseManager(DatabaseManager *db_mgr) {
	m_db_mgr = db_mgr;
}

bool ConfigManager::ReloadConfiguration(wxString prefix) {
	wxCriticalSectionLocker locker(m_reload_config_CS);
	std::wstring wprefix = prefix.c_str();
	if (m_ipks->ReadyConfigurationForLoad(wprefix) == false)
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

	TSzarpConfig *cfg = m_ipks->LoadConfig(prefix.c_str(), std::wstring());
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

	if (p.FileExists())
		m_defined_sets->LoadSets(p.GetFullPath());

	config_hash[m_defined_sets->GetPrefix()] = m_defined_sets;

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
					wxICON_ERROR);
			return false;
		}

	wxFileName p(sd.GetPath(), _T("defined.xml"));

	m_defined_sets->SaveSets(p.GetFullPath());

	return true;

}

ConfigManager::~ConfigManager() {
	SaveDefinedDrawsSets();

	delete m_defined_sets;
	m_defined_sets = NULL;

}
