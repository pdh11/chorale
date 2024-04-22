#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <map>
#include <string>

#if 0

static const unsigned char mpegps_header[] =
{
    0x00, 0x00, 0x01, 0xBA,
    0x44, 0x00, 0x04, 0x00, 0x04, 0x01,
    0x00, 0x9C, 0x43,  // Mux rate, bytes/s/50 http://dvd.sourceforge.net/dvdinfo/packhdr.html
    0x00 
};

/** Parse a MPEG TS containing a single audio stream (e.g. from linux-dvb) and
 * write out the corresponding ES (i.e. the MP2 data).
 */
int main(int, char**)
{
    unsigned char buffer[188];
    unsigned int n=0;
    std::map<unsigned int, std::string> buffers;
    unsigned int pes_packet_remain = 0;
    
    (void)!write(1, mpegps_header, sizeof(mpegps_header));

    for (;;)
    {
	ssize_t rc = ::read(0, buffer, 188);
	if (rc < 0)
	{
	    fprintf(stderr, "Read error %u\n", errno);
	    return 1;
	}
	if (rc == 0)
	    return 0;

	if (buffer[0] != 0x47)
	{
	    fprintf(stderr, "Packet %u bad\n", n);
	    return 1;
	}
	unsigned int pid = (buffer[1] & 0x1F) * 256 + buffer[2];

//	fprintf(stderr, "pid=%u\n", pid);
//	if (pid != 433)
//	    continue;

	if (buffers.find(pid) == buffers.end())
	{
	    if ((buffer[1] & 0x40) == 0)
	    {
		fprintf(stderr, "Skipping non-start packet in %u\n", pid);
		continue;
	    }
	}
	
	bool adaptation = (buffer[3] & 0x20) != 0;

	const unsigned char *pesdata;
	unsigned int peslen;

	if (adaptation)
	{
	    unsigned int alen = buffer[4];
	    pesdata = buffer + 5 + alen;
	    peslen = 188 - alen - 5;
	}
	else
	{
	    pesdata = buffer + 4;
	    peslen = 188 - 4;
	}

	std::string& s = buffers[pid];
	if (buffer[1] & 0x40)
	{
	    if (!s.empty())
	    {
		fprintf(stderr, "Writing %u-byte packet from pid %u stream %x\n",
			(unsigned)s.length(), pid, pesdata[3]);
		unsigned int packetlen = (unsigned)s.length() - 6;
		if (s[4] == 0 && s[5] == 0)
		{
		    s[4] = (char)(packetlen >> 8);
		    s[5] = (char)packetlen;
		}
		(void)!write(1, s.data(), s.length());
		s.clear();
	    }
	}
	s.append(pesdata, pesdata+peslen);

	continue;

	const unsigned char *esdata;
	unsigned int eslen;

	if (pes_packet_remain == 0)
	{
	    // Start of packet
	    unsigned int packlen = pesdata[4]*256 + pesdata[5] + 6;
	    unsigned int peshlen = pesdata[8];
	    esdata = pesdata + peshlen + 9;
	    eslen = peslen - peshlen - 9;
	    pes_packet_remain = packlen - peshlen - 9;
	}
	else
	{
	    esdata = pesdata;
	    eslen = peslen;
	    if (eslen > pes_packet_remain)
		eslen = pes_packet_remain;
	}

	(void)!write(1, esdata, eslen);

	pes_packet_remain -= eslen;

	++n;
    }
}

#endif
