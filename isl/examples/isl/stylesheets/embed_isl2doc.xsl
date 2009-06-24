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

	$Id: embed_isl2doc.xsl 4393 2007-09-13 16:37:10Z reksio $

	This stylesheet transforms ISL images embeded within HTML source to 
	SVG or PNG image with map.

	Elements <isl:schema src="source" alt="descr"/> are transformed to:

	<object src="SVG-SOURCE" width="W" height="H"
		type="image/xml-svg">
		<p>Visit <a href="http://www.adobe.com/svg">Adobe SVG Zone</a>
			to download SVG plugin.</p>
		<img src="PNG-SOURCE" width="W" height="H" usemap="#mapid"
			alt="descr"/>
		<map name="mapid">
			<area href="LINK-HREF" alt="LINK-HREF" shape="rect"
				coords="LINK-COORDS"/>
			...
		</map>
		<ul>
			<li><a href="LINK-HREF">LINK-HREF</a></li>
			...
		</ul>
	</object>

	The result is, that if browser supports SVG, an SVG image is inserted
	in HTML page. Otherwise, PNG image is inserted, with links in map or
	just listed below.
	"W" and "H" attributes are copied from ISL source to match SVG image
	size. If there are links in ISL source with "isl:link-coords"
	attributes, map is created with apropriate links. Otherwise, links are
	just listed below the image.

	"source" attribute must contain path to ISL file, relative to main isl
	directory.

 -->

<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns:svg="http://www.w3.org/2000/svg"
	xmlns:isl="http://www.praterm.com.pl/ISL/params">

	<!-- We need to know the name of processed document, to create link to
	IS+SVG version. -->
	<xsl:param name="name"/>
	
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
	
		<object src="{$src}" 
			width="{$w}" height="{$h}"
			type="image/svg+xml">
			<p>Ściągnij wtyczkę do SVG z 
				<a href="http://www.adobe.com/svg">
					Adobe SVG Zone</a></p>
					<xsl:variable name="mapid" select="generate-id($doc)"/>
			<p><ul>
				<xsl:for-each
					select="$doc//svg:a[not (@isl:link-coords)]">
					<li><a href="{@xlink:href}"><xsl:value-of 
						select="@xlink:href"/>
					</a></li>
				</xsl:for-each>
			</ul></p>

			<p><img src="{concat('../',substring-before(@src,'docs/'),
				'png/',substring-after(@src,'docs/'))}" width="{$w}" height="{$h}" 
				usemap="#{$mapid}" alt="{@alt}"/></p>
			<map name="{$mapid}">
				<xsl:for-each
					select="$doc//svg:a[@isl:link-coords]">
					<area href="{@xlink:href}"
						alt="{@xlink:href}"
						shape="rect"
						coords="{@isl:link-coords}"/>
				</xsl:for-each>
			</map>
			<p><br/>
			<xsl:for-each
				select="$doc//svg:a[not (@isl:link-coords)]">
			   <br/>
			</xsl:for-each>
			</p>
		</object>
		<p><a href="../iesvg/{$name}">
			Wersja dla Internet Explorer + Adobe SVG Viewer</a>.
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
