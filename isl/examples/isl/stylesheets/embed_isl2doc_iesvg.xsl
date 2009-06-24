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

	$Id: embed_isl2doc_iesvg.xsl 4393 2007-09-13 16:37:10Z reksio $

	This stylesheet transforms ISL images embeded within HTML source to 
	SVG embeded element. This is designed for Internet Explorer 5.0 - 6.0
	with Adobe SVG Viewer 3.0, which doesn't work correctly with "object"
	element.

	Elements <isl:schema src="source" alt="descr"/> are transformed to:

	<embed src="SVG-SOURCE" width="W" height="H"
		type="image/xml-svg">
	</embed>

	"W" and "H" attributes are copied from ISL source to match SVG image
	size. 

	"source" attribute must contain path to ISL file, relative to main isl
	directory.

 -->

<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns:svg="http://www.w3.org/2000/svg"
	xmlns:isl="http://www.praterm.com.pl/ISL/params">

	<xsl:template match="isl:schema">
		<xsl:variable name="a" 
			select="substring-after(@src,'docs/')"/>
		<xsl:variable name="src" 
			select="concat('../',substring-before(@src,'docs/'),
			'svg/',substring-before($a,'.isl'),'.svg')"/>
		<xsl:variable name="docname"
			select="concat('../',@src)"/>
		<xsl:variable
			name="doc" select="document($docname)//svg:svg"/>
		<xsl:variable name="w" select="$doc/@width"/>
		<xsl:variable name="h" select="$doc/@height"/>
	
		<embed src="{$src}" 
			width="{$w}" height="{$h}"
			type="image/svg+xml">
		</embed>
		<p>Oglądasz wersję dla Internet Explorer + Adobe SVG Viewer.
		Więcej informacji <a href="../main/browsers.html">
			tutaj</a>.</p>
	</xsl:template>

	
	<xsl:template match="*[@isl:uri]">
		<xsl:choose>
			<xsl:when test='@isl:target=""'>
				<xsl:copy>
					<xsl:for-each select="@*">
						<xsl:copy/>
					</xsl:for-each>
					<xsl:apply-templates 
						select="document(@isl:uri)"/>
				</xsl:copy>
			</xsl:when>
			<xsl:otherwise>
				<xsl:copy>
					<xsl:for-each select="@*">
						<xsl:copy/>
					</xsl:for-each>
					<xsl:attribute name="{@isl:target}">
						<xsl:apply-templates 
							select=
							"document(@isl:uri)"/>
					</xsl:attribute>
				</xsl:copy>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

 
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  
</xsl:stylesheet>
