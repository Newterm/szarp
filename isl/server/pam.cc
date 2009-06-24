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
 * PAM utility functions.
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <malloc.h>
#include <security/pam_appl.h>

#include <liblog.h>

#include "pam.h"

/**
 * Conversation function, called by PAM to get password.
 */
static int pam_conv(int num_msg, const struct pam_message **msg,
		struct pam_response **resp, void *appdata_ptr)
{
	struct pam_response *re;
	int i;
	
	re = (struct pam_response *) malloc 
		(num_msg * sizeof(struct pam_response));
	*resp = re;
	for (i = 0; i < num_msg; i++)
	{
		re[i].resp_retcode = 0;
		re[i].resp = (char*) appdata_ptr;
	}
	return PAM_SUCCESS;
}

/**
 * PAM "conversation" structure.
 */
static struct pam_conv conv = {
    pam_conv,
    NULL
};


/**
 * Uses PAM to authorize user with given password.
 * @param user user name
 * @param password user's password
 * @return 1 if user exist and password is correct, otherwise 0
 */
int pam_check(char *user, char *password)
{
	pam_handle_t *pamh=NULL;
	int retval;
	
	conv.appdata_ptr = (void *) password;
	retval = pam_start("paramd", user, &conv, &pamh);
	if (retval == PAM_SUCCESS) {
		retval = pam_authenticate(pamh, 0);
		if (retval != PAM_SUCCESS) {
			sz_log(3, "pam_authenticate() user %s failed : %s",
					user, pam_strerror(pamh, retval));
		}
	} else {
		sz_log(1, "pam_start() error: %s", pam_strerror(pamh, retval));
	}
	pam_end(pamh,retval);
	
	return retval == PAM_SUCCESS; 
}

