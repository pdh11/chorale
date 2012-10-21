#ifndef LIBISAM_DIRECT_PAGE_HANDLER_H
#define LIBISAM_DIRECT_PAGE_HANDLER_H 1

#include "page_handler.h"

namespace isam {

/** Handles DIRECT_KEY pages
 *
 * Structure is
 *    uint32_t type
 *    uint32_t header
 *    uint8_t  common_key_bytes
 *    char     common_key[]
 *    ..align to 32bits..
 *    struct {
 *      uint16_t keylen_bytes (b15 set => end of block)
 *      uint16_t valuelen_bytes (0x8000 => value is a blockno, 
 *                               0x8001 => value is a blockno with more key)
 *      char key[]
 *      ..align to 32bits..
 *      union {
 *        uint32_t blockno;
 *        char data[]
 *      }
 *      ..align to 32bits..
 *    } []
 */
class DirectPageHandler: public PageHandler
{
public:
    // Being a PageHandler
    unsigned int Find(const DataBlock *block, 
		      Key *key /* INOUT */, 
		      PageResult*) const;
    unsigned int ComposePage(const page_contents_t&, PageAllocator*,
			     File*, unsigned int *new_page) const;
    unsigned int DecomposePage(const DataBlock *page, 
			       page_contents_t *contents) const;
    void ReplacePageNumberInBlock(DataBlock *block,
				  uint32_t old_pageno,
				  uint32_t new_pageno) const;
};

} // namespace isam

#endif
