<?xml version="1.0"?>

<!--
	(C) Pawel Palucha 2002

	$Id: isl2svg.xsl 4616 2007-11-05 14:59:00Z koder $

	This stylesheet transforms isl to static svg.

	Only elements with attributes "isl:uri", are transformed. All
	attributes of transformed elements are preserved (including isl:*).
	
	If value of "isl:target" attribute is null, content of element is
	replaced by content of <isl:attribute> element in document given by
	"isl:uri" attribute.

	For example, if http://host:port/document content is:
	
	<?xml version="1.0"?>
	<params
		xmlns="http://www.praterm.com.pl/ISL/params">
		<attribute>120</attribute>
	</params>

	then
	
	<text x="5" y="10" isl:uri="http://host:port/document">
		TEXT
	</text>

	is transformed to:
	
	<text x="5" y="10" isl:uri="http://host:port/document">
		120
	</text>
	
	If "target" attribute is not null, a new attribute is
	created. Name of this attribute is the value of "isl:target"
	attribute. Value of this attribute is a content of document
	given by "isl:uri" attribute.

	For example:
	<circle x="5" y="10" isl:uri="http://host:port/document"
		isl:target="radius"/>

	is transformed to:
	
	<circle x="5" y="10" isl:uri="http://host:port/document"
		isl:target="radius" radius="120"/>

	If error occured during loading of document given by isl:uri attribute,
	string ERR becomes element's content. 
	String "unknown" at the begining of loaded isl:attribute element is
	replaced by "?" in case of element or null string in case of attribute.

	Some isl:uri attributes are treated specially. If isl:uri attribute
	ends with "@v_u", two documents are loaded - one with ending of URI
	replaced with "@value", second with ending of URI replaced with
	"@unit". Contents of loaded isl:attribute elements are concatenated
	(with one space between them).
	
 -->

<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:isl="http://www.praterm.com.pl/ISL/params">

	<!-- Load param value -->
	<xsl:template match="*[@isl:uri]">
		<xsl:variable name="value1">
			<xsl:choose>
				<!-- test for special "v_u" attribute -->
				<xsl:when test='substring-after(@isl:uri,"@") =	"v_u"'>
					<xsl:variable name="tmp1"
						select='substring-before(@isl:uri,"@")'/>
					<xsl:variable name="tmp2" 
						select="document(concat($tmp1,'@value'))/isl:params/isl:attribute/text()"/>
					<xsl:variable name="tmp3" 
						select="document(concat($tmp1,'@unit'))/isl:params/isl:attribute/text()"/>
					<!-- concat 'value' and 'unit' attrs -->
					<xsl:copy-of select="concat($tmp2, ' ',
						$tmp3)"/>
				</xsl:when>
				<xsl:otherwise>
					<!-- just copy value -->
					<xsl:copy-of 
						select="document(@isl:uri)/isl:params/isl:attribute/text()"/>
				</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		
		<xsl:choose>
			<!-- test if it's going to be content or attribute
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

  <!-- process other nodes --> 
  <xsl:template match="node()">
    <xsl:copy>
	<xsl:if test="name() = 'svg'">
		<xsl:attribute name="onload">
			<xsl:text>start()</xsl:text>
		</xsl:attribute>
	</xsl:if>
	
      <xsl:apply-templates select="@*|node()"/>
	<xsl:if test="name() = 'svg'">
			<script type="javascript" xmlns="http://www.w3.org/2000/svg">
				<xsl:attribute name="type">
					<xsl:text>text/javascript</xsl:text>
				</xsl:attribute>
				<xsl:text>

//refresh time [ms]
var refresh_time = 5000

function start(){
	window.setInterval('getRemoteData()', refresh_time)
}

function getRemoteData(){
	url=location.href + '?type=defs' // gets http address and adds ?type=defs
	makeRequest(url)
}

function makeRequest(url) {
	http_request = false

	if (window.XMLHttpRequest) { // Mozilla, Safari,...
		http_request = new XMLHttpRequest();
		if (http_request.overrideMimeType) {
			http_request.overrideMimeType('text/xml')
		}
	} else if (window.ActiveXObject) { // IE
		alert('Internet Explorer nie pokazuje grafiki SVG - standardu W3C. Polecamy inne programy, np. Firefox.')
	}

	if (!http_request) {
//		alert('I give up :( I cannot make instance of XMLHTTP object')
		return false
	}
	
	http_request.onreadystatechange = function() { 
		alertContents(http_request) 
	}
	
	http_request.open('POST', url, true)
	http_request.setRequestHeader('Content-Type','application/x-www-form-urlencoded')
	http_request.send('whatever=sent')
}


function alertContents(http_request) {
	if (http_request.readyState == 4) {
		if (http_request.status == 200) {
			var xmldoc = http_request.responseXML
			var local_defs_node = document.getElementsByTagName('defs')[0]
			var defs_node = xmldoc.getElementsByTagName('defs')[0]

			//processing all nodes from responseXML
			for (i=0, len = defs_node.childNodes.length; i &lt; len; i++){
				new_node = defs_node.childNodes[i]
				new_node_id = new_node.getAttribute('id')
				old_node = document.getElementById(new_node_id)

	  			//attributes comparison
				for (j=0, len2=new_node.attributes.length; j &lt; len2; j++){
					if (old_node.attributes[j] != new_node.attributes[j]) {
						old_node.attributes[j].value = new_node.attributes[j].value
					}
				}

				//update of text node (only if it exists)
				if (old_node.childNodes.length != 0){
					if (old_node.firstChild.data != new_node.firstChild.data) {
						old_node.firstChild.data = new_node.firstChild.data
					}
				}
			}
		} else {
		//There was a problem with request
		}
	}
}

			</xsl:text>
		</script>
	</xsl:if>

    </xsl:copy>
  </xsl:template>
  
  <xsl:template match="@*">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
 
</xsl:stylesheet>
