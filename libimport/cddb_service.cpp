#include "config.h"
#include "cddb_service.h"
#include "libutil/trace.h"
#include "libutil/counted_pointer.h"

#if HAVE_LIBCDDB

/* For entirely bonkers reason, cddb_conn.h includes netinet/in.h. It
 * doesn't need to, and on Win32 it plain can't.
 */
#undef HAVE_NETINET_IN_H
#include <cddb/cddb.h>

namespace import {

struct CDDBService::Impl
{
    cddb_conn_t *conn;
};

CDDBService::CDDBService()
    : m_impl(new Impl)
{
    m_impl->conn = cddb_new();
    cddb_set_server_port(m_impl->conn, 80);
}

CDDBService::~CDDBService()
{
    cddb_destroy(m_impl->conn);
    delete m_impl;
}

void CDDBService::SetUseProxy(bool whether)
{
    if (whether)
	cddb_http_proxy_enable(m_impl->conn);
    else
	cddb_http_enable(m_impl->conn);
}

void CDDBService::SetProxyHost(const std::string& s)
{
    cddb_set_http_proxy_server_name(m_impl->conn, s.c_str());
}

void CDDBService::SetProxyPort(unsigned short s)
{
    cddb_set_http_proxy_server_port(m_impl->conn, s);
}

static const char *safe(const char *s)
{
    return s ? s : "";
}

CDDBLookupPtr CDDBService::Lookup(AudioCDPtr cd)
{
    cddb_disc_t *disc = cddb_disc_new();

    cddb_disc_set_length(disc, cd->GetTotalSectors() / 75);

    for (AudioCD::const_iterator i = cd->begin(); i != cd->end(); ++i)
    {
	cddb_track_t *track = cddb_track_new();
	cddb_track_set_frame_offset(track, i->first_sector);
	cddb_disc_add_track(disc, track);
    }

    int matches = cddb_query(m_impl->conn, disc);
    TRACE << matches << " matches\n";
    if (matches <= 0)
	return CDDBLookupPtr();

    CDDBLookupPtr result(new CDDBLookup);

    while (matches)
    {
	cddb_read(m_impl->conn, disc);

	CDDBFound found;
	found.discid = cddb_disc_get_discid(disc);
	found.genre = cddb_disc_get_genre(disc);
	found.year = cddb_disc_get_year(disc);
	found.title = safe(cddb_disc_get_title(disc));
	found.artist = safe(cddb_disc_get_artist(disc));
	found.comment = safe(cddb_disc_get_ext_data(disc));

	TRACE << found.title << " by " << found.artist
	      << " extras '" << found.comment << "'\n";

	int n = cddb_disc_get_track_count(disc);

	for (int i=0; i<n; ++i)
	{
	    cddb_track_t *track = cddb_disc_get_track(disc, i);
	    if (track)
	    {
		CDDBTrack mytrack;
		mytrack.title = safe(cddb_track_get_title(track));
		mytrack.artist = safe(cddb_track_get_artist(track));
		mytrack.comment = safe(cddb_track_get_ext_data(track));
		found.tracks.push_back(mytrack);
		TRACE << "  " << (i+1) << " " << mytrack.title << "\n";
	    }
	}

	result->discs.push_back(found);

	--matches;
	if (matches)
	{
	    cddb_query_next(m_impl->conn, disc);
	}
    }

    cddb_disc_destroy(disc); // frees tracks too

    return result;
}

} // namespace import

#else // !HAVE_LIBCDDB

namespace import {

CDDBService::CDDBService() : m_impl(NULL) {}
CDDBService::~CDDBService() {}

void CDDBService::SetUseProxy(bool) {}
void CDDBService::SetProxyHost(const std::string&) {}
void CDDBService::SetProxyPort(unsigned short) {}
CDDBLookupPtr CDDBService::Lookup(AudioCDPtr) { return CDDBLookupPtr(); }

} // namespace import

#endif // !HAVE_LIBCDDB
