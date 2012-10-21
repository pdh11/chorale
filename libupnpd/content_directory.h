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
    unsigned int Browse(const std::string& ObjectID,
			const std::string& BrowseFlag,
			const std::string& Filter,
			uint32_t StartingIndex,
			uint32_t RequestedCount,
			const std::string& SortCriteria,
			std::string *Result,
			uint32_t *NumberReturned,
			uint32_t *TotalMatches,
			uint32_t *UpdateID);
    unsigned int Search(const std::string& ContainerID,
			const std::string& SearchCriteria,
			const std::string& Filter,
			uint32_t StartingIndex,
			uint32_t RequestedCount,
			const std::string& SortCriteria,
			std::string *Result,
			uint32_t *NumberReturned,
			uint32_t *TotalMatches,
			uint32_t *UpdateID);
    unsigned int GetSearchCapabilities(std::string *SearchCaps);
    unsigned int GetSortCapabilities(std::string *SortCaps);
    unsigned int GetFeatureList(std::string *feature_list);
};

} // namespace upnpd

#endif
