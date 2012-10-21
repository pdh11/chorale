#include "playlist_io.h"
#include "libutil/file.h"

namespace import {

PlaylistXMLObserver::PlaylistXMLObserver(const std::string& filename, 
					 std::list<std::string> *entries)
    : m_filename(filename),
      m_entries(entries) 
{
}

unsigned int PlaylistXMLObserver::OnHref(const std::string& value)
{
    m_entries->push_back(util::MakeAbsolutePath(m_filename, value));
    return 0;
}

} // namespace import
