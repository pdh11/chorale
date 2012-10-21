#ifndef UPNPD_CONTENT_DIRECTORY_H
#define UPNPD_CONTENT_DIRECTORY_H 1

#include "libupnp/ContentDirectory.h"

namespace mediadb { class Database; }
namespace upnp { namespace soap { class InfoSource; } }

namespace upnpd {

/** Actual implementation of upnp::ContentDirectory base class in terms of
 * a mediadb::Database.
 */
class ContentDirectoryImpl: public upnp::ContentDirectory
{
    mediadb::Database *m_db;
    upnp::soap::InfoSource *m_info_source;

public:
    ContentDirectoryImpl(mediadb::Database*, upnp::soap::InfoSource*);
    
    // Being a ContentDirectory
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
    unsigned int GetServiceResetToken(std::string*);
};

} // namespace upnpd

#endif
