#ifndef MEDIADB_SYNC_H
#define MEDIADB_SYNC_H 1

#include <set>
#include <map>
#include "libutil/observable.h"

namespace mediadb {

class Database;

class SyncObserver
{
public:
    virtual ~SyncObserver() {}

    enum stage_t {
	BEGIN,
	DELETING,
	PLAYLISTS,
	FILES,
	END
    };

    virtual void OnStage(stage_t) {}
    virtual void OnDelete(unsigned int /*destid*/) {}
    virtual void OnAddPlaylist(unsigned int /*srcid*/, 
			       size_t /*num*/, size_t /*denom*/) {}
    virtual void OnAmendPlaylist(unsigned int /*srcid*/, 
				 size_t /*num*/, size_t /*denom*/) {}
    virtual void OnAddFile(unsigned int /*srcid*/, 
			   size_t /*num*/, size_t /*denom*/) {}
    virtual void OnAmendMetadata(unsigned int /*srcid*/, 
				 size_t /*num*/, size_t /*denom*/) {}
};

/** Synchronise one mediadb::Database with another.
 *
 * By copying, deleting, and amending entries as necessary. Currently
 * uses the TITLE field as the identifying factor.
 */
class Synchroniser: public util::Observable<SyncObserver>
{
public:
    struct Statistics
    {
	unsigned int files_added;
	unsigned int files_deleted;
	unsigned int files_modified;
	unsigned int playlists_added;
	unsigned int playlists_deleted;
	unsigned int playlists_modified;
	unsigned long long bytes_transferred;
    };

private:
    Database *m_src;
    Database *m_dest;
    bool m_dry_run;

    typedef std::set<unsigned int> set_t;
    typedef std::map<unsigned int, unsigned int> map_t;
    typedef std::list<unsigned int> list_t;

    set_t m_srcids_seen;

    map_t m_srctodest;
    map_t m_desttosrc;

    set_t m_srcids_maybe_add;
    set_t m_destids_maybe_delete;

    set_t m_srcids_add_tune;
    set_t m_srcids_amend_metadata;
    list_t m_srcids_add_playlist; // Done in order, to avoid orphaning on crash
    set_t m_srcids_amend_playlist;

    bool m_any_deleted;

    Statistics m_statistics;

    unsigned int CheckPlaylistNames(unsigned int srcid, unsigned int destid,
				    bool *amend_parent = NULL);

    unsigned int MaybeDelete(unsigned int destid, set_t *already);

    unsigned int MaybeAdd(unsigned int srcid, set_t *already);
    unsigned int AmendPlaylist(unsigned int srcid, size_t num, size_t denom);
    unsigned int AmendTune(unsigned int srcid, size_t num, size_t denom);
    unsigned int AddPlaylist(unsigned int srcid, size_t num, size_t denom);
    unsigned int AddTune(unsigned int srcid, size_t num, size_t denom);

    unsigned int CopyPlaylist(unsigned int srcid, unsigned int destid);
    unsigned int CopyMetadata(unsigned int srcid, unsigned int destid);

public:
    Synchroniser(Database *src, Database *dest, bool dry_run);
    unsigned int Synchronise();

    const Statistics& GetStatistics() const { return m_statistics; }
};

} // namespace mediadb

#endif
