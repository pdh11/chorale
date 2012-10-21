<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str"
                version="1.0">

  <xsl:include href="scpd-common.xsl"/>

<xsl:template match="/">/* This is an automatically-generated file -- DO NOT EDIT */

#ifndef UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_CLIENT_H
#define UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_CLIENT_H 1

#include &quot;<xsl:value-of select="$class"/>.h&quot;
#include &quot;client.h&quot;

namespace upnp {

/** Client implementation generated automatically from SCPD <xsl:value-of select="$class"/>.xml
 *
 * This client is synchronous, which is probably not what you want; see <xsl:value-of select="$class"/>ClientAsync.
 */
class <xsl:value-of select="$class"/>Client: public <xsl:value-of select="$class"/>, public ServiceClient
{
public:
    <xsl:value-of select="$class"/>Client(DeviceClient *device, const char *service_id)
      : ServiceClient(device, service_id)
    {}

    // Being a ServiceObserver
    void OnEvent(const char *var, const std::string&amp; value);
    <xsl:for-each select="//action">
    unsigned int <xsl:value-of select="name"/>(
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
    </xsl:for-each>);
</xsl:for-each>
};

class <xsl:value-of select="$class"/>ClientAsync: public <xsl:value-of select="$class"/>Async, public ServiceClient
{ <xsl:for-each select="//action">
    class <xsl:value-of select="name"/>Responder;</xsl:for-each>

public:
    <xsl:value-of select="$class"/>ClientAsync(DeviceClient *device, const char *service_id)
      : ServiceClient(device, service_id)
    {}

    // Being a ServiceObserver
    void OnEvent(const char *var, const std::string&amp; value);
    <xsl:for-each select="//action">
    unsigned int <xsl:value-of select="name"/>(
            <xsl:for-each select="argumentList/argument[direction='in']">
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
        </xsl:call-template>,
            </xsl:for-each>
        <xsl:value-of select="name"/>Callback callback);
</xsl:for-each>
};

} // namespace upnp

#endif

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
