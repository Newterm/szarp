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

	$Id: remove_base.xsl 4393 2007-09-13 16:37:10Z reksio $

	Ten szablon zamienia usuwa wszystkie atrybuty 'base_ind' elementów
	param i wstawia zamiast nich atrybut 'tobase' o wartości '1'.
 -->
 
<xsl:stylesheet version="1.0" 
	xmlns:ipk="http://www.praterm.com.pl/SZARP/ipk"
	xmlns="http://www.praterm.com.pl/SZARP/ipk"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<!-- 
		Najpierw wszystkim elementom 'param', które mają atrybut
		'base_ind' dodajemy atrybut 'tobase'. 
	-->
	<xsl:template match="ipk:param[@base_ind]">
		<xsl:copy>
			<xsl:attribute name="tobase">1</xsl:attribute>
            		<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:template>

	<!-- Usuwamy elementom 'param' atrybuty 'base_ind' -->
	<xsl:template match="ipk:param//@base_ind">
	</xsl:template>

	<!-- Kopiujemy resztę bez zmian -->
  	<xsl:template match="@*|node()">
      		<xsl:copy>
            		<xsl:apply-templates select="@*|node()"/>
	        </xsl:copy>
	</xsl:template>
	
</xsl:stylesheet>
