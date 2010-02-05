#include "viszioTransparentFrame.h"
#include "viszioFetchFrame.h"
#include "viszioConfiguration.h"

Configuration::Configuration(wxString configurationName):
	m_defaultSizeWithFrame(wxSize(250, 84)),
	m_configuration_name(configurationName)
{
	wxconfig = new wxConfig(_T("viszio_") + configurationName);
    m_current_amount_of_frames = 0;
    m_all_frames = new TransparentFrame*[m_max_number_of_frames];
	m_pfetcher = NULL;
    m_ipk = NULL;
    m_fontThreshold = 19;
}


Configuration::~Configuration()
{
	delete wxconfig;
}