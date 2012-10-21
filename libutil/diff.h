#ifndef LIBUTIL_DIFF_H
#define LIBUTIL_DIFF_H

#include <set>
#include <algorithm>
#include <boost/dynamic_bitset.hpp>
#include "trace.h"

namespace util {

struct DiffPartialResult
{
    typedef boost::dynamic_bitset<> deletions_t;
    deletions_t deletions;

    typedef boost::dynamic_bitset<> additions_t;
    additions_t additions;
};

struct DiffResult
{
    typedef std::set<size_t> deletions_t;
    deletions_t deletions;

    typedef std::set<size_t> additions_t;
    additions_t additions;
};

/** Simple diff between two "strings".
 *
 * Like the Levenshtein distance
 * http://en.wikipedia.org/wiki/Levenshtein_distance -- but
 * considering only additions and deletions, not replacements.
 *
 * O(n*d) in space, O(n*m*log d) in time, where n and m are string
 * lengths and d is distance.
 *
 * @returns Edit distance (Levenshtein distance)
 */
template <class Iter>
unsigned int Diff(Iter s1_begin, Iter s1_end,
		  Iter s2_begin, Iter s2_end,
		  DiffResult *result)
{
    size_t len1 = s1_end - s1_begin;
    size_t len2 = s2_end - s2_begin;

    DiffPartialResult *p_res = new DiffPartialResult[len2 + 1];
    DiffPartialResult *q_res = new DiffPartialResult[len2 + 1];

    for (unsigned int j=0; j <= len2; ++j)
    {
	p_res[j].additions.resize(len2+1);
	p_res[j].deletions.resize(len1+1);
	q_res[j].additions.resize(len2+1);
	q_res[j].deletions.resize(len1+1);
    }

    unsigned int *p = new unsigned int[len2 + 1];
    unsigned int *q = new unsigned int[len2 + 1];

    p[0] = 0;
    for (unsigned int j=1; j <= len2; ++j)
    {
	p[j] = p[j-1] + 1;
	p_res[j] = p_res[j-1];
	p_res[j].additions.set(j);
    }
    
    for (unsigned int i=1; i <= len1; ++i)
    {
	q[0] = p[0] + 1;
	q_res[0] = p_res[0];
	q_res[0].deletions.set(i);

//	TRACE << i << "/" << len1 << "\n";

	for (unsigned int j=1; j <= len2; ++j)
	{
	    unsigned int cost = -1u;
	    enum { NOTHING, ADD, DEL } edit = NOTHING;
	    if (s1_begin[i-1] == s2_begin[j-1])
		cost = p[j-1];

	    unsigned int cost_add = q[j-1] + 1;
	    if (cost_add < cost)
	    {
		cost = cost_add;
		edit = ADD;
	    }

	    unsigned int cost_del = p[j] + 1;
	    if (cost_del < cost)
	    {
		cost = cost_del;
		edit = DEL;
	    }

	    q[j] = cost;
	    
	    switch (edit)
	    {
	    case ADD:
		q_res[j] = q_res[j-1];
		q_res[j].additions.set(j);
		break;

	    case DEL:
		q_res[j] = p_res[j];
		q_res[j].deletions.set(i);
		break;

	    case NOTHING:
		q_res[j] = p_res[j-1];
		break;
	    }

//	    printf("%.*s -> %.*s : %u\n",
//		   i, s1_begin, j, s2_begin, q[j]);
	}
	std::swap(p,q);
	std::swap(p_res, q_res);
    }

    result->deletions.clear();
    result->additions.clear();
    for (unsigned int j=0; j<=len2; ++j)
    {
	if (p_res[len2].additions[j])
	    result->additions.insert(j);
    }
    for (unsigned int j=0; j<=len1; ++j)
    {
	if (p_res[len2].deletions[j])
	    result->deletions.insert(j);
    }
//    *result = p_res[len2];
    unsigned int dist = p[len2];

    delete[] p;
    delete[] q;
    delete[] p_res;
    delete[] q_res;

    return dist;
}

} // namespace util

#endif
