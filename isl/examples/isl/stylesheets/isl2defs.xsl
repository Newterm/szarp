<?xml version="1.0"?>
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
	Pawel Palucha 2002

	$Id: isl2defs.xsl 4393 2007-09-13 16:37:10Z reksio $

	This stylesheet transforms isl to svg definitions, for use with
	dynamic scripted svg.

	A new XML document is created. All elements with attributes
	"isl:uri" are inserted in "<defs>" element. Other elements are
	ignored (not included in output).
	
	Each element gets id, generated automatically and based on element's
	location is original isl document. It is used to access defined element
	from within dynamic svg.

	Output from this stylesheet should be processed by isl2svg stylesheet,
	to insert param values.
	
 -->

<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:isl="http://www.praterm.com.pl/ISL/params"
	xmlns="http://www.w3.org/2000/svg"
	xmlns:svg="http://www.w3.org/2000/svg">

	<!-- Copy main SVG element and all it's attributes. -->
	<xsl:template match="svg:svg">
		<svg>
			<xsl:for-each select="@*">
				<xsl:copy/>
			</xsl:for-each>
			<defs>
				<xsl:apply-templates />
			</defs>
		</svg>
	</xsl:template> 

	<!-- Create isl elements definitions. -->
	<xsl:template match="*[@isl:uri]">
		<xsl:copy>
			<xsl:variable name="curr" select="."/>
			<xsl:variable name="gen_id" select="@id"/>
			<xsl:for-each select="@*">
				<xsl:copy/>
			</xsl:for-each>
			<xsl:attribute name="id">
				<xsl:value-of select="$gen_id"/>
			</xsl:attribute>
      			<xsl:apply-templates select="node()" mode="copy"/>
		</xsl:copy>
	</xsl:template> 

  <!-- Ignore other elements -->
  <xsl:template match="@*|node()|text()">
      <xsl:apply-templates />
  </xsl:template> 

  <!-- Copy elements -->
  <xsl:template match="@*|node()" mode="copy">
    <xsl:copy>
    </xsl:copy>
  </xsl:template> 
  
</xsl:stylesheet>
