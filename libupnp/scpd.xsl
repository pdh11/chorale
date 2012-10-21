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

  <xsl:key name="arguments" match="argument/name" use="."/>

  <xsl:template match="/">
/* This is an automatically-generated file -- DO NOT EDIT */

    <xsl:choose>

      <!-- Interface header -->

      <xsl:when test="$h">
#ifndef UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_H
#define UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_H 1

#include &lt;string&gt;
#include &lt;stdint.h&gt;
#include &quot;libutil/observable.h&quot;

namespace upnp {

/** Observer interface generated automatically from SCPD <xsl:value-of select="$class"/>.xml
 */
class <xsl:value-of select="$class"/>Observer
{
public:
    virtual ~<xsl:value-of select="$class"/>Observer() {}
      <xsl:for-each select="//stateVariable">
        <xsl:if test="sendEventsAttribute='yes'">
          <xsl:variable name="type" select="dataType"/>
    virtual void On<xsl:value-of select="name"/>(<xsl:choose>
        <xsl:when test="$type='boolean'">bool</xsl:when>
        <xsl:when test="$type='i2'">int16_t</xsl:when>
        <xsl:when test="$type='ui2'">uint16_t</xsl:when>
        <xsl:when test="$type='i4'">int32_t</xsl:when>
        <xsl:when test="$type='ui4'">uint32_t</xsl:when>
        <xsl:when test="$type='uri'">const std::string&amp;</xsl:when>
        <xsl:when test="$type='string'">const std::string&amp;</xsl:when>
          <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
        </xsl:choose>) {} </xsl:if>
      </xsl:for-each>
};

/** Base class generated automatically from SCPD <xsl:value-of select="$class"/>.xml
 *
 * Also includes enumerators and string-tables for action and argument names.
 */
class <xsl:value-of select="$class"/>: public util::Observable&lt;<xsl:value-of select="$class"/>Observer, util::OneObserver&lt;<xsl:value-of select="$class"/>Observer&gt;, util::NoLocking&gt;
{
public:
    virtual ~<xsl:value-of select="$class"/>() {}

    enum Action {
      <xsl:for-each select="//action/name">
        <xsl:sort select="."/>
        <xsl:value-of select="translate(current(),$lcase,$ucase)"/>,
      </xsl:for-each>
      NUM_ACTIONS
    };
    enum Param {
        <!-- This ick is the XSLT idiom for "select distinct" -->
        <xsl:for-each select="//argument/name[generate-id()=generate-id(key('arguments',.))]">
        <xsl:sort select="."/>
        <xsl:value-of select="translate(current(),$lcase,$ucase)"/>,
      </xsl:for-each>
        NUM_PARAMS
    };

    static const char *const sm_action_names[];
    static const char *const sm_param_names[];
</xsl:when>

      <!-- Client header -->

      <xsl:when test="$ch">
#ifndef UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_CLIENT_H
#define UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_CLIENT_H 1

#include &quot;<xsl:value-of select="$class"/>.h&quot;
#include &quot;client.h&quot;

namespace upnp {

/** Client implementation generated automatically from SCPD <xsl:value-of select="$class"/>.xml
 */
class <xsl:value-of select="$class"/>Client: public <xsl:value-of select="$class"/>, public ClientConnection
{
public:

    // Being a ServiceObserver
    void OnEvent(const char *var, const std::string&amp; value);
</xsl:when>
    <!-- Client source (or stubs) -->

    <xsl:when test="$cs or $s">
#include &quot;config.h&quot;
#include &quot;<xsl:value-of select="$class"/>_client.h&quot;
#include &quot;soap.h&quot;
#include &quot;libutil/trace.h&quot;
#include &lt;errno.h&gt;
#include &lt;string.h&gt;

namespace upnp {  
    </xsl:when>

    <!-- Server header -->

    <xsl:when test="$sh">
#ifndef UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_SERVER_H
#define UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_SERVER_H 1

#include &quot;device.h&quot;
#include &quot;<xsl:value-of select="$class"/>.h&quot;

namespace upnp {

class <xsl:value-of select="$class"/>;

/** Server implementation generated automatically from SCPD <xsl:value-of select="$class"/>.xml
 */
class <xsl:value-of select="$class"/>Server: public Service, public <xsl:value-of select="$class"/>Observer
{
    <xsl:value-of select="$class"/> *m_impl;

public:
    <xsl:value-of select="$class"/>Server(const char *type, const char *scpdurl, <xsl:value-of select="$class"/> *impl)
        : Service(type, scpdurl),
          m_impl(impl)
    {
        m_impl->AddObserver(this);
    }

    // Being a soap::Server
    unsigned int OnAction(const char *name, const soap::Inbound&amp; in,
                          soap::Outbound *out);
    //const char *const GetActionNameTable() const;
    //const char *const GetParamNameTable() const;

    // Being a <xsl:value-of select="$class"/>Observer <xsl:for-each select="//stateVariable">
        <xsl:if test="sendEventsAttribute='yes'">
          <xsl:variable name="type" select="dataType"/>
    void On<xsl:value-of select="name"/>(<xsl:choose>
        <xsl:when test="$type='boolean'">bool</xsl:when>
        <xsl:when test="$type='i2'">int16_t</xsl:when>
        <xsl:when test="$type='ui2'">uint16_t</xsl:when>
        <xsl:when test="$type='i4'">int32_t</xsl:when>
        <xsl:when test="$type='ui4'">uint32_t</xsl:when>
        <xsl:when test="$type='uri'">const std::string&amp;</xsl:when>
        <xsl:when test="$type='string'">const std::string&amp;</xsl:when>
          <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
        </xsl:choose>);</xsl:if>
      </xsl:for-each>
};
    </xsl:when>

    <!-- Server -->

    <xsl:when test="$ss">
#include &quot;config.h&quot;
#include &quot;<xsl:value-of select="$class"/>_server.h&quot;
#include &quot;<xsl:value-of select="$class"/>.h&quot;
#include &quot;soap.h&quot;
#include &quot;string.h&quot;

namespace upnp {
    </xsl:when>

    </xsl:choose>

    <!-- Body -->

    <xsl:choose>
      <xsl:when test="$sh">
      </xsl:when>
      <xsl:when test="$ss">
unsigned int <xsl:value-of select="$class"/>Server::OnAction(const char *name, const soap::Inbound&amp; in, soap::Outbound *result)
{
    unsigned int rc = 0;

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
        <xsl:text>        </xsl:text>
        <xsl:if test="direction='out'">&amp;<xsl:value-of select="name"/></xsl:if>
      <xsl:if test="direction='in'">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/><xsl:choose>
      <xsl:when test="$type='boolean'">in.GetBool</xsl:when>
      <xsl:when test="$type='i2'">(short)in.GetInt</xsl:when>
      <xsl:when test="$type='ui2'">(unsigned short)in.GetUInt</xsl:when>
      <xsl:when test="$type='i4'">in.GetInt</xsl:when>
      <xsl:when test="$type='ui4'">in.GetUInt</xsl:when>
      <xsl:when test="$type='uri' or $type='string'">in.GetString</xsl:when>
      <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
    </xsl:choose>(<xsl:value-of select="$class"/>::sm_param_names[<xsl:value-of select="$class"/>::<xsl:value-of select="translate(name,$lcase,$ucase)"/>])</xsl:if>
    <xsl:if test="not(position()=last())">,
</xsl:if>
    </xsl:for-each>
      );

    <!-- Assign from output variables -->
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='out'">
          <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
          result->Add(<xsl:value-of select="$class"/>::sm_param_names[<xsl:value-of select="$class"/>::<xsl:value-of select="translate(name,$lcase,$ucase)"/>], <xsl:value-of select="name"/>);</xsl:if>
      </xsl:for-each>
    }
      <xsl:if test="not(position()=last())">else
      </xsl:if>
    </xsl:for-each>

    return rc;
}
      <xsl:for-each select="//stateVariable">
        <xsl:if test="sendEventsAttribute='yes'">
          <xsl:variable name="type" select="dataType"/>
void <xsl:value-of select="$class"/>Server::On<xsl:value-of select="name"/>(<xsl:choose>
        <xsl:when test="$type='boolean'">bool</xsl:when>
        <xsl:when test="$type='i2'">int16_t</xsl:when>
        <xsl:when test="$type='ui2'">uint16_t</xsl:when>
        <xsl:when test="$type='i4'">int32_t</xsl:when>
        <xsl:when test="$type='ui4'">uint32_t</xsl:when>
        <xsl:when test="$type='uri'">const std::string&amp;</xsl:when>
        <xsl:when test="$type='string'">const std::string&amp;</xsl:when>
          <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
        </xsl:choose> value)
{
    FireEvent(&quot;<xsl:value-of select="name"/>&quot;, value);
}
</xsl:if>
      </xsl:for-each>

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
      <xsl:if test="direction='in'"><!-- Input types -->
      <xsl:choose>
        <xsl:when test="$type='boolean'">bool</xsl:when>
        <xsl:when test="$type='i2'">int16_t</xsl:when>
        <xsl:when test="$type='ui2'">uint16_t</xsl:when>
        <xsl:when test="$type='i4'">int32_t</xsl:when>
        <xsl:when test="$type='ui4'">uint32_t</xsl:when>
        <xsl:when test="$type='uri'">const std::string&amp;</xsl:when>
        <xsl:when test="$type='string'">const std::string&amp;</xsl:when>
        <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
      </xsl:choose>
      <xsl:text> </xsl:text>
      </xsl:if>
      <xsl:if test="direction='out'"><!-- Output types -->
      <xsl:choose>
        <xsl:when test="$type='boolean'">bool *</xsl:when>
        <xsl:when test="$type='i2'">int16_t *</xsl:when>
        <xsl:when test="$type='ui2'">uint16_t *</xsl:when>
        <xsl:when test="$type='i4'">int32_t *</xsl:when>
        <xsl:when test="$type='ui4'">uint32_t *</xsl:when>
        <xsl:when test="$type='uri'">std::string *</xsl:when>
        <xsl:when test="$type='string'">std::string *</xsl:when>
        <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
      </xsl:choose>
      </xsl:if>
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
    upnp::soap::Outbound out;
    upnp::soap::Inbound in;
      
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='in'">
          <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>    out.Add(sm_param_names[<xsl:value-of select="translate(name,$lcase,$ucase)"/>], <xsl:value-of select="name"/>);
</xsl:if>
      </xsl:for-each>
    unsigned int rc = SoapAction(&quot;<xsl:value-of select="name"/>&quot;, out, &amp;in);
    if (rc != 0)
        return rc;
      
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='out'">
    in.Get(<xsl:value-of select="name"/>, sm_param_names[<xsl:value-of select="translate(name,$lcase,$ucase)"/>]);</xsl:if>
      </xsl:for-each>
    return 0;
}

    </xsl:when>
  </xsl:choose>
</xsl:for-each><xsl:if test="$h or $ch">};</xsl:if>
      
      </xsl:otherwise>
    </xsl:choose>

<xsl:if test="$cs">

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
     
</xsl:if>

<xsl:if test="$s">

  const char *const <xsl:value-of select="$class"/>::sm_action_names[] = {
      <xsl:for-each select="//action/name">
        <xsl:sort select="."/>&quot;<xsl:value-of select="current()"/>&quot;,
      </xsl:for-each>
    };
  const char *const <xsl:value-of select="$class"/>::sm_param_names[] = {
        <!-- This ick is the XSLT idiom for "select distinct" -->
      <xsl:for-each select="//argument/name[generate-id()=generate-id(key('arguments',.))]">
        <xsl:sort select="."/>&quot;<xsl:value-of select="current()"/>&quot;,
      </xsl:for-each>
    };
</xsl:if>

} // namespace upnp

<xsl:if test="$h or $ch or $sh">#endif</xsl:if>

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
