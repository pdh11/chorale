#include "playlist.h"
#include "libutil/file.h"
#include "libutil/trace.h"
#include "playlist_asx.h"
#include "playlist_wpl.h"

#include <vector>

namespace import {

struct Playlist::Impl
{
    std::string filename;
    std::vector<std::string> entries;
};

PlaylistPtr Playlist::Create(const std::string& filename)
{
    std::string ext = util::GetExtension(filename.c_str());

    Playlist *result = NULL;

    if (ext == "asx")
    {
	result = new PlaylistASX;
	result->m_impl->filename = filename;
    }
    else if (ext == "wpl")
    {
	result = new PlaylistWPL;
	result->m_impl->filename = filename;
    }
    else
    {
	TRACE << "Unexpected playlist file " << filename << "\n";
    }

    return PlaylistPtr(result);
}

Playlist::Playlist()
    : m_impl(new Impl)
{
}

Playlist::~Playlist()
{
    delete m_impl;
    m_impl = NULL;
}

std::string Playlist::GetFilename() const
{
    return m_impl->filename;
}

void Playlist::SetEntry(size_t index, const std::string& path)
{
    if (index >= m_impl->entries.size())
	m_impl->entries.resize(index+1);
    m_impl->entries[index] = path;
}

void Playlist::AppendEntry(const std::string& path)
{
    m_impl->entries.push_back(path);
}

std::string Playlist::GetEntry(size_t index) const
{
    return m_impl->entries[index];
}

size_t Playlist::GetLength() const
{
    return m_impl->entries.size();
}

} // namespace import
