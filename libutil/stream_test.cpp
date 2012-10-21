#include "stream_test.h"
#include "trace.h"

namespace util {

void TestSeekableStream(SeekableStreamPtr msp)
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

    for (unsigned int i=0; i<1024; ++i)
    {
	unsigned int ii = i ^ 0x13b;
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

    msp->Seek(400000*4);
    msp->SetLength(300000*4);
    assert(msp->Tell() == 300000*4);
    size_t nread;
    char buf[4];
    rc = msp->Read(&buf, 4, &nread);
    assert(rc == 0);
    assert(nread == 0);
    size_t nwrote;
    rc = msp->Write(NULL, 0, &nwrote);
    assert(rc == 0);
    assert(nwrote == 0);
}

} // namespace util
