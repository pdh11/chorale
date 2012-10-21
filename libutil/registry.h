#ifndef LIBUTIL_REGISTRY_H
#define LIBUTIL_REGISTRY_H

#include <string>
#include <vector>
#include <map>

namespace util {

/** A Registry keeps track of (index, pointer, object-name) tuples.
 *
 * It takes ownership of pointers passed to Add, and deletes them in its
 * destructor.
 */
template <class T>
class Registry
{
    struct Entry
    {
	T *t;
	std::string name;
	unsigned int index;
    };

    /** This used to be a boost::multi_index, but the overhead was huge */
    typedef std::vector<Entry> entries_t;
    entries_t m_entries;
    typedef std::map<std::string, unsigned int> byname_t;
    byname_t m_byname;
    typedef std::map<T*, unsigned int> byptr_t;
    byptr_t m_byptr;

public:
    Registry() {}
    ~Registry()
    {
	(void)sizeof(T); // Make sure anyone deleting me knows T's real type
	for (typename entries_t::iterator i = m_entries.begin();
	     i != m_entries.end();
	     ++i)
	{
	    delete i->t;
	}
    }

    // Takes ownership
    void Add(const std::string& name, T *t)
    {
	Entry e;
	e.t = t;
	e.name = name;
	e.index = (unsigned int)m_entries.size() + 1;
	m_entries.push_back(e);
	m_byname[name] = e.index;
	m_byptr[t] = e.index;
    }

    unsigned int GetIndex(T* t) const
    {
	typename byptr_t::const_iterator i = m_byptr.find(t);
	if (i == m_byptr.end())
	    return 0;
	return i->second;
    }

    std::string GetName(T* t) const
    {
	typename byptr_t::const_iterator i = m_byptr.find(t);
	if (i == m_byptr.end())
	    return std::string();
	return m_entries[i->second - 1].name;
    }

    T *Get(unsigned int index) const
    {
	if (index == 0 || index-1 >= m_entries.size())
	    return NULL;
	return m_entries[index-1].t;
    }

    T *Get(const std::string& name) const
    {
	typename byname_t::const_iterator i = m_byname.find(name);
	if (i == m_byname.end())
	    return NULL;
	return m_entries[i->second - 1].t;
    }
};

} // namespace util

#endif
