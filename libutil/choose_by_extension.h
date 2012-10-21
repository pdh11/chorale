#ifndef LIBUTIL_CHOOSE_BY_EXTENSION_H
#define LIBUTIL_CHOOSE_BY_EXTENSION_H 1

#include <string>
#include <errno.h>
#include "file.h"

namespace util {

template <class T>
class ChooseByExtension
{
    struct ExtensionMap
    {
	const char *extension;
	T* (*factory)();
    };

    static const ExtensionMap sm_map[];
    
    template <class U>
    static T *Factory() { return new U; }

    std::string m_filename;
    T *m_impl;

public:
    ChooseByExtension(): m_impl(NULL) {}
    ~ChooseByExtension() { delete m_impl; }

    unsigned Init(const std::string& filename)
    {
	m_filename = filename;

	std::string extension = util::GetExtension(filename.c_str());
	for (unsigned int i=0; i < sizeof(sm_map)/sizeof(sm_map[0]); ++i)
	{
	    if (extension == sm_map[i].extension)
	    {
		m_impl = sm_map[i].factory();
		return 0;
	    }
	}
	return EINVAL;
    }

    const std::string& GetFilename() const { return m_filename; }

    bool IsValid() const { return m_impl != NULL; }

    T *operator->() { return m_impl; }
};

} // namespace util

#endif
