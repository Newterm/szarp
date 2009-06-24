<?xml version="1.0" encoding="ISO-8859-2"?>

<!--
	SZARP (C) Praterm S.A.
	Paweł Pałucha <pawel@praterm.com.pl>

	$Id: raport_html.xsl 2414 2005-07-02 21:49:41Z pawel $

	Szablon dla raportów HTML.
 -->

<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:isl="http://www.praterm.com.pl/ISL/params"
	xmlns:ipk="http://www.praterm.com.pl/SZARP/ipk"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns="http://www.w3.org/1999/xhtml">

	<!-- nie chcemy deklaracji XML -->
	<xsl:output omit-xml-declaration="yes" encoding="ISO-8859-2"/>

	<xsl:param name="title"/>
	<xsl:param name="uri"/>

	<xsl:template match="ipk:params">
	  <html>
	    <head>
	      <meta http-equiv="Content-type" content="text/html; charset=ISO-8859-2"/>
	    </head>
	    <body>
	      <p><xsl:value-of select="$title"/></p>
	      <table>
	        <xsl:for-each select="//ipk:param[ipk:raport[@title=$title]]">
		  <tr>
		    <td><xsl:value-of select="@short_name"/></td>
		    <td>
		    <!--
		      <xsl:value-of select="document(concat('/probes/',
		      translate(@name, '/ :', '__/')))//text()"/> -->
		      <xsl:variable name="val">
			      <xsl:copy-of select="document(concat($uri,
			      translate(@name, 'ąćęłńóśźżĄĆĘŁŃÓŚŹŻ/ :', 'acelnoszzACELNOSZZ__/')))/isl:params/isl:attribute/text()"/> 
		      </xsl:variable>
		      <xsl:value-of select="$val"/>
		    </td>
		    <td>
		    <xsl:choose>
		      <xsl:when test="ipk:raport/@description">
		        <xsl:value-of select="ipk:raport/@description"/>
		      </xsl:when>
		      <xsl:otherwise>
		        <xsl:value-of select="@name"/>
		      </xsl:otherwise>
		    </xsl:choose>
		    <xsl:value-of select="concat(' [', @unit, ']')"/>
		    </td>
		  </tr>
		</xsl:for-each>
	      </table>
	    </body>
	  </html>
	</xsl:template>
  
</xsl:stylesheet>
