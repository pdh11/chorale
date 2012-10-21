<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str"
                version="1.0">

  <xsl:include href="scpd-common.xsl"/>

<xsl:template match="/">/* This is an automatically-generated file -- DO NOT EDIT */

#include &quot;config.h&quot;
#include &quot;<xsl:value-of select="$class"/>_server.h&quot;
#include &quot;<xsl:value-of select="$class"/>.h&quot;
#include &quot;soap.h&quot;
#include &lt;string.h&gt;
#include &lt;stdlib.h&gt;
#include &lt;errno.h&gt;

namespace upnp {

unsigned int <xsl:value-of select="$class"/>Server::OnAction(unsigned int which, const soap::Params&amp; in, soap::Params *out)
{
    unsigned int rc = 0;
    switch (which)
    {
    <xsl:for-each select="//action">
    case <xsl:value-of select="$class"/>::<xsl:call-template name="camelcase-to-upper-underscore"><xsl:with-param name="camelcase" select="name"/></xsl:call-template>:
    {<xsl:variable name="bargs1">
  <xsl:for-each select="argumentList/argument[direction='in']">
  <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
  <xsl:if test="$type = 'i1' or $type = 'ui1' or $type='boolean'"><xsl:value-of select="name"/><xsl:text> </xsl:text>
</xsl:if>
</xsl:for-each>
</xsl:variable>

<xsl:variable name="hargs1">
  <xsl:for-each select="argumentList/argument[direction='in']">
  <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
  <xsl:if test="$type = 'i2' or $type = 'ui2'"><xsl:value-of select="name"/><xsl:text> </xsl:text>
</xsl:if>
</xsl:for-each>
</xsl:variable>

<xsl:variable name="wargs1">
  <xsl:for-each select="argumentList/argument[direction='in']">
  <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
  <xsl:if test="$type = 'i4' or $type = 'ui4' or $type='int' or (//stateVariable[name=current()/relatedStateVariable]/allowedValueList)"><xsl:value-of select="name"/><xsl:text> </xsl:text>
</xsl:if>
</xsl:for-each>
</xsl:variable>

<xsl:variable name="sargs1">
  <xsl:for-each select="argumentList/argument[direction='in']">
  <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
  <xsl:if test="$type = 'string' or $type = 'uri' or $type='bin.base64'"><xsl:value-of select="name"/><xsl:text> </xsl:text>
</xsl:if>
</xsl:if>
</xsl:for-each>
</xsl:variable>

<xsl:variable name="bargs2">
  <xsl:for-each select="argumentList/argument[direction='out']">
  <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
  <xsl:if test="$type = 'i1' or $type = 'ui1' or $type='boolean'"><xsl:value-of select="name"/><xsl:text> </xsl:text>
</xsl:if>
</xsl:for-each>
</xsl:variable>

<xsl:variable name="hargs2">
  <xsl:for-each select="argumentList/argument[direction='out']">
  <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
  <xsl:if test="$type = 'i2' or $type = 'ui2'"><xsl:value-of select="name"/><xsl:text> </xsl:text>
</xsl:if>
</xsl:for-each>
</xsl:variable>

<xsl:variable name="wargs2">
  <xsl:for-each select="argumentList/argument[direction='out']">
  <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
  <xsl:if test="$type = 'i4' or $type = 'ui4' or $type='int' or (//stateVariable[name=current()/relatedStateVariable]/allowedValueList)"><xsl:value-of select="name"/><xsl:text> </xsl:text>
</xsl:if>
</xsl:for-each>
</xsl:variable>

<xsl:variable name="sargs2">
  <xsl:for-each select="argumentList/argument[direction='out']">
  <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
  <xsl:if test="$type = 'string' or $type = 'uri' or $type='bin.base64'"><xsl:value-of select="name"/><xsl:text> </xsl:text>
</xsl:if>
</xsl:if>
</xsl:for-each>
</xsl:variable>

<xsl:for-each select="argumentList/argument">
    <xsl:if test="direction='out' and (//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
      <xsl:variable name="thisarg"><xsl:value-of select="name"/></xsl:variable><xsl:text>
        </xsl:text>
    <xsl:value-of select="$class"/>Base::<xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/><xsl:text> </xsl:text><xsl:value-of select="$thisarg"/>;</xsl:if></xsl:for-each>
        rc = m_impl-><xsl:value-of select="name"/>(
            <xsl:for-each select="argumentList/argument">
    <xsl:variable name="thisarg"><xsl:value-of select="name"/></xsl:variable>
    <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
    <xsl:variable name="names">
      <xsl:if test="direction='in'">
    <xsl:choose>
      <xsl:when test="$type='boolean' or $type='i1' or $type='ui1'"><xsl:value-of select="$bargs1"/></xsl:when>
      <xsl:when test="$type='i2' or $type='ui2'"><xsl:value-of select="$hargs1"/></xsl:when>
      <xsl:when test="$type='i4' or $type='ui4' or $type='int' or (//stateVariable[name=current()/relatedStateVariable]/allowedValueList)"><xsl:value-of select="$wargs1"/></xsl:when>
      <xsl:otherwise><xsl:value-of select="$sargs1"/></xsl:otherwise>
    </xsl:choose>      
      </xsl:if>
      <xsl:if test="direction='out'">
    <xsl:choose>
      <xsl:when test="$type='boolean' or $type='i1' or $type='ui1'"><xsl:value-of select="$bargs2"/></xsl:when>
      <xsl:when test="$type='i2' or $type='ui2'"><xsl:value-of select="$hargs2"/></xsl:when>
      <xsl:when test="$type='i4' or $type='ui4' or $type='int' or (//stateVariable[name=current()/relatedStateVariable]/allowedValueList)"><xsl:value-of select="$wargs2"/></xsl:when>
      <xsl:otherwise><xsl:value-of select="$sargs2"/></xsl:otherwise>
    </xsl:choose>      
  </xsl:if>
    </xsl:variable>
    <xsl:if test="direction='out' and (//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">&amp;<xsl:value-of select="$thisarg"/>
    </xsl:if>
    <xsl:if test="direction='in' or not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
    <xsl:if test="direction='in'"><xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">(<xsl:value-of select="$class"/>Base::<xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>)</xsl:if>in.</xsl:if>
    <xsl:if test="direction='out'">
    <xsl:choose>
      <xsl:when test="$type='boolean'">(bool*)</xsl:when>
      <xsl:when test="$type='i1'">(int8_t*)</xsl:when>
      <xsl:when test="$type='i2'">(int16_t*)</xsl:when>
      <xsl:when test="$type='i4' or $type='int'">(int32_t*)</xsl:when>
    </xsl:choose>&amp;out-></xsl:if>
    <xsl:choose>
      <xsl:when test="$type='boolean' or $type='i1' or $type='ui1'">bytes</xsl:when>
      <xsl:when test="$type='i2' or $type='ui2'">shorts</xsl:when>
      <xsl:when test="$type='i4' or $type='ui4' or $type='int' or (//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">ints</xsl:when>
      <xsl:otherwise>strings</xsl:otherwise>
    </xsl:choose>   
    <xsl:for-each select="str:split($names)">
          <xsl:if test="current() = $thisarg">[<xsl:value-of select="position()-1"/>] /* <xsl:value-of select="$thisarg"/> */</xsl:if>
        </xsl:for-each>
      </xsl:if>
    <xsl:if test="not(position()=last())">,
            </xsl:if>
  </xsl:for-each> );<xsl:for-each select="argumentList/argument">
    <xsl:if test="direction='out' and (//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
    <xsl:variable name="thisarg"><xsl:value-of select="name"/></xsl:variable>
        out->ints[<xsl:for-each select="str:split($wargs2)">
          <xsl:if test="current() = $thisarg"><xsl:value-of select="position()-1"/></xsl:if>
        </xsl:for-each>] = (int)<xsl:value-of select="$thisarg"/>;</xsl:if>
</xsl:for-each>
        break;
    }
    </xsl:for-each>
    }

    return rc;
}
      <xsl:for-each select="//stateVariable">
        <xsl:if test="sendEventsAttribute='yes'">
          <xsl:variable name="type" select="dataType"/>
void <xsl:value-of select="$class"/>Server::On<xsl:value-of select="name"/>(<xsl:call-template name="argtype">
            <xsl:with-param name="type" select="$type"/>
          </xsl:call-template> value)
{
    FireEvent(&quot;<xsl:value-of select="name"/>&quot;, value);
}
</xsl:if>
      </xsl:for-each>

} // namespace upnp

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
