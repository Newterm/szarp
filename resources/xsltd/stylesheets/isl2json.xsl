<?xml version="1.0"?>

<!--
	(C) Pawel Palucha 2002

	$Id: isl2defs.xsl 4588 2007-11-02 12:20:46Z pawel $

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
	xmlns:svg="http://www.w3.org/2000/svg"
        xmlns:pxslt="http://www.praterm.com.pl/ISL/pxslt">

	<xsl:output method="text" encoding="UTF-8" media-type="text/plain"/>

	<xsl:template match="svg:svg">
	    <xsl:text>{
	    </xsl:text>
		<xsl:apply-templates select="//*[@isl:uri]" />
	    <xsl:text>}
	    </xsl:text>
	</xsl:template> 

	<!-- Load param value -->
	<xsl:template match="*">
		<xsl:variable name="value1">
			<!-- just copy value -->
                        <xsl:copy-of select="pxslt:isl_vu(@isl:uri)"/>
		</xsl:variable>
		
		<xsl:text>"</xsl:text><xsl:value-of select="@id"/><xsl:text>" : </xsl:text>
		<xsl:choose>
			<!-- test if it's going to be content ar attribute replacement -->
			<xsl:when test='not (@isl:target) or (@isl:target="")'>
					<!-- insert param value as content -->
					<xsl:choose>
						<xsl:when test='$value1 = ""'>"ERR"</xsl:when>
						<xsl:when test='starts-with($value1,"unknown")'>"?"</xsl:when>
						<xsl:otherwise>"<xsl:copy-of select="$value1"/>"</xsl:otherwise>
					</xsl:choose>
			</xsl:when>
			<xsl:otherwise>
				<!-- attribute replacement -->
				<!-- set 0 if value cannot be loaded -->
				<xsl:text>{</xsl:text>
				<xsl:variable name="value">
					<xsl:choose>
						<xsl:when test='$value1 = ""'>0</xsl:when>
						<xsl:when test='starts-with($value1,"unknown")'>0</xsl:when>
						<xsl:otherwise>
							<xsl:copy-of select="$value1"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:variable>
		
				<xsl:copy>
					<xsl:variable name="atr_name" select="@isl:target"/>
					<xsl:variable name="atr_val" select="$value"/>
					<xsl:variable name="val2">
					  <!-- scale and shift arrribute -->
					  <xsl:choose>
					    <xsl:when test="@isl:scale and @isl:shift">
					      <xsl:copy-of select="$atr_val * @isl:scale + @isl:shift"/>
					    </xsl:when>
					    <xsl:when test="@isl:scale">
					       <xsl:copy-of select="$atr_val * @isl:scale"/>
					    </xsl:when>
					    <xsl:when test="@isl:shift">
					       <xsl:copy-of select="$atr_val + @isl:shift"/>
					    </xsl:when>
					    <xsl:otherwise>
				              <xsl:copy-of select="$atr_val"/>
					    </xsl:otherwise>
				          </xsl:choose>
				        </xsl:variable>
				        <!-- insert attribute -->
					<xsl:text>"</xsl:text>
					<xsl:value-of select="$atr_name"/>
					<xsl:text>": "</xsl:text>
					<xsl:value-of select="$val2"/>
					<xsl:text>"</xsl:text>
				</xsl:copy>
				<xsl:text>}</xsl:text>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:if test="position() != last()">
		    <xsl:text>, 
		    </xsl:text>
		</xsl:if>
	</xsl:template>

  
</xsl:stylesheet>
