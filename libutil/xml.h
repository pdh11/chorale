#ifndef LIBUTIL_XML_H
#define LIBUTIL_XML_H 1

#include <list>
#include <string>
#include <stdlib.h>

namespace util { class Stream; }

/** Classes for XML support
 *
 * There are two choices of parsing schemes here: SAX-like, where you
 * get told about each tag and attribute as it goes past, and
 * table-driven, where you can get the parser itself to build you a
 * data structure corresponding to the XML. The latter is implemented
 * using a DSEL-like collection of templates: see @ref xml.
 */
namespace xml {

class SaxParserObserver
{
public:
    virtual ~SaxParserObserver() {}
    virtual unsigned int OnBegin(const char* /*tag*/) { return 0; }
    virtual unsigned int OnEnd(const char* /*tag*/) { return 0; }
    virtual unsigned int OnAttribute(const char* /*name*/,
				     const char* /*value*/) { return 0; }
    virtual unsigned int OnContent(const char* /*content*/) { return 0; }
};

/** Simple, SAX-like XML parser.
 *
 * This parser is fairly forgiving of broken XML, which is necessary
 * for parsing Windows ASX playlist files -- they look a bit like XML
 * but aren't really.
 */
class SaxParser
{
    SaxParserObserver *m_observer;

    enum { BUFSIZE = 1024 };

    char m_buffer[BUFSIZE+1];
    size_t m_buffered;
    bool m_eof;
    std::string m_tagname;
    std::string m_attrname;
    
    enum { 
	CONTENT,
	TAG,
	IN_TAG,
	IN_ATTR,
	SEEK_GT,
	ATTR_SEEKEQ,
	ATTR_SEEKVALUE,
	IN_QUOTEDVALUE, 
	IN_VALUE,
	CDATA
    } m_state;

    void Parse();

public:
    explicit SaxParser(SaxParserObserver *observer);
    
    unsigned int Parse(util::Stream*);
    unsigned int WriteAll(const void *buffer, size_t len);
};

namespace details {

typedef unsigned int (*TextFn)(void*, const std::string&);
typedef void* (*StructureBeginFn)(void *prev_target);

struct Data {
    const char *name;
    TextFn text;
    StructureBeginFn sbegin;
    size_t n;
    const Data *const *children;

    bool IsAttribute() const { return children == NULL; }
};

template <typename... Selectors>
struct ArrayHelper
{
    static const Data *const data[];
};

template <typename... Selectors>
constexpr const Data *const ArrayHelper<Selectors...>::data[] = {
    &Selectors::data...
};

template<>
constexpr const Data *const ArrayHelper<>::data[];

unsigned int Parse(util::Stream*, void *target,
		   const Data *table);

} // namespace details


        /* Parser */


/** Base (root) element of a table-driven XML parser; see @ref xml.
 *
 * Makes no callbacks, but constrains its contained elements to only match
 * root elements.
 *
 * @param Selector0 Contained tags
 * @param Selector1 ...
 */
template <typename Selector, typename... Selectors>
class Parser
{
public:
    typedef typename Selector::Target Target;

    static const details::Data data;

    unsigned int Parse(util::Stream *s, Target *obs)
    {
	return details::Parse(s, (void*)obs, &data);
    }
};

template <typename Selector0, typename... Selectors>
constexpr const details::Data Parser<Selector0, Selectors...>::data = {
    NULL, NULL, NULL,
    sizeof...(Selectors) + 1,
    details::ArrayHelper<Selector0, Selectors...>::data
};


        /* Tag */


/** Match an XML element; see @ref xml.
 *
 * Makes no callbacks, but constrains its contained elements to only match
 * if inside this element.
 *
 * @param name      XML name of the element
 * @param Selector0 Contained tags
 * @param Selector1 ...
 */
template <const char *name, typename Selector, typename... Selectors>
class Tag
{
public:
    typedef typename Selector::Target Target;

    static const details::Data data;
};

template <const char *name, typename Selector0, typename... Selectors>
const details::Data Tag<name, Selector0, Selectors...>::data = {
    name, NULL, NULL,
    sizeof...(Selectors) + 1,
    details::ArrayHelper<Selector0, Selectors...>::data
};


        /* Tag with contents (callback) */


/** Parse out the contents of an XML element; see @ref xml.
 *
 * Makes a callback into the target object, with the contents of an XML
 * element. @sa TagMember.
 *
 * @param name      XML name of the element
 * @param T         C++ type of the target object
 * @param fn       Pointer-to-member of the T object's callback method
 * @param Selector0 Contained tags, if any.
 * @param Selector1 ...
 */
template <const char *name,
	  class T, unsigned int (T::*fn)(const std::string&),
          typename... Selectors>
class TagCallback
{
    static unsigned int OnText(void *obs, const std::string& s)
    {
	return (((T*)obs)->*fn)(s);
    }

public:
    typedef T Target;

    static const details::Data data;
};

template <const char *name,
	  class T, unsigned int (T::*fn)(const std::string&),
          typename... Selectors>
const details::Data TagCallback<name, T, fn, Selectors...>::data = {
    name, &OnText, NULL,
    sizeof...(Selectors),
    details::ArrayHelper<Selectors...>::data
};


        /* Tag with contents (member) */


/** Parse out the contents of an XML element; see @ref xml.
 *
 * Stores the contents of an XML element, into a member variable of
 * the target object. @sa TagCallback.
 *
 * @param name      XML name of the element
 * @param T         C++ type of the target object
 * @param pmem      Pointer-to-member of the T object's string member
 * @param Selector0 Contained tags, if any.
 * @param Selector1 ...
 */
template <const char *name,
	  class T, std::string (T::*pmem),
          typename... Selectors>
class TagMember
{
    static unsigned int OnText(void *product, const std::string& s)
    {
	((T*)product)->*pmem = s;
	return 0;
    }

public:
    typedef T Target;

    static const details::Data data;
};

template <const char *name,
	  class T, std::string (T::*pmem),
          typename... Selectors>
const details::Data TagMember<name, T, pmem, Selectors...>::data = {
    name, &OnText, NULL,
    sizeof...(Selectors),
    details::ArrayHelper<Selectors...>::data
};


        /* Tag with uint contents (member) */


/** Parse out the integer contents of an XML element; see @ref xml.
 *
 * Stores the contents of an XML element, into a member variable of
 * the target object. @sa TagCallback.
 *
 * @param name      XML name of the element
 * @param T         C++ type of the target object
 * @param pmem      Pointer-to-member of the T object's unsigned int member
 * @param Selector0 Contained tags, if any.
 * @param Selector1 ...
 */
template <const char *name,
	  class T, unsigned int (T::*pmem),
          typename... Selectors>
class TagMemberInt
{
    static unsigned int OnText(void *product, const std::string& s)
    {
	((T*)product)->*pmem = (unsigned int)strtoul(s.c_str(), NULL, 10);
	return 0;
    }

public:
    typedef T Target;

    static const details::Data data;
};

template <const char *name,
	  class T, unsigned int (T::*pmem),
          typename... Selectors>
const details::Data TagMemberInt<name, T, pmem, Selectors...>::data = {
    name, &OnText, NULL,
    sizeof...(Selectors),
    details::ArrayHelper<Selectors...>::data
};


        /* Attribute */


/** Parse out a specific attribute of an XML element; see @ref xml.
 *
 * Makes a callback into the target object, whenever a specific attribute of
 * the enclosing xml::Tag is found.
 *
 * @param name     XML name of the attribute
 * @param T        C++ type of the target object
 * @param fn       Pointer-to-member of the T object's callback method
 */
template <const char *name, class T,
	  unsigned int (T::*fn)(const std::string&) >
class Attribute
{
    static unsigned int OnText(void *obs, const std::string& s)
    {
	return (((T*)obs)->*fn)(s);
    }
public:
    typedef T Target;

    static const details::Data data;
};

template <const char *name, class T,
	  unsigned int (T::*fn)(const std::string&) >
const details::Data Attribute<name, T, fn>::data = {
    name, &OnText, NULL,
    0, NULL
};


        /* Structure */


/** Parse contained XML elements into a struct; see @ref xml.
 *
 * Store (strings or structures corresponding to) contained XML elements, into
 * structure members.
 *
 * @param name      XML name of the container element
 * @param NewTarget C++ type of the struct to become the new target object
 * @param T         C++ type of the parent target object of the new struct
 * @param pmem      Pointer-to-member of the T object's NewTarget member
 * @param Selector0 Contained tags, usually including TagMember.
 * @param Selector1 ...
 */
template <const char *name, class NewTarget,
	  class T, NewTarget (T::*pmem),
          typename... Selectors>
class Structure
{
    static void *OnBegin(void *target)
    {
	return (void*) &((T*)target->*pmem);
    }

public:
    typedef T Target;

    static const details::Data data;
};

template <const char *name, class NewTarget,
	  class T, NewTarget (T::*pmem),
          typename... Selectors>
const details::Data Structure<name, NewTarget, T, pmem, Selectors...>::data = {
    name, NULL, &OnBegin,
    sizeof...(Selectors),
    details::ArrayHelper<Selectors...>::data
};


        /* List */


/** Parse a sequence of XML elements into a list; see @ref xml.
 *
 * Store (structures corresponding to) a sequence of similar XML
 * elements, into a std::list.
 *
 * @param name      XML name of the repeating element
 * @param NewTarget C++ type of the new target object for each repeated item
 * @param T         C++ type of the target object in which to create the list
 * @param pmem      Pointer-to-member of the T object's list<NewTarget> member
 * @param Selector0 Contained tags, usually including TagMember.
 * @param Selector1 ...
 */
template <const char *name, class NewTarget,
	  class T, std::list<NewTarget> (T::*pmem),
          typename... Selectors>
class List
{
    static void *OnBegin(void *target)
    {
	NewTarget newt;
	((T*)target->*pmem).push_back(newt);
	return (void*) &(((T*)target->*pmem).back());
    }

public:
    typedef T Target;

    static const details::Data data;
};

template <const char *name, class NewTarget,
	  class T, std::list<NewTarget> (T::*pmem),
          typename... Selectors>
const details::Data List<name, NewTarget, T, pmem, Selectors...>::data = {
    name, NULL, &OnBegin,
    sizeof...(Selectors),
    details::ArrayHelper<Selectors...>::data
};

} // namespace xml

#endif
