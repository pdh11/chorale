#include "playlist.h"
#include "libutil/file.h"
#include "libutil/trace.h"
#include "playlist_asx.h"
#include "playlist_pls.h"
#include "playlist_wpl.h"

#include <vector>

namespace import {

template<>
const util::ChooseByExtension<PlaylistIO>::ExtensionMap 
util::ChooseByExtension<PlaylistIO>::sm_map[] = {
    { "wpl",  &Factory<import::PlaylistWPL> },
    { "asx",  &Factory<import::PlaylistASX> },
    { "pls",  &Factory<import::PlaylistPLS> },
};

Playlist::Playlist()
{
}

Playlist::~Playlist()
{
}

unsigned Playlist::Init(const std::string& filename)
{
    return m_chooser.Init(filename);
}

unsigned Playlist::Load(std::list<std::string> *entries)
{
    entries->clear();
    if (!m_chooser.IsValid())
	return EINVAL;
    return m_chooser->Load(m_chooser.GetFilename(), entries);
}

unsigned Playlist::Save(const std::list<std::string> *entries)
{
    if (!m_chooser.IsValid())
	return EINVAL;
    return m_chooser->Save(m_chooser.GetFilename(), entries);
}

} // namespace import
