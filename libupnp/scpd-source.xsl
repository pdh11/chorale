<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str"
                version="1.0">

  <xsl:include href="scpd-common.xsl"/>

<xsl:template match="/">/* This is an automatically-generated file -- DO NOT EDIT */

#include &quot;config.h&quot;
#include &quot;<xsl:value-of select="$class"/>_client.h&quot;
#include &quot;soap.h&quot;
#include &quot;libutil/trace.h&quot;
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
    </xsl:if>,
    </xsl:for-each><xsl:value-of select="name"/>Callback callback)
{
    TRACE &lt;&lt; "Unhandled <xsl:value-of select="name"/>\n";<xsl:choose>
      <xsl:when test="not(argumentList/argument[direction='out'])">
    return callback(ENOSYS);</xsl:when>
      <xsl:otherwise>
    return callback(ENOSYS, NULL);</xsl:otherwise>
    </xsl:choose>
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

const char *const <xsl:value-of select="$class"/>Base::sm_action_names[] = {
      <xsl:for-each select="//action/name">
        <xsl:sort select="."/>&quot;<xsl:value-of select="current()"/>&quot;,
      </xsl:for-each>
};

const char *const <xsl:value-of select="$class"/>Base::sm_param_names[] = {
<!-- This ick is the XSLT idiom for "select distinct" -->
    <xsl:for-each select="//argument/name[generate-id()=generate-id(key('arguments',.))]">
    <xsl:sort select="."/>    &quot;<xsl:value-of select="current()"/>&quot;,
</xsl:for-each>};

      <xsl:for-each select="//stateVariable">
        <xsl:if test="allowedValueList">
          <xsl:variable name="name" select="str:replace(name,'A_ARG_TYPE_','')"/>
const char *const <xsl:value-of select="$class"/>Base::sm_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="$name"/>
        </xsl:call-template>_names[] = {<xsl:for-each select="allowedValueList/allowedValue">
    &quot;<xsl:value-of select="current()"/>&quot;,</xsl:for-each>
};
      </xsl:if>
    </xsl:for-each>
} // namespace upnp

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
