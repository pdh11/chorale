#include "scsi.h"
#include "libutil/trace.h"
#include "config.h"

#if HAVE_LINUX_CDROM_H

#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <boost/static_assert.hpp>

/* Check that our portable enum matches the Linux one */
BOOST_STATIC_ASSERT(import::READ_TOC_PMA_ATIP == GPCMD_READ_TOC_PMA_ATIP);
BOOST_STATIC_ASSERT(import::CD_CAPABILITIES == GPMODE_CAPABILITIES_PAGE);

namespace import {

LinuxScsiTransport::LinuxScsiTransport()
    : m_fd(-1)
{
}

LinuxScsiTransport::~LinuxScsiTransport()
{
    Close();
}

unsigned LinuxScsiTransport::Open(const char *device, int flags)
{
    m_fd = ::open(device, flags);
    if (m_fd < 0)
    {
	TRACE << "Can't open " << device << ":\n";
	perror("open");
	return errno;
    }
    return 0;
}

void LinuxScsiTransport::Close()
{
    if (m_fd >= 0)
	::close(m_fd);
}


    /* cdrom transport */


unsigned LinuxCDTransport::Open(const char *device)
{
    return LinuxScsiTransport::Open(device, O_RDONLY|O_NONBLOCK);
}

unsigned LinuxCDTransport::SendCommand(const unsigned char *command, 
				       unsigned clen,
				       void *buffer, size_t len)
{
    cdrom_generic_command cgc;
    memset(&cgc, '\0', sizeof(cgc));
    request_sense rs;
    memset(&rs, '\0', sizeof(rs));

    memcpy(cgc.cmd, command, clen);
    cgc.buffer = (unsigned char*)buffer;
    cgc.buflen = (unsigned)len;
    cgc.sense = &rs;
    cgc.data_direction = buffer ? CGC_DATA_READ : CGC_DATA_NONE;

    for (unsigned int i=0; i<clen; ++i)
	printf(" %02X", command[i]);
    printf("\n");
    fflush(stdout);

    int rc = ::ioctl(m_fd, CDROM_SEND_PACKET, &cgc);
    if (rc<0)
    {
	TRACE << "cd ioctl returned " << rc << " errno=" << errno << "\n";
	return errno;
    }

    if (cgc.buflen)
	printf("%u bytes of buffer left\n", (unsigned)cgc.buflen);
    
    if (!rs.sense_key)
	return 0;

    printf("sense key is %u\n", rs.sense_key);
    return EIO;
}


    /* scsi-generic transport */


unsigned LinuxSGTransport::Open(const char *device)
{
    return LinuxScsiTransport::Open(device, O_RDWR|O_NONBLOCK);
}

unsigned LinuxSGTransport::SendCommand(const unsigned char *command,
				       unsigned clen,
				       void *buffer, size_t len)
{
retry:

    sg_io_hdr_t sgio;
    memset(&sgio, '\0', sizeof(sgio));
    request_sense rs;
    memset(&rs, '\0', sizeof(rs));

    sgio.interface_id = 'S';
    sgio.dxfer_direction = buffer ? SG_DXFER_FROM_DEV : SG_DXFER_NONE;
    sgio.cmdp = const_cast<unsigned char*>(command);
    sgio.cmd_len = (unsigned char)clen;
    sgio.sbp = (unsigned char*)&rs;
    sgio.mx_sb_len = sizeof(rs);
    sgio.dxferp = buffer;
    sgio.dxfer_len = (unsigned)len;
    sgio.timeout = 50*1000; // in ms
    sgio.flags = SG_FLAG_DIRECT_IO;

    memset(buffer, '\0', len);

    if (m_use_ioctl)
    {
	printf("Calling ioctl\n");
	int rc = ::ioctl(m_fd, SG_IO, &sgio);
	if (rc<0)
	{
	    perror("sg ioctl");
	    return errno;
	}
    }
    else
    {
	printf("Writing\n");
	ssize_t n = ::write(m_fd, &sgio, sizeof(sgio));
	printf("write returned %d\n", (int)n);
	if (n != sizeof(sgio))
	{
	    perror("sgiowrite");
	    return errno;
	}
	printf("waiting\n");
	n = ::read(m_fd, &sgio, sizeof(sgio));
	if (n != sizeof(sgio))
	{
	    perror("sgioread");
	    return errno;
	}
    }

    printf("info=0x%x\n", sgio.info);
    if ((sgio.info & SG_INFO_OK_MASK) == SG_INFO_OK)
    {
	printf("sgio OK\n");
	return 0;
    }
    printf("status=%02x masked=%02x msg=%02x sblen=%u hs=%04x ds=%04x resid=%u\n",
	   sgio.status, sgio.masked_status, sgio.msg_status, sgio.sb_len_wr,
	   sgio.host_status, sgio.driver_status, sgio.resid);
    if (sgio.driver_status & 8)
    {
	// Check sense buffer
	printf("sense error=%u key=%u len=%u\n",
	       rs.error_code, rs.sense_key, rs.add_sense_len);
	if (rs.add_sense_len >= 6)
	{
	    printf("sense asc=%x ascq=%x\n", rs.asc, rs.ascq);
	    if ((rs.asc == 4 && rs.ascq == 1)
		|| (rs.asc == 0x28 && rs.ascq == 0))
	    {
		printf("retrying\n");
		sleep(1);
		goto retry;
	    }
	}
    }
    
    if (sgio.status == 0 && sgio.driver_status == 0 && sgio.host_status == 0)
    {
	printf("sgio OK2\n");
	return 0;
    }

    return EIO;
}

} // namespace import

#endif // HAVE_LINUX_CDROM_H
