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
 * uri.h - URI parsing
 *
 * $Id$
 */

#ifndef __URI_H__
#define __URI_H__

/**
 * Parsed URI from client's request.
 */
class ParsedURI {
	public:
	/** Private class to hold list of nodes in URI (from first to last). */
	class NodeList {
		public:
			char *name;	/**< Name of node */
			NodeList *next;	/**< Next node, NULL if last */
	};
	/** Private class to hold list of options in URI. */
	class OptionList {
		public:
			char *name;	/**< Name of option */
			char *value;	/**< Value of option */
			OptionList *next;	
					/**< Next option, NULL if last */
	};
	public:
		ParsedURI(char *uri);
		~ParsedURI();
		char *getOption(const char *name);
		NodeList *nodes;	/**< List of nodes in uri. */
		char *attribute;	/**< Attribute (after '@' char) */
		int last_slash;		/**< Is '/' a last char of uri ? */
		OptionList *options;	/**< Options ("?option=val&...") */
};

#endif

