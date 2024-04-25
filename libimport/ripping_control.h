#ifndef LIBIMPORT_RIPPING_CONTROL_H
#define LIBIMPORT_RIPPING_CONTROL_H

#include "libutil/task_observer.h"
#include "libutil/observable.h"
#include "libdb/db.h"
#include "cd_drives.h"
#include "audio_cd.h"

namespace util { class TaskQueue; }

namespace import {

/** Values for RippingControlObserver::OnProgress's type */
enum {
    PROGRESS_NONE,
    PROGRESS_RIP,
    PROGRESS_FLAC,
    PROGRESS_MP3
};

class RippingControlObserver
{
public:
    virtual ~RippingControlObserver() {}

    virtual void OnProgress(unsigned int track, unsigned int type,
			    unsigned int num, unsigned int denom) = 0;
};

class RippingControl: public util::TaskObserver,
		      public util::Observable<RippingControlObserver>
{
    util::TaskQueue *m_cpu_queue;
    util::TaskQueue *m_disk_queue;
    std::string m_mp3_root;
    std::string m_flac_root;
    std::string m_track_template;
    std::string m_playlist_template;
    std::string m_album_name;

    unsigned int m_ntracks;
    struct Track;
    Track *m_tracks;

    static unsigned int sm_index;

    // Being a TaskObserver
    void OnProgress(const util::Task *task, unsigned num, unsigned denom);

public:
    RippingControl(CDDrivePtr drive, AudioCDPtr cd,
		   util::TaskQueue *cpu_queue, util::TaskQueue *disk_queue,
		   const std::string& mp3_root, const std::string& flac_root);
    ~RippingControl();

    db::RecordsetPtr Tags(unsigned int track);

    /** Sets the template for the output filenames.
     *
     * Available fields are:
     *   %a Artist
     *   %A Artist with any leading "The " removed
     *   %c Album
     *   %y Year
     *   %t Track name
     *   %n Track number (e.g. %02n for two-digit, zero-filled)
     *
     * The template result is appended to the mp3_root and flac_root.
     * RippingControl takes care of escaping awkward characters in the tags.
     *
     * If playlist_template is empty, no playlist is made.
     *
     * Do not include the filename extension (.mp3, .asx) in the templates.
     */
    void SetTemplates(const std::string& track_template,
		      const std::string& playlist_template)
    {
	m_track_template = track_template;
	m_playlist_template = playlist_template;
    }

    void SetAlbumName(const char *name);

    unsigned int Done();
};

} // namespace import

#endif
