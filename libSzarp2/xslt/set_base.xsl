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

	$Id: set_base.xsl 4393 2007-09-13 16:37:10Z reksio $

	Ten szablon ustawia elementom 'param' w dokumencie atrybut 'base_ind'
	na taki sam, jaki ma element o tym samym atrybucie 'name' w dokumencie
	podanym jako parametr 'list'.

	Parametr:
		list - ścieżka do dokumentu z referencyjną listą parametrów
 -->
 
<xsl:stylesheet version="1.0" 
	xmlns:ipk="http://www.praterm.com.pl/SZARP/ipk"
	xmlns="http://www.praterm.com.pl/SZARP/ipk"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<!-- Deklaracja parametru 'list' -->
	<xsl:param name="list"/>

	<!-- Przetwarzamy żądane elementy 'param' -->
	<xsl:template match="ipk:param[@tobase]">
		<xsl:variable name="name">
			<xsl:value-of select="@name"/>
		</xsl:variable>
		<!-- Ustalamy indeks na podstawie listy referencyjnej -->
		<xsl:variable name="index">
			<xsl:value-of
				select="document($list)//ipk:param[@name=$name]/@base_ind"/>
		</xsl:variable>
		<!-- Kopiujemy element dodając atrybut 'base_ind' -->
		<xsl:copy>
			<xsl:if test="$index != ''">
				<xsl:attribute name="base_ind">
					<xsl:value-of select="$index"/>
				</xsl:attribute>
			</xsl:if>
			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:template>

	<!-- Usuwamy atrybuty 'tobase' -->
	<xsl:template match="@tobase">
	</xsl:template>

	<!-- Kopiujemy resztę -->
	<xsl:template match="@*|node()|text()|comment()">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()|text()"/>
		</xsl:copy>
	</xsl:template>

</xsl:stylesheet>

