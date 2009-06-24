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
	Paweł Pałucha 2002
	$Id: doc2html.xsl 4393 2007-09-13 16:37:10Z reksio $

	Based on work of Berin Loritsch "bloritsch@infoplanning.com".

	This stylesheets creates beatiful HTML document...
	
 -->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:param name="object"/>
  <xsl:template match="document">
    <html>
      <head>
        <title><xsl:value-of select="header/title"/></title>
        <xsl:if test="@redirect">
          <meta http-equiv="Refresh" content="{@time};URL={@redirect}"/>
	</xsl:if>
      </head>

      <body bgcolor="#ffffff" link="#006633" alink="#009900" vlink="#f4373c">
    <table border="0" width="100%" height="100%" 
	    cellpadding="0" cellspacing="0">
      <tr height="120"><td width="100%">
        <!-- TOP PANEL -->		    
	<table width="100%" height="100%" border="0"
		cellpadding="0" cellspacing="0">
	  <tr height="16">
		  <td width="16" background="images/tp_ul_bck.png"></td>
	    <td bgcolor="#ff6246"></td>
	    <td width="22" background="images/tp_ur_bck.png"></td>
	  </tr>
	  <tr><td width="16" bgcolor="#ff6246"></td><td bgcolor="#ff6246" text="#ffffff">
	    <!-- PANEL CONTENT -->
	    <font size="+2" face="Helvetica" color="#ffffff"><div align="right">
              <b><i> 
                <xsl:variable name="title" select="header/title"/>
		<xsl:choose>
		  <xsl:when test="$title=''">
		    Projekt ISL
		  </xsl:when>
		  <xsl:otherwise>
	             <xsl:value-of select="$title"/> 
		  </xsl:otherwise>
	        </xsl:choose>
	      </i></b>
	    </div></font>
	  </td>
	  <td width="22" background="images/tp_r_img.png" valign="top">
          </td></tr>
	  <tr height="30">
		  <td width="16" background="images/tp_bl_bck.png"></td>
		  <td background="images/tp_b_bck.png" valign="top"
			  align="right">
		    <font size="-2" color="#ffffff">
		      <a href="http://www.praterm.com.pl">
			<font color="#ffffff">www.praterm.com.pl</font>
		      </a>
		    </font>
		  </td>
	    <td width="22" background="images/tp_br_bck.png"></td>
	  </tr>
	</table>
      </td></tr>
      <tr><td width="100%">
	<table height="100%" width="100%"  cellspacing="0"
			      cellpadding="0">
	  <tr height="100%">
	    <td width="150" valign="top">
	      <table width="150" cellspacing="0"  height="100%">
	        <tr height="20">
		  <td background="images/sp_l_bck.png" width="16"><p></p>  </td>
		  <td bgcolor="#bdbdbd"> <p> </p> </td>
		  <td bgcolor="#bdbdbd" width="16"> <p> </p> </td>
	        </tr>
		<!-- MENU -->
		<xsl:apply-templates
			select="document(concat('../',
			$object,'/menu.xml'))"/>
	        <tr height="16">
		  <td width="16" background="images/sp_bl_bck.png">
	<!--            <img src="images/sp_bl_bck.png" vspace="0" hspace="0"
	valign="top" align="left" width="16" height="16"/> -->
	          </td>
		  <td background="images/sp_b_bck.png"> </td>
		  <td width="16" background="images/sp_br_bck.png"> </td>
	        </tr>
	        <tr><td width="16"/><td/><td width="16"/></tr>
	      </table>
	    </td>
	    <td valign="top" vspace="10" hspace="10">
	      <table width="100%" height="100%" cellpadding="5" border="0">
	        <tr valign="top" height="100%">
		  <td width="100%" link="#ff0000" alink="#ff0000" 
			  vlink="#aa0000"><font color="#000000"
				face="arial,helvetica, sanserif">
		    <!-- PAGE CONTENT -->
		    <div align="left">		   
	              <xsl:apply-templates/> 
		    </div>	
	          </font></td>
	        </tr>
		<tr height="10" valign="top">
		  <td cellpadding="0" bgcolor="#ff6246" width="10">
		    <font size="-1" face="arial,helvetica,sanserif"
			    color="#ffffff">
		      <div align="right">
		        <a href="mailto:pawel@praterm.com.pl">
			  <font color="#ffffff"><i>pawel@praterm.com.pl</i>
			  </font>
			</a>
	              </div>
	            </font>
	          </td>
		</tr>
	      </table>
            </td>
          </tr>
        </table>
      </td></tr>
    </table>
  </body>
      
    </html>
  </xsl:template>

  <xsl:template match="h1">
	<table width="80%" cellpadding="0" cellspacing="0" height="36">
	  <tr>
	    <td width="12" background="images/h_l_bck.png"></td>
	    <td bgcolor="#ff6246">
	      <font size="+1" face="arial,helvetia,sanserif" color="#ffffff">
		<b><xsl:apply-templates/></b>
	      </font>
	    </td>
	    <td width="12" background="images/h_r_bck.png"></td>
	  </tr>
	</table>
	  
  </xsl:template>
  
  <xsl:template match="body">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="menu">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="group">
    <a class="menu" href="{@link}"><font size="+1" color="#ffff80"><xsl:value-of select="@title"/></font></a><br/>
    <xsl:apply-templates select="item"/>
  </xsl:template>
  
  <xsl:template match="item">
	  
	<tr height="30">
	  <td background="images/sp_l_bck.png" width="16"></td>
	  <td bgcolor="#bdbdbd">
		  <a class="menu" href="{@link}">
		    <font color="#ffffff"><xsl:value-of
				    select="@title"/></font>
		  </a>
	  </td>
	  <td bgcolor="#bdbdbd" width="16"></td>
        </tr>
  
  </xsl:template>

  <xsl:template match="p">
    <p align="justify"><xsl:apply-templates/></p>
  </xsl:template>

  <xsl:template match="quote">
    <div class="ctr"><xsl:apply-templates/></div>
  </xsl:template>

 <xsl:template match="code">
    <code><font face="courier, monospaced"><xsl:apply-templates/></font></code>
 </xsl:template>
 
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
</xsl:stylesheet>
