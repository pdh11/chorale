<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str"
                version="1.0">

  <xsl:output method="text"/>
  <xsl:param name="class"/>

  <xsl:variable name="ucase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'" />
  <xsl:variable name="lcase" select="'abcdefghijklmnopqrstuvwxyz'" />
  <xsl:variable name="letters" select="concat($ucase, $lcase)" />
  <xsl:variable name="digits" select="'0123456789'" />

  <xsl:key name="arguments" match="argument/name" use="."/>

  <!-- split camel case into words and insert underscore 
   adapted from similar template in libxcb-1.0
    -->
  <xsl:template name="camelcase-to-underscore">
    <xsl:param name="camelcase"/>
    <xsl:for-each select="str:split($camelcase, '')">
      <xsl:variable name="a" select="."/>
      <xsl:variable name="b" select="following::*[1]"/>
      <xsl:variable name="c" select="following::*[2]"/>
      <xsl:value-of select="translate(., $ucase, $lcase)"/>
      <xsl:if test="($b and contains($lcase, $a) and contains($ucase, $b))
                    or ($b and contains($digits, $a)
                        and contains($letters, $b))
                    or ($b and contains($letters, $a)
                        and contains($digits, $b))
                    or ($c and contains($ucase, $a)
                        and contains($ucase, $b)
                        and contains($lcase, $c))">
        <xsl:text>_</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="camelcase-to-upper-underscore">
    <xsl:param name="camelcase"/>
    <xsl:for-each select="str:split($camelcase, '')">
      <xsl:variable name="a" select="."/>
      <xsl:variable name="b" select="following::*[1]"/>
      <xsl:variable name="c" select="following::*[2]"/>
      <xsl:value-of select="translate(., $lcase, $ucase)"/>
      <xsl:if test="($b and contains($lcase, $a) and contains($ucase, $b))
                    or ($b and contains($digits, $a)
                        and contains($letters, $b))
                    or ($b and contains($letters, $a)
                        and contains($digits, $b))
                    or ($c and contains($ucase, $a)
                        and contains($ucase, $b)
                        and contains($lcase, $c))">
        <xsl:text>_</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- convert basic SCPD type into C++ type -->
  <xsl:template name="cpptype">
    <xsl:param name="type"/>
    <xsl:choose>
      <xsl:when test="$type='boolean'">bool</xsl:when>
      <xsl:when test="$type='i1'">int8_t</xsl:when>
      <xsl:when test="$type='ui1'">uint8_t</xsl:when>
      <xsl:when test="$type='i2'">int16_t</xsl:when>
      <xsl:when test="$type='ui2'">uint16_t</xsl:when>
      <xsl:when test="$type='i4'">int32_t</xsl:when>
      <xsl:when test="$type='ui4'">uint32_t</xsl:when>
      <xsl:when test="$type='uri'">std::string</xsl:when>
      <xsl:when test="$type='string'">std::string</xsl:when>
      <xsl:otherwise>upnp::scpd::<xsl:value-of select="$type"/></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- convert basic SCPD type into C++ type for argument passing
    which could be done entirely in C++ using template specialisation, but this
    way creates clearer (easier-to-use) header files
    -->
  <xsl:template name="argtype">
    <xsl:param name="type"/>
    <xsl:choose>
      <xsl:when test="$type='boolean'">bool</xsl:when>
      <xsl:when test="$type='i1'">int8_t</xsl:when>
      <xsl:when test="$type='ui1'">uint8_t</xsl:when>
      <xsl:when test="$type='i2'">int16_t</xsl:when>
      <xsl:when test="$type='ui2'">uint16_t</xsl:when>
      <xsl:when test="$type='i4'">int32_t</xsl:when>
      <xsl:when test="$type='ui4'">uint32_t</xsl:when>
      <xsl:when test="$type='uri'">const std::string&amp;</xsl:when>
      <xsl:when test="$type='string'">const std::string&amp;</xsl:when>
      <xsl:otherwise>upnp::scpd::<xsl:value-of select="$type"/></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- convert SCPD type into C++ parameter type, including enums -->
  <xsl:template name="intype">
    <xsl:param name="var"/>
    <xsl:if test="//stateVariable[name=$var]/allowedValueList">
        <xsl:value-of select="str:replace($var,'A_ARG_TYPE_','')"/>
    </xsl:if>
    <xsl:if test="not(//stateVariable[name=$var]/allowedValueList)">
      <xsl:call-template name="argtype">
        <xsl:with-param name="type"
          select="//stateVariable[name=$var]/dataType"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- convert SCPD type into C++ declaration type, including enums -->
  <xsl:template name="decltype">
    <xsl:param name="var"/>
    <xsl:if test="//stateVariable[name=$var]/allowedValueList">
        <xsl:value-of select="str:replace($var,'A_ARG_TYPE_','')"/>
    </xsl:if>
    <xsl:if test="not(//stateVariable[name=$var]/allowedValueList)">
      <xsl:call-template name="cpptype">
        <xsl:with-param name="type"
          select="//stateVariable[name=$var]/dataType"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- convert SCPD type into C++ result type, including enums -->
  <xsl:template name="outtype">
    <xsl:param name="var"/>
    <xsl:if test="//stateVariable[name=$var]/allowedValueList">
        <xsl:value-of select="str:replace($var,'A_ARG_TYPE_','')"/>
    </xsl:if>
    <xsl:if test="not(//stateVariable[name=$var]/allowedValueList)">
      <xsl:call-template name="cpptype">
        <xsl:with-param name="type"
          select="//stateVariable[name=$var]/dataType"/>
      </xsl:call-template>*<xsl:text />
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
