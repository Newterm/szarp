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


	<!-- Load param value -->
	<xsl:template match="*[@isl:uri]">
		<xsl:variable name="value1">
			<!-- just copy value -->
			<xsl:copy-of select="pxslt:isl_vu(@isl:uri)"/>
		</xsl:variable>
		
		<xsl:choose>
			<!-- test if it's going to be content ar attribute
			replacement -->
			<xsl:when test='not (@isl:target) or (@isl:target="")'>
				<!-- content replacement -->
				<xsl:copy>
					<!-- copy all attributes -->
					<xsl:for-each select="@*">
						<xsl:copy/>
					</xsl:for-each>
					<!-- insert param value as content -->
					<xsl:choose>
						<xsl:when test='$value1 = ""'>ERR</xsl:when>
						<xsl:when test='starts-with($value1,"unknown")'>?</xsl:when>
						<xsl:otherwise>
							<xsl:copy-of select="$value1"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:copy>
			</xsl:when>
			<xsl:otherwise>
				<!-- attribute replacement -->
				<!-- set 0 if value cannot be loaded -->
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
					<xsl:variable name="atr_name"
						select="@isl:target"/>
					<!-- copy other attributes -->
					<xsl:for-each select="@*">
						<xsl:variable name="cur"
							select="."/>
						<xsl:if 
						  test="$cur != $atr_name">
							<xsl:copy/>
						</xsl:if>
					</xsl:for-each>
					<xsl:variable name="atr_val"
						select="$value"/>
					<xsl:variable name="val2">
					  <!-- scale and shift arrribute -->
					  <xsl:choose>
					    <xsl:when test=
					      "@isl:scale and @isl:shift">
					      <xsl:copy-of select=
					        "$atr_val * @isl:scale +
						@isl:shift"/>
					    </xsl:when>
					    <xsl:when test=
					      "@isl:scale">
					       <xsl:copy-of select=
						 "$atr_val * @isl:scale"/>
					    </xsl:when>
					    <xsl:when test=
					      "@isl:shift">
					       <xsl:copy-of select=
						 "$atr_val + @isl:shift"/>
					    </xsl:when>
					    <xsl:otherwise>
				              <xsl:copy-of 
						    select="$atr_val"/>
					    </xsl:otherwise>
				          </xsl:choose>
				        </xsl:variable>
				        <!-- insert attribute -->
					<xsl:attribute name="{$atr_name}">
						<xsl:value-of
							select="$val2"/>
					</xsl:attribute> 
				</xsl:copy>
			</xsl:otherwise>
		</xsl:choose>
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
