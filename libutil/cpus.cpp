#include "cpus.h"
#include <sys/sysinfo.h>
#include <fstream>
#include <sstream>

unsigned int util::CountCPUs()
{
    return get_nprocs();
}
