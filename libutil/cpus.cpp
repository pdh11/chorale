#include "cpus.h"
#include <fstream>
#include <sstream>

unsigned int util::CountCPUs()
{
    static unsigned int n = 0;

    if (!n)
    {
	std::ifstream f("/proc/cpuinfo");
	while (!f.eof())
	{
	    std::string line;
	    std::getline(f, line);
	    
	    std::istringstream is(line);
	    std::string token;
	    is >> token;
	    if (token == "processor")
		++n;
	}
	if (!n)
	    n = 1;
    }
    return n;
}
