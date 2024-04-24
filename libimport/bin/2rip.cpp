#include "libimport/audio_cd.h"
#include "libimport/cd_drives.h"
#include "libimport/ripping_control.h"
#include "libutil/counted_pointer.h"
#include "libutil/worker_thread_pool.h"

#include <unistd.h>

#include <list>

class TextObserver: public import::RippingControlObserver
{
    unsigned int m_percent[300]; // 6 tracks * 3 figures

public:
    TextObserver()
        : m_percent()
    {
    }

    void OnProgress(unsigned int, unsigned int,
		    unsigned int, unsigned int)
    {
    }

    bool IsDone();
};

int main()
{
    import::CDDrives drives;
    
    drives.Refresh();
    import::CDDrivePtr drv1 = drives.GetDriveForDevice("/dev/sr0");
    import::CDDrivePtr drv2 = drives.GetDriveForDevice("/dev/sr1");

    import::AudioCDPtr cd1, cd2;

    unsigned rc = import::LocalAudioCD::Create("/dev/sr0", &cd1);
    if (rc) {
        perror("cd1");
        return rc;
    }
    rc = import::LocalAudioCD::Create("/dev/sr1", &cd2);
    if (rc) {
        perror("cd2");
        return rc;
    }

    util::WorkerThreadPool cpupool(util::WorkerThreadPool::NORMAL, 24);
    util::WorkerThreadPool diskpool(util::WorkerThreadPool::NORMAL, 24);

    TextObserver tobs;
    import::RippingControl rc1(drv1, cd1, &cpupool, &diskpool,
                               "/home/peter/src/chorale/tmp1",
                               "/home/peter/src/chorale/tmp1");
    import::RippingControl rc2(drv2, cd2, &cpupool, &diskpool,
                               "/home/peter/src/chorale/tmp2",
                               "/home/peter/src/chorale/tmp2");
    rc1.AddObserver(&tobs);
    rc2.AddObserver(&tobs);
    rc1.SetTemplates("a%02n", "b%02n");
    rc2.SetTemplates("a%02n", "b%02n");
    rc1.Done();
    rc2.Done();

    do {
        printf("cpu=%u disk=%u cd1=%u cd2=%u\n", unsigned(cpupool.Count()),
               unsigned(diskpool.Count()),
               unsigned(drv1->GetTaskQueue()->Count()),
               unsigned(drv2->GetTaskQueue()->Count()));
        sleep(1);
    } while ( cpupool.Count() || diskpool.Count()
              || drv1->GetTaskQueue()->Count()
              || drv2->GetTaskQueue()->Count() );

    printf("Leaving\n");

    return 0;
}
