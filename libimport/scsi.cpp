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
			     std::auto_ptr<ScsiTransport> *result)
{
    result->reset(NULL);

#if HAVE_LINUX_CDROM_H
    {
	std::auto_ptr<LinuxCDTransport> lcd(new LinuxCDTransport);
	unsigned rc = TryTransport(lcd.get(), device);
	if (!rc)
	{
	    *result = lcd;
	    TRACE << "Got CD transport\n";
	    return 0;
	}
    }

    {
	std::auto_ptr<LinuxSGTransport> lsg(new LinuxSGTransport(true));
	unsigned rc = TryTransport(lsg.get(), device);
	if (!rc)
	{
	    *result = lsg;
	    TRACE << "Got SG transport\n";
	    return 0;
	}
    }
#endif
#if HAVE_WINDOWS_H
    {
	std::auto_ptr<Win32ScsiTransport> wst(new Win32ScsiTransport);
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
