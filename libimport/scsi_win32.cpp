#include "scsi.h"
#include "config.h"

#if HAVE_WINDOWS_H

#include "libutil/trace.h"
#include <ddk/ntddscsi.h>
#include <windows.h>
#include <io.h>

namespace import {

Win32ScsiTransport::Win32ScsiTransport()
    : m_handle(NULL)
{
}

Win32ScsiTransport::~Win32ScsiTransport()
{
    if (m_handle)
	::CloseHandle(m_handle);
}

unsigned Win32ScsiTransport::Open(const char *device)
{
    m_handle = ::CreateFileA(device,
			     GENERIC_READ | GENERIC_WRITE,
			     FILE_SHARE_READ | FILE_SHARE_WRITE,
			     NULL, 
			     OPEN_EXISTING, 
			     FILE_ATTRIBUTE_NORMAL,
			     NULL);
    if (m_handle == INVALID_HANDLE_VALUE)
    {
	unsigned rc = GetLastError();
        TRACE << "CreateFile failed: " << rc << "\n";
	m_handle = NULL;
        return rc;
    }
    return 0;
}

unsigned Win32ScsiTransport::SendCommand(const unsigned char *cmd,
					 unsigned commandlen,
					 void *buffer,
					 size_t bufsize)
{
    SCSI_PASS_THROUGH_DIRECT sptd;
    memset(&sptd, '\0', sizeof(sptd));
    memcpy(sptd.Cdb, cmd, commandlen);
    sptd.CdbLength = (unsigned char)commandlen;
    sptd.DataIn = buffer ? SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_UNSPECIFIED;
    sptd.DataBuffer = buffer;
    sptd.DataTransferLength = bufsize;
    sptd.TimeOutValue = 50*1000;

    DWORD nbytes;

    bool ok = ::DeviceIoControl(m_handle, IOCTL_SCSI_PASS_THROUGH_DIRECT,
				(void*)&sptd, (DWORD)sizeof(sptd), NULL, 0, 
				&nbytes, NULL);

    if (!ok)
    {
	unsigned rc = ::GetLastError();
	TRACE << "DeviceIoControl failed: " << rc << "\n";
	return rc;
    }

    TRACE << "DeviceIoControl OK\n";
    return 0;
}

}

#endif // HAVE_WINDOWS_H
