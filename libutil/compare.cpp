#include "compare.h"
#include "utf8.h"

#include "simplify_tables.h"

namespace util {

namespace {

/** Simplify a (wide) character by folding accents and case. Return 0 for
 * punctuation and control characters.
 */
utf32_t Simplify(utf32_t ch)
{
    unsigned int ui = ch;
    for (const simplify::Table *t = &simplify::tables[0]; t->from; ++t)
    {
	if (ui < t->from)
	    return 0;
	if (ui < t->to)
	{
	    if (t->table)
		return t->table[ui - t->from];
	}
    }
    return 0;
}

int CompareDigits(const char *d1start, const char *d1end,
		  const char *d2start, const char *d2end, int *pTiebreak)
{
    int count1 = 0;
    int count2 = 0;
    for (const char *p1 = d1start; p1 != d1end; ++p1)
	if (*p1 >= '0' && *p1 <= '9')
	    ++count1;
    for (const char *p2 = d2start; p2 != d2end; ++p2)
	if (*p2 >= '0' && *p2 <= '9')
	    ++count2;
    
    if (count1 > count2)
	return 1;
    if (count1 < count2)
	return -1;

    // Same number of digits -- compare digit sequences
    const char *p1 = d1start;
    const char *p2 = d2start;

    char c1 = 0;
    char c2 = 0;

    while (count1)
    {
//	TRACE << p1 << "< - >" << p2 << "\n";

	if (!c1)
	    c1 = *p1++;
	if (!c2)
	    c2 = *p2++;

//	TRACE << c1 << "<?>" << c2 << "\n";

	if (c1 != c2 && !*pTiebreak)
	{
	    if (c1 > c2)
		*pTiebreak = 1;
	    else if (c1 < c2)
		*pTiebreak = -1;
	}

	if (c1 < '0' || c1 > '9')
	{
	    c1 = 0;
	    continue;
	}

	if (c2 < '0' || c2 > '9')
	{
	    c2 = 0;
	    continue;
	}

	if (c1 > c2)
	    return 1;
	else if (c2 > c1)
	    return -1;

	c1 = c2 = 0;
	--count1;
    }

    return 0;
}

} // anon namespace

int Compare(const char *s1, const char *s2, bool total)
{
    int tiebreak = 0;
    utf32_t c1 = 0;
    utf32_t c2 = 0;
    utf32_t sc1, sc2;
    const char *digits1 = NULL;
    const char *digits2 = NULL;

    for (;;)
    {
//	TRACE << s1 << "<->" << s2 << " (" << c1 << "," << c2 << ")\n";

	if (!c1)
	{
	    c1 = GetChar(&s1);
	}
	if (!c2)
	{
	    c2 = GetChar(&s2);
	}

	if (!c1 || !c2)
	    break; // End of one or both strings
	
	// Valid characters in both strings
	if (total && !tiebreak)
	{
	    if (c1 > c2)
		tiebreak = 1;
	    else if (c1 < c2)
		tiebreak = -1;
	}

	// Simplified away?
	sc1 = Simplify(c1);
	if (!sc1)
	{
	    c1 = 0;
	}
	sc2 = Simplify(c2);
	if (!sc2)
	{
	    c2 = 0;
	}

        if (!c1 || !c2) {
            continue;
        }
	
	bool digit1 = c1 >= '0' && c1 <= '9';
	bool digit2 = c2 >= '0' && c2 <= '9';

	if (digit1 && digit2)
	{
	    digits1 = s1-1;
	    digits2 = s2-1;

	    for (;;)
	    {
		c1 = GetChar(&s1);
		if (!c1)
		    break;
		sc1 = Simplify(c1);
		if (!sc1 && c1 == ',')
		    continue;
		if (c1 < '0' || c1 > '9')
		    break;
	    }

	    for (;;)
	    {
		c2 = GetChar(&s2);
		if (!c2)
		    break;
		sc2 = Simplify(c2);
		if (!sc2 && c2 == ',')
		    continue;
		if (c2 < '0' || c2 > '9')
		    break;
	    }

	    /* Both digit sequences have ended */
	    int rc = CompareDigits(digits1, s1, digits2, s2, &tiebreak);
	    if (rc)
		return rc;

	    if (!c1 || !c2) // End of one or both strings
		break;

	    continue; // More to do, carry on
	}

	if (sc1 > sc2)
	    return 1;
	else if (sc1 < sc2)
	    return -1;

	c1 = c2 = 0;
    }

//    TRACE << c1 << "," << c2 << " " << tiebreak << "\n";

    if (c1)
	return 1;
    if (c2)
	return -1;
    if (total)
	return tiebreak;
    return 0;
}

} // namespace util

#ifdef TEST

# include <assert.h>

static const struct {
    const char *s1;
    const char *s2;
    int how;
} tests [] = {
    { "ABBA", "Abba", -1 },
    { "ABA",  "ABB",  -2 },
    { "ABBA", "Abb",   2 },
    { "ABB",  "Abba", -2 },
    { "ABB  ","Abba", -2 },
    { "ABB",  "Abba  ", -2 },
    { "ABB  ","Abba  ", -2 },
    { "A B B","A b b a", -2 },
    { "808 State", "10,000 Maniacs", -2 },
    { "808", "10,000", -2 },
    { "1,000,000", "999999", 2 },
    { "1,000", "1000", -1 },
    { "AB1000BA", "AB1000CA", -2 },
    { "AB1000BA", "AB1,000CA", -2 },
    { "AB1000BA", "AB1,000BA", 1 },
    { "AB1000BA", "AB1,000BB", -2 },
    { "01", "02", -2 },
    { "01 99 Red Balloons", "02 Foo", -2 },
    { "01", "10", -2 },
    { "01 Foo", "02 Bar", -2 },
    { "01 Foo", "10 Foo", -2 },
    { "L.E.F.", "L.E.F.", 0 },
};

int main()
{
    for (unsigned int i=0; i<(sizeof(tests)/sizeof(*tests)); ++i)
    {
	const char *s1 = tests[i].s1;
	const char *s2 = tests[i].s2;
	int how = tests[i].how;
	
	switch (how)
	{
	case -2: // First string clearly first
	    assert(util::Compare(s1, s2, false) < 0);
	    assert(util::Compare(s2, s1, false) > 0);
	    assert(util::Compare(s1, s2, true) < 0);
	    assert(util::Compare(s2, s1, true) > 0);
	    break;
	case -1: // First string first on tie-break
	    assert(util::Compare(s1, s2, false) == 0);
	    assert(util::Compare(s2, s1, false) == 0);
	    assert(util::Compare(s1, s2, true) < 0);
	    assert(util::Compare(s2, s1, true) > 0);
	    break;
	case 0: // Identical strings
	    assert(util::Compare(s1, s2, false) == 0);
	    assert(util::Compare(s2, s1, false) == 0);
	    assert(util::Compare(s1, s2, true) == 0);
	    assert(util::Compare(s2, s1, true) == 0);
	    break;
	case 1: // Second string first on tie-break
	    assert(util::Compare(s1, s2, false) == 0);
	    assert(util::Compare(s2, s1, false) == 0);
	    assert(util::Compare(s1, s2, true) > 0);
	    assert(util::Compare(s2, s1, true) < 0);
	    break;
	case 2: // Second string clearly first
	    assert(util::Compare(s1, s2, false) > 0);
	    assert(util::Compare(s2, s1, false) < 0);
	    assert(util::Compare(s1, s2, true) > 0);
	    assert(util::Compare(s2, s1, true) < 0);
	    break;
	}
    }

    return 0;
}

#endif
