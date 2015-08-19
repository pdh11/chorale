#include "audio_cd.h"
#include "cd_drives.h"
#include "ripping_control.h"
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

    void OnProgress(unsigned int which, unsigned int type,
		    unsigned int num, unsigned int denom)
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

    while (cpupool.Count() || diskpool.Count()) {
        printf("cpu=%u disk=%u\n", unsigned(cpupool.Count()),
               unsigned(diskpool.Count()));
        sleep(1);
    }

    return 0;
}
