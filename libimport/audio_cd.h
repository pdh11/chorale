#ifndef AUDIO_CD_H
#define AUDIO_CD_H

#include "libutil/counted_object.h"
#include <vector>
#include <string>
#include "libutil/stream.h"

namespace import {

class AudioCD: public util::CountedObject<>
{
protected:
    /** No, they shouldn't be unsigned -- sector numbers are allowed to be -ve.
     */
    struct TocEntry
    {
	int first_sector;
	int last_sector; // inclusive!
    };

    typedef std::vector<TocEntry> toc_t;
    toc_t m_toc;

    unsigned int m_total_sectors;

public:
    AudioCD() : m_total_sectors(0) {}

    /** Iterator over the TOC. Returns audio tracks only.
     */
    typedef toc_t::const_iterator const_iterator;

    const_iterator begin() { return m_toc.begin(); }
    const_iterator end() { return m_toc.end(); }

    /** Counts audio tracks only.
     */
    size_t GetTrackCount() { return m_toc.size(); }

    /** Counts audio tracks only.
     */
    unsigned int GetTotalSectors() { return m_total_sectors; }

    /** Returns a SeekableStream of the raw PCM data for a particular track
     */
    virtual util::SeekableStreamPtr GetTrackStream(unsigned int track) = 0;
};

typedef util::CountedPointer<AudioCD> AudioCDPtr;

class LocalAudioCD: public AudioCD
{
    friend class LocalCDDrive;

    void *m_cdt;
    std::string m_device_name;

public:
    LocalAudioCD() : m_cdt(NULL) {}
    ~LocalAudioCD();

    static unsigned int Create(const std::string& device, AudioCDPtr *result);

    // Being an AudioCD
    util::SeekableStreamPtr GetTrackStream(unsigned int track);
};

} // namespace import

#endif
