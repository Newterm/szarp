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
	2004 Paweł Pałucha PRATERM S.A. pawel@praterm.com.pl

	$Id: extr_csv.xsl 4393 2007-09-13 16:37:10Z reksio $

	Ten szablon przeprowadza konwersję dokumentu z ekatrahowanymi
	danymi z bazy SzarpBase na format CSV.

	Parametr o nazwie 'delimiter' określa tekst stosowany jako
	separator pól. Domyślnie jest to ', '.
 -->
 
<xsl:stylesheet version="1.0" 
	xmlns:extr="http://www.praterm.com.pl/SZARP/extr"
	xmlns="http://www.praterm.com.pl/SZARP/extr"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<!-- nie chcemy deklaracji XML -->
	<xsl:output omit-xml-declaration = "yes" encoding="ISO-8859-2"/>
	
	<!-- parametr - stosowany separator -->
	<xsl:param name="delimiter" select="', '"/>

	<!-- nagłówek -->
	<xsl:template match="extr:header">
		<xsl:text>Data</xsl:text>
		<xsl:apply-templates/>
		<xsl:text>
</xsl:text>
	</xsl:template>

	<!-- opisy parametrów -->
	<xsl:template match="extr:param">
		<xsl:value-of select="$delimiter"/>
		<xsl:apply-templates select="text()" mode="copy"/>
	</xsl:template>

	<!-- wiersze z danymi -->
	<xsl:template match="extr:row">
		<xsl:apply-templates/>
		<xsl:text>
</xsl:text>
	</xsl:template>

	<!-- data -->
	<xsl:template match="extr:time">
		<xsl:apply-templates select="text()" mode="copy"/>
	</xsl:template>

	<!-- wartości -->
	<xsl:template match="extr:value">
		<xsl:value-of select="$delimiter"/>
		<xsl:apply-templates select="text()" mode="copy"/>
	</xsl:template>

	<!-- kopiuj tekst -->
	<xsl:template match="text()" mode="copy">
		<xsl:copy/>
	</xsl:template>
	
	<!-- olej pozostały tekst -->
	<xsl:template match="text()">
	</xsl:template>
	
</xsl:stylesheet>

