#ifndef LOCAL_CD_DRIVE_H
#define LOCAL_CD_DRIVE_H

#include "cd_drive.h"

namespace util { class TaskQueue; }

namespace import {

class CDDrives;

class LocalCDDrive: public CDDrive
{
    class Impl;
    Impl *m_impl;

    friend class Impl;

    LocalCDDrive(const std::string& device);

    friend class CDDrives;

public:
    ~LocalCDDrive();
    std::string GetDevice() const;

    // Being a CDDrive
    std::string GetName() const override;
    bool SupportsDiscPresent() const override;
    bool DiscPresent() const override;
    unsigned int Eject() override;
    util::TaskQueue *GetTaskQueue() override;
    unsigned int GetCD(util::CountedPointer<AudioCD> *result) override;
};

} // namespace import

#endif
