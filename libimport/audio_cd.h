#ifndef AUDIO_CD_H
#define AUDIO_CD_H

#include "libutil/counted_object.h"
#include <vector>
#include "cd_drives.h"

namespace import {

class AudioCD: public CountedObject
{
    /** No, they shouldn't be unsigned -- sector numbers are allowed to be -ve.
     */
    struct TocEntry
    {
	int firstsector;
	int lastsector; // inclusive!
    };

    typedef std::vector<TocEntry> toc_t;
    toc_t m_toc;

    unsigned int m_total_sectors;
    void *m_cdt;
    std::string m_device_name;

    ~AudioCD();

public:
    typedef boost::intrusive_ptr<AudioCD> AudioCDPtr;

    /** Attempt to create an AudioCDPtr. Can take several seconds (CD spinup).
     * Fails if no CD, or if no audio tracks.
     */
    static unsigned Create(CDDrivePtr drive, AudioCDPtr *pcd);

    /** Iterator over the TOC. Returns audio tracks only.
     */
    typedef toc_t::const_iterator iterator;

    iterator begin() { return m_toc.begin(); }
    iterator end() { return m_toc.end(); }

    size_t GetTrackCount() { return m_toc.size(); }
    unsigned int GetTotalSectors() { return m_total_sectors; }

    /** For handing to libcdio.
     */
    void *GetHandle() { return m_cdt; }
};

typedef boost::intrusive_ptr<AudioCD> AudioCDPtr;

} // namespace import

#endif
