#ifndef LIBUTIL_COMPARE_H
#define LIBUTIL_COMPARE_H 1

namespace util {

/** Like strcoll, but is UTF-8 everywhere.
 *
 * @arg total If true, force total order by using tie-breaker; if false,
 *            equivalent strings compare equal.
 */
int Compare(const char *s1, const char *s2, bool total = false);

} // namespace util

#endif
