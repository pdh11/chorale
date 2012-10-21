#ifndef LIBUPNPD_SEARCH_H
#define LIBUPNPD_SEARCH_H 1

#include <string>
#include "libdb/db.h"

namespace upnpd {

/** Parse UPnP ContentDirectory search criteria (CDSv2 s2.3.11)
 */
unsigned int ApplySearchCriteria(db::QueryPtr qp, const std::string& s,
				 unsigned int *collatefield);

} // namespace upnpd

#endif
