<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

  <!-- Generate C++ source directly from the SCPD (service description).
     @todo This could be more optimal (assemble the SOAP string directly)
           once not using Intel libupnp.
    -->

  <xsl:output method="text"/>
  <xsl:param name="mode"/>
  <xsl:param name="class"/>

  <xsl:variable name="ucase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'" />
  <xsl:variable name="lcase" select="'abcdefghijklmnopqrstuvwxyz'" />

  <xsl:variable name="h" select="$mode = 'header'" />
  <xsl:variable name="s" select="$mode = 'stubs'" />
  <xsl:variable name="ch" select="$mode = 'clientheader'" />
  <xsl:variable name="cs" select="$mode = 'client'" />
  <xsl:variable name="sh" select="$mode = 'serverheader'" />
  <xsl:variable name="ss" select="$mode = 'server'" />

  <xsl:template match="/">
/* This is an automatically-generated file -- DO NOT EDIT */

    <xsl:choose>

      <!-- Interface header -->

      <xsl:when test="$h">
#ifndef UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_H
#define UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_H 1

#include &lt;string&gt;
#include &lt;stdint.h&gt;

namespace upnp {

class <xsl:value-of select="$class"/>
{
public:
    virtual ~<xsl:value-of select="$class"/>() {}
</xsl:when>

      <!-- Client header -->

      <xsl:when test="$ch">
#ifndef UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_CLIENT_H
#define UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_CLIENT_H 1

#include &quot;<xsl:value-of select="$class"/>.h&quot;

namespace upnp {

namespace soap { class Connection; };

class <xsl:value-of select="$class"/>Client: public <xsl:value-of select="$class"/>
{
    upnp::soap::Connection *m_soap;

public:
    explicit <xsl:value-of select="$class"/>Client(upnp::soap::Connection *soap)
      : m_soap(soap) {}
</xsl:when>

    <!-- Client source (or stubs) -->

    <xsl:when test="$cs or $s">
#include &quot;config.h&quot;
#include &quot;<xsl:value-of select="$class"/>_client.h&quot;
#include &quot;soap.h&quot;
#include &lt;boost/format.hpp&gt;
#include &lt;errno.h&gt;

namespace upnp {  
    </xsl:when>

    <!-- Server header -->

    <xsl:when test="$sh">
#ifndef UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_SERVER_H
#define UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_SERVER_H 1

#include &quot;soap.h&quot;

namespace upnp {

class <xsl:value-of select="$class"/>;

class <xsl:value-of select="$class"/>Server: public soap::Server
{
    <xsl:value-of select="$class"/> *m_impl;

public:
    explicit <xsl:value-of select="$class"/>Server(<xsl:value-of select="$class"/> *impl)
        : m_impl(impl)
    {}

    // Being a soap::Server
    soap::Params OnAction(const char *name, const soap::Params&amp; in);
};
    </xsl:when>

    <!-- Server -->

    <xsl:when test="$ss">
#include &quot;config.h&quot;
#include &quot;<xsl:value-of select="$class"/>_server.h&quot;
#include &quot;<xsl:value-of select="$class"/>.h&quot;
#include &quot;soap.h&quot;
#include &lt;boost/format.hpp&gt;

namespace upnp {
    </xsl:when>

    </xsl:choose>

    <!-- Body -->

    <xsl:choose>
      <xsl:when test="$sh">
      </xsl:when>
      <xsl:when test="$ss">
soap::Params <xsl:value-of select="$class"/>Server::OnAction(const char *name, const soap::Params&amp; params0)
{
    soap::Params in = params0;
    soap::Params result;
    unsigned int rc;

    <xsl:for-each select="//action">
    if (!strcmp(name, &quot;<xsl:value-of select="name"/>&quot;))
    {
<!-- Declare output variables -->
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='out'">
          <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
          <xsl:text>        </xsl:text>
          <xsl:choose>
          <xsl:when test="$type='boolean'">bool</xsl:when>
        <xsl:when test="$type='i2'">int16_t</xsl:when>
        <xsl:when test="$type='ui2'">uint16_t</xsl:when>
        <xsl:when test="$type='i4'">int32_t</xsl:when>
        <xsl:when test="$type='ui4'">uint32_t</xsl:when>
        <xsl:when test="$type='uri'">std::string</xsl:when>
        <xsl:when test="$type='string'">std::string</xsl:when>
          <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
        </xsl:choose><xsl:text> </xsl:text><xsl:value-of select="name"/>;
</xsl:if>
      </xsl:for-each>

      <!-- Call implementation -->
      rc = m_impl-&gt;<xsl:value-of select="name"/>(
      <xsl:for-each select="argumentList/argument">
        <xsl:text>                </xsl:text>
        <xsl:if test="direction='out'">&amp;<xsl:value-of select="name"/></xsl:if>
      <xsl:if test="direction='in'">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/><xsl:choose>
          <xsl:when test="$type='boolean'">upnp::soap::ParseBool(in[&quot;<xsl:value-of select="name"/>&quot;])</xsl:when>
          <xsl:when test="$type='i2' or $type='i4'">strtol(in[&quot;<xsl:value-of select="name"/>&quot;].c_str(), NULL, 10)</xsl:when>
          <xsl:when test="$type='ui2' or $type='ui4'">strtoul(in[&quot;<xsl:value-of select="name"/>&quot;].c_str(), NULL, 10)</xsl:when>
          <xsl:when test="$type='uri' or $type='string'">in[&quot;<xsl:value-of select="name"/>&quot;]</xsl:when>
          <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
          </xsl:choose>
      </xsl:if>
      <xsl:if test="not(position()=last())">,
</xsl:if>
    </xsl:for-each>
      );

    <!-- Assign from output variables -->
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='out'">
          <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
        result[&quot;<xsl:value-of select="name"/>&quot;] = <xsl:choose>
            <xsl:when test="$type='boolean'"><xsl:value-of select="name"/> ? &quot;1&quot; : &quot;0&quot;</xsl:when>
            <xsl:when test="$type='i2' or $type='i4'">(boost::format(&quot;%d&quot;) % <xsl:value-of select="name"/>).str()</xsl:when>
            <xsl:when test="$type='ui2' or $type='ui4'">(boost::format(&quot;%u&quot;) % <xsl:value-of select="name"/>).str()</xsl:when>
            <xsl:when test="$type='uri' or $type='string'"><xsl:value-of select="name"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
          </xsl:choose>;</xsl:if>
      </xsl:for-each>
    }
      <xsl:if test="not(position()=last())">else
      </xsl:if>
    </xsl:for-each>

    return result;
}
      </xsl:when>
      <xsl:otherwise>
    <xsl:for-each select="//action">
<xsl:text>
    </xsl:text>
    <xsl:if test="$h">virtual </xsl:if>unsigned int <xsl:if test="$cs">
    <xsl:value-of select="$class"/>Client::</xsl:if><xsl:if test="$s">
    <xsl:value-of select="$class"/>::</xsl:if>
      <xsl:value-of select="name"/>(
      <xsl:for-each select="argumentList/argument">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:text>      </xsl:text>
      <xsl:choose>
        <xsl:when test="$type='boolean'">bool</xsl:when>
        <xsl:when test="$type='i2'">int16_t</xsl:when>
        <xsl:when test="$type='ui2'">uint16_t</xsl:when>
        <xsl:when test="$type='i4'">int32_t</xsl:when>
        <xsl:when test="$type='ui4'">uint32_t</xsl:when>
        <xsl:when test="$type='uri'">std::string</xsl:when>
        <xsl:when test="$type='string'">std::string</xsl:when>
        <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
      </xsl:choose>
      <xsl:if test="direction='in'" xml:space="preserve"> </xsl:if>
      <xsl:if test="direction='out'"> *</xsl:if>
      <xsl:if test="not($s)"><xsl:value-of select="name"/></xsl:if>
      <xsl:if test="not(position()=last())">,
      </xsl:if>
    </xsl:for-each>)<xsl:choose>
    <xsl:when test="$h">;</xsl:when>
    <xsl:when test="$ch">;</xsl:when>
    <xsl:when test="$s">
{
    return ENOSYS;
}
    </xsl:when>
    <xsl:when test="$cs">
{
    upnp::soap::Params in;
      
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='in'">
          <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>    in[&quot;<xsl:value-of select="name"/>&quot;] = <xsl:choose>
            <xsl:when test="$type='boolean'"><xsl:value-of select="name"/> ? &quot;1&quot; : &quot;0&quot;</xsl:when>
            <xsl:when test="$type='i2' or $type='i4'">(boost::format(&quot;%d&quot;) % <xsl:value-of select="name"/>).str()</xsl:when>
            <xsl:when test="$type='ui2' or $type='ui4'">(boost::format(&quot;%u&quot;) % <xsl:value-of select="name"/>).str()</xsl:when>
            <xsl:when test="$type='uri' or $type='string'"><xsl:value-of select="name"/></xsl:when>
            <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
          </xsl:choose>;
</xsl:if>
      </xsl:for-each>
    upnp::soap::Params out = m_soap->Action(&quot;<xsl:value-of select="name"/>&quot;, in);
      
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='out'">
          <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>    *<xsl:value-of select="name"/> = <xsl:choose>
          <xsl:when test="$type='boolean'">upnp::soap::ParseBool(out[&quot;<xsl:value-of select="name"/>&quot;])</xsl:when>
          <xsl:when test="$type='i2' or $type='i4'">strtol(out[&quot;<xsl:value-of select="name"/>&quot;].c_str(), NULL, 10)</xsl:when>
          <xsl:when test="$type='ui2' or $type='ui4'">strtoul(out[&quot;<xsl:value-of select="name"/>&quot;].c_str(), NULL, 10)</xsl:when>
          <xsl:when test="$type='uri' or $type='string'">out[&quot;<xsl:value-of select="name"/>&quot;]</xsl:when>
          <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
          </xsl:choose>;
</xsl:if>
      </xsl:for-each>
    return 0;
}

    </xsl:when>
  </xsl:choose>
</xsl:for-each><xsl:if test="$h or $ch">};</xsl:if>
      
      </xsl:otherwise>
    </xsl:choose>

}; // namespace upnp

<xsl:if test="$h or $ch or $sh">#endif</xsl:if>

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
