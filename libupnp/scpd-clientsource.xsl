<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:str="http://exslt.org/strings"
  xmlns:fn="http://www.w3.org/2005/xpath-functions"
  extension-element-prefixes="str fn"
                version="1.0">

  <xsl:include href="scpd-common.xsl"/>

<xsl:template match="/">/* This is an automatically-generated file -- DO NOT EDIT */

#include &quot;config.h&quot;
#include &quot;<xsl:value-of select="$class"/>_client.h&quot;
#include &quot;soap.h&quot;
#include &quot;libutil/bind.h&quot;
#include &quot;libutil/trace.h&quot;
#include &lt;errno.h&gt;
#include &lt;string.h&gt;

namespace upnp {  


        /* Sync API (deprecated) */

    <xsl:for-each select="//action">
  <xsl:sort select="name"/>

unsigned int <xsl:value-of select="$class"/>Client::<xsl:value-of select="name"/>(
      <xsl:for-each select="argumentList/argument">
      <xsl:text>      </xsl:text>
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
        <xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
        <xsl:text> </xsl:text>
        <xsl:if test="direction='out'">*</xsl:if>
      </xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:if test="direction='in'"><!-- Input types -->
      <xsl:call-template name="argtype">
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
      </xsl:if>
      <xsl:if test="direction='out'"><!-- Output types -->
      <xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template> *</xsl:if>
    </xsl:if>
        <xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>
      <xsl:if test="not(position()=last())">,
      </xsl:if>
    </xsl:for-each>)
{
    return SoapAction2(<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>
    <xsl:for-each select="argumentList/argument[direction='in']">
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">,
        <xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>
      </xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">,
        <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
        <xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>
        <xsl:if test="$type='string' or $type='uri' or $type='bin.base64'">.c_str()</xsl:if>
      </xsl:if>
    </xsl:for-each>
    <xsl:for-each select="argumentList/argument[direction='out']">,
        <xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>
    </xsl:for-each>);
}
</xsl:for-each>

void <xsl:value-of select="$class"/>Client::OnEvent(const char *var, const std::string&amp; value)
{
    <xsl:for-each select="//stateVariable">
        <xsl:if test="sendEventsAttribute='yes'">
          <xsl:variable name="type" select="dataType"/>if (!strcmp(var, &quot;<xsl:value-of select="name"/>&quot;))
        Fire(&amp;<xsl:value-of select="$class"/>Observer::On<xsl:value-of select="name"/>, <xsl:choose>
        <xsl:when test="$type='boolean'">GenaBool(value)</xsl:when>
        <xsl:when test="$type='i2'or $type='i4'">GenaInt(value)</xsl:when>
        <xsl:when test="$type='ui2' or $type='ui4'">GenaUInt(value)</xsl:when>
        <xsl:when test="$type='uri' or $type='string'">value</xsl:when>
          <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
        </xsl:choose>);
    else </xsl:if>
      </xsl:for-each>
      { TRACE &lt;&lt; "Unexpected event " &lt;&lt; var &lt;&lt; "\n"; }
}


      /* Async API */

      <xsl:for-each select="//action">
        <xsl:sort select="name"/>
unsigned int <xsl:value-of select="$class"/>ClientAsync::On<xsl:value-of select="name"/>Done(unsigned int rc, const soap::Params *<xsl:if test="count(argumentList/argument[direction='out'])>0">in</xsl:if>)
{<xsl:variable name="bargs2">
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
    m_observer->On<xsl:value-of select="name"/>Done(rc<xsl:for-each select="argumentList/argument[direction='out']"><xsl:variable name="thisarg"><xsl:value-of select="name"/></xsl:variable>
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>,
        <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">(<xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>)</xsl:if>in-><xsl:choose>
      <xsl:when test="$type = 'i1' or $type = 'ui1' or $type = 'boolean'">bytes<xsl:for-each select="str:split($bargs2)">
          <xsl:if test="current() = $thisarg">[<xsl:value-of select="position()-1"/>] /* <xsl:value-of select="$thisarg"/> */</xsl:if>
        </xsl:for-each></xsl:when>
      <xsl:when test="$type = 'i2' or $type = 'ui2'">shorts<xsl:for-each select="str:split($hargs2)">
          <xsl:if test="current() = $thisarg">[<xsl:value-of select="position()-1"/>] /* <xsl:value-of select="$thisarg"/> */</xsl:if>
        </xsl:for-each></xsl:when>
      <xsl:when test="$type = 'i4' or $type = 'ui4' or (//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">ints<xsl:for-each select="str:split($wargs2)">
          <xsl:if test="current() = $thisarg">[<xsl:value-of select="position()-1"/>] /* <xsl:value-of select="$thisarg"/> */</xsl:if>
        </xsl:for-each></xsl:when>
      <xsl:otherwise>strings<xsl:for-each select="str:split($sargs2)">
          <xsl:if test="current() = $thisarg">[<xsl:value-of select="position()-1"/>] /* <xsl:value-of select="$thisarg"/> */</xsl:if>
        </xsl:for-each>
</xsl:otherwise>
    </xsl:choose></xsl:for-each>);
    return 0;
}

unsigned int <xsl:value-of select="$class"/>ClientAsync::<xsl:value-of select="name"/>(
      <xsl:for-each select="argumentList/argument[direction='in']">
        <xsl:call-template name="intype">
          <xsl:with-param name="var" select="current()/relatedStateVariable"/>
        </xsl:call-template><xsl:text> </xsl:text>
        <xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>
      <xsl:if test="not(position()=last())">,
      </xsl:if>
      </xsl:for-each>)
{
    return SoapAction(<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>,
        util::Bind(this).To&lt;unsigned int, const soap::Params*,
            &amp;<xsl:value-of select="$class"/>ClientAsync::On<xsl:value-of select="name"/>Done&gt;()<xsl:for-each select="argumentList/argument">
<xsl:if test="direction='in'">,
        <xsl:call-template name="camelcase-to-underscore">
  <xsl:with-param name="camelcase" select="name"/>
</xsl:call-template>
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
        <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
          <xsl:if test="$type='string' or $type='uri' or $type='bin.base64'">.c_str()</xsl:if></xsl:if>
</xsl:if></xsl:for-each>);
}
    </xsl:for-each>

void <xsl:value-of select="$class"/>ClientAsync::OnEvent(const char *var, const std::string&amp; value)
{
    <xsl:for-each select="//stateVariable">
      <xsl:sort select="name"/>
        <xsl:if test="sendEventsAttribute='yes'">
          <xsl:variable name="type" select="dataType"/>if (!strcmp(var, &quot;<xsl:value-of select="name"/>&quot;))
        Fire(&amp;<xsl:value-of select="$class"/>Observer::On<xsl:value-of select="name"/>, <xsl:choose>
        <xsl:when test="$type='boolean'">GenaBool(value)</xsl:when>
        <xsl:when test="$type='i2'or $type='i4'">GenaInt(value)</xsl:when>
        <xsl:when test="$type='ui2' or $type='ui4'">GenaUInt(value)</xsl:when>
        <xsl:when test="$type='uri' or $type='string'">value</xsl:when>
          <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
        </xsl:choose>);
    else </xsl:if>
      </xsl:for-each>
      { TRACE &lt;&lt; "Unexpected event " &lt;&lt; var &lt;&lt; "\n"; }
}

} // namespace upnp

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
