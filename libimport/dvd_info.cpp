/* dvd_info.cpp */

#include <stdio.h>
#include <unistd.h>
#include <string>
#include <set>
#include <vector>
#include <errno.h>

struct AspectRatio
{
    int width, height; // eg. 16, 9
};

struct FrameSize
{
    int xpixels, ypixels; // eg. 720, 576
};

class DVDInfo
{
    std::string m_device;
    std::string m_host;

    unsigned int m_nTitles;
    unsigned int m_nChapters;
    unsigned int m_nSeconds;
    AspectRatio  m_aspectRatio;
    FrameSize    m_frameSize;
    double       m_frameRate;
    unsigned int m_nFrames;
    std::vector<std::string> m_audioTrackList;

    enum {
	GOT_TITLES = 1,
	GOT_ASPECT  = 2,
	GOT_AUDIO  = 4,
	GOT_TIME   = 8,
	GOT_SIZE   = 0x10,
	GOT_RATE   = 0x20,
	GOT_ALL    = 0x3F
    };
    
public:
    DVDInfo(std::string dvd, std::string host=std::string())
	: m_device(dvd), m_host(host) {}

    int ReadInfo(unsigned int title=1); // returns -errno or 0

    unsigned int GetNumTitles() const { return m_nTitles; }
    unsigned int GetNumChapters() const { return m_nChapters; }
    unsigned int GetDurationSec() const { return m_nSeconds; }
    AspectRatio  GetAspectRatio() const { return m_aspectRatio; }
    FrameSize    GetFrameSize() const { return m_frameSize; }
    double       GetFrameRate() const { return m_frameRate; }

    std::vector<std::string> GetAudioTrackList() const { return m_audioTrackList; }
};

int DVDInfo::ReadInfo(unsigned int title)
{
    char buffer[256];
    sprintf(buffer, "%u", title);
    std::string cmd = ("/usr/bin/tcprobe -i " + m_device + " -T ") + buffer;
    
    if (!m_host.empty())
	cmd = ("rsh " + m_host) + " " + cmd;

    cmd += " 2>&1";

//    fprintf(stderr, "%s\n", cmd.c_str());

    FILE *f = popen(cmd.c_str(), "r");
    if (!f)
    {
	fprintf(stderr, "popen(%s): %m\n", cmd.c_str());
	return -errno;
    }

    m_audioTrackList.clear();
    int got_what = 0;

    while (!feof(f) && fgets(buffer, sizeof(buffer), f))
    {
//	printf("%s", buffer);
	char buffer2[256];
	unsigned int i;
	
	if (sscanf(buffer, "(dvd_reader.c)\tDVD title %d/%d: %d chapter(s)",
		   &i, &m_nTitles, &m_nChapters) == 3)
	{
	    got_what |= GOT_TITLES;
	}
	else if (sscanf(buffer, "[tcprobe] V: %d frames, %d sec",
			&m_nFrames, &m_nSeconds) == 2)
	{
	    got_what |= GOT_TIME;
	}
	else if (sscanf(buffer, "(dvd_reader.c) ac3 %s", buffer2) == 1)
	{
	    m_audioTrackList.push_back(std::string(buffer2));
	    got_what |= GOT_AUDIO;
	}
	else if (sscanf(buffer, "     aspect ratio: %d:%d",
			&m_aspectRatio.width, &m_aspectRatio.height) == 2)
	{
	    got_what |= GOT_ASPECT;
	}
	else if (sscanf(buffer, "       frame rate: -f %lf", &m_frameRate) == 1)
	{
	    got_what |= GOT_RATE;
	}
	else if (sscanf(buffer, "import frame size: -g %dx%d", 
			&m_frameSize.xpixels, &m_frameSize.ypixels) == 2)
	{
	    got_what |= GOT_SIZE;
	}
    }
    int rc = pclose(f);
    if (rc<0)
    {
	fprintf(stderr, "pclose: %m\n");
	return -errno;
    }
	    
    if (got_what != GOT_ALL)
    {
	printf("Got 0x%x\n", got_what);
	return -EMEDIUMTYPE;
    }

    return 0;
}

void DumpInfo(unsigned int i, const DVDInfo *ifo)
{
    AspectRatio ar = ifo->GetAspectRatio();
    FrameSize fs =   ifo->GetFrameSize();
    unsigned int sec = ifo->GetDurationSec();
    printf("Title %d: %d chapters, %dm%02ds, %d:%d %dx%d %ffps video\n",
	   i, ifo->GetNumChapters(), sec/60, sec%60, ar.width, ar.height,
	   fs.xpixels, fs.ypixels, ifo->GetFrameRate());
}

int main(int argc, char *argv[])
{
    DVDInfo ifo("/dev/dvd", "jalfrezi");
    
    int rc = ifo.ReadInfo(1);
    if (rc<0)
    {
	fprintf(stderr, "ReadInfo(1): %m\n");
	return 1;
    }

    DumpInfo(1,&ifo);

    for (unsigned int i=2; i<=ifo.GetNumTitles(); i++)
    {
	rc = ifo.ReadInfo(i);
	if (rc<0)
	{
	    fprintf(stderr, "ReadInfo(%u): %m\n", i);
	    continue;
	}
	DumpInfo(i,&ifo);
    }

    return 0;
}
