#ifndef LIBEMPEG_PROTOCOL_CLIENT_H
#define LIBEMPEG_PROTOCOL_CLIENT_H

#include <stdint.h>
#include <boost/scoped_array.hpp>
#include "libutil/mutex.h"
#include "libutil/observable.h"
#include "libutil/socket.h"

namespace empeg {

/** Special (non-music-related) file-IDs (fids).
 */
enum {
    FID_VERSION = 0,
    FID_ID = 1,
    FID_TAGS = 2,
    FID_DATABASE = 3,
    FID_PLAYLISTS = 5,
    FID_CONFIGINI = 6,
    FID_TYPE = 7
};

class ProtocolObserver
{
public:
    virtual ~ProtocolObserver() {}
    virtual void OnProgress(uint32_t stage_num, uint32_t stage_denom,
			    uint32_t num, uint32_t denom) = 0;
};

class ProtocolClient: public util::Observable<ProtocolObserver,
					      util::OneObserver,
					      util::NoLocking>
{
    util::IPAddress m_address;
    util::StreamSocket m_socket;
    util::StreamSocket m_fastsocket;
    uint64_t *m_aligned_buffer;
    unsigned char *m_buffer;
    unsigned int m_packet_id;
    util::Mutex m_mutex;
    unsigned int m_fast;

    enum { 
	PROTOCOL_PORT = 8300,
	PROTOCOL_FAST_PORT = 8301
    };

    void GetNextPacketId();

    unsigned int Transaction();

    unsigned int SendCommand(uint32_t command,
			     uint32_t param0 = 0,
			     uint32_t param1 = 0,
			     const char *param2 = NULL);

public:
    ProtocolClient();
    ~ProtocolClient();

    unsigned int Init(util::IPAddress);

    enum { MAX_PAYLOAD = 16384 };

    /* Individual exchanges */

    unsigned int Ping(uint16_t *version_minor, uint16_t *version_major);
    unsigned int Stat(uint32_t fid, uint32_t *size);
    unsigned int Read(uint32_t fid, uint32_t offset, uint32_t size,
		      void *buffer, uint32_t *nread);
    unsigned int Prepare(uint32_t fid, uint32_t size);
    unsigned int Write(uint32_t fid, uint32_t offset, uint32_t size,
		       const void *buffer, uint32_t *nwritten);
    unsigned int EnableWrite(bool writable);
    unsigned int Delete(uint32_t fid, uint16_t mask = 0xFFFF);

    unsigned int RestartPlayer(); // Orderly exit and re-exec player
    unsigned int Reboot();        // Complete reboot from 0000
    unsigned int RebuildDatabase();
    unsigned int LockUI(bool locked);

    /* Higher-level functions */

    unsigned int ReadFidToBuffer(uint32_t fid, uint32_t *size,
				 boost::scoped_array<char> *buffer);
    unsigned int ReadFidToString(uint32_t fid, std::string*);
    unsigned int WriteFidFromString(uint32_t fid, const std::string&);

    unsigned int StartFastWrite(uint32_t fid, uint32_t size);
};

} // namespace empeg

#endif
