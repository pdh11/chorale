<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str"
                version="1.0">

  <xsl:include href="scpd-common.xsl"/>

<xsl:template match="/">/* This is an automatically-generated file -- DO NOT EDIT */

#include &quot;config.h&quot;
#include &quot;<xsl:value-of select="$class"/>.h&quot;
#include &quot;libutil/trace.h&quot;
#include &quot;data.h&quot;
#include &lt;errno.h&gt;
#include &lt;string.h&gt;

namespace upnp {

        /* Async API stubs */
<xsl:for-each select="//action">
unsigned int <xsl:value-of select="$class"/>Async::<xsl:value-of select="name"/>(
    <xsl:for-each select="argumentList/argument[direction='in']">
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
        <xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
      </xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:if test="direction='in'"><!-- Input types -->
      <xsl:call-template name="argtype">
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template>
      </xsl:if>
    </xsl:if>
    <xsl:if test="not(position()=last())">,
    </xsl:if>
    </xsl:for-each>)
{
    TRACE &lt;&lt; "Unhandled <xsl:value-of select="name"/>\n";
    return ENOSYS;
}
</xsl:for-each>
  <xsl:for-each select="//stateVariable">
    <xsl:if test="not(contains(name,'A_ARG_TYPE')) and sendEventsAttribute='yes'">
      <xsl:variable name="getter" select="concat('Get',name)"/>
      <xsl:if test="not(//action[name=$getter])">
unsigned int <xsl:value-of select="$class"/>Async::<xsl:value-of select="$getter"/>(<xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="dataType"/>
      </xsl:call-template>*)
{
    return ENOENT;
}</xsl:if>
    </xsl:if>
  </xsl:for-each>

        /* Async observer stubs */

  <xsl:for-each select="//action"><xsl:sort select="name"/>
void <xsl:value-of select="$class"/>AsyncObserver::On<xsl:value-of select="name"/>Done(
            unsigned int<xsl:for-each select="argumentList/argument">
  <xsl:if test="direction='out'">,
            <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList"><xsl:value-of select="$class"/>Base::<xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
        </xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:call-template name="argtype">
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template></xsl:if></xsl:if>
    </xsl:for-each>)
{}
</xsl:for-each>

        /* Sync API stubs */
<xsl:for-each select="//action">
unsigned int <xsl:value-of select="$class"/>::<xsl:value-of select="name"/>(
      <xsl:for-each select="argumentList/argument">
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
        <xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
        <xsl:if test="direction='out'">*</xsl:if>
      </xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:if test="direction='in'"><!-- Input types -->
      <xsl:call-template name="argtype">
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template>
      </xsl:if>
      <xsl:if test="direction='out'"><!-- Output types -->
      <xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template>*</xsl:if>
    </xsl:if>
      <xsl:if test="not(position()=last())">,
</xsl:if>
    </xsl:for-each>)
{
    TRACE &lt;&lt; "Unhandled <xsl:value-of select="name"/>\n";
    return ENOSYS;
}
</xsl:for-each>
  <xsl:for-each select="//stateVariable">
    <xsl:if test="not(contains(name,'A_ARG_TYPE')) and sendEventsAttribute='yes'">
      <xsl:variable name="getter" select="concat('Get',name)"/>
      <xsl:if test="not(//action[name=$getter])">
unsigned int <xsl:value-of select="$class"/>::<xsl:value-of select="$getter"/>(<xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="dataType"/>
      </xsl:call-template>*)
{
    return ENOENT;
}</xsl:if>
    </xsl:if>
  </xsl:for-each>
  
        /* Text items */

static const char *const <xsl:value-of select="$class"/>_action_names[] = {
      <xsl:for-each select="//action/name">
        <xsl:sort select="."/>&quot;<xsl:value-of select="current()"/>&quot;,
      </xsl:for-each>
};

<xsl:variable name="allparams">
  <xsl:for-each select="//argument/name[generate-id()=generate-id(key('arguments',.))]">
    <xsl:sort select="translate(.,$lcase,$ucase)"/>
    <xsl:value-of select="."/><xsl:text> </xsl:text>
  </xsl:for-each>
</xsl:variable>

// <xsl:value-of select="$allparams"/>

<xsl:variable name="ascii48" select="'0123456789:;&lt;=&gt;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ'"/>

static const char *const <xsl:value-of select="$class"/>_action_args[] = {
<xsl:for-each select="//action/name">
  <xsl:sort select="."/>
  (<xsl:for-each select="//action[name=current()]/argumentList/argument">
    <xsl:if test="direction='in'">
  "<xsl:variable name="thisarg"><xsl:value-of select="name"/></xsl:variable>
  <xsl:for-each select="str:split($allparams)">
    <xsl:if test="current() = $thisarg"><xsl:value-of select="substring($ascii48,position(),1)"/></xsl:if>
  </xsl:for-each>" // <xsl:value-of select="$thisarg"/></xsl:if></xsl:for-each>
  ""),
</xsl:for-each>};

static const char *const <xsl:value-of select="$class"/>_action_results[] = {
<xsl:for-each select="//action/name">
  <xsl:sort select="."/>
  (<xsl:for-each select="//action[name=current()]/argumentList/argument">
    <xsl:if test="direction='out'">
  "<xsl:variable name="thisarg"><xsl:value-of select="name"/></xsl:variable>
  <xsl:for-each select="str:split($allparams)">
    <xsl:if test="current() = $thisarg"><xsl:value-of select="substring($ascii48,position(),1)"/></xsl:if>
  </xsl:for-each>" // <xsl:value-of select="$thisarg"/></xsl:if></xsl:for-each>
  ""),
</xsl:for-each>};

static const char *const <xsl:value-of select="$class"/>_param_names[] = {
<!-- This ick is the XSLT idiom for "select distinct" -->
    <xsl:for-each select="//argument/name[generate-id()=generate-id(key('arguments',.))]">
    <xsl:sort select="translate(.,$lcase,$ucase)"/>    &quot;<xsl:value-of select="current()"/>&quot;,
</xsl:for-each>};

      <xsl:for-each select="//stateVariable">
        <xsl:if test="allowedValueList">
          <xsl:variable name="name" select="str:replace(name,'A_ARG_TYPE_','')"/>
static const char *const <xsl:value-of select="$class"/>_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="$name"/>
        </xsl:call-template>_names[] = {<xsl:for-each select="allowedValueList/allowedValue">
    &quot;<xsl:value-of select="current()"/>&quot;,</xsl:for-each>
};
      </xsl:if>
    </xsl:for-each>

enum {
      <xsl:for-each select="//stateVariable">
        <xsl:sort select="./name"/>
        <xsl:if test="allowedValueList">
          <xsl:variable name="name" select="str:replace(name,'A_ARG_TYPE_','')"/>
          <xsl:value-of select="$class"/>_<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="$name"/>
        </xsl:call-template>,
      </xsl:if>
    </xsl:for-each>
    <xsl:value-of select="$class"/>_NUM_ENUMS
};

static const unsigned char <xsl:value-of select="$class"/>_param_types[] =
{
<xsl:for-each select="//argument/name[generate-id()=generate-id(key('arguments',.))]">
      <xsl:sort select="translate(.,$lcase,$ucase)"/>
<xsl:variable name="rsv" select="//argument[name=current()]/relatedStateVariable"/>
<xsl:variable name="type" select="//stateVariable[name=$rsv]/dataType"/>    (unsigned char)Data::<xsl:if test="//stateVariable[name=$rsv]/allowedValueList">ENUM + (unsigned char)<xsl:value-of select="$class"/>_<xsl:call-template name="camelcase-to-upper-underscore"><xsl:with-param name="camelcase" select="str:replace(//stateVariable[name=$rsv]/name,'A_ARG_TYPE_','')"/></xsl:call-template>, // <xsl:value-of select="current()"/>.
</xsl:if>
<xsl:if test="not(//stateVariable[name=$rsv]/allowedValueList)">
<xsl:choose select="$type">
  <xsl:when test="$type='boolean'">BOOL</xsl:when>
  <xsl:when test="$type='i1'">I8</xsl:when>
  <xsl:when test="$type='ui1'">UI8</xsl:when>
  <xsl:when test="$type='i2'">I16</xsl:when>
  <xsl:when test="$type='ui2'">UI16</xsl:when>
  <xsl:when test="$type='i4' or $type='int'">I32</xsl:when>
  <xsl:when test="$type='ui4'">UI32</xsl:when>
  <xsl:when test="$type='string' or $type='bin.base64' or $type='uri'">STRING</xsl:when>
</xsl:choose>, // <xsl:value-of select="current()"/> is a <xsl:value-of select="$type"/>.
</xsl:if>
</xsl:for-each>};

<xsl:if test="//allowedValueList">
static const EnumeratedString <xsl:value-of select="$class"/>_enums[] =
{
<xsl:for-each select="//stateVariable">
  <xsl:sort select="./name"/>
  <xsl:if test="allowedValueList">
  <xsl:variable name="name" select="str:replace(name,'A_ARG_TYPE_','')"/>
    { <xsl:value-of select="$class"/>Base::NUM_<xsl:value-of select="translate($name,$lcase,$ucase)"/>, <xsl:value-of select="$class"/>_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="$name"/>
        </xsl:call-template>_names },
      </xsl:if>
    </xsl:for-each>
};
</xsl:if>

const Data <xsl:value-of select="$class"/>Base::sm_data =
{
    { <xsl:value-of select="$class"/>Base::NUM_PARAMS, <xsl:value-of select="$class"/>_param_names },
    <xsl:value-of select="$class"/>_param_types,
    { <xsl:value-of select="$class"/>Base::NUM_ACTIONS, <xsl:value-of select="$class"/>_action_names },
    (const unsigned char *const*)<xsl:value-of select="$class"/>_action_args,
    (const unsigned char *const*)<xsl:value-of select="$class"/>_action_results,
    <xsl:if test="//allowedValueList">
<xsl:value-of select="$class"/>_enums
</xsl:if>
<xsl:if test="not(//allowedValueList)">
NULL
</xsl:if>};

} // namespace upnp

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
