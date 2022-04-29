#include "scsi.h"
#include "config.h"
#include "libutil/errors.h"
#include "libutil/trace.h"

namespace import {

static unsigned TryTransport(ScsiTransport *scsi, const char *device)
{
    unsigned rc = scsi->Open(device);
    if (rc)
	return rc;
    unsigned char capabilities[16];
    GetCDCapabilities gcdc;
    return gcdc.Submit(scsi, capabilities, sizeof(capabilities));
}

unsigned CreateScsiTransport(const char *device,
			     std::unique_ptr<ScsiTransport> *result)
{
    result->reset(NULL);

#if HAVE_LINUX_CDROM_H
    {
	std::unique_ptr<LinuxCDTransport> lcd(new LinuxCDTransport);
	unsigned rc = TryTransport(lcd.get(), device);
	if (!rc)
	{
	    *result = std::move(lcd);
	    TRACE << "Got CD transport\n";
	    return 0;
	}
    }

    {
	std::unique_ptr<LinuxSGTransport> lsg(new LinuxSGTransport(true));
	unsigned rc = TryTransport(lsg.get(), device);
	if (!rc)
	{
	    *result = std::move(lsg);
	    TRACE << "Got SG transport\n";
	    return 0;
	}
    }
#endif
#if HAVE_WINDOWS_H
    {
	std::unique_ptr<Win32ScsiTransport> wst(new Win32ScsiTransport);
	unsigned rc = TryTransport(wst.get(), device);
	if (!rc)
	{
	    *result = wst;
	    TRACE << "Got Win32 transport\n";
	    return 0;
	}
    }
#endif

    TRACE << "No transport found\n";

    return ENODEV;
}

} // namespace import
