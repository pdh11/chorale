#ifndef LIBISAM_PAGE_HANDLER_H
#define LIBISAM_PAGE_HANDLER_H 1

#include <string>
#include <map>
#include <stdint.h>

namespace isam {

class DataBlock;
class File;
class PageAllocator;

struct PageResult
{
    std::string direct_value;
    uint32_t value_page;
    uint32_t key_page;

    PageResult(const std::string& dv, uint32_t vp, uint32_t kp)
	: direct_value(dv), value_page(vp), key_page(kp)
	{
	}

    PageResult() {}
};

struct Key
{
    const unsigned char *data;
    unsigned int size;

    Key(const std::string& s)
	: data((const unsigned char*)s.c_str()),
	  size((unsigned int)s.length())
    {
    }

    std::string ToString() const
    {
	return std::string((const char*)data, (const char*)(data+size));
    }
};

/** A map from key fragments to page results.
 *
 * Note that we rely on the (default) lexicographic byte ordering of
 * std::strings.
 */
typedef std::map<std::string, PageResult> page_contents_t;

class PageHandler
{
public:
    virtual ~PageHandler() {}

    /** Search for a key in this page.
     *
     * On return,
     *   result==0, pr.value_page=pr.key_page == -1 => direct_value is valid
     *   result==0, pr.value_page != -1             => value page
     *   result==0, pr.key_page != -1               => key page
     *   result==ENOENT              => this is the right page, key isn't on it
     *   other result                => error
     *
     * @todo Return PageResult to make the value page thing easier
     */
    virtual unsigned int Find(const DataBlock *page, 
			      Key *key /* INOUT */, 
			      PageResult *pr) const = 0;

    virtual unsigned int ComposePage(const page_contents_t&, PageAllocator*,
				     File*, unsigned int *new_page) const = 0;

    virtual unsigned int DecomposePage(const DataBlock *page, 
				       page_contents_t *contents) const = 0;

    virtual void ReplacePageNumberInBlock(DataBlock *block,
					  uint32_t old_pageno,
					  uint32_t new_pageno) const = 0;
};

} // namespace isam

#endif
