#ifndef LIBUTIL_XML_H
#define LIBUTIL_XML_H 1

#include <string>
#include "stream.h"

//namespace util { typedef int StreamPtr ; };

namespace xml {

class NullSelector;
struct Data;

class BaseParser
{
protected:
    unsigned int Parse(util::StreamPtr, void *observer, const Data*);
public:
    virtual ~BaseParser() {}
};


        /* Parser */


template <class Selector0,
	  class Selector1 = NullSelector,
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class Parser: public BaseParser
{
public:
    typedef typename Selector0::Observer Observer;

    static const Data data;

    unsigned int Parse(util::StreamPtr s, Observer *obs)
    {
	return BaseParser::Parse(s, (void*)obs, &data);
    }
};


        /* Tag */


template <const char *name,
	  class Selector0,
	  class Selector1 = NullSelector,
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class Tag
{
public:
    typedef typename Selector0::Observer Observer;

    static const Data data;
};


        /* Attribute */


template <const char *name, class T,
	  unsigned int (T::*fn)(const std::string&) >
class Attribute
{
    static unsigned int OnText(void *obs, const std::string& s)
    {
	return (((T*)obs)->*fn)(s);
    }
public:
    typedef T Observer;

    static const Data data;
};


        /* Structure */


template <const char *name, class Product,
	  class Obs, unsigned int (Obs::*fn)(const Product&),
	  class Selector0, 
	  class Selector1, 
	  class Selector2 = NullSelector,
	  class Selector3 = NullSelector,
	  class Selector4 = NullSelector,
	  class Selector5 = NullSelector,
	  class Selector6 = NullSelector,
	  class Selector7 = NullSelector>
class Structure
{
    static void *OnBegin()
    {
	return (void*) new Product;
    }

    static unsigned int OnEnd(void *obs, void *prod)
    {
	Obs *observer = (Obs*)obs;
	Product *product = (Product*)prod;
	return (observer->*fn)(*product);
    }

public:
    typedef Obs Observer;

    static const Data data;
};


        /* StructureContent */


template <const char *name, class Product, std::string (Product::*pmem)>
class StructureContent
{
    static void OnText(void *product, const std::string& s)
    {
	((Product*)product)->*pmem = s;
    } 
public:
    static const Data data;
};

};

#include "xml_internal.h"

#endif
