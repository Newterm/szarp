<?xml version="1.0" encoding="ISO-8859-2"?>
<!-- 
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
-->

<!--
	2003 Paweł Pałucha PRATERM S.A. pawel@praterm.com.pl

	$Id: remove_empty.xsl 4393 2007-09-13 16:37:10Z reksio $

	Ten szablon usuwa z dokumentu wszystkie elementy 'device', 'defined' i
	'drawdefinable', które nie zawierają elementów 'param'.
 -->
 
<xsl:stylesheet version="1.0" 
	xmlns:ipk="http://www.praterm.com.pl/SZARP/ipk"
	xmlns="http://www.praterm.com.pl/SZARP/ipk"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	
	<!-- Usuń puste elementy -->
	<xsl:template match="ipk:device[count(.//ipk:param) = 0]">
	</xsl:template>

	<xsl:template match="ipk:defined[count(.//ipk:param) = 0]">
	</xsl:template>

	<xsl:template match="ipk:drawdefinable[count(.//ipk:param) = 0]">
	</xsl:template>

	<!-- Reszta bez zmian -->
  	<xsl:template match="@*|node()">
      		<xsl:copy>
            		<xsl:apply-templates select="@*|node()|comment()"/>
	        </xsl:copy>
	</xsl:template>
	
</xsl:stylesheet>
