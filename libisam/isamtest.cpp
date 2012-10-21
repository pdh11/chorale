#include "key_storage.h"
#include "file.h"
#include "page_allocator.h"
#include "page_locking.h"
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <map>
#include "libutil/trace.h"

const char vowels[] = "aeiouyaeiouaeiou";
const char consonants[] = "bcdefghjklmnpqrstvwxzbcdfgjklmnprstvwbcdfgjklmnprst";

std::string RandomString()
{
    std::string result;

    if (rand() & 1)
	goto vowel;

consonant:
    result += consonants[rand() % strlen(consonants)];
    if (rand() & 1)
	result += consonants[rand() % strlen(consonants)];
    if (!(rand() % 20))
	return result;

vowel:
    result += vowels[rand() % strlen(vowels)];
    if (!(rand() % 40))
	return result;
    goto consonant;
}

class Timer
{
    struct timeval m_start;

public:
    Timer()
    {
	::gettimeofday(&m_start, 0);
    }
    ~Timer()
    {
	struct timeval end;
	::gettimeofday(&end, 0);
	uint64_t start_usec = m_start.tv_usec + 1000000ull*m_start.tv_sec;
	uint64_t end_usec = end.tv_usec + 1000000ull*end.tv_sec;
	uint64_t usec = end_usec - start_usec;
	printf("%u.%06us\n", (unsigned int)(usec/1000000),
	       (unsigned int)(usec%1000000));
    }
};

typedef std::map<std::string, std::string> map_t;

enum {
    NUM_REQUESTS = 2000
};

int main()
{
    isam::File file;

	unlink("key_storage2.isam");

	unsigned int rc = file.Open("key_storage2.isam", 0);
	assert(rc == 0);

	isam::PageAllocator pa(&file);
	isam::PageLocking pl(&file);

	isam::KeyStorage ks(&file, &pa, &pl);

	rc = ks.InitialiseFile();
	assert(rc == 0);

	srand(42);

	std::string key, value;
	map_t map;

	{
	    Timer t;

	    for (unsigned int i=0; i<NUM_REQUESTS; ++i)
	    {
		key = RandomString();
		value = RandomString();
		map[key] = value;
		rc = ks.Store(key, value);
//	    TRACE << "Stored " << key << " = " << value << "\n";
		assert(rc == 0);
		
		for (map_t::const_iterator j = map.begin();
		     j != map.end();
		     ++j)
		{
		    rc = ks.Fetch(j->first, &value);
		    if (rc)
		    {
			TRACE << "Can't fetch " << j->first << ": " << rc << "\n";
		    }
		    assert(rc == 0);
		    if (value != j->second)
		    {
			TRACE << "Key '" << j->first << "' expected '"
			      << j->second << "' got '" << value << "'\n";
			ks.Dump(0,"");
		    }
		    assert(value == j->second);
		}
	    }
	}

	return 0;
}
