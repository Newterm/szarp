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
#ifndef __COOBS_H__
#define __COOBS_H__
/*
 * draw3
 * SZARP
 
 *
 * $Id$
 */


/**Config observer interface, object of a class impelemnting this 
 * intrafce in notified about changes in configurations, i.e. @see IPKConcifg objects*/
class ConfigObserver {
public:
	/** Called upon removal of set from a configuration
	 * @param prefix - prefix of the configuration
	 * @param name - name of the set*/
	virtual void SetRemoved(wxString prefix, wxString name) {};

	/** Called upon change of name of a set
	 * @param prefix - prefix of the configuration
	 * @param from - previous name of the set
	 * @param name - new name of the set*/
	virtual void SetRenamed(wxString prefix, wxString from, wxString to, DrawSet *set) {};

	/** Called upon set modification
	 * @param prefix prefix of the configuration
	 * @param name name of the set
	 * @param set pointer set that has changed*/
	virtual void SetModified(wxString prefix, wxString name, DrawSet *set) {};

	/** Notifies observers that a set has been added to the configuration
	 * @param prefix prefix of the configuration
	 * @param name name of the set
	 * @param set pointer set that has changed*/
	virtual void SetAdded(wxString prefix, wxString name, DrawSet *added) {};

	/** Notifies observers that a given configuration is about to be reloaded
	 * @param prefix prefix of configuration that is about to be reloaded*/
	virtual void ConfigurationIsAboutToReload(wxString prefix)  {};

	/** Notifies observers that a given configuration was reloaded
	 * @param prefix prefix of configuration that is about to be reloaded*/
	virtual void ConfigurationWasReloaded(wxString prefix) {}

	virtual ~ConfigObserver();
	
};

#endif 
