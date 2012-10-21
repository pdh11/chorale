<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="str"
                version="1.0">

  <!-- Generate C++ source directly from the SCPD (service description).
     @todo This could be more optimal (assemble the SOAP string directly)
           once not using Intel libupnp.
    -->

  <xsl:include href="scpd-common.xsl"/>

<xsl:template match="/">/* This is an automatically-generated file -- DO NOT EDIT */

#ifndef UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_H
#define UPNP_<xsl:value-of select="translate($class,$lcase,$ucase)"/>_H 1

#include &lt;string&gt;
#include &lt;stdint.h&gt;
#include &quot;libutil/observable.h&quot;
#include &quot;libutil/bind.h&quot;

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
 * Inherited by normal (i.e. async) and synchronous classes which contain the
 * actual API.
 */
class <xsl:value-of select="$class"/>Base: public util::Observable&lt;<xsl:value-of select="$class"/>Observer,
    util::OneObserver, util::NoLocking&gt;
{
public:
    virtual ~<xsl:value-of select="$class"/>Base() {}

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
};


/** Asynchronous API for <xsl:value-of select="$class"/> service.
 */
class <xsl:value-of select="$class"/>Async: public <xsl:value-of select="$class"/>Base
{
public:<xsl:for-each select="//action">
<xsl:text>
    </xsl:text>
      <xsl:if test="not(argumentList/argument[direction='out'])">
    typedef util::Callback1&lt;unsigned int&gt;
        <xsl:value-of select="name"/>Callback;
      </xsl:if>
      <xsl:if test="count(argumentList/argument[direction='out'])>1">
    struct <xsl:value-of select="name"/>Response {
<xsl:for-each select="argumentList/argument[direction='out']">
      <xsl:text>        </xsl:text>
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
        <xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
      </xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
      <xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template></xsl:if><xsl:text> </xsl:text>
        <xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>;
</xsl:for-each>    };
    typedef util::Callback2&lt;unsigned int, const <xsl:value-of select="name"/>Response*&gt;
        <xsl:value-of select="name"/>Callback;
</xsl:if>
      <xsl:if test="count(argumentList/argument[direction='out'])=1">
    typedef util::Callback2&lt;unsigned int, const <xsl:for-each select="argumentList/argument[direction='out']">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
        <xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
      </xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
      <xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template></xsl:if>
</xsl:for-each>*&gt;
        <xsl:value-of select="name"/>Callback;
</xsl:if>
    virtual unsigned int <xsl:value-of select="name"/>(
            <xsl:for-each select="argumentList/argument[direction='in']">
      <xsl:if test="//stateVariable[name=current()/relatedStateVariable]/allowedValueList">
        <xsl:value-of select="str:replace(current()/relatedStateVariable,'A_ARG_TYPE_','')"/>
        <xsl:text> </xsl:text>
      </xsl:if>
      <xsl:if test="not(//stateVariable[name=current()/relatedStateVariable]/allowedValueList)">
      <xsl:variable name="type" select="//stateVariable[name=current()/relatedStateVariable]/dataType"/>
      <xsl:call-template name="argtype">
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
    </xsl:if>
        <xsl:call-template name="camelcase-to-underscore">
          <xsl:with-param name="camelcase" select="name"/>
        </xsl:call-template>,
      </xsl:for-each><xsl:value-of select="name"/>Callback callback)<!--<xsl:if test="not(Optional)"> = 0</xsl:if>-->;</xsl:for-each>
  
    /* Getters for evented state variables not otherwise covered */  <xsl:for-each select="//stateVariable">
    <xsl:if test="not(contains(name,'A_ARG_TYPE')) and sendEventsAttribute='yes'">
      <xsl:variable name="getter" select="concat('Get',name)"/>
      <xsl:if test="not(//action[name=$getter])">
    virtual unsigned int <xsl:value-of select="$getter"/>(<xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="dataType"/>
      </xsl:call-template>*);</xsl:if>
    </xsl:if>
  </xsl:for-each>
};


/** Synchronous API for <xsl:value-of select="$class"/> service.
 */
class <xsl:value-of select="$class"/>: public <xsl:value-of select="$class"/>Base
{
public:<xsl:for-each select="//action">
    virtual unsigned int <xsl:value-of select="name"/>(
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
    </xsl:for-each>)<!--<xsl:if test="not(Optional)"> = 0</xsl:if>-->;</xsl:for-each>
  
    /* Getters for evented state variables not otherwise covered */  <xsl:for-each select="//stateVariable">
    <xsl:if test="not(contains(name,'A_ARG_TYPE')) and sendEventsAttribute='yes'">
      <xsl:variable name="getter" select="concat('Get',name)"/>
      <xsl:if test="not(//action[name=$getter])">
    virtual unsigned int <xsl:value-of select="$getter"/>(<xsl:call-template name="cpptype">
        <xsl:with-param name="type" select="dataType"/>
      </xsl:call-template>*);</xsl:if>
    </xsl:if>
  </xsl:for-each>
};

} // namespace upnp

#endif

/* This is an automatically-generated file -- DO NOT EDIT */
</xsl:template>
</xsl:stylesheet>
