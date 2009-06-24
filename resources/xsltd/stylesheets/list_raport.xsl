<?xml version="1.0" encoding="iso-8859-2"?>

<!--
	$Id: list_raport.xsl 2924 2006-01-06 12:55:17Z pawel $
	Konwersja listy parametrów na raport.
 -->

<xsl:stylesheet version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:isl="http://www.praterm.com.pl/ISL/params"
  xmlns:list="http://www.praterm.com.pl/SZARP/list"
  xmlns:rap="http://www.praterm.com.pl/SZARP/reports"
  xmlns:xlink="http://www.w3.org/1999/xlink"
  extension-element-prefixes="mx2"
  xmlns:mx2="http://www.mod-xslt2.com/ns/1.0"
  >


  <xsl:output method="xml" indent="yes" media-type="text/xml" encoding="UTF-8"/>
  <xsl:variable name="title"> 
     <mx2:value-of select="$title"/>
  </xsl:variable>
  <xsl:variable name="uri">
      <mx2:value-of select="$uri"/>
  </xsl:variable>

  <xsl:template match="list:parameters">

    <xsl:copy>
      <xsl:apply-templates select="@*"/>

      <xsl:attribute name="rap:title">
        <xsl:value-of select="$title"/>
      </xsl:attribute>
      <xsl:attribute name="source">
        <xsl:value-of select="@source"/>
      </xsl:attribute>

      <xsl:for-each select="//list:param">

        <xsl:copy>
	  
	  <!-- apply copy template to all children and attributes -->
	  <xsl:apply-templates select="@*|node()"/>

          <xsl:variable name="val">
            <xsl:copy-of select="document(
	    	concat(
			$uri,
			translate(@name, 'ąćęłńóśźżĄĆĘŁŃÓŚŹŻ/ :', 'acelnoszzACELNOSZZ__/'),
			'@value'
			)
		)/isl:params/isl:attribute/text()"/> 
          </xsl:variable>

          <xsl:attribute name="rap:value">
            <xsl:value-of select="$val"/>
          </xsl:attribute>

        </xsl:copy> <!-- "list:param" -->

      </xsl:for-each>

    </xsl:copy> <!-- "list:parameters" -->

  </xsl:template> <!-- match="ipk:params" -->

  <!-- copy everything else -->
  <xsl:template match="@*|node()">
     <xsl:copy/>
  </xsl:template>
								  

</xsl:stylesheet>
