<?xml version="1.0" encoding="iso-8859-2" standalone="yes"?>

<!--
	Stylesheet for SZARP config files served by Apache mod-xslt2. This
	stylesheet does nothing, buy mod-xslt2 requires some stylesheet to
	apply.
	
	Paweł Pałucha <pawel@praterm.com.pl>
	$Id: all.xsl 2867 2005-11-27 00:04:15Z pawel $
 -->

<xsl:stylesheet version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  >

  <xsl:output method="xml" indent="yes" media-type="text/xml" encoding="UTF-8"/>
  
  <!-- text and comment nodes are copied by default -->
  <xsl:template match="@*|node()">
    <xsl:copy>
        <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  
</xsl:stylesheet>
