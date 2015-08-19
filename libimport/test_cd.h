#ifndef LIBIMPORT_TEST_CD_H
#define LIBIMPORT_TEST_CD_H

#include "audio_cd.h"
#include "cd_drives.h"
#include "libutil/worker_thread_pool.h"

namespace import {

class TestCD: public AudioCD
{
public:
    TestCD();

    std::unique_ptr<util::Stream> GetTrackStream(unsigned int);
};

class TestCDDrive: public CDDrive
{
    util::WorkerThreadPool m_wtp;

public:
    TestCDDrive();

    std::string GetName() const { return "TestCDDrive"; }
    bool SupportsDiscPresent() const { return true; }
    bool DiscPresent() const { return true; }
    unsigned int Eject() { return 0; }
    util::TaskQueue *GetTaskQueue() { return &m_wtp; }
    unsigned int GetCD(AudioCDPtr *result)
    {
	result->reset(new TestCD); 
	return 0;
    }
};

} // namespace import

#endif
