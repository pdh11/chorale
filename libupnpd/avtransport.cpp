#include "config.h"
#include "avtransport.h"
#include "liboutput/urlplayer.h"

namespace upnpd {

AVTransportImpl::AVTransportImpl(output::URLPlayer *player)
    : m_player(player)
{
}

AVTransportImpl::~AVTransportImpl()
{
    m_player->SetPlayState(output::STOP);
}

unsigned int AVTransportImpl::SetAVTransportURI(uint32_t, 
						std::string CurrentURI,
						std::string metadata)
{
    return m_player->SetURL(CurrentURI, metadata);
}

unsigned int AVTransportImpl::SetNextAVTransportURI(uint32_t,
						    std::string NextURI,
						    std::string metadata)
{
    return m_player->SetNextURL(NextURI, metadata);
}

unsigned int AVTransportImpl::Stop(uint32_t)
{
    return m_player->SetPlayState(output::STOP);
}

unsigned int AVTransportImpl::Play(uint32_t,std::string)
{
    return m_player->SetPlayState(output::PLAY);
}

unsigned int AVTransportImpl::Pause(uint32_t)
{
    return m_player->SetPlayState(output::PAUSE);
}

}; // namespace upnpd
