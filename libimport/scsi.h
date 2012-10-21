#ifndef LIBIMPORT_SCSI_H
#define LIBIMPORT_SCSI_H

#include <stdlib.h>
#include "libutil/endian.h"
#include <memory>
#include <string.h>
#include <stdio.h>

namespace import {

/** A generic SCSI transport
 *
 * Note that even ATAPI CD-ROM drives are programmed using SCSI MMC-3 commands.
 */
class ScsiTransport
{
public:
    virtual ~ScsiTransport() {}

    virtual unsigned Open(const char *device) = 0;
    virtual unsigned SendCommand(const unsigned char *command, unsigned clen,
				 void *buffer = NULL, size_t len = 0) = 0;

    unsigned CheckSense(const unsigned char *sense);
};

class LinuxScsiTransport: public ScsiTransport
{
protected:
    int m_fd;

    unsigned Open(const char *device, int flags);

public:
    LinuxScsiTransport();
    ~LinuxScsiTransport();

    virtual unsigned Open(const char *device) = 0;
    void Close();

    int GetHandle() { return m_fd; }
};

class LinuxCDTransport: public LinuxScsiTransport
{
public:
    unsigned Open(const char *device);
    unsigned SendCommand(const unsigned char*, unsigned, void*, size_t);
};

class LinuxSGTransport: public LinuxScsiTransport
{
    bool m_use_ioctl;

public:
    explicit LinuxSGTransport(bool use_ioctl) : m_use_ioctl(use_ioctl) {}

    unsigned Open(const char *device);
    unsigned SendCommand(const unsigned char*, unsigned, void*, size_t);
};

class Win32ScsiTransport: public ScsiTransport
{
    void *m_handle;

public:
    Win32ScsiTransport();
    ~Win32ScsiTransport();

    unsigned Open(const char *device);
    unsigned SendCommand(const unsigned char*, unsigned, void*, size_t);
};

/** Open and return the right sort of ScsiTransport object for this device
 */
unsigned CreateScsiTransport(const char *device, 
			     std::auto_ptr<ScsiTransport>*);

/** cf MMC-3 table 11 */
enum {
    READ_TOC_PMA_ATIP = 0x43,
    MODE_SENSE_10     = 0x5A,
    READ_CD_MSF       = 0xB9, /* 5.1.9 */
    SET_CD_SPEED      = 0xBB, /* 5.1.15 */
    READ_CD           = 0xBE  /* 5.1.8 */
};

enum {
    CD_CAPABILITIES = 0x2A  /* 5.2.3.4 */
};

template <unsigned LEN>
class ScsiCommand
{
protected:
    unsigned char m_buffer[LEN];

    ScsiCommand(unsigned char c)
    {
	memset(m_buffer, '\0', LEN);
	m_buffer[0] = c;
    }
    unsigned Submit(ScsiTransport *st, void *buffer=NULL, size_t len=0)
    {
	for (unsigned int i=0; i<LEN; ++i)
	    printf(" %02X", m_buffer[i]);
	printf("\n");
	fflush(stdout);
	return st->SendCommand(m_buffer, LEN, buffer, len);
    }
};

class GetCDCapabilities: public ScsiCommand<10>
{
public:
    GetCDCapabilities()
	: ScsiCommand(MODE_SENSE_10)
    {
	m_buffer[2] = CD_CAPABILITIES;
    }

    unsigned Submit(ScsiTransport *st, void *buffer, size_t len)
    {
	write_be16(m_buffer+7, (uint16_t)len);
	return ScsiCommand::Submit(st, buffer, len);
    }
};

enum {
    USE_MSF  = 0x01,
    USE_C2B  = 0x02,
    USE_C2BB = 0x04,
    USE_Q    = 0x08,
    USE_RAW  = 0x10
};

inline unsigned GetSectorSize(unsigned flags)
{
    unsigned size = 2352;
    if (flags & USE_C2BB)
	size += 296;
    else if (flags & USE_C2B)
	size += 294;
    if (flags & USE_Q)
	size += 16;
    else if (flags & USE_RAW)
	size += 96;
    return size;
}

class ReadCD: public ScsiCommand<12>
{
    static void write_msf(int lba, unsigned char *msf)
    {
	*msf = (unsigned char)(lba/(75*60));
	msf[1] = (unsigned char)((lba/75) % 60);
	msf[2] = (unsigned char)(lba % 75);
    }

public:
    ReadCD(unsigned flags, unsigned int lba, unsigned int n)
	: ScsiCommand((flags & USE_MSF) ? READ_CD_MSF : READ_CD)
    {
	if (flags & USE_MSF)
	{
	    write_msf(lba, m_buffer+3);
	    write_msf(lba+n, m_buffer+6);
	}
	else
	{
	    write_be24(m_buffer+3, lba);
	    write_be24(m_buffer+6, n);
	}
	m_buffer[9] = 0x10;
	if (flags & USE_C2BB)
	    m_buffer[9] |= 0x04;
	else if (flags & USE_C2B)
	    m_buffer[9] |= 0x02;
	if (flags & USE_Q)
	    m_buffer[10] = 2;
	else if (flags & USE_RAW)
	    m_buffer[10] = 1;
    }

    unsigned Submit(ScsiTransport *st, void *buffer, size_t len)
    {
	return ScsiCommand::Submit(st, buffer, len);
    }
};

} // namespace import

#endif
