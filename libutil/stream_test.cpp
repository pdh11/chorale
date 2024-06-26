#include "stream_test.h"
#include "trace.h"

namespace util {

#if 0
SlowStream::SlowStream(Scheduler *poller,
		       size_t size, unsigned int interval_ms,
		       unsigned int count)
    : m_poller(poller),
      m_size(size),
      m_interval_ms(interval_ms),
      m_count(count),
      m_offset(0)
{
}

void SlowStream::Init()
{
    m_poller->Wait(util::Bind(TaskPtr(this)).To<&SlowStream::OnTimer>(),
		   0, m_interval_ms);
}
#endif

void TestSeekableStream(Stream *msp)
{
    unsigned int rc;

    for (unsigned int i=0; i<614400; ++i)
    {
	unsigned int j = (i * 37) ^ 0x6c503e21;
	rc = msp->WriteAll(&j, sizeof(j));
	assert(rc == 0);
    }

    assert(msp->GetLength() == 614400 * sizeof(unsigned int));
    assert(msp->Tell() == 614400 * sizeof(unsigned int));

    msp->Seek(0);

    for (unsigned int i=0; i<614400; ++i)
    {
	unsigned int j = (i * 37) ^ 0x6c503e21;
	rc = msp->WriteAllAt(&j, i*sizeof(j), sizeof(j));
	assert(rc == 0);
    }

    assert(msp->GetLength() == 614400 * sizeof(unsigned int));

    for (size_t i=0; i<1024; ++i)
    {
	size_t ii = i ^ 0x13b;
	msp->Seek(ii*600*4);

	unsigned int j[600];
	rc = msp->ReadAll(&j[0], 600*sizeof(unsigned int));
	assert(rc == 0);
	for (unsigned int k=0; k<600; ++k)
	{
	    unsigned int iii = ii*600 + k;
	    unsigned int x = iii*37 ^ 0x6c503e21;
	    if (j[k] != x)
	    {
		TRACE << "pos " << iii << " is " << j[k] << " should be " << x << "\n";
		TRACE << j[k] << " might belong at " << ((j[k] ^ 0x6c503e21)/37) << "\n";
	    }
	    assert(j[k] == x);
	}
    }

    msp->Seek((size_t)400000*4);
    msp->SetLength((size_t)300000*4);
    assert(msp->Tell() == (size_t)300000*4);
    size_t nread;
    char buf[4];
    rc = msp->Read(&buf, 4, &nread);
    assert(rc == 0);
    assert(nread == 0);

    size_t nwrote;
    rc = msp->Write(NULL, 0, &nwrote);
    assert(rc == 0);
    assert(nwrote == 0);
    
    msp->SetLength(0);
    assert(msp->Tell() == 0);
    rc = msp->WriteString("foo\nbar");
    assert(rc == 0);
    rc = msp->WriteAll(&rc, 1); // write a 0 byte
    assert(rc == 0);
    rc = msp->WriteString("wurdle");
    assert(rc == 0);
    msp->Seek(0);
    std::string line;
    rc = msp->ReadLine(&line);
    assert(rc == 0);
    assert(line == "foo");
    rc = msp->ReadLine(&line);
    assert(rc == 0);
    assert(line == "bar");
    rc = msp->ReadLine(&line);
    assert(rc == 0);
    assert(line == "wurdle");
    assert(msp->Tell() == msp->GetLength());
    msp->Seek(0);
    rc = DiscardStream(msp);
    assert(rc == 0);
    assert(msp->Tell() == msp->GetLength());
}

} // namespace util
