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
/* Remarks handling code 
 * SZARP
 * 
 *
 * $Id$
 */

#ifndef __AUTHDIAG_H__
#define __AUTHDIAG_H__

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/**Simple dialog for username and password retrieval.*/
class AuthInfoDialog : public wxDialog {
	/** username entered by user*/
	wxString m_username;
	/** passowrd entered by user*/
	wxString m_password;
public:
	AuthInfoDialog(wxWindow *parent);	
	/*@return username provided by a user*/
	wxString GetUsername();
	/*@return password provided by a user*/
	wxString GetPassword();
	/**Clears username and password controls*/
	void Clear();
};

#endif
