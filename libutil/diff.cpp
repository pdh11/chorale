#include "diff.h"
#include "trace.h"
#include <assert.h>
#include <string.h>

#ifdef TEST

const struct {
    const char *s1;
    const char *s2;
    unsigned int dist;
} tests[] = {

    /** NB do not use '#' in test strings, as the test harness uses this */

    { "abc", "abc", 0 },
    { "ac",  "abc", 1 },
    { "abc", "adc", 2 },
    { "ac", "abcd", 2 },
    { "a", "x", 2 },
    { "abc", "xyz", 6 },
    { "abcd", "axyz", 6 },
    { "abcd", "xyzd", 6 },
    { "abcde", "axyze", 6 },
    { "xaybzcw", "abc", 4 },
    { "xaybzcw", "abdce", 6 },
    { "xaybzcw", "fgabdce", 8 },
    { "xaybzcw", "zgabdce", 8 },
    { "", "abc", 3 },

    /** These are not the same distances as the Wikipedia article, as we count
     * substitution as two (add+del), not one.
     */
    { "kitten", "sitting", 5 },
    { "Sunday", "Saturday", 4 },

};

static void Test(const char *s1, const char *s2, unsigned int dist)
{
    util::DiffResult res;

    unsigned int dist2 = util::Diff(s1, s1+strlen(s1),
				    s2, s2+strlen(s2),
				    &res);

//    TRACE << "distance(" << s1 << "," << s2 << ")=" << dist2 << "\n";

    std::string s3 = s1;

    for (util::DiffResult::deletions_t::iterator i = res.deletions.begin();
	 i != res.deletions.end();
	 ++i)
    {
//	TRACE << "delete at " << *i << "\n";
	s3[*i -1] = '#';
//	TRACE << s3 << "\n";
    }

    for (std::string::iterator i = s3.begin(); i != s3.end(); )
    {
	if (*i == '#')
	    i = s3.erase(i);
	else
	    ++i;
    }

    for (util::DiffResult::additions_t::iterator i = res.additions.begin();
	 i != res.additions.end();
	 ++i)
    {
//	TRACE << "add " << s2[*i -1] << " at " << *i << "\n";
	s3.insert(s3.begin() + *i -1, s2[*i -1]);
//	TRACE << s3 << "\n";
    }

    assert(dist2 == dist);

//    TRACE << "dist2=" << dist2 << "add
    
    assert(dist2 == res.additions.size() + res.deletions.size());

//    TRACE << "result " << s3 << "\n";

    assert(s3 == s2);
}

int main()
{
    for (size_t i = 0; i < sizeof(tests)/sizeof(tests[0]); ++i)
    {
	// Because of the way we define distance, dist(a,b)===dist(b,a)
	Test(tests[i].s1, tests[i].s2, tests[i].dist);
	Test(tests[i].s2, tests[i].s1, tests[i].dist);
    }

    return 0;
}

#endif
