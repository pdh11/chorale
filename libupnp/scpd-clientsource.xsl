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


        /* Sync API (deprecated) */


    <xsl:for-each select="//action">
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

struct <xsl:value-of select="$class"/>ClientAsync::<xsl:value-of select="name"/>Responder
{
    <xsl:value-of select="$class"/>Async::<xsl:value-of select="name"/>Callback callback;

    unsigned int SoapCallback(unsigned int rc, const soap::Inbound *<xsl:if test="argumentList/argument[direction='out']">in</xsl:if>)
    {
    <xsl:choose>
      <xsl:when test="count(argumentList/argument[direction='out'])=0">
         callback(rc);
      </xsl:when>
      <xsl:when test="count(argumentList/argument[direction='out'])=1">
         if (rc)
             callback(rc, NULL);
         else
         {
             <xsl:for-each select="argumentList/argument[direction='out']">
             <xsl:call-template name="decltype">
          <xsl:with-param name="var" select="current()/relatedStateVariable"/>
        </xsl:call-template> result;
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
             in->Get(&amp;result, sm_param_names[<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>]);
      </xsl:if>
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
             result = (<xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>)in->GetEnum(
            sm_param_names[<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>],
            sm_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
        </xsl:call-template>_names,
            NUM_<xsl:value-of select="translate(str:replace(current()/relatedStateVariable,'A_ARG_TYPE_',''),$lcase,$ucase)"/>);
      </xsl:if>
      </xsl:for-each>
             callback(rc, &amp;result);
         }
      </xsl:when>
      <xsl:when test="count(argumentList/argument[direction='out'])>1">
         if (rc)
             callback(rc, NULL);
         else
         {
             <xsl:value-of select="$class"/>Async::<xsl:value-of select="name"/>Response result;

<xsl:for-each select="argumentList/argument[direction='out']">
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
        result.<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template> = (<xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>)in->GetEnum(
            sm_param_names[<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>],
            sm_<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
        </xsl:call-template>_names,
            NUM_<xsl:value-of select="translate(str:replace(current()/relatedStateVariable,'A_ARG_TYPE_',''),$lcase,$ucase)"/>);</xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
        in->Get(&amp;result.<xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>, sm_param_names[<xsl:call-template name="camelcase-to-upper-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>]);</xsl:if>
      </xsl:for-each>

             callback(rc, &amp;result);
         }
      </xsl:when>
    </xsl:choose>
         delete this;
         return 0;
    }
};

unsigned int <xsl:value-of select="$class"/>ClientAsync::<xsl:value-of select="name"/>(
      <xsl:for-each select="argumentList/argument[direction='in']">
        <xsl:call-template name="intype">
          <xsl:with-param name="var" select="current()/relatedStateVariable"/>
        </xsl:call-template><xsl:text> </xsl:text>
        <xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template><xsl:text>,
      </xsl:text>
      </xsl:for-each>
      <xsl:value-of select="name"/>Callback callback)
{
    <xsl:if test="argumentList/argument[direction='in']">upnp::soap::Outbound out;
    </xsl:if>

<xsl:for-each select="argumentList/argument[direction='in']">
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
        </xsl:call-template></xsl:if>);<xsl:text />
</xsl:for-each>
<xsl:text>
    </xsl:text>
    <xsl:value-of select="name"/>Responder *r =
        new <xsl:value-of select="name"/>Responder;
    r->callback = callback;

    unsigned int rc = SoapAction(&quot;<xsl:value-of select="name"/>&quot;, 
          <xsl:if test="argumentList/argument[direction='in']">out,</xsl:if>
          util::Bind2&lt;unsigned int, const soap::Inbound*,
              <xsl:value-of select="name"/>Responder,
              &amp;<xsl:value-of select="name"/>Responder::SoapCallback&gt;(r));
    if (rc)
        delete r;
    return rc;
}
    </xsl:for-each>

void <xsl:value-of select="$class"/>ClientAsync::OnEvent(const char *var, const std::string&amp; value)
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

} // namespace upnp

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
