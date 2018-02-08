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
/* $Id$
 *
 * reporter4 program
 * SZARP

 * ecto@praterm.com.pl
 */

#include <wx/wxprec.h>
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <wx/cmdline.h>
#include <wx/image.h>
#include <wx/config.h>

#include <iostream>

#include "../../libSzarp/include/libpar.h"
#include "../common/cconv.h"

#include "repapp.h"

#include "../common/fonts.h"
#include "../common/version.h"
#include "../common/szframe.h"

#include "config_handler.h"
#include "data_provider.h"

#include "../../resources/wx/icons/rap16.xpm"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

bool repApp::OnInit() 
{
	if (!ParseCMDLineOptions()) return false;

	SetTitle();
	SetImageHandler();
	SetLocale();
	SetIcon();

	InitializeReporter();

	SetExitOnFrameDelete(true);

	return true;
}

void repApp::InitializeReporter() {
	ServerInfoHolder srv_info = {m_server, m_port, m_base};

	auto chandler = std::make_shared<ReportsConfigHandler>(srv_info);
	auto dprov = std::make_shared<IksReportDataProvider>(srv_info);

	auto view = new ReporterWindow();
	reporter = std::make_shared<Reporter>(chandler, dprov, view, initial_report);
	view->AssignController(reporter);

	SetTopWindow(static_cast<wxFrame*>(view));
}

static char doc[] = "\n\
Config file defaults:\n\
:reporter:iks_server - address to connect to iks\n\
:reporter:iks_server_port - port to connect to iks\n\
\n\
:global:config_prefix - SZARP config base\n\
\n\
IMPORTANT NOTE:\n\
make sure iks-server is configured with parhub live connection (hub_url in config section in iks configuration)!\n\
";

bool repApp::ParseCMDLineOptions() {
	po::options_description desc("Reporter4"); 

	desc.add_options() 
		("help,h", "Print this help messages")
		("base,b", po::value<std::string>(), "Szarp prefix")
		("report,r", po::value<std::string>(), "Initial report")
		("server,s", po::value<std::string>(), "Servers name")
		("port,p", po::value<unsigned int>(), "Server port");

	po::variables_map vm; 

	libpar_init();

	char ** normal_argv = nullptr;

	try { 
		normal_argv = new char*[argc+1];
		for (int i = 0; i < argc; ++i) {
			normal_argv[i] = new char[strlen(argv[i])+1];
			std::wstring warg(static_cast<wchar_t**>(argv)[i]);
			std::string t_arg = SC::S2A(warg);
			strcpy(normal_argv[i], t_arg.c_str());
		}

		po::store(po::parse_command_line(argc, normal_argv, desc), vm);

		if ( vm.count("help")  ) { 
			std::cout << desc << std::endl; 
			std::cout << doc << std::endl;
			return false; 
		} 

		if ( vm.count("base") ) {
			m_base = SC::A2S(vm["base"].as<std::string>());
			if (m_base.substr(0,4) == L"ase=" || m_base.substr(0,4) == L"ase:" || m_base.substr(0,4) == L"ase ") {
				// deprecated "-base=..."
				m_base = m_base.substr(4);
			}
		}

		if ( m_base.empty() ) {
			const auto prefix = libpar_getpar("", "config_prefix", 0);
			if (prefix != NULL)
				m_base = SC::A2S(prefix);
		}

		if ( vm.count("report") ) {
			initial_report = SC::A2S(vm["report"].as<std::string>());
		}

		if ( vm.count("server") ) {
			m_server = vm["server"].as<std::string>();
		} else {
			const auto server = libpar_getpar("reporter", "iks_server", 0);
			if (server != nullptr) {
				m_server = std::string(server);
			} else {
				m_server = SC::S2A(m_base);
			}
		}

		if ( vm.count("port") ) {
			m_port = vm["port"].as<unsigned int>();
		} else {
			const auto port = libpar_getpar("reporter", "iks_server_port", 0);
			if (port != nullptr) {
				m_port = std::string(port);
			} else {
				m_port = "9002";
			}
		}

	} catch(po::error& e) { 
		libpar_done();
		std::cerr << "Options parse error: " << e.what() << std::endl << std::endl; 
		std::cerr << desc << std::endl; 
		return false; 
	} 

	if (normal_argv) delete[] normal_argv;

	libpar_done();
	return true;
}

void repApp::SetImageHandler() {
#if wxUSE_LIBPNG
	wxImage::AddHandler( new wxPNGHandler() );
#endif //wxUSE_LIBPNG
}

void repApp::SetTitle() {
	SetAppName(_T("reporter4"));
	this->SetProgName(_("Reporter 4"));
}

void repApp::SetIcon() {
	wxIcon icon(rap16_xpm);
	if (icon.IsOk()) {
		szFrame::setDefaultIcon(icon);
	}
}

void repApp::SetLocale() {
	wxArrayString catalogs;
	catalogs.Add(_T("reporter4"));
	catalogs.Add(_T("common"));
	catalogs.Add(_T("wx"));
	this->InitializeLocale(catalogs, locale);
}

int repApp::OnExit()
{
	xmlCleanupParser();
	szFontProvider::Cleanup();
	wxConfigBase* conf = wxConfig::Get();
	if (conf) delete conf;
	return 0;
}

IMPLEMENT_APP(repApp)

