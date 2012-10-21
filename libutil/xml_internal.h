/* Internal details of the XML template classes, which clients shouldn't need
 * to know about.
 */
#ifndef LIBUTIL_XML_INTERNAL_H
#define LIBUTIL_XML_INTERNAL_H 1

namespace xml {

/** These classes are used to implement the table-based XML parsing, and are
 * not intended for direct use.
 */
namespace internals {


        /* Arrays of child pointers, of various sizes */


/** The main template representing a collection of sub-selectors.
 *
 * This template is used as-is for selectors with 8 sub-selectors, and then
 * is partially-specialised for 1..7 sub-selectors (so as not to pay the price
 * of all 8 slots each time).
 */
template <class Selector0, class Selector1, class Selector2, class Selector3,
	  class Selector4,
	  class Selector5,
	  class Selector6,
	  class Selector7>
struct Children
{
    enum { N = 8 };
    static const Data *const data[];
};

template <class Selector0, 
	  class Selector1, 
	  class Selector2,
	  class Selector3,
	  class Selector4,
	  class Selector5,
	  class Selector6,
	  class Selector7>
const Data *const Children<Selector0, Selector1, Selector2, Selector3,
		     Selector4, Selector5, Selector6, Selector7>::data[] = {
    &Selector0::data, &Selector1::data, &Selector2::data, &Selector3::data,
    &Selector4::data, &Selector5::data, &Selector6::data, &Selector7::data
};

/* Partial specialisations */

template <class Selector0, 
	  class Selector1, 
	  class Selector2,
	  class Selector3,
	  class Selector4,
	  class Selector5,
	  class Selector6>
struct Children<Selector0, Selector1, Selector2, Selector3,
		Selector4, Selector5, Selector6, NullSelector>
{
    enum { N = 7 };
    static const Data *const data[];
};

template <class Selector0, 
	  class Selector1, 
	  class Selector2,
	  class Selector3,
	  class Selector4,
	  class Selector5,
	  class Selector6>
const Data *const Children<Selector0, Selector1, Selector2, Selector3,
		     Selector4, Selector5, Selector6, NullSelector>::data[] = {
    &Selector0::data, &Selector1::data, &Selector2::data, &Selector3::data,
    &Selector4::data, &Selector5::data, &Selector6::data
};

template <class Selector0, 
	  class Selector1, 
	  class Selector2,
	  class Selector3,
	  class Selector4,
	  class Selector5>
struct Children<Selector0, Selector1, Selector2, Selector3,
		Selector4, Selector5, NullSelector, NullSelector>
{
    enum { N = 6 };
    static const Data *const data[];
};

template <class Selector0, 
	  class Selector1, 
	  class Selector2,
	  class Selector3,
	  class Selector4,
	  class Selector5>
const Data *const Children<Selector0, Selector1, Selector2, Selector3,
		     Selector4, Selector5, NullSelector, NullSelector>::data[] = {
    &Selector0::data, &Selector1::data, &Selector2::data, &Selector3::data,
    &Selector4::data, &Selector5::data
};

template <class Selector0, 
	  class Selector1, 
	  class Selector2,
	  class Selector3,
	  class Selector4>
struct Children<Selector0, Selector1, Selector2, Selector3,
		Selector4, NullSelector, NullSelector, NullSelector>
{
    enum { N = 5 };
    static const Data *const data[];
};

template <class Selector0, 
	  class Selector1, 
	  class Selector2,
	  class Selector3,
	  class Selector4>
const Data *const Children<Selector0, Selector1, Selector2, Selector3,
		     Selector4, NullSelector, NullSelector, NullSelector>::data[] = {
    &Selector0::data, &Selector1::data, &Selector2::data, &Selector3::data,
    &Selector4::data
};

template <class Selector0, 
	  class Selector1, 
	  class Selector2,
	  class Selector3>
struct Children<Selector0, Selector1, Selector2, Selector3,
		NullSelector, NullSelector, NullSelector, NullSelector>
{
    enum { N = 4 };
    static const Data *const data[];
};

template <class Selector0, 
	  class Selector1, 
	  class Selector2,
	  class Selector3>
const Data *const Children<Selector0, Selector1, Selector2, Selector3,
		     NullSelector, NullSelector, NullSelector, NullSelector>::data[] = {
    &Selector0::data, &Selector1::data, &Selector2::data, &Selector3::data
};


template <class Selector0, 
	  class Selector1, 
	  class Selector2>
struct Children<Selector0, Selector1, Selector2, NullSelector,
		NullSelector, NullSelector, NullSelector, NullSelector>
{
    enum { N = 3 };
    static const Data *const data[];
};

template <class Selector0, 
	  class Selector1, 
	  class Selector2>
const Data *const Children<Selector0, Selector1, Selector2, NullSelector,
		     NullSelector, NullSelector, NullSelector, NullSelector>::data[] = {
    &Selector0::data, &Selector1::data, &Selector2::data
};

template <class Selector0, 
	  class Selector1>
struct Children<Selector0, Selector1, NullSelector, NullSelector,
		NullSelector, NullSelector, NullSelector, NullSelector>
{
    enum { N = 2 };
    static const Data *const data[];
};

template <class Selector0, 
	  class Selector1>
const Data *const Children<Selector0, Selector1, NullSelector, NullSelector,
		     NullSelector, NullSelector, NullSelector, NullSelector>::data[] = {
    &Selector0::data, &Selector1::data
};

template <class Selector0>
struct Children<Selector0, NullSelector, NullSelector, NullSelector,
		NullSelector, NullSelector, NullSelector, NullSelector>
{
    enum { N = 1 };
    static const Data *const data[];
};

template <class Selector0>
const Data *const Children<Selector0, NullSelector, NullSelector, NullSelector,
		     NullSelector, NullSelector, NullSelector, NullSelector>::data[] = {
    &Selector0::data
};

template <>
struct Children<NullSelector, NullSelector, NullSelector, NullSelector,
		NullSelector, NullSelector, NullSelector, NullSelector>
{
    enum { N = 0 };
    static const Data *const data[]; // declared in xml.cpp as not templated
};


        /* xml::Data and its members */


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

} // namespace internals

using internals::Children;

template <class Selector0, class Selector1, class Selector2, class Selector3,
	  class Selector4, class Selector5, class Selector6, class Selector7>
const internals::Data Parser<Selector0, Selector1, Selector2, Selector3,
			    Selector4, Selector5, Selector6, Selector7>::data = {
    NULL, NULL, NULL,
    Children<Selector0, Selector1, Selector2, Selector3,
             Selector4, Selector5, Selector6, Selector7>::N,
    Children<Selector0, Selector1, Selector2, Selector3,
              Selector4, Selector5, Selector6, Selector7>::data
};

template <const char *name,
	  class Selector0, class Selector1, class Selector2, class Selector3,
	  class Selector4, class Selector5, class Selector6, class Selector7>
const internals::Data Tag<name,
			 Selector0, Selector1, Selector2, Selector3,
			 Selector4, Selector5, Selector6, Selector7>::data = {
    name, NULL, NULL,
    Children<Selector0, Selector1, Selector2, Selector3,
             Selector4, Selector5, Selector6, Selector7>::N,
    Children<Selector0, Selector1, Selector2, Selector3,
              Selector4, Selector5, Selector6, Selector7>::data
};

template <const char *name,
	  class T, unsigned int (T::*fn)(const std::string&),
	  class Selector0, class Selector1, class Selector2, class Selector3,
	  class Selector4, class Selector5, class Selector6, class Selector7>
const internals::Data TagCallback<name, T, fn, 
				 Selector0, Selector1, Selector2, Selector3,
				 Selector4, Selector5, Selector6, Selector7>::data = {
    name, &OnText, NULL,

    Children<Selector0, Selector1, Selector2, Selector3,
             Selector4, Selector5, Selector6, Selector7>::N,
    Children<Selector0, Selector1, Selector2, Selector3,
              Selector4, Selector5, Selector6, Selector7>::data
};

template <const char *name,
	  class T, std::string (T::*pmem),
	  class Selector0, class Selector1, class Selector2, class Selector3,
	  class Selector4, class Selector5, class Selector6, class Selector7>
const internals::Data TagMember<name, T, pmem, 
			       Selector0, Selector1, Selector2, Selector3,
			       Selector4, Selector5, Selector6, Selector7>::data = {
    name, &OnText, NULL,

    Children<Selector0, Selector1, Selector2, Selector3,
             Selector4, Selector5, Selector6, Selector7>::N,
    Children<Selector0, Selector1, Selector2, Selector3,
              Selector4, Selector5, Selector6, Selector7>::data
};

template <const char *name,
	  class T, unsigned int (T::*pmem),
	  class Selector0, class Selector1, class Selector2, class Selector3,
	  class Selector4, class Selector5, class Selector6, class Selector7>
const internals::Data TagMemberInt<name, T, pmem, 
				   Selector0, Selector1, Selector2, Selector3,
				   Selector4, Selector5, Selector6, Selector7>::data = {
    name, &OnText, NULL,

    Children<Selector0, Selector1, Selector2, Selector3,
             Selector4, Selector5, Selector6, Selector7>::N,
    Children<Selector0, Selector1, Selector2, Selector3,
              Selector4, Selector5, Selector6, Selector7>::data
};

template <const char *name, class T,
	  unsigned int (T::*fn)(const std::string&) >
const internals::Data Attribute<name, T, fn>::data = {
    name, &OnText, NULL,
    0, NULL
};

template <const char *name, class NewTarget,
	  class T, NewTarget (T::*pmem),
	  class Selector0, class Selector1, class Selector2, class Selector3,
	  class Selector4, class Selector5, class Selector6, class Selector7>
const internals::Data Structure<name, NewTarget, T, pmem,
	       Selector0, Selector1, Selector2, Selector3,
	       Selector4, Selector5, Selector6, Selector7>::data = {
    name, NULL, &OnBegin,

    Children<Selector0, Selector1, Selector2, Selector3,
             Selector4, Selector5, Selector6, Selector7>::N,
    Children<Selector0, Selector1, Selector2, Selector3,
              Selector4, Selector5, Selector6, Selector7>::data
};

template <const char *name, class NewTarget,
	  class T, std::list<NewTarget> (T::*pmem),
	  class Selector0, class Selector1, class Selector2, class Selector3,
	  class Selector4, class Selector5, class Selector6, class Selector7>
const internals::Data List<name, NewTarget, T, pmem,
	       Selector0, Selector1, Selector2, Selector3,
	       Selector4, Selector5, Selector6, Selector7>::data = {
    name, NULL, &OnBegin,

    Children<Selector0, Selector1, Selector2, Selector3,
             Selector4, Selector5, Selector6, Selector7>::N,
    Children<Selector0, Selector1, Selector2, Selector3,
              Selector4, Selector5, Selector6, Selector7>::data
};

} // namespace xml

#endif
