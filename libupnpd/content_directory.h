#ifndef UPNPD_CONTENT_DIRECTORY_H
#define UPNPD_CONTENT_DIRECTORY_H 1

#include "libupnp/ContentDirectory2.h"

namespace mediadb { class Database; }

namespace upnpd {

/** Actual implementation of upnp::ContentDirectory2 base class in terms of
 * a mediadb::Database.
 */
class ContentDirectoryImpl: public upnp::ContentDirectory2
{
    mediadb::Database *m_db;
    unsigned short m_port;

public:
    ContentDirectoryImpl(mediadb::Database *db, unsigned short port);
    
    // Being a ContentDirectory2
    unsigned int Browse(const std::string& object_id,
			BrowseFlag browse_flag,
			const std::string& filter,
			uint32_t starting_index,
			uint32_t requested_count,
			const std::string& sort_criteria,
			std::string *result,
			uint32_t *number_returned,
			uint32_t *total_matches,
			uint32_t *update_id);
    unsigned int Search(const std::string& container_id,
			const std::string& search_criteria,
			const std::string& filter,
			uint32_t starting_index,
			uint32_t requested_count,
			const std::string& sort_criteria,
			std::string *result,
			uint32_t *number_returned,
			uint32_t *total_matches,
			uint32_t *update_id);
    unsigned int GetSearchCapabilities(std::string *search_caps);
    unsigned int GetSortCapabilities(std::string *sort_caps);
    unsigned int GetFeatureList(std::string *feature_list);
    unsigned int GetSystemUpdateID(uint32_t*);
};

} // namespace upnpd

#endif
