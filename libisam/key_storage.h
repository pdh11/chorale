#ifndef KEY_STORAGE_H
#define KEY_STORAGE_H 1

#include <string>
#include <stdint.h>

namespace isam {

class DataBlock;
class File;
class Key;
class PageAllocator;
class PageHandler;
class PageLocking;

class KeyStorage
{
    File *m_file;
    PageAllocator *m_allocator;
    PageLocking *m_locking;
    bool m_ok;

    /** Search for a key down the page tree.
     *
     * On return,
     *   result==0,  key.empty(), value_page == -1 => direct_value is valid
     *   result==0,  key.empty(), value_page != -1 => value_page is valid
     *   result==0, !key.empty()  => this is the right page, key isn't on it
     *   other result             => error
     *
     * In all non-error cases, the lock chain ends at the page the key is
     * on (if present) or the one it should be added to (if not).
     */
    template <class LockChain>
    unsigned int FindPage(LockChain *chain,
			  Key *key /* INOUT */,
			  std::string *direct_value,
			  uint32_t *value_page);

    unsigned int ReplacePageNumberInBlock(DataBlock *block, 
					  unsigned int old_pageno,
					  unsigned int new_pageno);    

    struct PageBinding
    {
	unsigned int type;
	const PageHandler *handler;
    };

    static const PageBinding sm_page_bindings[];

    static const PageHandler *Handler(unsigned int type);

public:
    KeyStorage(File *file, PageAllocator *allocator, PageLocking *locking);
    ~KeyStorage();

    /** Makes sure initial data structures are set up. Fails if file exists
     * but isn't one of ours.
     */
    unsigned int InitialiseFile();

    unsigned int Store(const std::string& key, const std::string& value);

    unsigned int Delete(const std::string& key);

    unsigned int Fetch(const std::string& key, std::string *pValue);

    unsigned int FetchNext(const std::string& key,
			   std::string *pNextKey, std::string *pValue);

    void Dump(unsigned int page, const std::string& prefix) const;
};

} // namespace isam

#endif
