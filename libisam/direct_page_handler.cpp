#include "direct_page_handler.h"
#include "format.h"
#include "file.h"
#include "page_allocator.h"
#include "libutil/errors.h"
#include "libutil/trace.h"
#include <assert.h>
#include <string.h>

namespace isam {

struct DirectBlock
{
    DataBlock header;
    uint16_t table[256];
};

unsigned int DirectPageHandler::Find(const DataBlock *dblock, 
				     Key *pkey, 
				     PageResult *pr) const
{
    assert(dblock->type == DIRECT_KEY);

    const DirectBlock *block = (const DirectBlock*)dblock;

    const unsigned char *common_key = (const unsigned char*)(block+1);
    unsigned int common_bytes = *common_key;
    if (pkey->size < common_bytes)
    {
//	TRACE << "Too short (<" << common_bytes << ")\n";
	return ENOENT;
    }
    if (memcmp(pkey->data, common_key+1, common_bytes))
    {
//	TRACE << "Doesn't match common\n";
	return ENOENT;
    }
    
    const unsigned char *keydata = pkey->data + common_bytes;
    unsigned int keysize = pkey->size - common_bytes;

    common_key += (common_bytes + 1 + 3) & ~3;

    const uint16_t *entry = (const uint16_t*)common_key;

    if (keysize && dblock->count > 1)
    {
	uint16_t skip = block->table[*keydata];
	if (!skip)
	{
//	    TRACE << "Skiptable but no skip\n";
	    return ENOENT;
	}
	entry = (uint16_t*)((const char*)block + skip);
    }

    while ((const char*)entry - (const char*)block < isam::PAGE)
    {
	if (*entry & 0x8000)
	{
	    // End of block -- no exact match, but this is our page
//	    TRACE << "EOB(1)\n";
	    return ENOENT;
	}

	unsigned int keylen_bytes = *entry;
	unsigned int valuelen_bytes = entry[1];

	const char *entry_data = (const char*)(entry+2);
	const char *value = entry_data + ((keylen_bytes + 3) & ~3);

	unsigned int compare_bytes = std::min(keylen_bytes, keysize);

	int cmp = memcmp(keydata, entry_data, compare_bytes);
	if (cmp < 0)
	{
//	    TRACE << "No match\n";
	    return ENOENT;
	}
	if (cmp == 0)
	{
	    // Prefix match
	    if (keylen_bytes <= keysize && valuelen_bytes == 0x8001)
	    {
		// Prefix match pointing to further key page
		pkey->data += keylen_bytes + common_bytes;
		pkey->size -= keylen_bytes + common_bytes;
		pr->key_page = *(uint32_t*)value;
		pr->value_page = (uint32_t)-1;
		return 0;
	    }

	    if (keylen_bytes == keysize)
	    {
		// Exact match
		pr->key_page = (uint32_t)-1;
		if (valuelen_bytes == 0x8000)
		    pr->value_page = *(uint32_t*)value;
		else
		{
		    pr->value_page = (uint32_t)-1;
		    pr->direct_value.assign(value, value + valuelen_bytes);
		}
		return 0;
	    }
	    
	    // Else, if match was partial, carry on, as we might have
	    // found "f" while looking for "foo"
	}

	// our-key < entry, carry on looking in this page
	if (valuelen_bytes & 0x8000)
	    valuelen_bytes = 4;
	
	entry = (const uint16_t*)(value + ((valuelen_bytes + 3) & ~3));
    }

    // Got to end of block, didn't find continuation; this is our page
//    TRACE << "EOB(2)\n";
    return ENOENT;
}

unsigned int DirectPageHandler::ComposePage(const page_contents_t& cpc,
					    PageAllocator *allocator,
					    File *file,
					    unsigned int *new_page) const
{
    page_contents_t pc(cpc);

    /* Work out common key prefix, if any */
    
    std::string common_key = pc.begin()->first;

    for (page_contents_t::const_iterator i = pc.begin(); 
	 i != pc.end() && !common_key.empty(); 
	 ++i)
    {
	size_t len = std::min(i->first.size(), common_key.size());
	common_key.erase(len);
	for (size_t j=0; j<len; ++j)
	{
	    if (common_key[j] != i->first[j])
	    {
		len = j;
		break;
	    }
	}
	common_key.erase(len);
    }

    if (common_key.size() > 255)
	common_key.erase(255);

//    TRACE << "Common key length " << common_key.size() << " bytes\n";


    /* Work out size of resulting page */

    unsigned int counts[257];
    unsigned int sizes[257];
    memset(&counts, '\0', sizeof(counts));
    memset(&sizes, '\0', sizeof(sizes));

    unsigned int size;

    do {
	size = sizeof(DirectBlock);
	size += (unsigned int)common_key.size() + 1;
	size = (size+3) & ~3;

	for (page_contents_t::const_iterator i = pc.begin(); i != pc.end(); ++i)
	{
	    unsigned int this_size = 4;
	    this_size += (unsigned int)(i->first.size() - common_key.size());
	    this_size = (this_size+3) & ~3;
	    if (i->second.value_page || i->second.key_page)
		this_size += 4;
	    else
	    {
		this_size += (unsigned int)i->second.direct_value.size();
		this_size = (this_size+3) & ~3;
	    }

	    size += this_size;

	    unsigned int c = 256;
	    if (i->first.size() > common_key.size())
		c = i->first[common_key.size()];
	    counts[c] += 1;
	    sizes[c] += this_size;
	}
	
//	TRACE << "Page size " << size << " bytes\n";

	if (size > isam::PAGE)
	{
	    /** @todo Work out which byte (as a key prefix) or nil, would
	     *        give the biggest reduction, and offload that prefix
	     *        onto a child page. Use a value page if there's only
	     *        one AND it's only one byte of key, otherwise a
	     *        direct-key page.
	     */
	    unsigned int best = 0;
	    for (unsigned int i=0; i<257; ++i)
	    {
		if (sizes[i] > sizes[best])
		    best = i;
	    }

//	    TRACE << "Spinning off prefix " << best << " would remove "
//		  << sizes[best] << " bytes (" << counts[best]
//		  << " entries)\n";

	    if (1) //counts[best] > 1)
	    {
		page_contents_t child_contents;
		for (page_contents_t::iterator i = pc.begin();
		     i != pc.end();
		     )
		{
		    if (i->first.size() >= 1
			&& (unsigned char)i->first[0] == best)
		    {
			std::string child_key(i->first, 1);
			child_contents[child_key] = i->second;
			page_contents_t::iterator j = i;
			++i;
			pc.erase(j);
		    }
		    else
			++i;
		}

		unsigned int child_page;
		unsigned rc = ComposePage(child_contents, allocator, file, 
					  &child_page);
		if (rc)
		    return rc;

		std::string newkey;
		newkey.push_back((char)best);
		pc[newkey] = PageResult(std::string(), 0, child_page);
	    }
	    else
	    {
		TRACE << "Value page needed -- NYI\n";
		return ENOSPC;
	    }
	}
    } while (size > isam::PAGE);


    /* Allocate and lay out page */

    unsigned int pageno = allocator->Allocate();
    if (pageno == (unsigned int)-1)
	return ENOMEM;
    DirectBlock *block = (DirectBlock*)file->Page(pageno);
    if (!block)
	return ENOMEM;

    memset(block, '\0', isam::PAGE);

    block->header.type = DIRECT_KEY;
    block->header.count = (uint32_t)pc.size();

    unsigned char *common = (unsigned char*)(block+1);
    *common = (unsigned char)common_key.size();
    memcpy(common+1, common_key.c_str(), common_key.size());
    common += (*common + 1 + 3) & ~3;

    uint16_t *entry = (uint16_t*)common;

    unsigned int last = 256;

    for (page_contents_t::const_iterator i = pc.begin(); i != pc.end(); ++i)
    {
	*entry = (uint16_t)(i->first.size() - common_key.size());

	if (i->second.value_page)
	    entry[1] = 0x8000;
	else if (i->second.key_page)
	    entry[1] = 0x8001;
	else
	    entry[1] = (uint16_t)(i->second.direct_value.size());

	char *entry_data = (char*)(entry+2);
	memcpy(entry_data, i->first.c_str() + common_key.size(), *entry);

	if (*entry)
	{
	    if ((unsigned char)*entry_data != last)
	    {
		block->table[(unsigned char)*entry_data] = (uint16_t)((char*)entry - (char*)block);
		last = *entry_data;
	    }
	}

	entry_data += (*entry + 3) & ~3;
	
	if (i->second.value_page)
	{
	    *(uint32_t*)entry_data = i->second.value_page;
	    entry_data += 4;
	}
	else if (i->second.key_page)
	{
	    *(uint32_t*)entry_data = i->second.key_page;
	    entry_data += 4;
	}
	else
	{
	    memcpy(entry_data, i->second.direct_value.c_str(),
		   i->second.direct_value.size());
	    entry_data += (i->second.direct_value.size() + 3) & ~3;
	}

	entry = (uint16_t*)entry_data;
    }

    ptrdiff_t block_fill = (char*)entry - (char*)block;
    assert(block_fill <= isam::PAGE);

    if (block_fill < isam::PAGE)
	*(uint16_t*)((char*)block + block_fill) = 0xffff;

    file->WriteOut(pageno);

    *new_page = pageno;

    return 0;
}

unsigned int DirectPageHandler::DecomposePage(const DataBlock *dblock,
					      page_contents_t *contents) const
{
    assert(dblock->type == DIRECT_KEY);

    const DirectBlock *block = (const DirectBlock*)dblock;

    contents->clear();

    const unsigned char *common_key_ptr = (const unsigned char*)(block+1);
    std::string common_key(common_key_ptr + 1, 
			   common_key_ptr + 1 + *common_key_ptr);

    common_key_ptr += (*common_key_ptr + 1 + 3) & ~3;

    const uint16_t *entry = (const uint16_t*)common_key_ptr;

    while ((const char*)entry - (const char*)block < isam::PAGE)
    {
	if (*entry & 0x8000)
	{
	    // End of block
	    return 0;
	}

	unsigned int keylen_bytes = *entry;
	unsigned int valuelen_bytes = entry[1];

	const char *entry_data = (const char*)(entry+2);
	const char *value = entry_data + ((keylen_bytes + 3) & ~3);

	std::string key = common_key + std::string(entry_data, keylen_bytes);

	PageResult pr(std::string(), 0, 0);

	if (valuelen_bytes == 0x8001)
	    pr.key_page = *(uint32_t*)value;
	else if (valuelen_bytes == 0x8000)
	    pr.value_page = *(uint32_t*)value;
	else
	    pr.direct_value = std::string(value, value+valuelen_bytes);

	contents->insert(std::make_pair(key, pr));

	if (valuelen_bytes & 0x8000)
	    valuelen_bytes = 4;
	entry = (const uint16_t*)(value + ((valuelen_bytes + 3) & ~3));
    }
    
    return 0;
}

void DirectPageHandler::ReplacePageNumberInBlock(DataBlock *dblock,
						 uint32_t old_pageno,
						 uint32_t new_pageno) const
{
    assert(dblock->type == DIRECT_KEY);

    DirectBlock *block = (DirectBlock*)dblock;

    const unsigned char *common_key_ptr = (const unsigned char*)(block+1);
    common_key_ptr += (*common_key_ptr + 1 + 3) & ~3;

    const uint16_t *entry = (const uint16_t*)common_key_ptr;

    while ((const char*)entry - (const char*)block < isam::PAGE)
    {
	if (*entry & 0x8000)
	{
	    // End of block
	    TRACE << "RPNIB: page " << old_pageno << " not found!\n";
	    return;
	}

	unsigned int keylen_bytes = *entry;
	unsigned int valuelen_bytes = entry[1];

	const char *entry_data = (const char*)(entry+2);
	const char *value = entry_data + ((keylen_bytes + 3) & ~3);

	if (valuelen_bytes == 0x8001)
	{
	    if (*(uint32_t*)value == old_pageno)
	    {
		*(uint32_t*)value = new_pageno;
		return;
	    }
	}

	if (valuelen_bytes & 0x8000)
	    valuelen_bytes = 4;
	entry = (const uint16_t*)(value + ((valuelen_bytes + 3) & ~3));
    }
}

} // namespace isam

#ifdef TEST

int main()
{
    isam::File file;

    unsigned int rc = file.Open(NULL, 0);
    assert(rc == 0);

    isam::PageAllocator allocator(&file);

    isam::page_contents_t pc;
    pc["foo"] = isam::PageResult("bar", 0, 0);

    isam::DirectPageHandler dph;

    unsigned int pageno;
    rc = dph.ComposePage(pc, &allocator, &file, &pageno);
    assert(rc == 0);

    isam::DataBlock *block = (isam::DataBlock*)file.Page(pageno);
    std::string skey = "foo";

    isam::PageResult pr;

    isam::Key key(skey);
    rc = dph.Find(block, &key, &pr);
    assert(rc == 0);
    assert(pr.value_page == (unsigned int)-1);
    assert(pr.direct_value == "bar");

    isam::page_contents_t pc2;
    rc = dph.DecomposePage(block, &pc2);
    assert(pc2.size() == 1);
    assert(pc2["foo"].direct_value == "bar");

    pc2["fob"] = isam::PageResult("oar", 0, 0);

    rc = dph.ComposePage(pc2, &allocator, &file, &pageno);
    assert(rc == 0);

    block = (isam::DataBlock*)file.Page(pageno);
    key = skey;

    rc = dph.Find(block, &key, &pr);
    assert(rc == 0);
    assert(pr.value_page == (unsigned int)-1);
    assert(pr.direct_value == "bar");

    isam::page_contents_t pc3;
    rc = dph.DecomposePage(block, &pc3);
    assert(pc3.size() == 2);
    assert(pc3["fob"].direct_value == "oar");
    assert(pc3["foo"].direct_value == "bar");

    return 0;
}

#endif
