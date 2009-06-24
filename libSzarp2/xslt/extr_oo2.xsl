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
	2007 PRATERM S.A. 
	Krzysztof Ołowski koder@praterm.com.pl

	$Id: extr_oo2.xsl 4393 2007-09-13 16:37:10Z reksio $

	Conversion from SzarpBase data to XML for Open Office2 (content.xml)
 -->
 
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns="http://www.w3.org/1999/xhtml"
xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0" xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0" xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0" xmlns:table="urn:oasis:names:tc:opendocument:xmlns:table:1.0" xmlns:draw="urn:oasis:names:tc:opendocument:xmlns:drawing:1.0" xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0" xmlns:number="urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0" xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0" xmlns:chart="urn:oasis:names:tc:opendocument:xmlns:chart:1.0" xmlns:dr3d="urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0" xmlns:math="http://www.w3.org/1998/Math/MathML" xmlns:form="urn:oasis:names:tc:opendocument:xmlns:form:1.0" xmlns:script="urn:oasis:names:tc:opendocument:xmlns:script:1.0" xmlns:ooo="http://openoffice.org/2004/office" xmlns:ooow="http://openoffice.org/2004/writer" xmlns:oooc="http://openoffice.org/2004/calc" xmlns:dom="http://www.w3.org/2001/xml-events" xmlns:xforms="http://www.w3.org/2002/xforms" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" office:version="1.0">

	<xsl:output omit-xml-declaration="no" encoding="UTF-8" method="xml" indent="yes"/>
	
	<xsl:template match="extr:extracted">
	
<office:document-content xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0" xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0" xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0" xmlns:table="urn:oasis:names:tc:opendocument:xmlns:table:1.0" xmlns:draw="urn:oasis:names:tc:opendocument:xmlns:drawing:1.0" xmlns:fo="urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:meta="urn:oasis:names:tc:opendocument:xmlns:meta:1.0" xmlns:number="urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0" xmlns:svg="urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0" xmlns:chart="urn:oasis:names:tc:opendocument:xmlns:chart:1.0" xmlns:dr3d="urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0" xmlns:math="http://www.w3.org/1998/Math/MathML" xmlns:form="urn:oasis:names:tc:opendocument:xmlns:form:1.0" xmlns:script="urn:oasis:names:tc:opendocument:xmlns:script:1.0" xmlns:ooo="http://openoffice.org/2004/office" xmlns:ooow="http://openoffice.org/2004/writer" xmlns:oooc="http://openoffice.org/2004/calc" xmlns:dom="http://www.w3.org/2001/xml-events" xmlns:xforms="http://www.w3.org/2002/xforms" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" office:version="1.0">

  <office:scripts/>
  <office:font-face-decls/>
  <office:automatic-styles>
    <style:style style:name="co1" style:family="table-column">
      <style:properties fo:break-before="auto" />
    </style:style>
    <style:style style:name="co2" style:family="table-column">
      <style:properties fo:break-before="auto"/>
    </style:style>
    <style:style style:name="ro1" style:family="table-row">
      <style:properties fo:break-before="auto" style:use-optimal-row-height="true"/>
    </style:style>
    <style:style style:name="ta1" style:family="table" style:master-page-name="Default">
      <style:properties table:display="true"/>
    </style:style>
    <number:date-style style:name="N50" style:family="data-style" number:automatic-order="true" number:format-source="language">
      <number:day/>
      <number:text>.</number:text>
      <number:month/>
      <number:text>.</number:text>
      <number:year/>
      <number:text> </number:text>
      <number:hours number:style="long"/>
      <number:text>:</number:text>
      <number:minutes number:style="long"/>
    </number:date-style>
    <style:style style:name="ce1" style:family="table-cell" style:parent-style-name="Default" style:data-style-name="N50"/>
    <style:style style:name="ce2" style:family="table-cell" style:parent-style-name="Default" style:data-style-name="N0"/>
  </office:automatic-styles>
  <office:body>
	<office:spreadsheet>
		<table:table table:name="Arkusz1" table:style-name="ta1">
      			<table:table-column table:style-name="co1" table:default-cell-style-name="ce1"/>
			<table:table-column table:style-name="co2" table:default-cell-style-name="Default"  table:number-columns-repeated="{count(.//extr:param)}"/>
			<xsl:apply-templates/>
		</table:table>
	</office:spreadsheet>
</office:body>
</office:document-content>

</xsl:template>

	<!-- HEADER -->
	
	<xsl:template match="extr:header">
	
      <table:table-row table:style-name="ro1">
        <table:table-cell table:style-name="Default"/>

	<xsl:apply-templates/>
	
      </table:table-row>
	
	</xsl:template>
	
	<!-- /HEADER -->


	<!-- PARAM -->

	<xsl:template match="extr:param">

	<table:table-cell>
		<text:p><xsl:value-of select="./text()"/></text:p>
	</table:table-cell>
	
	</xsl:template>
	
	<!-- /PARAM -->


	<!-- DATA -->

	<xsl:template match="extr:data">
		<xsl:apply-templates/>
	</xsl:template>
	
	<!-- /DATA -->


	<!-- ROW -->
	
	<xsl:template match="extr:row">
		
		<table:table-row table:style-name="ro1">
			<xsl:apply-templates/>
		</table:table-row>
		
	</xsl:template>
	
	<!-- /ROW -->


	<!-- TIME -->

	<xsl:template match="extr:time">
		<!-- sprawdzamy czy data zapisana po bożemu -->
		<xsl:choose>
		<xsl:when test="string-length(translate(./text(), '1234567890:- ', '')) = 0">
			<table:table-cell table:value-type="date" 
				table:date-value="{translate(./text(), ' ', 'T')}">
				<text:p><xsl:value-of select="./text()"/></text:p>
			</table:table-cell>
			
		</xsl:when>
		<xsl:otherwise>
			<table:table-cell>
		          <text:p><xsl:value-of select="./text()"/></text:p>
			</table:table-cell>
		
		</xsl:otherwise>
		</xsl:choose>
	
	</xsl:template>
	
	<!-- /TIME -->


	<!-- VALUE -->

	<xsl:template match="extr:value">

		<!-- Sprawdzamy czy mamy do czynienia z liczbą -->
		<xsl:choose>
			<xsl:when test="string-length(translate(./text(), '1234567890,.', '')) = 0">
			<table:table-cell table:value-type="float" 
				table:value="{translate(./text(), ',', '.')}">
			          <text:p><xsl:value-of select="./text()"/></text:p>
			</table:table-cell>
			
			</xsl:when>
			<xsl:otherwise>
				<table:table-cell>
			          <text:p><xsl:value-of select="./text()"/></text:p>
				</table:table-cell>
			
			</xsl:otherwise>
		</xsl:choose>
		
				  
	</xsl:template>
	
	<!-- /VALUE -->



</xsl:stylesheet>

