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

	$Id: merge.xsl 4393 2007-09-13 16:37:10Z reksio $

	Ten szablon skleja dwa dokumenty. Konfigurację podaje się jako dokument
	do przetworzenia, a ścieżkę do wklejanego szablonu jako parametr 'add'.
	Elementy 'defined' są doklejane, natomiast zawartość elementów
	'defined' i 'drawdefinable' jest doklejana.
	
 -->
 
<xsl:stylesheet version="1.0" 
	xmlns:ipk="http://www.praterm.com.pl/SZARP/ipk"
	xmlns:extra="http://www.praterm.com.pl/SZARP/ipk-extra"
	xmlns="http://www.praterm.com.pl/SZARP/ipk"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<!-- Deklaracja parametru 'add' -->
	<xsl:param name="add"/>

	<xsl:template match="ipk:params">
		<xsl:copy>
			<xsl:apply-templates select="@*"/>
			<xsl:apply-templates select="comment()"/>
			<xsl:apply-templates select="document($add)/ipk:params/comment()"/>
			<xsl:apply-templates select="ipk:device"/>
			<xsl:apply-templates select="extra:*"/>
			<xsl:apply-templates
				select="document($add)//ipk:device"/>
			<xsl:choose>
				<xsl:when test="count(ipk:defined) > 0">
					<xsl:apply-templates
						select="ipk:defined"
						mode="insert_add"/>
				</xsl:when>
				<xsl:otherwise>
					<xsl:apply-templates
						select="document($add)//ipk:defined"
						mode="ignore_add"/>
				</xsl:otherwise>
			</xsl:choose>
			
			<xsl:choose>
				<xsl:when test="count(ipk:drawdefinable) > 0">
					<xsl:apply-templates
						select="ipk:drawdefinable"
						mode="insert_add"/>
				</xsl:when>
				<xsl:otherwise>
					<xsl:apply-templates
						select="document($add)//ipk:drawdefinable"
						mode="ignore_add"/>
				</xsl:otherwise>
			</xsl:choose>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="ipk:defined" mode="insert_add">
		<xsl:copy>
			<xsl:apply-templates select="@*"/>
			<xsl:apply-templates select="node()|comment()"/>
			<xsl:apply-templates
				select="document($add)//ipk:defined/node()|document($add)//ipk:defined/comment()|document($add)//ipk:defined/@*"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="ipk:defined" mode="ignore_add">
		<xsl:copy>
			<xsl:apply-templates select="@*"/>
			<xsl:apply-templates select="node()|comment()"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="ipk:drawdefinable" mode="insert_add">
		<xsl:copy>
			<xsl:apply-templates select="@*"/>
			<xsl:apply-templates select="node()|comment()"/>
			<xsl:apply-templates
				select="document($add)//ipk:drawdefinable/node()|document($add)//ipk:drawdefinable/comment()|document($add)//ipk:drawdefinable/@*"/> 
		</xsl:copy>
	</xsl:template>

	<xsl:template match="ipk:drawdefinable" mode="ignore_add">
		<xsl:copy>
			<xsl:apply-templates select="@*"/>
			<xsl:apply-templates select="node()|comment()"/>
		</xsl:copy>
	</xsl:template>

	<!-- Kopiujemy resztę -->
	<xsl:template match="@*|node()|comment()|text()">
		<xsl:copy>
			<xsl:apply-templates select="@*|node()"/>
		</xsl:copy>
	</xsl:template>

</xsl:stylesheet>

