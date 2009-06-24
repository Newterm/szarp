<?xml version="1.0" encoding="iso-8859-2"?>

<!--
	$Id: raport_xml_all.xsl 2414 2005-07-02 21:49:41Z pawel $
 -->

<xsl:stylesheet version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:isl="http://www.praterm.com.pl/ISL/params"
  xmlns:ipk="http://www.praterm.com.pl/SZARP/ipk"
  xmlns:xlink="http://www.w3.org/1999/xlink">

  <xsl:output method="xml" indent="yes" media-type="text/xml" encoding="ISO-8859-2"/>
  <xsl:param name="uri"/>

  <xsl:template match="ipk:params">

    <xsl:element name="report">

      <xsl:for-each select="//ipk:param[ipk:raport]">

        <xsl:element name="param">

          <xsl:attribute name="name">
            <xsl:value-of select="@name"/>
          </xsl:attribute>

          <xsl:variable name="val">
            <xsl:copy-of select="document(concat($uri,
            translate(@name, 'ąćęłńóśźżĄĆĘŁŃÓŚŹŻ/ :', 'acelnoszzACELNOSZZ__/')))/isl:params/isl:attribute/text()"/> 
          </xsl:variable>

          <xsl:attribute name="value">
            <xsl:value-of select="$val"/>
          </xsl:attribute>

        </xsl:element> <!-- name="param" -->

      </xsl:for-each> <!-- select="//ipk:param[ipk:raport[@title=$title]]" -->

    </xsl:element> <!-- name="report" -->

  </xsl:template> <!-- match="ipk:params" -->

</xsl:stylesheet>
