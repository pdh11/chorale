
#if 0
static unsigned int in_new = 0;

boost::mutex mem_mx;

struct header
{
    unsigned int magic;
    unsigned int sz;
    struct header *next;
    struct header *prev;
};

header blist = { 0, 0, &blist, &blist };

void *operator new(size_t sz) throw(std::bad_alloc)
{
//    check();

    boost::mutex::scoped_lock lock(mem_mx);

    if (!in_new++)
    {
//	TRACE << "new " << sz << "\n";
    }

    header *result = (header*)malloc(sz + sizeof(header) + 1);
    result->magic = 0x91729172;
    result->sz = sz;
    result->next = blist.next;
    result->prev = &blist;
    blist.next = result;
    char *data = (char*)(result+1);
    memset(data, 0xC5, sz);
    data[sz] = (char)0x81;
    --in_new;
    return data;
}

void operator delete(void *p) throw()
{
    if (!p)
	return;

    boost::mutex::scoped_lock lock(mem_mx);

    header *block = ((header*)p)-1;
    assert(block->magic == 0x91729172);
    header *next = block->next;
    block->next->prev = block->prev;
    block->prev->next = next;
    free(block);
}
#endif
