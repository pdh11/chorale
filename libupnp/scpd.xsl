<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str"
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
  <xsl:variable name="letters" select="concat($ucase, $lcase)" />
  <xsl:variable name="digits" select="'0123456789'" />

  <xsl:variable name="h" select="$mode = 'header'" />
  <xsl:variable name="s" select="$mode = 'stubs'" />
  <xsl:variable name="ch" select="$mode = 'clientheader'" />
  <xsl:variable name="cs" select="$mode = 'client'" />
  <xsl:variable name="sh" select="$mode = 'serverheader'" />
  <xsl:variable name="ss" select="$mode = 'server'" />

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

  <!-- convert SCPD type into C++ type -->
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

  <!-- convert SCPD type into C++ type for argument passing
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

<xsl:template match="/">/* This is an automatically-generated file -- DO NOT EDIT */
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
    virtual void On<xsl:value-of select="name"/>(<xsl:call-template name="argtype">
          <xsl:with-param name="type" select="$type"/>
        </xsl:call-template>) {} </xsl:if>
      </xsl:for-each>
};

/** Base class generated automatically from SCPD <xsl:value-of select="$class"/>.xml
 *
 * Also includes enumerators and string-tables for action and argument names.
 */
class <xsl:value-of select="$class"/>: public util::Observable&lt;<xsl:value-of select="$class"/>Observer, util::OneObserver, util::NoLocking&gt;
{
public:
    virtual ~<xsl:value-of select="$class"/>() {}

    enum Action {
        <xsl:for-each select="//action/name">
          <xsl:sort select="."/>
          <xsl:call-template name="camelcase-to-upper-underscore">
            <xsl:with-param name="camelcase" select="current()"/>
          </xsl:call-template>,
        </xsl:for-each>
        NUM_ACTIONS
    };
    static const char *const sm_action_names[];

    enum Param {
        <!-- This ick is the XSLT idiom for "select distinct" -->
        <xsl:for-each select="//argument/name[generate-id()=generate-id(key('arguments',.))]">
          <xsl:sort select="."/>
          <xsl:call-template name="camelcase-to-upper-underscore">
            <xsl:with-param name="camelcase" select="current()"/>
          </xsl:call-template>,
        </xsl:for-each>
        NUM_PARAMS
    };
    static const char *const sm_param_names[];
      <xsl:for-each select="//stateVariable">
        <xsl:if test="allowedValueList">
          <xsl:variable name="name" select="str:replace(name,'A_ARG_TYPE_','')"/>
    enum <xsl:value-of select="$name"/> {
        <xsl:for-each select="allowedValueList/allowedValue">
          <xsl:value-of select="translate($name,$lcase,$ucase)"/>_<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="current()"/>
        </xsl:call-template>,
        </xsl:for-each>
        NUM_<xsl:value-of select="translate($name,$lcase,$ucase)"/>
    };
    static const char *const sm_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="$name"/>
        </xsl:call-template>_names[];
        </xsl:if>
      </xsl:for-each>
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
    <xsl:value-of select="$class"/>Server(Device *device, const char *service_id, const char *type, const char *scpdurl, <xsl:value-of select="$class"/> *impl)
        : Service(device, service_id, type, scpdurl),
          m_impl(impl)
    {
        m_impl->AddObserver(this);
    }

    // Being a soap::Server
    unsigned int OnAction(const char *name, const soap::Inbound&amp; in,
                          soap::Outbound *out);
    // Being a upnp::Service
    void GetEventedVariables(soap::Outbound*);

    // Being a <xsl:value-of select="$class"/>Observer <xsl:for-each select="//stateVariable">
        <xsl:if test="sendEventsAttribute='yes'">
          <xsl:variable name="type" select="dataType"/>
    void On<xsl:value-of select="name"/>(<xsl:call-template name="argtype">
          <xsl:with-param name="type" select="$type"/>
        </xsl:call-template>);</xsl:if>
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
          <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
            <xsl:value-of select="$class"/>::<xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
          </xsl:if>
          <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
          <xsl:call-template name="cpptype">
            <xsl:with-param name="type" select="$type"/>
          </xsl:call-template></xsl:if><xsl:text> </xsl:text><xsl:value-of select="name"/>;
</xsl:if>
      </xsl:for-each>

      <!-- Call implementation -->
      rc = m_impl-&gt;<xsl:value-of select="name"/>(
<xsl:for-each select="argumentList/argument">
        <xsl:text>        </xsl:text>
        <xsl:if test="direction='out'">&amp;<xsl:value-of select="name"/></xsl:if>
      <xsl:if test="direction='in'">
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
        (<xsl:value-of select="$class"/>::<xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>)in.GetEnum(
            <xsl:value-of select="$class"/>::sm_param_names[<xsl:value-of select="$class"/>::<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>],
            <xsl:value-of select="$class"/>::sm_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
        </xsl:call-template>_names,
            <xsl:value-of select="$class"/>::NUM_<xsl:value-of select="translate(str:replace(current()/relatedStateVariable,'A_ARG_TYPE_',''),$lcase,$ucase)"/>)</xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/><xsl:choose>
      <xsl:when test="$type='boolean'">in.GetBool</xsl:when>
      <xsl:when test="$type='i1'">(int8_t)in.GetInt</xsl:when>
      <xsl:when test="$type='ui1'">(uint8_t)in.GetUInt</xsl:when>
      <xsl:when test="$type='i2'">(short)in.GetInt</xsl:when>
      <xsl:when test="$type='ui2'">(unsigned short)in.GetUInt</xsl:when>
      <xsl:when test="$type='i4'">in.GetInt</xsl:when>
      <xsl:when test="$type='ui4'">in.GetUInt</xsl:when>
      <xsl:when test="$type='uri' or $type='string'">in.GetString</xsl:when>
      <xsl:otherwise><xsl:value-of select="$type"/></xsl:otherwise>
    </xsl:choose>(<xsl:value-of select="$class"/>::sm_param_names[<xsl:value-of select="$class"/>::<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>])</xsl:if>
      </xsl:if>
    <xsl:if test="not(position()=last())">,
</xsl:if>
    </xsl:for-each>
      );

    <!-- Assign from output variables -->
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='out'">
          <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
          <xsl:variable name="name" select="str:replace(name,'A_ARG_TYPE_','')"/>
        if ((unsigned)<xsl:value-of select="$name"/> &lt; <xsl:value-of select="$class"/>::NUM_<xsl:value-of select="translate(str:replace(current()/relatedStateVariable,'A_ARG_TYPE_',''),$lcase,$ucase)"/>)
            result->Add(<xsl:value-of select="$class"/>::sm_param_names[<xsl:value-of select="$class"/>::<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="$name"/>
        </xsl:call-template>], <xsl:value-of select="$class"/>::sm_<xsl:call-template name="camelcase-to-underscore">
        <xsl:with-param name="camelcase" select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/></xsl:call-template>_names[<xsl:value-of select="name"/>]);
      </xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
        result->Add(<xsl:value-of select="$class"/>::sm_param_names[<xsl:value-of select="$class"/>::<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>], <xsl:value-of select="name"/>);</xsl:if>
      </xsl:if>
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
void <xsl:value-of select="$class"/>Server::On<xsl:value-of select="name"/>(<xsl:call-template name="argtype">
            <xsl:with-param name="type" select="$type"/>
          </xsl:call-template> value)
{
    FireEvent(&quot;<xsl:value-of select="name"/>&quot;, value);
}
</xsl:if>
      </xsl:for-each>

      </xsl:when>
      <xsl:otherwise>
    <xsl:for-each select="//action">
<xsl:if test="$h or $ch or $sh"><xsl:text>
    </xsl:text></xsl:if>
    <xsl:if test="$h">virtual </xsl:if>unsigned int <xsl:if test="$cs">
    <xsl:value-of select="$class"/>Client::</xsl:if><xsl:if test="$s">
    <xsl:value-of select="$class"/>::</xsl:if>
      <xsl:value-of select="name"/>(
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
      <xsl:if test="not($s)">
        <xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template></xsl:if>
      <xsl:if test="not(position()=last())">,
      </xsl:if>
    </xsl:for-each>)<xsl:choose>
    <xsl:when test="$h"><!--<xsl:if test="not(Optional)"> = 0</xsl:if>-->;</xsl:when>
    <xsl:when test="$ch">;</xsl:when>
    <xsl:when test="$s">
{
    TRACE &lt;&lt; "Unhandled <xsl:value-of select="name"/>\n";
    return ENOSYS;
}
    </xsl:when>
    <xsl:when test="$cs">
{
    <xsl:if test="argumentList/argument[direction='in']">upnp::soap::Outbound out;
    </xsl:if>
    <xsl:if test="argumentList/argument[direction='out']">upnp::soap::Inbound in;
    </xsl:if>
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='in'">
          <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
    out.Add(sm_param_names[<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>], <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">sm_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
        </xsl:call-template>_names[<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>]</xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)"><xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template></xsl:if>);</xsl:if>
</xsl:for-each>

<xsl:if test="argumentList/argument[direction='out']">
  <xsl:if test="argumentList/argument[direction='in']">
    <!-- Both in and out -->
    unsigned int rc = SoapAction(&quot;<xsl:value-of select="name"/>&quot;, out, &amp;in);
  </xsl:if>
  <xsl:if test="not(argumentList/argument[direction='in'])">
    <!-- Out only -->
    unsigned int rc = SoapAction(&quot;<xsl:value-of select="name"/>&quot;, &amp;in);
  </xsl:if>
</xsl:if>
<xsl:if test="not(argumentList/argument[direction='out'])">
  <xsl:if test="argumentList/argument[direction='in']">
    <!-- In only -->
    unsigned int rc = SoapAction(&quot;<xsl:value-of select="name"/>&quot;, out);
  </xsl:if>
  <xsl:if test="not(argumentList/argument[direction='in'])">
    <!-- Neither -->
    unsigned int rc = SoapAction(&quot;<xsl:value-of select="name"/>&quot;);
  </xsl:if>
</xsl:if>
    if (rc != 0)
        return rc;
<xsl:for-each select="argumentList/argument">
        <xsl:if test="direction='out'">
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
    if (<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>)
        *<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template> = (<xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>)in.GetEnum(
            sm_param_names[<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>],
            sm_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
        </xsl:call-template>_names,
            NUM_<xsl:value-of select="translate(str:replace(current()/relatedStateVariable,'A_ARG_TYPE_',''),$lcase,$ucase)"/>);</xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
    in.Get(<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>, sm_param_names[<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>]);</xsl:if>
      </xsl:if>
      </xsl:for-each>
    return 0;
}

    </xsl:when>
  </xsl:choose>
</xsl:for-each>

<xsl:if test="$h">
  
    /* Getters for evented state variables not otherwise covered */
  <xsl:for-each select="//stateVariable">
    <xsl:if test="not(contains(name,'A_ARG_TYPE')) and sendEventsAttribute='yes'">
      <xsl:variable name="getter" select="concat('Get',name)"/>
      <xsl:if test="not(//action[name=$getter])">
    virtual unsigned int <xsl:value-of select="$getter"/>(<xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="dataType"/>
      </xsl:call-template>*);</xsl:if>
    </xsl:if>
  </xsl:for-each>

</xsl:if>

<xsl:if test="$h or $ch">
};</xsl:if>
      
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
  
    /* Getters for state variables not otherwise covered */
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

      <xsl:for-each select="//stateVariable">
        <xsl:if test="allowedValueList">
          <xsl:variable name="name" select="str:replace(name,'A_ARG_TYPE_','')"/>
    const char *const <xsl:value-of select="$class"/>::sm_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="$name"/>
        </xsl:call-template>_names[] = {<xsl:for-each select="allowedValueList/allowedValue">
        &quot;<xsl:value-of select="current()"/>&quot;,</xsl:for-each>
    };
      </xsl:if>
    </xsl:for-each>
</xsl:if>
<xsl:if test="$ss">

void <xsl:value-of select="$class"/>Server::GetEventedVariables(soap::Outbound *vars)
{
  <xsl:for-each select="//stateVariable">
    <xsl:if test="not(contains(name,'A_ARG_TYPE')) and sendEventsAttribute='yes'">
      <xsl:variable name="getter" select="concat('Get',name)"/>
      <xsl:if test="not(//action[name=$getter])">
    {
        <xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="dataType"/>
        </xsl:call-template> val;
        if (m_impl-><xsl:value-of select="$getter"/>(&amp;val) == 0)
            vars->Add("<xsl:value-of select="name"/>", val);
    }
</xsl:if>
    </xsl:if>
  </xsl:for-each>
}

</xsl:if>

} // namespace upnp

<xsl:if test="$h or $ch or $sh">#endif</xsl:if>

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
