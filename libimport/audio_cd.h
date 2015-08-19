#ifndef AUDIO_CD_H
#define AUDIO_CD_H

#include "libutil/counted_object.h"
#include <vector>
#include <string>
#include <memory>
#include "config.h"
#if HAVE_STDINT_H
#include <stdint.h>
#endif

namespace util { class Stream; }

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

    /** Returns a Stream of the raw PCM data for a particular track
     */
    virtual std::unique_ptr<util::Stream> GetTrackStream(unsigned int track) = 0;
};

typedef util::CountedPointer<AudioCD> AudioCDPtr;

class LocalAudioCD: public AudioCD
{
    friend class LocalCDDrive;

    void *m_cdt;
    std::string m_device_name;
    int m_fd;

    struct FullSector {
	uint8_t pcm[2352]; // 16-bit sample pairs
	uint8_t errors[294];
	uint8_t all_errors;
	uint8_t reserved;  // With the all_errors and reserved bytes, is aligned
	uint8_t subchannel[96];
    };

    unsigned m_first_sector;
    FullSector *m_sectors;

public:
    LocalAudioCD() : m_cdt(NULL), m_fd(-1), m_first_sector(0), m_sectors(NULL) {}
    ~LocalAudioCD();

    static unsigned int Create(const std::string& device, AudioCDPtr *result);

    unsigned ReadSector(unsigned int n, const uint8_t **pdata);

    // Being an AudioCD
    std::unique_ptr<util::Stream> GetTrackStream(unsigned int track) override;
};

} // namespace import

#endif
